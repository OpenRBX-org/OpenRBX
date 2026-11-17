#include "util/Http.h"

#include "util/ContentProvider.h"
#include "util/Utilities.h"
#include "util/standardout.h"

#include <G3D/format.h>
#include <memory>
#include <mutex>
#include <new>
#include <sstream>
#include <stdexcept>
#include <zlib.h>

#ifdef _WIN32
#include <atlbase.h>
#include <atlstr.h>
#include <atlutil.h>
#include <wininet.h>
#else
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <system_error>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>
#endif

#ifdef _WIN32
class CHINTERNET
{
	HINTERNET handle;

public:
	CHINTERNET(const CHINTERNET&) = delete;
	CHINTERNET& operator=(const CHINTERNET&) = delete;

	CHINTERNET(HINTERNET handle) : handle(handle) {}
	CHINTERNET() : handle(nullptr) {}

	CHINTERNET& operator=(HINTERNET handle)
	{
		this->handle = handle;
		return *this;
	}

	operator bool() { return handle != nullptr; }
	operator void*() { return handle; }

	~CHINTERNET() { InternetCloseHandle(handle); }
};
#endif

class HttpRequest_source
{
#ifdef _WIN32
	HINTERNET request;
#else
	const std::string& body;
	std::string::size_type offset;
#endif
	char buffer[4096];
	int position;
	int size;

public:
#ifdef _WIN32
	HttpRequest_source(HINTERNET request) : request(request), position(0), size(0) {}
#else
	HttpRequest_source(const std::string& body) : body(body), offset(0), position(0), size(0) {}
#endif

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
	int port = 0;
	std::string path;
};

bool isWebScheme(const std::string& scheme)
{
	return scheme == "ftp" || scheme == "http" || scheme == "https";
}

} // namespace

#ifdef _WIN32

namespace RBX {

void ThrowIfFailure(bool success, const char* message)
{
	if (!success) {
		DWORD error = GetLastError();

		char buffer[256];
		if (!FormatMessage(
				FORMAT_MESSAGE_FROM_SYSTEM,
				nullptr,
				error,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
				buffer,
				256,
				nullptr
			)) {
			throw std::runtime_error(G3D::format("%s, err=0x%X", message, error));
		}

		throw std::runtime_error(G3D::format("%s, %s", message, buffer));
	}
}

} // namespace RBX

int HttpRequest_source::read(char* s, int n)
{
	DWORD numBytes;
	if (!InternetQueryDataAvailable(request, &numBytes, 0, 0)) {
		numBytes = 0;
	}
	if (numBytes == 0) {
		return -1;
	}

	DWORD bytesRead;
	if (!InternetReadFile(request, s, n, &bytesRead)) {
		throw std::runtime_error("InternetReadFile failed");
	}
	return bytesRead;
}

static UrlParts crackUrl(const char* url)
{
	CUrl crack;
	crack.CrackUrl(url);

	UrlParts parts;

	switch (crack.GetScheme()) {
	case ATL_URL_SCHEME_FTP:
		parts.scheme = "ftp";
		break;
	case ATL_URL_SCHEME_HTTP:
		parts.scheme = "http";
		break;
	case ATL_URL_SCHEME_HTTPS:
		parts.scheme = "https";
		break;
	}

	parts.host = crack.GetHostName();

	return parts;
}

static bool readSkipTrustCheck()
{
	DWORD value = 0;
	CRegKey key;

	if (key.Open(HKEY_CURRENT_USER, "Software\\ROBLOX Corporation\\Roblox", KEY_READ) == ERROR_SUCCESS) {
		key.QueryDWORDValue("SkipTrustCheck", value);
	}

	if (key.Open(HKEY_LOCAL_MACHINE, "Software\\Roblox", KEY_READ) == ERROR_SUCCESS) {
		key.QueryDWORDValue("SkipTrustCheck", value);
	}

	return value == 1;
}

#else

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

