#ifndef UTIL_THREAD_H
#define UTIL_THREAD_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace RBX {

void set_thread_name(const char* name);
const char* get_thread_name();

std::function<void()> background_function(const std::function<void()>& threadfunc, const char* name);

class worker_thread
{
	struct data
	{
		std::mutex sync;
		std::condition_variable wakeCondition;
		bool endRequest = false;
	};

	std::shared_ptr<data> _data;
	std::thread thread;

public:
	enum work_result
	{
		done,
		more
	};

	worker_thread(const std::function<work_result()>& work_function, const char* name);
	worker_thread(const worker_thread&) = delete;
	worker_thread& operator=(const worker_thread&) = delete;
	~worker_thread();

	void wake();
	void join();

private:
	static void threadProc(const std::shared_ptr<data>& data, const std::function<work_result()>& work_function);
};

} // namespace RBX

#endif // UTIL_THREAD_H
