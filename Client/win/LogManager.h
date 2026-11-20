#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include "util/Log.h"
#include "util/Thread.h"

#include <exception>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#include "network/CrashReporter.h"

#include <imagehlp.h>
#include <windows.h>
#endif

class MainLogManager;

class LogManager
{
	RBX::Log* log;
	static bool logsEnabled;

protected:
	const unsigned long threadID;
	std::string name;
	static MainLogManager* mainLogManager;

public:
	enum EventType
	{
		EventSuccess,
		EventError,
		EventWarning,
		EventInformation,
		EventAuditSuccess,
		EventAuditFailure,
	};

	RBX::Log* getLog();
	static MainLogManager* getMainLogManger();
	static void ReportEvent(EventType type, const char* message);
	static unsigned long currentThreadId();
	std::string GetLogPath() const;
	virtual ~LogManager();
	virtual std::string getLogFileName() = 0;

protected:
	LogManager(const char* name) : log(nullptr), threadID(currentThreadId()), name(name) {}
};

#ifdef _WIN32
class RobloxCrashReporter : public CrashReporter
{
public:
	static bool silent;

	RobloxCrashReporter(const char* outputPath, const char* appName, MINIDUMP_TYPE mdType);

	LONG ProcessException(_EXCEPTION_POINTERS* info) override;
};
#endif

class MainLogManager : public RBX::ILogProvider, public LogManager
{
#ifdef _WIN32
	std::unique_ptr<RobloxCrashReporter> crashReporter;
#endif

public:
	MainLogManager(const char* productName);
	~MainLogManager() override;

	RBX::Log* provideLog() override;
	std::string getLogFileName() override;

	std::string MakeLogFileName(const char* prefix);

	bool hasErrorLogs() const;
	bool hasCrashLogs() const;
	std::vector<std::string> archiveCrashLogs();
	void CullLogs(const char* folder, int maxLogs);
#ifdef _WIN32
	void WriteCrashDump();
#endif

private:
	std::string guid;

	static bool handleDebugAssert(
		const char* expression,
		const std::string& message,
		const char* filename,
		int lineNumber,
		bool& ignoreAlways,
		bool useGuiPrompt
	);
	static bool handleG3DFailure(
		const char* expression,
		const std::string& message,
		const char* filename,
		int lineNumber,
		bool& ignoreAlways,
		bool useGuiPrompt
	);
};

class ThreadLogManager : public LogManager
{
	ThreadLogManager() : LogManager(RBX::get_thread_name()) {}

public:
	static ThreadLogManager* getCurrent();

	~ThreadLogManager() override = default;

protected:
	std::string getLogFileName() override;
};

void printException(const std::exception& e);

#endif // LOGMANAGER_H