static UrlParts crackUrl(const char* url)
{
	UrlParts parts;

	const std::string text(url);
	const std::string::size_type schemeEnd = text.find("://");
	if (schemeEnd == std::string::npos) {
		return parts;
	}

	parts.scheme = text.substr(0, schemeEnd);
	RBX::toLower(parts.scheme);

	const std::string::size_type authorityBegin = schemeEnd + 3;
	const std::string::size_type pathBegin = text.find_first_of("/?#", authorityBegin);
	std::string authority = text.substr(authorityBegin, pathBegin - authorityBegin);

	const std::string::size_type userInfoEnd = authority.rfind('@');
	if (userInfoEnd != std::string::npos) {
		authority.erase(0, userInfoEnd + 1);
	}

	const std::string::size_type portBegin = authority.rfind(':');
	if (portBegin != std::string::npos && authority.find(']', portBegin) == std::string::npos) {
		parts.port = std::atoi(authority.c_str() + portBegin + 1);
		authority.erase(portBegin);
	}
	else if (parts.scheme == "https") {
		parts.port = 443;
	}
	else if (parts.scheme == "ftp") {
		parts.port = 21;
	}
	else {
		parts.port = 80;
	}

	parts.host = authority;

	if (pathBegin != std::string::npos) {
		parts.path = text.substr(pathBegin, text.find('#', pathBegin) - pathBegin);
	}

	if (parts.path.empty() || parts.path[0] != '/') {
		parts.path.insert(0, "/");
	}

	return parts;
}

static bool readSkipTrustCheck()
{
	return false;
}

static const char* findCertificateBundle()
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

static void addHeaders(httplib::Headers& headers, const char* additionalHeaders)
{
	std::istringstream lines(additionalHeaders);
	std::string line;

	while (std::getline(lines, line)) {
		const std::string::size_type colon = line.find(':');
		if (colon == std::string::npos) {
			continue;
		}

		std::string name = line.substr(0, colon);
		std::string value = line.substr(colon + 1);
		RBX::trim(name);
		RBX::trim(value);
		headers.emplace(name, value);
	}
}

#endif

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

#ifdef _WIN32

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

	CUrl u;
	u.CrackUrl(url.c_str());

	if (u.GetHostNameLength() == 0) {
		throw std::runtime_error(G3D::format("'%s' is missing a hostName", url.c_str()));
	}

	CHINTERNET session(InternetOpen("Roblox", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0));
	CHINTERNET connection(InternetConnect(
		session, u.GetHostName(), u.GetPortNumber(), u.GetUserName(), u.GetPassword(), INTERNET_SERVICE_HTTP, 0, 1
	));

	CString s = u.GetUrlPath();
	s += u.GetExtraInfo();

	CHINTERNET request(HttpOpenRequest(
		connection,
		isPost ? "POST" : "GET",
		s,
		nullptr,
		nullptr,
		nullptr,
		INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NEED_FILE,
		1
	));
	if (!request) {
		throw std::runtime_error(G3D::format("HttpOpenRequest failed for %s", url.c_str()));
	}

	if (additionalHeaders) {
		if (!HttpAddRequestHeaders(request, additionalHeaders, strlen(additionalHeaders), HTTP_ADDREQ_FLAG_ADD)) {
			ThrowIfFailure(false, "HttpAddRequestHeaders failed");
		}
	}

	const char* acceptEncoding = "Accept-Encoding: gzip\r\n";
	if (!HttpAddRequestHeaders(request, acceptEncoding, strlen(acceptEncoding), HTTP_ADDREQ_FLAG_ADD)) {
		ThrowIfFailure(false, "HttpAddRequestHeaders failed");
	}

	if (!isPost) {
		if (!HttpSendRequest(request, nullptr, 0, nullptr, 0)) {
			ThrowIfFailure(false, "HttpSendRequest failed");
		}
	}
	else {
		std::string uploadData = readStream(data);

		if (compressData) {
			std::string contentEncodingHeader = "Content-Encoding: gzip\r\n";
			if (!HttpAddRequestHeaders(
					request, contentEncodingHeader.c_str(), contentEncodingHeader.size(), HTTP_ADDREQ_FLAG_ADD
				)) {
				ThrowIfFailure(false, "HttpAddRequestHeaders failed");
			}
			if (!uploadData.empty()) {
				uploadData = gzipCompress(uploadData);
			}
		}

		const size_t uploadSize = uploadData.size();

		if (uploadSize > 10000) {
			std::string contentSizeHeader = G3D::format("Roblox-Content-Size: %d\r\n", uploadSize);
			if (!HttpAddRequestHeaders(
					request, contentSizeHeader.c_str(), contentSizeHeader.size(), HTTP_ADDREQ_FLAG_ADD
				)) {
				ThrowIfFailure(false, "HttpAddRequestHeaders failed");
			}

			std::string contentFirstHeader = G3D::format("Roblox-Content-First: %d\r\n", uploadData[0]);
			if (!HttpAddRequestHeaders(
					request, contentFirstHeader.c_str(), contentFirstHeader.size(), HTTP_ADDREQ_FLAG_ADD
				)) {
				ThrowIfFailure(false, "HttpAddRequestHeaders failed");
			}

			std::string contentLastHeader = G3D::format("Roblox-Content-Last: %d\r\n", uploadData[uploadSize - 1]);
			if (!HttpAddRequestHeaders(
					request, contentLastHeader.c_str(), contentLastHeader.size(), HTTP_ADDREQ_FLAG_ADD
				)) {
				ThrowIfFailure(false, "HttpAddRequestHeaders failed");
			}

			std::unique_ptr<MD5Hasher> hasher(MD5Hasher::create());
			hasher->addData(uploadData);
			std::string contentHashHeader = G3D::format("Roblox-Content-Hash: %s\r\n", hasher->c_str());
			if (!HttpAddRequestHeaders(
					request, contentHashHeader.c_str(), contentHashHeader.size(), HTTP_ADDREQ_FLAG_ADD
				)) {
				ThrowIfFailure(false, "HttpAddRequestHeaders failed");
			}
		}

		INTERNET_BUFFERS buffer = {0};
		buffer.dwStructSize = sizeof(INTERNET_BUFFERS);
		buffer.dwBufferTotal = uploadSize;

		if (!HttpSendRequestEx(request, &buffer, nullptr, 0, 0)) {
			throw std::runtime_error("HttpSendRequestEx failed");
		}

		try {
			DWORD bytesWritten;
			if (!InternetWriteFile(request, uploadData.c_str(), uploadSize, &bytesWritten)) {
				ThrowIfFailure(false, "InternetWriteFile failed");
			}

			if (bytesWritten != uploadSize) {
				throw std::runtime_error("Failed to upload content");
			}
		}
		catch (...) {
			HttpEndRequest(request, nullptr, 0, 0);
			throw;
		}

		if (!HttpEndRequest(request, nullptr, 0, 0)) {
			ThrowIfFailure(false, "HttpEndRequest failed");
		}
	}

	DWORD statusCode;
	{
		char szBuffer[80];
		DWORD dwLen = 80;
		if (!HttpQueryInfo(request, HTTP_QUERY_STATUS_CODE, szBuffer, &dwLen, nullptr)) {
			ThrowIfFailure(false, "HttpQueryInfo failed");
		}

		statusCode = atol(szBuffer);
	}

	bool gzip = false;

	{
		char buffer[256];
		DWORD length = 256;
		if (HttpQueryInfo(request, HTTP_QUERY_CONTENT_ENCODING, buffer, &length, nullptr)) {
			gzip = std::string(buffer) == "gzip";
		}
	}

	HttpRequest_source source(request);
	readResponse(source, gzip, statusCode, url, response);
}

