#include "util/Profiling.h"

#include <G3D/System.h>
#include <algorithm>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <ctime>
#include <sys/resource.h>
#endif

namespace RBX::Profiling {

bool Mark::enabled = false;

static thread_local CodeProfiler* currentSection = nullptr;

#ifdef _WIN32

ThreadHandle currentThread()
{
	return GetCurrentThread();
}

static bool getThreadTimes(ThreadHandle thread, std::int64_t& kernel, std::int64_t& user)
{
	FILETIME creationTime;
	FILETIME exitTime;
	FILETIME kernelTime;
	FILETIME userTime;

	if (!GetThreadTimes(thread, &creationTime, &exitTime, &kernelTime, &userTime)) {
		return false;
	}

	kernel = static_cast<std::int64_t>(
		(static_cast<std::uint64_t>(kernelTime.dwHighDateTime) << 32) | kernelTime.dwLowDateTime
	);
	user =
		static_cast<std::int64_t>((static_cast<std::uint64_t>(userTime.dwHighDateTime) << 32) | userTime.dwLowDateTime);

	return true;
}

#else

ThreadHandle currentThread()
{
	return pthread_self();
}

static bool getThreadTimes(ThreadHandle thread, std::int64_t& kernel, std::int64_t& user)
{
	if (pthread_equal(thread, pthread_self())) {
		rusage usage;

		if (getrusage(RUSAGE_THREAD, &usage) != 0) {
			return false;
		}

		kernel = usage.ru_stime.tv_sec * 10000000LL + usage.ru_stime.tv_usec * 10LL;
		user = usage.ru_utime.tv_sec * 10000000LL + usage.ru_utime.tv_usec * 10LL;

		return true;
	}

	if (thread == ThreadHandle()) {
		return false;
	}

	clockid_t clock;
	timespec time;

	if (pthread_getcpuclockid(thread, &clock) != 0 || clock_gettime(clock, &time) != 0) {
		return false;
	}

	kernel = 0;
	user = time.tv_sec * 10000000LL + time.tv_nsec / 100;

	return true;
}

#endif

static bool getThreadTimes(std::int64_t& kernel, std::int64_t& user)
{
	return getThreadTimes(currentThread(), kernel, user);
}

void init(bool enabled)
{
	Mark::enabled = enabled;
}

void Mark::begin()
{
	enclosingSection = currentSection;
	getThreadTimes(kernelTime, userTime);
	currentSection = &section;
}

void Mark::end()
{
	currentSection = enclosingSection;

	std::int64_t endKernelTime = kernelTime;
	std::int64_t endUserTime = userTime;
	getThreadTimes(endKernelTime, endUserTime);

	const std::int64_t kernel = endKernelTime - kernelTime;
	const std::int64_t user = endUserTime - userTime;

	section.log(kernel, user, frameTick);

	if (enclosingSection != nullptr && enclosingSection != &section && enclosingSection != section.parent) {
		enclosingSection->log(-kernel, -user, false);
	}
}

void ThreadProfiler::sample(ThreadHandle thread)
{
	double time = G3D::System::getTick();

	if (time >= lastSampleTime + bucketTimeSpan) {
		std::int64_t kern;
		std::int64_t user;

		if (getThreadTimes(thread, kern, user)) {
			if (initialized) {
				buckets[currentBucket].sampleTimeSpan = time - lastSampleTime;
				buckets[currentBucket].kernTimeSpan += kern;
				buckets[currentBucket].userTimeSpan += user;

				currentBucket = (currentBucket + 1) % bucketCount;
			}
			else {
				initialized = true;
			}

			lastSampleTime = time;

			buckets[currentBucket].kernTimeSpan = -kern;
			buckets[currentBucket].userTimeSpan = -user;
		}
	}
}

void CodeProfiler::log(std::int64_t kern, std::int64_t user, bool frameTick)
{
	double now = G3D::System::getTick();

	if (now >= lastSampleTime + bucketTimeSpan) {
		buckets[currentBucket].sampleTimeSpan = now - lastSampleTime;

		currentBucket = (currentBucket + 1) % bucketCount;

		buckets[currentBucket].frames = frameTick ? 1 : 0;
		buckets[currentBucket].kernTimeSpan = kern;
		buckets[currentBucket].userTimeSpan = user;

		lastSampleTime = now;
	}
	else {
		if (frameTick) {
			buckets[currentBucket].frames++;
		}

		buckets[currentBucket].kernTimeSpan += kern;
		buckets[currentBucket].userTimeSpan += user;
	}
}

double Bucket::getActualFPS() const
{
	if (sampleTimeSpan > 0.0) {
		return frames / sampleTimeSpan;
	}

	return 0.0;
}

double Bucket::getFrameTime() const
{
	return getTotalTime() / frames;
}

double Bucket::getTotalTime() const
{
	return (kernTimeSpan + userTimeSpan) * 1e-7;
}

Profiler::Profiler(const char* name)
	: bucketTimeSpan(1.0), currentBucket(0), lastSampleTime(G3D::System::getTick()), name(name)
{
}

CodeProfiler::CodeProfiler(const char* name) : Profiler(name), parent(nullptr)
{
}

ThreadProfiler::ThreadProfiler(const char* name) : Profiler(name), initialized(false)
{
}

Bucket Profiler::getData(double seconds) const
{
	Bucket result;

	if (Mark::enabled) {
		double span = seconds - (G3D::System::getTick() - lastSampleTime);

		unsigned int firstBucket = currentBucket + bucketCount - 1;

		unsigned int count = std::min((unsigned int) (span / bucketTimeSpan), bucketCount - 2);

		for (unsigned int i = 0; i < count; i++) {
			if (result.sampleTimeSpan >= span) {
				break;
			}

			result += buckets[(firstBucket - i) % bucketCount];
		}
	}

	return result;
}

double Bucket::getNominalFPS() const
{
	return frames / getTotalTime();
}

} // namespace RBX::Profiling
