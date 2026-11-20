#include "LogManager.h"

#include "VersionInfo.h"
#include "util/FileSystem.h"
#include "util/Guid.h"
#include "util/standardout.h"

#include <G3D/Log.h>
#include <G3D/debugAssert.h>
#include <G3D/format.h>
#include <algorithm>
#include <filesystem>
#include <list>
#include <memory>

#ifdef _WIN32
#include <direct.h>

static const char pathSeparator = '\\';
#else
#include <unistd.h>

static const char pathSeparator = '/';
#endif

bool LogManager::logsEnabled = true;

MainLogManager* LogManager::mainLogManager;
#ifdef _WIN32
bool RobloxCrashReporter::silent;
#endif

unsigned long LogManager::currentThreadId()
{
#ifdef _WIN32
	return GetCurrentThreadId();
#else
	return static_cast<unsigned long>(gettid());
#endif
}

MainLogManager* LogManager::getMainLogManger()
{
	return mainLogManager;
}

std::string LogManager::GetLogPath() const
{
	return RBX::FileSystem::getUserAppDataDirectory(true, "logs");
}

static std::string logFolder(const std::string& path)
{
	if (!path.empty() && path.back() != pathSeparator) {
		return path + pathSeparator;
	}
	return path;
}

std::string MainLogManager::getLogFileName()
{
	std::string fileName = "log_";
	fileName += guid;

	fileName.erase(12);
	fileName.erase(4, 3);
	fileName += ".txt";

	return logFolder(GetLogPath()) + fileName;
}

std::string MainLogManager::MakeLogFileName(const char* prefix)
{
	std::string fileName = "log_";
	fileName += guid;

	fileName.erase(12);
	fileName.erase(4, 3);
	fileName += prefix;
	fileName += ".txt";

	return logFolder(GetLogPath()) + fileName;
}

std::string ThreadLogManager::getLogFileName()
{
	std::string fileName = mainLogManager->getLogFileName();
	fileName.insert(fileName.size() - 4, G3D::format("_%s_%lu", name.c_str(), threadID));
	return fileName;
}

RBX::Log* LogManager::getLog()
{
	if (!logsEnabled) {
		return nullptr;
	}

	if (log == nullptr) {
		log = new RBX::Log(getLogFileName().c_str(), name.c_str());
	}

	return log;
}

RBX::Log* MainLogManager::provideLog()
{
	if (currentThreadId() == threadID) {
		return getLog();
	}
	else {
		return ThreadLogManager::getCurrent()->getLog();
	}
}

#ifdef _WIN32
RobloxCrashReporter::RobloxCrashReporter(const char* outputPath, const char* appName, MINIDUMP_TYPE mdType)
{
	controls.actionToTake = AOC_WRITE_TO_DISK | AOC_SILENT_MODE;
	controls.minidumpType = mdType;
	strcpy(controls.pathToMinidump, outputPath);

	strcpy(controls.appName, appName);

	strcpy(controls.appVersion, fileVersionString().c_str());
}

LONG RobloxCrashReporter::ProcessException(_EXCEPTION_POINTERS* info)
{
	LONG result = CrashReporter::ProcessException(info);
	static bool showedMessage = silent;
	if (!showedMessage) {
		showedMessage = true;
		MessageBox(
			nullptr, "An unexpected error occurred and ROBLOX needs to quit.  We're sorry!", "ROBLOX Crash", MB_OK
		);
	}

	return result;
}

void MainLogManager::WriteCrashDump()
{
	std::string appName = "log_";
	appName += guid;
	appName.erase(12);
	appName.erase(4, 3);
	crashReporter.reset(new RobloxCrashReporter(GetLogPath().c_str(), appName.c_str(), MiniDumpWithDataSegs));
	crashReporter->Start();
}

static void __cdecl purecallHandler()
{
	LogManager::ReportEvent(LogManager::EventError, "Pure Call Error");

	int* p = nullptr;
	*p = p[1];
}
#endif

