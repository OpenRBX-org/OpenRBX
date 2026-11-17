#include "util/ContentProvider.h"

#include "util/FileSystem.h"
#include "util/Http.h"
#include "util/Utilities.h"
#include "util/standardout.h"
#include "v8xml/SerializerV2.h"
#include "v8xml/XmlElement.h"
#include "v8xml/XmlSerializer.h"

#include <G3D/format.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

#ifdef _WIN32
#include <wincrypt.h>
#include <windows.h>
#else
#include <openssl/evp.h>
#endif

namespace RBX {

class MD5HasherImpl : public MD5Hasher
{
#ifdef _WIN32
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
#else
	EVP_MD_CTX* context;
#endif
	std::string result;

public:
	MD5HasherImpl();
	~MD5HasherImpl();

	void addData(std::istream& data) override;
	void addData(const std::string& data) override;
	void addData(const char* data, unsigned int nBytes) override;
	std::string toString() override;
	const char* c_str() override;
};

bool ContentProvider::isHttpUrl(const std::string& s)
{
	if (s.starts_with("http://")) {
		return true;
	}
	if (s.starts_with("https://")) {
		return true;
	}
	return false;
}

std::string ContentProvider::assetFolder() const
{
	return assetFolderPath;
}

bool operator<(const ContentId& a, const ContentId& b)
{
	return a.toString() < b.toString();
}

bool operator!=(const ContentId& a, const ContentId& b)
{
	return a.toString() != b.toString();
}

bool operator==(const ContentId& a, const ContentId& b)
{
	return a.toString() == b.toString();
}

ContentId ContentId::fromFile(const std::string& filePath)
{
	if (filePath.starts_with("file://")) {
		return ContentId(filePath);
	}

	std::string header("file://");
	return ContentId(header + filePath);
}

ContentId ContentId::fromAssets(const std::string& filePath)
{
	std::string header("rbxasset://");
	return ContentId(header + filePath);
}

std::string MD5Hasher::convertToLegacyHash(std::string hash)
{
	std::string result;
	for (int i = 0; i < 32; i += 2) {
		if (hash[i] != '0') {
			result += hash[i];
		}
		result += hash[i + 1];
	}
	return result;
}

#ifdef _WIN32

MD5HasherImpl::MD5HasherImpl() : hProv(0), hHash(0)
{
	if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
		StandardOut::singleton()->print(
			MESSAGE_ERROR, "Error during CryptAcquireContext. GetLastError = %d", GetLastError()
		);
	}

	if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
		StandardOut::singleton()->print(
			MESSAGE_ERROR, "Error during CryptCreateHash. GetLastError = %d", GetLastError()
		);
	}
}

MD5HasherImpl::~MD5HasherImpl()
{
	if (hHash) {
		CryptDestroyHash(hHash);
	}

	if (hProv) {
		CryptReleaseContext(hProv, 0);
	}
}

void MD5HasherImpl::addData(const std::string& data)
{
	if (!CryptHashData(hHash, (const BYTE*) data.c_str(), data.size(), 0)) {
		StandardOut::singleton()->print(MESSAGE_ERROR, "Error during CryptHashData. GetLastError = %d", GetLastError());
	}
}

void MD5HasherImpl::addData(const char* data, unsigned int nBytes)
{
	if (!CryptHashData(hHash, (const BYTE*) data, nBytes, 0)) {
		StandardOut::singleton()->print(MESSAGE_ERROR, "Error during CryptHashData. GetLastError = %d", GetLastError());
	}
}

const char* MD5HasherImpl::c_str()
{
	if (result.size() == 0) {
		DWORD hashLength;
		DWORD foo = sizeof(DWORD);
		if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*) &hashLength, &foo, 0)) {
			StandardOut::singleton()->print(
				MESSAGE_ERROR, "Error during CryptGetHashParam. GetLastError = %d", GetLastError()
			);
		}

		BYTE* hash = new BYTE[hashLength];
		if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLength, 0)) {
			StandardOut::singleton()->print(
				MESSAGE_ERROR, "Error during CryptGetHashParam. GetLastError = %d", GetLastError()
			);
		}

		for (DWORD i = 0; i < hashLength; i++) {
			char s[3];
			std::snprintf(s, sizeof(s), "%02x", hash[i]);
			result += s;
		}

		delete[] hash;
	}

	return result.c_str();
}

#else

MD5HasherImpl::MD5HasherImpl() : context(EVP_MD_CTX_new())
{
	if (!context || !EVP_DigestInit_ex(context, EVP_md5(), nullptr)) {
		StandardOut::singleton()->print(MESSAGE_ERROR, "Error during EVP_DigestInit_ex");
	}
}

