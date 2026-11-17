#include "util/Thread.h"

#include "util/standardout.h"

#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace RBX {

static thread_local std::unique_ptr<std::string> threadName;

#if defined(_DEBUG) && defined(_WIN32)
static void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName)
{
	struct THREADNAME_INFO
	{
		DWORD dwType;
		LPCSTR szName;
		DWORD dwThreadID;
		DWORD dwFlags;
	};

	THREADNAME_INFO info = {0x1000, szThreadName, dwThreadID, 0};

	__try {
		RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), reinterpret_cast<ULONG_PTR*>(&info));
	}
	__except (EXCEPTION_CONTINUE_EXECUTION) {
	}
}
#endif

#ifdef _WIN32
static const int backgroundPriority = THREAD_PRIORITY_BELOW_NORMAL;

class ThreadPrioritySetter
{
	const int old;
	const bool success;

public:
	explicit ThreadPrioritySetter(int priority)
		: old(GetThreadPriority(GetCurrentThread())), success(SetThreadPriority(GetCurrentThread(), priority) == TRUE)
	{
	}

	ThreadPrioritySetter(const ThreadPrioritySetter&) = delete;
	ThreadPrioritySetter& operator=(const ThreadPrioritySetter&) = delete;

	~ThreadPrioritySetter()
	{
		if (success) {
			SetThreadPriority(GetCurrentThread(), old);
		}
	}
};
#else
static const int backgroundPriority = 1;

class ThreadPrioritySetter
{
	const int old;
	const bool success;

public:
	explicit ThreadPrioritySetter(int niceIncrement)
		: old(getpriority(PRIO_PROCESS, gettid())),
		  success(setpriority(PRIO_PROCESS, gettid(), old + niceIncrement) == 0)
	{
	}

	ThreadPrioritySetter(const ThreadPrioritySetter&) = delete;
	ThreadPrioritySetter& operator=(const ThreadPrioritySetter&) = delete;

	~ThreadPrioritySetter()
	{
		if (success) {
			setpriority(PRIO_PROCESS, gettid(), old);
		}
	}
};
#endif

void set_thread_name(const char* name)
{
	threadName = std::make_unique<std::string>(name);
#if defined(_DEBUG) && defined(_WIN32)
	SetThreadName(GetCurrentThreadId(), name);
#endif
}

const char* get_thread_name()
{
	return threadName ? threadName->c_str() : "rbx_thread";
}

static void run_in_background(const std::function<void()>& threadfunc, const std::string& name)
{
	try {
		set_thread_name(name.c_str());
		ThreadPrioritySetter set(backgroundPriority);
		threadfunc();
	}
	catch (std::exception& e) {
		StandardOut::singleton()->print(MESSAGE_ERROR, "Crashed via C++ exception in run_in_background:", e.what());
	}
}

std::function<void()> background_function(const std::function<void()>& threadfunc, const char* name)
{
	return [threadfunc, name = std::string(name)] { run_in_background(threadfunc, name); };
}

worker_thread::worker_thread(const std::function<work_result()>& work_function, const char* name)
	: _data(std::make_shared<data>()),
	  thread(background_function([data = _data, work_function] { threadProc(data, work_function); }, name))
{
}

worker_thread::~worker_thread()
{
	{
		std::scoped_lock lock(_data->sync);
		_data->endRequest = true;
		_data->wakeCondition.notify_all();
	}

	if (thread.joinable()) {
		thread.detach();
	}
}

void worker_thread::join()
{
	{
		std::scoped_lock lock(_data->sync);
		_data->endRequest = true;
		_data->wakeCondition.notify_all();
	}

	thread.join();
}

void worker_thread::wake()
{
	std::scoped_lock lock(_data->sync);
	_data->wakeCondition.notify_all();
}

void worker_thread::threadProc(const std::shared_ptr<data>& data, const std::function<work_result()>& work_function)
{
	while (true) {
		{
			std::scoped_lock lock(data->sync);

			if (data->endRequest) {
				return;
			}
		}

		if (work_function() != more) {
			std::unique_lock lock(data->sync);

			if (!data->endRequest) {
				data->wakeCondition.wait(lock);
			}
		}
	}
}

} // namespace RBX