MainLogManager::MainLogManager(const char* productName) : LogManager(productName)
{
	RBX::Guid::generateGUID(guid);
	CullLogs("", 32);
	CullLogs("error", 32);
	CullLogs("archive", 32);

	std::string g3dLogFile = getLogFileName();
	g3dLogFile.erase(g3dLogFile.size() - 4);
	g3dLogFile += "_G3D.txt";
	new G3D::Log(g3dLogFile);

	mainLogManager = this;
	RBX::Log::setLogProvider(this);

	G3D::setAssertionHook(handleDebugAssert);
	G3D::setFailureHook(handleG3DFailure);

#ifdef _WIN32
	_set_purecall_handler(purecallHandler);
#endif
}

std::string MoveRelative(const char* fileName, std::string path)
{
	std::filesystem::path source(fileName);
	std::filesystem::path dir = source.parent_path() / path;
	std::string target = (dir / source.filename()).string();
#ifdef _WIN32
	if (!std::filesystem::exists(dir)) {
		_mkdir(dir.string().c_str());
	}
	MoveFile(fileName, target.c_str());
#else
	std::error_code error;
	std::filesystem::create_directory(dir, error);
	std::filesystem::rename(source, target, error);
#endif
	return target;
}

#ifdef _WIN32
bool MainLogManager::hasCrashLogs() const
{
	std::string path = logFolder(GetLogPath());

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile((path + "*.dmp").c_str(), &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				return true;
			}
		} while (FindNextFile(hFind, &FindFileData));
	}

	return false;
}

std::vector<std::string> MainLogManager::archiveCrashLogs()
{
	std::vector<std::string> result;

	std::string path = logFolder(GetLogPath());

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile((path + "*.dmp").c_str(), &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				result.push_back(MoveRelative((path + FindFileData.cFileName).c_str(), "archive"));

				std::string wildCard = FindFileData.cFileName;
				wildCard = wildCard.substr(0, 9) + "*.*";

				WIN32_FIND_DATA FindOtherFileData;
				HANDLE hFindOther = FindFirstFile((path + wildCard).c_str(), &FindOtherFileData);
				if (hFindOther != INVALID_HANDLE_VALUE) {
					do {
						if (!(FindOtherFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
							result.push_back(MoveRelative((path + FindOtherFileData.cFileName).c_str(), "archive"));
						}
					} while (FindNextFile(hFindOther, &FindOtherFileData));
				}
			}
		} while (FindNextFile(hFind, &FindFileData));
	}

	return result;
}

struct DateCompare
{
	FILETIME fileTime;

	DateCompare(const WIN32_FIND_DATA& data) : fileTime(data.ftCreationTime) {}

	bool operator()(WIN32_FIND_DATA data) const { return CompareFileTime(&fileTime, &data.ftCreationTime) < 0; }
};

void MainLogManager::CullLogs(const char* folder, int maxLogs)
{
	std::string path = logFolder(logFolder(GetLogPath()) + folder);

	std::list<WIN32_FIND_DATA> files;

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile((path + "*.*").c_str(), &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				files.insert(std::find_if(files.begin(), files.end(), DateCompare(FindFileData)), FindFileData);
			}
		} while (FindNextFile(hFind, &FindFileData));
	}

	std::list<WIN32_FIND_DATA>::iterator iter = files.begin();
	for (size_t i = maxLogs; i < files.size(); ++i) {
		DeleteFile((path + iter->cFileName).c_str());
		++iter;
	}
}
#else
static std::vector<std::filesystem::path> crashDumps(const std::string& path)
{
	std::vector<std::filesystem::path> dumps;
	std::error_code error;
	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path, error)) {
		if (entry.is_regular_file() && entry.path().extension() == ".dmp") {
			dumps.push_back(entry.path());
		}
	}
	return dumps;
}

bool MainLogManager::hasCrashLogs() const
{
	return !crashDumps(logFolder(GetLogPath())).empty();
}