MD5HasherImpl::~MD5HasherImpl()
{
	EVP_MD_CTX_free(context);
}

void MD5HasherImpl::addData(const std::string& data)
{
	addData(data.c_str(), data.size());
}

void MD5HasherImpl::addData(const char* data, unsigned int nBytes)
{
	if (!EVP_DigestUpdate(context, data, nBytes)) {
		StandardOut::singleton()->print(MESSAGE_ERROR, "Error during EVP_DigestUpdate");
	}
}

const char* MD5HasherImpl::c_str()
{
	if (result.size() == 0) {
		unsigned char hash[EVP_MAX_MD_SIZE];
		unsigned int hashLength = 0;
		if (!EVP_DigestFinal_ex(context, hash, &hashLength)) {
			StandardOut::singleton()->print(MESSAGE_ERROR, "Error during EVP_DigestFinal_ex");
		}

		for (unsigned int i = 0; i < hashLength; i++) {
			char s[3];
			std::snprintf(s, sizeof(s), "%02x", hash[i]);
			result += s;
		}
	}

	return result.c_str();
}

#endif

void MD5HasherImpl::addData(std::istream& data)
{
	char buffer[1024];
	data.clear();
	data.seekg(0, std::ios_base::beg);

	do {
		data.read(buffer, 1024);
		addData(buffer, data.gcount());
	} while (data.gcount() > 0);
}

std::string MD5HasherImpl::toString()
{
	c_str();
	return result;
}

MD5Hasher* MD5Hasher::create()
{
	return new MD5HasherImpl();
}

bool ContentProvider::isRequestQueueEmpty()
{
	std::scoped_lock lock(requestSync);
	return requestQueue.empty();
}

ContentProvider::FailedUrl::FailedUrl(const char* url)
	: url(url),
	  expiration(std::chrono::floor<std::chrono::seconds>(std::chrono::steady_clock::now()) + std::chrono::minutes(5))
{
}

bool ContentProvider::FailedUrl::expired() const
{
	return std::chrono::floor<std::chrono::seconds>(std::chrono::steady_clock::now()) >= expiration;
}

bool ContentProvider::isUrlBad(const char* url)
{
	std::scoped_lock lock(requestSync);
	std::list<FailedUrl>::iterator end = failedUrls.end();
	for (std::list<FailedUrl>::iterator iter = failedUrls.begin(); iter != end; ++iter) {
		if (iter->expired()) {
			failedUrls.erase(iter, end);
			break;
		}
		if (iter->url == url) {
			return true;
		}
	}
	return false;
}

static std::filesystem::path getLocalCachePath(bool createPath)
{
	if (createPath) {
		static std::string s = FileSystem::getCacheDirectory(true);
		return s;
	}
	else {
		static std::string s = FileSystem::getCacheDirectory(false);
		return s;
	}
}

static bool fileExists(const std::filesystem::path& path)
{
	std::error_code error;
	return std::filesystem::exists(path, error);
}

static bool equalsIgnoreCase(std::string a, std::string b)
{
	toLower(a);
	toLower(b);
	return a == b;
}

static std::string findIgnoringCase(const std::string& folder, const std::string& relativePath)
{
	std::filesystem::path result(folder);

	for (const std::filesystem::path& component : std::filesystem::path(relativePath)) {
		std::filesystem::path match = component;

		std::error_code error;
		for (std::filesystem::directory_iterator iter(result, error), end; !error && iter != end;
			 iter.increment(error)) {
			if (equalsIgnoreCase(iter->path().filename().string(), component.string())) {
				match = iter->path().filename();
				break;
			}
		}

		result /= match;
	}

	return result.string();
}

#ifdef _WIN32

void ContentProvider::clearFileCache()
{
	std::string path = getLocalCachePath(false).string();

	if (path != "") {
		WIN32_FIND_DATA FindFileData;
		HANDLE hFind = FindFirstFile((path + "*.*").c_str(), &FindFileData);

		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					DeleteFile((path + FindFileData.cFileName).c_str());
				}
			} while (FindNextFile(hFind, &FindFileData));

			FindClose(hFind);
		}
	}
}

#else

void ContentProvider::clearFileCache()
{
	std::filesystem::path path = getLocalCachePath(false);

	if (!path.empty()) {
		std::error_code error;
		for (std::filesystem::directory_iterator iter(path, error), end; !error && iter != end; iter.increment(error)) {
			std::error_code fileError;
			if (!iter->is_directory(fileError)) {
				std::filesystem::remove(iter->path(), fileError);
			}
		}
	}
}

#endif

