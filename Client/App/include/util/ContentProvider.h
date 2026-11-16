#ifndef UTIL_CONTENTPROVIDER_H
#define UTIL_CONTENTPROVIDER_H

#include "util/Name.h"
#include "util/Thread.h"

#include <chrono>
#include <iosfwd>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace RBX {

class Instance;

class ContentId
{
public:
	static ContentId fromUrl(const std::string&);
	static ContentId fromFile(const std::string&);
	static ContentId fromAssets(const std::string& id);
	static ContentId fromMD5Hash(const std::string&);

	ContentId(const std::string& id, const Name& mimeType) : id(id), mimeTypePtr(&mimeType) {}

	ContentId(const char* id) : id(id), mimeTypePtr(&Name::getNullName()) {}

	ContentId(const std::string& id) : id(id), mimeTypePtr(&Name::getNullName()) {}

	ContentId() : mimeTypePtr(&Name::getNullName()) {}

	const Name& mimeType() const { return *mimeTypePtr; }

	const char* c_str() const { return id.c_str(); }

	const std::string& toString() const { return id; }

	bool isNull() const { return id.size() == 0; }

	bool isFile() const { return id.starts_with("file://"); }

	bool isAsset() const { return id.starts_with("rbxasset://"); }

	bool isHttp() const { return id.starts_with("http"); }

private:
	std::string id;
	const Name* mimeTypePtr;
};

bool operator<(const ContentId& a, const ContentId& b);
bool operator!=(const ContentId& a, const ContentId& b);
bool operator==(const ContentId& a, const ContentId& b);

class __declspec(novtable) MD5Hasher
{
public:
	static MD5Hasher* create();

	virtual void addData(std::istream& data) = 0;
	virtual void addData(const std::string& data) = 0;
	virtual void addData(const char* data, unsigned int nBytes) = 0;
	virtual std::string toString() = 0;
	virtual const char* c_str() = 0;

	static std::string convertToLegacyHash(std::string hash);
};

class ContentProvider
{
	std::recursive_mutex requestSync;
	std::list<ContentId> requestQueue;

	struct FailedUrl
	{
		std::string url;
		std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds> expiration;

		FailedUrl(const char* url);

		bool expired() const;
	};

	std::list<FailedUrl> failedUrls;
	std::mutex contentCacheMutex;

	struct CachedContent
	{
		std::shared_ptr<const std::string> data;
		std::shared_ptr<const std::string> filename;
	};

	using ContentCache = std::map<ContentId, CachedContent>;

	ContentCache contentCache;
	std::string assetFolderPath;
	std::unique_ptr<worker_thread> requestProcessor;

	ContentProvider();
	~ContentProvider();

public:
	ContentProvider(const ContentProvider&) = delete;
	ContentProvider& operator=(const ContentProvider&) = delete;

	static ContentProvider& singleton()
	{
		static ContentProvider sing;
		return sing;
	}

	ContentId registerContent(const char* filename, const Name& mimeType);
	ContentId registerContent(std::istream& content, const Name& mimeType);

	void clearFileCache();

	bool isUrlBad(const char* url);

	void load(ContentId id, std::vector<std::shared_ptr<Instance>>& instances);

	void clearContentCache();

	bool isRequestQueueEmpty();

	bool hasContent(ContentId id);

	std::shared_ptr<const std::string> requestContentString(ContentId id);

	bool requestContentFile(ContentId id, std::string& filename);

	std::shared_ptr<const std::string> getContentString(ContentId id);

	std::unique_ptr<std::istream> getContent(ContentId ticket);

	std::string getFile(ContentId id);

	std::string getAssetFile(const std::string& id);

	void setAssetFolder(const char* path);

	std::string assetFolder() const;

	static bool isUrl(const std::string& s);
	static bool isHttpUrl(const std::string& s);

	ContentId readContent(const char* url, std::istream& stream, unsigned long size);

	enum HttpRequestType
	{
		NoHttpRequest,
		AsyncHttpRequest,
		SyncHttpRequest
	};

private:
	CachedContent* loadContent(ContentId id, HttpRequestType requestType);
	worker_thread::work_result processRequests();
	std::string findFile(ContentId id);
	std::string findAsset(ContentId id);
	std::string findHashFile(ContentId id);
	bool registerFile(CachedContent* content);
};

} // namespace RBX

#endif // UTIL_CONTENTPROVIDER_H
