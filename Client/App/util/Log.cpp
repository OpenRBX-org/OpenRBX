#include "util/Log.h"

#include <chrono>
#include <cstdio>
#include <ctime>

namespace RBX {

Log::Severity Log::aggregateWorstSeverity = Log::Information;
ILogProvider* Log::provider;

static const char* information = "          ";
static const char* warning = " Warning: ";
static const char* error = " Error:   ";

void Log::setLogProvider(ILogProvider* provider)
{
	Log::provider = provider;
}

std::string Log::formatMem(unsigned int bytes)
{
	char buffer[64];

	if (bytes < 1000) {
		sprintf(buffer, "%dB", bytes);
	}
	else if (bytes < 1000000) {
		sprintf(buffer, "%dKB", bytes / 1000);
	}
	else if (bytes < 1000000000) {
		sprintf(buffer, "%dMB", bytes / 1000000);
	}
	else {
		sprintf(buffer, "%dGB", bytes / 1000000000);
	}

	return std::string(buffer);
}

std::string Log::formatTime(double time)
{
	char buffer[64];

	if (time == 0.0) {
		sprintf(buffer, "0s");
	}

	if (time < 0.0) {
		sprintf(buffer, "%.3gs", time);
	}
	else if (time >= 0.1) {
		sprintf(buffer, "%.3gs", time);
	}
	else {
		sprintf(buffer, "%.3gms", time * 1000.0);
	}

	return std::string(buffer);
}

} // namespace RBX

static std::tm localTime(int& milliseconds)
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t time = std::chrono::system_clock::to_time_t(now);
	milliseconds = (int) (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000);

	std::tm local;
#ifdef _WIN32
	localtime_s(&local, &time);
#else
	localtime_r(&time, &local);
#endif

	return local;
}

static void timeStamp(std::ofstream& stream, bool includeDate)
{
	int milliseconds;
	std::tm local = localTime(milliseconds);

	char s[256];

	if (includeDate) {
		sprintf(s, "%02d.%02d.%d ", local.tm_mday, local.tm_mon + 1, local.tm_year + 1900);
		stream << s;
	}

	sprintf(s, "%02d:%02d.%03d ", local.tm_hour, local.tm_min, milliseconds);
	stream << s;
	stream.flush();
}

namespace RBX {

Log::Log(const char* logFile, const char* name) : worstSeverity(Information), logFile(logFile), stream(logFile)
{
	::timeStamp(stream, true);
	stream << "Log \"" << name << "\"\n";
	stream.flush();
}

Log::~Log()
{
	::timeStamp(stream, true);
	stream << "End Log\n";
}

void Log::writeEntry(Severity severity, const char* message)
{
	std::ofstream* out = currentStream();

	int milliseconds;
	std::tm now = ::localTime(milliseconds);

	char stamp[256];
	sprintf(stamp, "%02d:%02d.%03d ", now.tm_hour, now.tm_min, milliseconds);

	*out << stamp;
	out->flush();

	switch (severity) {
	case Error:
		*currentStream() << error;
	case Warning:
		*currentStream() << warning;
	case Information:
		*currentStream() << information;
	}

	*currentStream() << message;
	stream << '\n';
	currentStream()->flush();
}

std::string Log::timeStamp()
{
	return std::string();
}

std::ofstream* Log::currentStream()
{
	return &provider->provideLog()->stream;
}

} // namespace RBX