#else

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

	httplib::Client client((u.scheme == "https" ? "https://" : "http://") + u.host + ":" + std::to_string(u.port));
	client.set_follow_location(true);

	static const char* const certificateBundle = findCertificateBundle();
	if (certificateBundle) {
		client.set_ca_cert_path(certificateBundle);
	}

	httplib::Headers headers{{"User-Agent", "Roblox"}};

	if (additionalHeaders) {
		addHeaders(headers, additionalHeaders);
	}

	headers.emplace("Accept-Encoding", "gzip");

	httplib::Result result;

	if (!isPost) {
		result = client.Get(u.path, headers);
	}
	else {
		std::string uploadData = readStream(data);

		if (compressData) {
			headers.emplace("Content-Encoding", "gzip");
			if (!uploadData.empty()) {
				uploadData = gzipCompress(uploadData);
			}
		}

		const size_t uploadSize = uploadData.size();

		if (uploadSize > 10000) {
			headers.emplace("Roblox-Content-Size", std::to_string(uploadSize));
			headers.emplace("Roblox-Content-First", std::to_string(uploadData[0]));
			headers.emplace("Roblox-Content-Last", std::to_string(uploadData[uploadSize - 1]));

			std::unique_ptr<MD5Hasher> hasher(MD5Hasher::create());
			hasher->addData(uploadData);
			headers.emplace("Roblox-Content-Hash", hasher->c_str());
		}

		result = client.Post(u.path, headers, uploadData, "");
	}

	if (!result) {
		throw std::runtime_error(
			G3D::format("HTTP request failed for %s, %s", url.c_str(), httplib::to_string(result.error()).c_str())
		);
	}

	const bool gzip = result->get_header_value("Content-Encoding") == "gzip";

	HttpRequest_source source(result->body);
	readResponse(source, gzip, result->status, url, response);
}

#endif

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