std::vector<std::string> MainLogManager::archiveCrashLogs()
{
	std::vector<std::string> result;

	std::string path = logFolder(GetLogPath());
	for (const std::filesystem::path& dump : crashDumps(path)) {
		result.push_back(MoveRelative(dump.string().c_str(), "archive"));

		std::string prefix = dump.filename().string().substr(0, 9);
		std::error_code error;
		std::vector<std::filesystem::path> others;
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path, error)) {
			if (entry.is_regular_file() && entry.path().filename().string().starts_with(prefix)) {
				others.push_back(entry.path());
			}
		}
		for (const std::filesystem::path& other : others) {
			result.push_back(MoveRelative(other.string().c_str(), "archive"));
		}
	}

	return result;
}

void MainLogManager::CullLogs(const char* folder, int maxLogs)
{
	std::string path = logFolder(logFolder(GetLogPath()) + folder);

	std::vector<std::filesystem::directory_entry> files;
	std::error_code error;
	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path, error)) {
		if (entry.is_regular_file()) {
			files.push_back(entry);
		}
	}
	std::stable_sort(files.begin(), files.end(), [](const auto& left, const auto& right) {
		return left.last_write_time() < right.last_write_time();
	});

	for (size_t i = maxLogs; i < files.size(); ++i) {
		std::filesystem::remove(files[i - maxLogs].path(), error);
	}
}
#endif

MainLogManager::~MainLogManager()
{
	if (G3D::Log::common()) {
		std::string g3dLogFile = G3D::Log::getCommonLogFilename();
		delete G3D::Log::common();

		if (RBX::Log::aggregateWorstSeverity == RBX::Log::Error) {
			MoveRelative(g3dLogFile.c_str(), "error");
		}
		else {
			MoveRelative(g3dLogFile.c_str(), "archive");
		}
	}

	mainLogManager = nullptr;
}

LogManager::~LogManager()
{
	if (log) {
		std::string logFile = log->logFile;
		RBX::Log::Severity severity = log->worstSeverity;
		delete log;
		log = nullptr;

		if (severity == RBX::Log::Error) {
			MoveRelative(logFile.c_str(), "error");
		}
		else {
			MoveRelative(logFile.c_str(), "archive");
		}
	}
}

bool MainLogManager::handleDebugAssert(
	const char* expression,
	const std::string& message,
	const char* filename,
	int lineNumber,
	bool& ignoreAlways,
	bool useGuiPrompt
)
{
	std::string eventMessage;
	if (message.length() > 0) {
		eventMessage = G3D::format(
			"Assertion failed: %s\n\nMessage: %s\n\n%s(%d)", expression, message.c_str(), filename, lineNumber
		);
	}
	else {
		eventMessage = G3D::format("Assertion failed: %s\n%s(%d)", expression, filename, lineNumber);
	}
	ReportEvent(EventWarning, eventMessage.c_str());

	return false;
}

bool MainLogManager::handleG3DFailure(
	const char* expression,
	const std::string& message,
	const char* filename,
	int lineNumber,
	bool& ignoreAlways,
	bool useGuiPrompt
)
{
	std::string eventMessage;
	if (message.length() > 0) {
		eventMessage =
			G3D::format("G3D Error: %s\n\nMessage: %s\n\n%s(%d)", expression, message.c_str(), filename, lineNumber);
	}
	else {
		eventMessage = G3D::format("G3D Error: %s\n%s(%d)", expression, filename, lineNumber);
	}
	ReportEvent(EventError, eventMessage.c_str());

	int* p = nullptr;
	*p = p[1];

	return false;
}

void printException(const std::exception& e)
{
	RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "%s", e.what());
}

void LogManager::ReportEvent(EventType type, const char* message)
{
	switch (type) {
	case EventSuccess:
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "%s", message);
		break;
	case EventError:
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "%s", message);
		break;
	case EventInformation:
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "%s", message);
		break;
	case EventAuditSuccess:
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "%s", message);
		break;
	case EventAuditFailure:
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "%s", message);
		break;
	default:
		break;
	}
}

ThreadLogManager* ThreadLogManager::getCurrent()
{
	static thread_local std::unique_ptr<ThreadLogManager> current;

	if (!current) {
		current.reset(new ThreadLogManager());
	}

	return current.get();
}
