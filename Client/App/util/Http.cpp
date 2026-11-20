#include "util/Http.h"

#include "util/ContentProvider.h"
#include "util/Utilities.h"
#include "util/standardout.h"

#include <G3D/format.h>
#include <algorithm>
#include <cstring>
#include <curl/curl.h>
#include <memory>
#include <mutex>
#include <new>
#include <sstream>
#include <stdexcept>
#include <zlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <filesystem>
#include <system_error>
#endif

class HttpRequest_source
{
	const std::string& body;
	std::string::size_type offset;
	char buffer[4096];
	int position;
	int size;

public:
	HttpRequest_source(const std::string& body) : body(body), offset(0), position(0), size(0) {}

	int read(char* s, int n);

	bool fill()
	{
		while (position == size) {
			const int amount = read(buffer, sizeof(buffer));

			if (amount == -1) {
				return false;
			}

			position = 0;
			size = amount;
		}

		return true;
	}

	int get() { return fill() ? static_cast<unsigned char>(buffer[position++]) : EOF; }

	const char* data() const { return buffer + position; }
	int available() const { return size - position; }
	void consume() { position = size; }
};

int HttpRequest_source::read(char* s, int n)
{
	if (offset == body.size()) {
		return -1;
	}

	const std::string::size_type amount = std::min<std::string::size_type>(n, body.size() - offset);
	body.copy(s, amount, offset);
	offset += amount;
	return static_cast<int>(amount);
}

namespace {

enum GzipError
{
	GzipZlibError = 1,
	GzipBadCrc = 2,
	GzipBadLength = 3,
	GzipBadHeader = 4,
	GzipBadFooter = 5
};

struct GzipException
{
	int error;
	int zlibError;
};

void checkZlib(int result)
{
	if (result == Z_MEM_ERROR) {
		throw std::bad_alloc();
	}

	if (result != Z_OK && result != Z_STREAM_END) {
		throw std::runtime_error("zlib error");
	}
}

void appendUint32(std::string& output, uLong value)
{
	for (int shift = 0; shift < 32; shift += 8) {
		output += static_cast<char>(0xFF & (value >> shift));
	}
}

uLong readUint32(const std::string& input, std::string::size_type offset)
{
	uLong value = 0;

	for (int i = 0; i < 4; ++i) {
		value |= static_cast<uLong>(static_cast<unsigned char>(input[offset + i])) << (8 * i);
	}

	return value;
}

std::string gzipCompress(const std::string& input)
{
	static const char header[] = {0x1f, static_cast<char>(0x8b), Z_DEFLATED, 0, 0, 0, 0, 0, 0, static_cast<char>(0xff)};

	std::string output(header, sizeof(header));

	z_stream stream = {};
	checkZlib(deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY));

	stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));
	stream.avail_in = static_cast<uInt>(input.size());

	int result;

	do {
		char buffer[4096];
		stream.next_out = reinterpret_cast<Bytef*>(buffer);
		stream.avail_out = sizeof(buffer);

		result = deflate(&stream, Z_FINISH);

		if (result != Z_OK && result != Z_STREAM_END) {
			deflateEnd(&stream);
			checkZlib(result);
		}

		output.append(buffer, sizeof(buffer) - stream.avail_out);
	} while (result != Z_STREAM_END);

	const uLong totalIn = stream.total_in;
	deflateEnd(&stream);

	const Bytef* bytes = reinterpret_cast<const Bytef*>(input.data());
	appendUint32(output, crc32(crc32(0, Z_NULL, 0), bytes, static_cast<uInt>(input.size())));
	appendUint32(output, totalIn);

	return output;
}

int readGzipByte(HttpRequest_source& source, int error)
{
	const int c = source.get();

	if (c == EOF) {
		throw GzipException{error, Z_OK};
	}

	return c;
}

void readGzipHeader(HttpRequest_source& source)
{
	int flags;

	if (source.get() != 0x1f || source.get() != 0x8b || source.get() == EOF || (flags = source.get()) == EOF) {
		throw GzipException{GzipBadHeader, Z_OK};
	}

	for (int i = 0; i < 6; ++i) {
		readGzipByte(source, GzipBadHeader);
	}

	if (flags & 4) {
		int length = readGzipByte(source, GzipBadHeader);
		length += readGzipByte(source, GzipBadHeader) << 8;

		while (length-- != 0 && source.get() != EOF) {
		}
	}

	for (int field = 8; field <= 16; field <<= 1) {
		if (flags & field) {
			while (readGzipByte(source, GzipBadHeader) != 0) {
			}
		}
	}

	if (flags & 2) {
		readGzipByte(source, GzipBadHeader);
		readGzipByte(source, GzipBadHeader);
	}
}

