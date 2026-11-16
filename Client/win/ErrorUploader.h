#ifndef ERRORUPLOADER_H
#define ERRORUPLOADER_H

#include "util/Thread.h"

#include <memory>
#include <mutex>
#include <queue>
#include <string>

class ErrorUploader
{
	struct data
	{
		std::queue<std::string> files;
		std::recursive_mutex sync;
		std::string url;
	};

	std::shared_ptr<data> _data;
	RBX::worker_thread thread;

public:
	ErrorUploader() : _data(new data()), thread([_data = _data] { return run(_data); }, "rbx_erroruploader") {}

	void Cancel();
	void Upload(std::string url, bool synchronous);
	bool IsUploading() { return !_data->files.empty(); }

private:
	static RBX::worker_thread::work_result run(std::shared_ptr<data> _data);
};

#endif // ERRORUPLOADER_H