ContentId ContentProvider::registerContent(std::istream& content, const Name& mimeType)
{
	ContentId id;
	{
		std::unique_ptr<MD5Hasher> hasher(MD5Hasher::create());
		hasher->addData(content);
		id = ContentId(hasher->toString(), mimeType);
	}
	std::filesystem::path filePath = getLocalCachePath(true) / id.c_str();

	if (!fileExists(filePath)) {
		std::ofstream outStream(filePath, std::ios_base::out | std::ios_base::binary);

		content.clear();
		content.seekg(0, std::ios_base::beg);

		char buffer[1024];
		do {
			content.read(buffer, 1024);
			outStream.write(buffer, content.gcount());
		} while (content.gcount() > 0);
	}

	return id;
}

std::string ContentProvider::findHashFile(ContentId contentId)
{
	std::filesystem::path filePath = getLocalCachePath(true) / contentId.c_str();

	if (!fileExists(filePath)) {
		return "";
	}
	return filePath.string();
}

bool ContentProvider::registerFile(CachedContent* item)
{
	CachedContent temp;
	{
		std::scoped_lock lock(contentCacheMutex);
		temp = *item;
	}

	if (!temp.filename) {
		std::istringstream ss(*temp.data);
		ContentId hash = registerContent(ss, Name::getNullName());
		std::string filename = findHashFile(hash);
		if (filename.empty()) {
			return false;
		}

		temp.filename.reset(new std::string(filename));

		std::scoped_lock lock(contentCacheMutex);
		*item = temp;
	}

	return true;
}

void ContentProvider::clearContentCache()
{
	std::scoped_lock lock(contentCacheMutex);
	contentCache.clear();
}

std::string ContentProvider::findFile(ContentId contentId)
{
	std::string filePath(contentId.c_str() + 7);

	if (!fileExists(filePath)) {
		return "";
	}
	return filePath;
}

std::string ContentProvider::findAsset(ContentId contentId)
{
	std::string filePath = assetFolderPath + (contentId.c_str() + 11);

	if constexpr (std::filesystem::path::preferred_separator == '/') {
		std::replace(filePath.begin(), filePath.end(), '\\', '/');

		if (!fileExists(filePath)) {
			filePath = findIgnoringCase(assetFolderPath, filePath.substr(assetFolderPath.size()));
		}
	}

	if (!fileExists(filePath)) {
		return "";
	}
	return filePath;
}

void ContentProvider::setAssetFolder(const char* sPath)
{
	StandardOut::singleton()->print(MESSAGE_INFO, "setAssetFolder %s", sPath);

	std::filesystem::path path(sPath);
	if (path.is_relative()) {
		throw std::runtime_error(G3D::format("The path '%s' is a relative path", sPath));
	}

	std::error_code error;
	if (!std::filesystem::is_directory(path, error)) {
		throw std::runtime_error(G3D::format("The path '%s' does not exist", sPath));
	}

	assetFolderPath = sPath;
	if (assetFolderPath.back() != std::filesystem::path::preferred_separator) {
		assetFolderPath += (char) std::filesystem::path::preferred_separator;
	}
}

worker_thread::work_result ContentProvider::processRequests()
{
	ContentId id;
	{
		std::scoped_lock lock(requestSync);
		if (requestQueue.empty()) {
			return worker_thread::done;
		}
		id = requestQueue.front();
	}

	try {
		std::shared_ptr<std::string> response(new std::string());
		try {
			Http(id.toString()).get(*response);
		}
		catch (...) {
			std::scoped_lock lock(requestSync);
			failedUrls.push_front(id.c_str());
			throw;
		}

		std::scoped_lock lock(contentCacheMutex);
		contentCache[id].data = response;
	}
	catch (std::exception& e) {
		StandardOut::singleton()->print(MESSAGE_ERROR, "Content failed for %s because %s", id.c_str(), e.what());
	}

	{
		std::scoped_lock lock(requestSync);
		requestQueue.pop_front();
	}

	return worker_thread::more;
}