void gzipDecompress(HttpRequest_source& source, std::string& response)
{
	readGzipHeader(source);

	z_stream stream = {};
	checkZlib(inflateInit2(&stream, -MAX_WBITS));

	uLong crc = crc32(0, Z_NULL, 0);
	int result;

	do {
		if (stream.avail_in == 0 && source.fill()) {
			stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(source.data()));
			stream.avail_in = source.available();
			source.consume();
		}

		char buffer[4096];
		stream.next_out = reinterpret_cast<Bytef*>(buffer);
		stream.avail_out = sizeof(buffer);

		result = inflate(&stream, Z_SYNC_FLUSH);

		const uInt produced = sizeof(buffer) - stream.avail_out;
		crc = crc32(crc, reinterpret_cast<const Bytef*>(buffer), produced);
		response.append(buffer, produced);

		if (result != Z_OK && result != Z_STREAM_END) {
			inflateEnd(&stream);

			if (result == Z_MEM_ERROR) {
				throw std::bad_alloc();
			}

			throw GzipException{GzipZlibError, result};
		}
	} while (result != Z_STREAM_END);

	std::string footer(reinterpret_cast<const char*>(stream.next_in), stream.avail_in);
	const uLong totalOut = stream.total_out;
	inflateEnd(&stream);

	for (int c = source.get(); c != EOF; c = source.get()) {
		footer += static_cast<char>(c);
	}

	if (footer.size() < 4) {
		throw GzipException{GzipBadFooter, Z_OK};
	}

	if (readUint32(footer, 0) != crc) {
		throw GzipException{GzipBadCrc, Z_OK};
	}

	if (footer.size() < 8) {
		throw GzipException{GzipBadFooter, Z_OK};
	}

	if (static_cast<int>(readUint32(footer, 4)) != static_cast<int>(totalOut)) {
		throw GzipException{GzipBadLength, Z_OK};
	}
}

void readResponse(HttpRequest_source& source, bool gzip, int statusCode, const std::string& url, std::string& response)
{
	if (gzip) {
		try {
			gzipDecompress(source, response);
		}
		catch (const GzipException& e) {
			throw std::runtime_error(
				G3D::format("GZip error %d, ZLib error %d, '%s'", e.error, e.zlibError, url.c_str())
			);
		}
	}
	else {
		while (source.fill()) {
			response.append(source.data(), source.available());
			source.consume();
		}
	}

	if (statusCode != 200) {
		throw std::runtime_error(G3D::format("statusCode = %d", statusCode));
	}
}

struct UrlParts
{
	std::string scheme;
	std::string host;
};

bool isWebScheme(const std::string& scheme)
{
	return scheme == "ftp" || scheme == "http" || scheme == "https";
}

struct CurlDeleter
{
	void operator()(CURL* curl) const { curl_easy_cleanup(curl); }
};

struct CurlUrlDeleter
{
	void operator()(CURLU* url) const { curl_url_cleanup(url); }
};

struct CurlListDeleter
{
	void operator()(curl_slist* list) const { curl_slist_free_all(list); }
};

typedef std::unique_ptr<curl_slist, CurlListDeleter> CurlList;

std::string getUrlPart(CURLU* url, CURLUPart what)
{
	std::string result;
	char* part = nullptr;
	if (curl_url_get(url, what, &part, 0) == CURLUE_OK) {
		result = part;
		curl_free(part);
	}
	return result;
}

UrlParts crackUrl(const char* url)
{
	UrlParts parts;

	std::unique_ptr<CURLU, CurlUrlDeleter> handle(curl_url());
	if (!handle) {
		throw std::bad_alloc();
	}

	if (curl_url_set(handle.get(), CURLUPART_URL, url, CURLU_NON_SUPPORT_SCHEME) == CURLUE_OK) {
		parts.scheme = getUrlPart(handle.get(), CURLUPART_SCHEME);
		parts.host = getUrlPart(handle.get(), CURLUPART_HOST);
	}

	return parts;
}

void appendHeader(CurlList& headers, const std::string& header)
{
	curl_slist* list = curl_slist_append(headers.get(), header.c_str());
	if (!list) {
		throw std::bad_alloc();
	}
	headers.release();
	headers.reset(list);
}

bool addHeaders(CurlList& headers, const char* additionalHeaders)
{
	bool contentType = false;

	std::istringstream lines(additionalHeaders);
	std::string line;

	while (std::getline(lines, line)) {
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		if (line.empty()) {
			continue;
		}

		std::string name = line.substr(0, line.find(':'));
		RBX::trim(name);
		RBX::toLower(name);
		contentType |= name == "content-type";

		appendHeader(headers, line);
	}

	return contentType;
}

#ifdef _WIN32

