#ifndef UTIL_PROFILING_H
#define UTIL_PROFILING_H

#include <cstddef>
#include <cstdint>
#include <string>

#ifndef _WIN32
#include <pthread.h>
#endif

namespace RBX::Profiling {

#ifdef _WIN32
using ThreadHandle = void*;
#else
using ThreadHandle = pthread_t;
#endif

void init(bool enabled);
ThreadHandle currentThread();

struct Bucket
{
public:
	Bucket() : sampleTimeSpan(0.0), kernTimeSpan(0), userTimeSpan(0), frames(0) {}

	double getActualFPS() const;
	double getNominalFPS() const;
	double getFrameTime() const;
	double getTotalTime() const;

	Bucket& operator+=(const Bucket& other)
	{
		sampleTimeSpan += other.sampleTimeSpan;
		kernTimeSpan += other.kernTimeSpan;
		userTimeSpan += other.userTimeSpan;
		frames += other.frames;

		return *this;
	}

	double sampleTimeSpan;
	std::int64_t kernTimeSpan;
	std::int64_t userTimeSpan;
	int frames;
};

class Profiler
{
public:
	Profiler(const Profiler&) = delete;
	Profiler& operator=(const Profiler&) = delete;

	Profiler(const char* name);

	Bucket getData(double seconds) const;

protected:
	static const unsigned int bucketCount = 4096;

	const double bucketTimeSpan;
	int currentBucket;
	Bucket buckets[bucketCount];
	double lastSampleTime;

public:
	const std::string name;
};

class CodeProfiler : public Profiler
{
public:
	CodeProfiler(const char* name);

	CodeProfiler* parent;

private:
	void log(std::int64_t kern, std::int64_t user, bool frameTick);

	friend class Mark;
};

class ThreadProfiler : public Profiler
{
public:
	ThreadProfiler(const char* name);

	void sample(ThreadHandle thread);

private:
	bool initialized;
};

class Mark
{
public:
	Mark(CodeProfiler& section, bool frameTick);
	~Mark();

	static bool enabled;

private:
	void begin();
	void end();

	CodeProfiler& section;
	CodeProfiler* enclosingSection;
	std::int64_t kernelTime;
	std::int64_t userTime;
	bool frameTick;
};

inline Mark::Mark(CodeProfiler& section, bool frameTick) : section(section), frameTick(frameTick)
{
	if (enabled) {
		begin();
	}
}

inline Mark::~Mark()
{
	if (enabled) {
		end();
	}
}

} // namespace RBX::Profiling

#endif // UTIL_PROFILING_H