ContentProvider::CachedContent* ContentProvider::loadContent(ContentId id, HttpRequestType httpRequestType)
{
	if (id.isNull()) {
		return nullptr;
	}

	{
		std::scoped_lock lock(contentCacheMutex);
		ContentCache::iterator iter = contentCache.find(id);
		if (iter != contentCache.end()) {
			return &iter->second;
		}
	}

	if (id.isFile()) {
		std::shared_ptr<const std::string> filename(new std::string(findFile(id)));
		if (filename->empty()) {
			return nullptr;
		}

		std::scoped_lock lock(contentCacheMutex);
		CachedContent& content = contentCache[id];
		content.filename = filename;
		return &content;
	}

	if (id.isAsset()) {
		std::shared_ptr<const std::string> filename(new std::string(findAsset(id)));
		if (filename->empty()) {
			return nullptr;
		}

		std::scoped_lock lock(contentCacheMutex);
		CachedContent& content = contentCache[id];
		content.filename = filename;
		return &content;
	}

	{
		std::shared_ptr<const std::string> filename(new std::string(findHashFile(id)));
		if (!filename->empty()) {
			std::scoped_lock lock(contentCacheMutex);
			CachedContent& content = contentCache[id];
			content.filename = filename;
			return &content;
		}
	}

	if (id.isHttp()) {
		switch (httpRequestType) {
		case AsyncHttpRequest: {
			std::scoped_lock lock(requestSync);
			if (!isUrlBad(id.c_str())) {
				if (std::find(requestQueue.begin(), requestQueue.end(), id) == requestQueue.end()) {
					requestQueue.push_back(id);
				}
			}
			requestProcessor->wake();
		} break;

		case SyncHttpRequest: {
			std::shared_ptr<std::string> response(new std::string());
			try {
				Http(id.toString()).get(*response);
			}
			catch (...) {
				std::scoped_lock lock(requestSync);
				failedUrls.push_front(id.c_str());
				throw;
			}

			std::scoped_lock lock(contentCacheMutex);
			CachedContent& content = contentCache[id];
			content.data = response;
			return &content;
		}
		}
	}

	return nullptr;
}

std::string ContentProvider::getFile(ContentId ticket)
{
	CachedContent* content = loadContent(ticket, SyncHttpRequest);
	if (!content || !registerFile(content)) {
		throw std::runtime_error(G3D::format("Unable to load %s", ticket.c_str()));
	}

	return *content->filename;
}

std::unique_ptr<std::istream> ContentProvider::getContent(ContentId ticket)
{
	CachedContent* content = loadContent(ticket, SyncHttpRequest);
	if (!content) {
		throw std::runtime_error(G3D::format("Unable to load %s", ticket.c_str()));
	}

	if (content->data) {
		return std::unique_ptr<std::istream>(new std::istringstream(*content->data));
	}
	else {
		std::istream* stream = new std::ifstream(content->filename->c_str(), std::ios_base::in | std::ios_base::binary);
		return std::unique_ptr<std::istream>(stream);
	}
}

ContentProvider::~ContentProvider()
{
	requestProcessor->join();
}

std::string ContentProvider::getAssetFile(const std::string& filePath)
{
	return getFile(ContentId::fromAssets(filePath));
}

bool ContentProvider::hasContent(ContentId id)
{
	return loadContent(id, NoHttpRequest) != nullptr;
}

bool ContentProvider::requestContentFile(ContentId id, std::string& filename)
{
	CachedContent* content = loadContent(id, AsyncHttpRequest);
	if (!content) {
		return false;
	}

	if (!registerFile(content)) {
		return false;
	}

	filename = *content->filename;
	return true;
}

std::shared_ptr<const std::string> ContentProvider::requestContentString(ContentId id)
{
	CachedContent* content = loadContent(id, AsyncHttpRequest);
	if (!content) {
		return std::shared_ptr<const std::string>();
	}

	std::scoped_lock lock(contentCacheMutex);
	if (!content->data) {
		std::ifstream stream(content->filename->c_str(), std::ios_base::in | std::ios_base::binary);

		content->data.reset(new std::string(readStream(stream)));
	}
	return content->data;
}

ContentId ContentProvider::readContent(const char* id, std::istream& stream, unsigned long dataLength)
{
	ContentId result(id);

	if (hasContent(result)) {
		stream.clear();
		stream.seekg(dataLength, std::ios_base::cur);
	}
	else {
		std::filesystem::path filePath = getLocalCachePath(true) / result.c_str();

		std::ofstream content;
		content.open(filePath, std::ios_base::out | std::ios_base::binary);

		for (; dataLength > 0; --dataLength) {
			char data;
			stream.get(data);
			content << data;
		}
	}

	return result;
}

void ContentProvider::load(ContentId id, std::vector<std::shared_ptr<Instance>>& instances)
{
	std::unique_ptr<std::istream> stream(getContent(id));

	TextXmlParser machine(stream->rdbuf());
	std::unique_ptr<XmlElement> root(machine.parse());

	SerializerV2().loadInstances(root.get(), instances);
}

ContentProvider::ContentProvider()
{
	std::string path = FileSystem::getContentDirectory();
	if (!path.empty()) {
		setAssetFolder(path.c_str());
	}
	else {
		StandardOut::singleton()->print(MESSAGE_WARNING, "Failed to get asset folder");
	}

	requestProcessor.reset(new worker_thread([this] { return processRequests(); }, "rbx_content"));
}

} // namespace RBX