bool readSkipTrustCheck()
{
	DWORD value = 0;
	auto query = [&value](HKEY root, const char* path) {
		HKEY key;
		if (RegOpenKeyExA(root, path, 0, KEY_READ, &key) == ERROR_SUCCESS) {
			DWORD data;
			DWORD size = sizeof(data);
			DWORD type;
			if (RegQueryValueExA(key, "SkipTrustCheck", nullptr, &type, reinterpret_cast<LPBYTE>(&data), &size) ==
					ERROR_SUCCESS &&
				type == REG_DWORD) {
				value = data;
			}
			RegCloseKey(key);
		}
	};

	query(HKEY_CURRENT_USER, "Software\\ROBLOX Corporation\\Roblox");
	query(HKEY_LOCAL_MACHINE, "Software\\Roblox");

	return value == 1;
}

std::string readInternetSetting(HKEY key, const char* name)
{
	char buffer[2048];
	DWORD size = sizeof(buffer);
	DWORD type;
	if (RegQueryValueExA(key, name, nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &size) != ERROR_SUCCESS ||
		type != REG_SZ) {
		return std::string();
	}
	return std::string(buffer, strnlen(buffer, size));
}

void setSystemProxy(CURL* curl, const std::string& scheme)
{
	HKEY key;
	if (RegOpenKeyExA(
			HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 0, KEY_READ, &key
		) != ERROR_SUCCESS) {
		return;
	}

	DWORD enable = 0;
	DWORD size = sizeof(enable);
	DWORD type;
	std::string server;
	std::string bypassList;
	if (RegQueryValueExA(key, "ProxyEnable", nullptr, &type, reinterpret_cast<LPBYTE>(&enable), &size) ==
			ERROR_SUCCESS &&
		type == REG_DWORD && enable) {
		server = readInternetSetting(key, "ProxyServer");
		bypassList = readInternetSetting(key, "ProxyOverride");
	}
	RegCloseKey(key);

	std::string proxy;
	std::istringstream entries(server);
	std::string entry;
	while (std::getline(entries, entry, ';')) {
		RBX::trim(entry);
		const std::string::size_type equals = entry.find('=');
		if (equals == std::string::npos) {
			if (proxy.empty()) {
				proxy = entry;
			}
		}
		else if (entry.compare(0, equals, scheme) == 0) {
			proxy = entry.substr(equals + 1);
			break;
		}
	}

	if (proxy.empty()) {
		return;
	}

	std::string bypass;
	std::istringstream hosts(bypassList);
	std::string host;
	while (std::getline(hosts, host, ';')) {
		RBX::trim(host);
		if (host == "<local>") {
			host = "localhost,127.0.0.1,::1";
		}
		else if (host.starts_with("*.")) {
			host.erase(0, 2);
		}
		if (host.empty() || host.find('*') != std::string::npos) {
			continue;
		}
		if (!bypass.empty()) {
			bypass += ',';
		}
		bypass += host;
	}

	curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
	curl_easy_setopt(curl, CURLOPT_NOPROXY, bypass.c_str());
}

void setCertificates(CURL* curl)
{
	curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, static_cast<long>(CURLSSLOPT_NATIVE_CA));
}

#else

bool readSkipTrustCheck()
{
	return false;
}

const char* findCertificateBundle()
{
	static const char* const bundles[] = {
		"/etc/pki/tls/certs/ca-bundle.crt",
		"/etc/ssl/certs/ca-certificates.crt",
		"/etc/ssl/ca-bundle.pem",
		"/etc/pki/tls/cacert.pem",
		"/etc/ssl/cert.pem",
	};

	for (const char* bundle : bundles) {
		std::error_code error;
		if (std::filesystem::is_regular_file(bundle, error)) {
			return bundle;
		}
	}

	return nullptr;
}

void setSystemProxy(CURL*, const std::string&)
{
}

void setCertificates(CURL* curl)
{
	static const char* const certificateBundle = findCertificateBundle();
	if (certificateBundle) {
		curl_easy_setopt(curl, CURLOPT_CAINFO, certificateBundle);
	}
}

#endif

CURL* threadHandle()
{
	static std::once_flag initialized;
	std::call_once(initialized, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });

	thread_local std::unique_ptr<CURL, CurlDeleter> handle(curl_easy_init());
	if (!handle) {
		throw std::bad_alloc();
	}

	curl_easy_reset(handle.get());
	return handle.get();
}

size_t writeBody(char* data, size_t size, size_t count, void* body)
{
	static_cast<std::string*>(body)->append(data, size * count);
	return size * count;
}

} // namespace

namespace RBX {

bool Http::isScript(const char* url)
{
	if (isWebScheme(crackUrl(url).scheme)) {
		return false;
	}

	std::string sUrl(url);
	if (sUrl.starts_with("javascript:")) {
		return true;
	}
	if (sUrl.starts_with("jscript:")) {
		return true;
	}
	return false;
}

bool Http::isRobloxSite(const char* url)
{
	UrlParts parts = crackUrl(url);

	if (isWebScheme(parts.scheme)) {
		toLower(parts.host);
		return parts.host.ends_with("roblox.com");
	}

	return false;
}

static std::mutex trustCheckMutex;

bool Http::trustCheck(const char* url)
{
	{
		std::scoped_lock lock(trustCheckMutex);
		static std::unique_ptr<bool> skipTrustCheck;
		if (!skipTrustCheck) {
			skipTrustCheck.reset(new bool(readSkipTrustCheck()));
		}

		if (*skipTrustCheck) {
			return true;
		}
	}

	if (std::string("about:blank") == url) {
		return true;
	}

	if (isRobloxSite(url)) {
		return true;
	}

	if (isScript(url)) {
		return true;
	}

	UrlParts parts = crackUrl(url);

	if (isWebScheme(parts.scheme)) {
		toLower(parts.host);
		if (parts.host.ends_with("googlesyndication.com")) {
			return true;
		}
	}
	else if (std::string(url).starts_with("res:")) {
		return true;
	}

	return false;
}

static void httpGetPost(
	bool isPost,
	const std::string& url,
	std::istream& data,
	bool compressData,
	const char* additionalHeaders,
	std::string& response
)
{
	if (url.empty()) {
		throw std::runtime_error("empty url");
	}

	if (!Http::trustCheck(url.c_str())) {
		throw std::runtime_error(G3D::format("trust check failed for %s", url.c_str()));
	}

	const UrlParts u = crackUrl(url.c_str());

	if (u.host.empty()) {
		throw std::runtime_error(G3D::format("'%s' is missing a hostName", url.c_str()));
	}

	CURL* curl = threadHandle();

	char error[CURL_ERROR_SIZE] = "";
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Roblox");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	setSystemProxy(curl, u.scheme);
	setCertificates(curl);

	CurlList headers;
	bool contentType = false;

	if (additionalHeaders) {
		contentType = addHeaders(headers, additionalHeaders);
	}

	appendHeader(headers, "Accept-Encoding: gzip");

	std::string uploadData;

	if (isPost) {
		uploadData = readStream(data);

		if (compressData) {
			appendHeader(headers, "Content-Encoding: gzip");
			if (!uploadData.empty()) {
				uploadData = gzipCompress(uploadData);
			}
		}

		const size_t uploadSize = uploadData.size();

		if (uploadSize > 10000) {
			appendHeader(headers, "Roblox-Content-Size: " + std::to_string(uploadSize));
			appendHeader(headers, G3D::format("Roblox-Content-First: %d", uploadData[0]));
			appendHeader(headers, G3D::format("Roblox-Content-Last: %d", uploadData[uploadSize - 1]));

			std::unique_ptr<MD5Hasher> hasher(MD5Hasher::create());
			hasher->addData(uploadData);
			appendHeader(headers, G3D::format("Roblox-Content-Hash: %s", hasher->c_str()));
		}

		if (!contentType) {
			appendHeader(headers, "Content-Type:");
		}
		appendHeader(headers, "Expect:");

		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, uploadData.data());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(uploadSize));
	}

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers.get());

	std::string body;
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeBody);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

	const CURLcode result = curl_easy_perform(curl);
	if (result != CURLE_OK) {
		throw std::runtime_error(
			G3D::format("HTTP request failed for %s, %s", url.c_str(), *error ? error : curl_easy_strerror(result))
		);
	}

	long statusCode = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);

	curl_header* encoding = nullptr;
	const bool gzip = curl_easy_header(curl, "Content-Encoding", 0, CURLH_HEADER, -1, &encoding) == CURLHE_OK &&
					  std::strcmp(encoding->value, "gzip") == 0;

	HttpRequest_source source(body);
	readResponse(source, gzip, statusCode, url, response);
}

void Http::post(std::istream& input, bool compress, std::string& response)
{
	if (additionalHeaders.empty()) {
		httpGetPost(true, url, input, compress, nullptr, response);
	}
	else {
		httpGetPost(true, url, input, compress, additionalHeaders.c_str(), response);
	}
}

void Http::get(std::string& response)
{
	const char* h = nullptr;
	if (!additionalHeaders.empty()) {
		h = additionalHeaders.c_str();
	}

	std::istringstream dummy;
	try {
		httpGetPost(false, url, dummy, false, h, response);
	}
	catch (...) {
		StandardOut::singleton()->print(MESSAGE_WARNING, "httpGet failed, trying again");
		httpGetPost(false, url, dummy, false, h, response);
	}
}

} // namespace RBX
