#ifndef ROBLOX_H
#define ROBLOX_H

#include "Command.h"

#include <QString>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class CMainFrame;
class CRobloxDoc;
class Document;
class DocumentTemplate;
class ErrorUploader;
class IDEDocumentManager;
class MainLogManager;
class QApplication;
class QTimer;

namespace RBX {
class Script;
}

void deferException(const std::exception& e);
void rethrowDeferredException();

class CRobloxCommandLineInfo
{
public:
	enum ShellCommand
	{
		FileNew,
		FileOpen,
		FilePrint,
		FilePrintTo,
		FileDDE,
		AppRegister,
		AppUnregister,
	};

	CRobloxCommandLineInfo(int argc, char** argv);

	ShellCommand shellCommand = FileNew;
	QString fileName;
	QString printerName;
	QString driverName;
	QString portName;
	bool runEmbedded = false;
	bool runAutomated = false;
	bool browser = false;
	bool no3D = false;
	std::vector<std::string> scripts;

private:
	void parseParam(const char* param, bool flag);

	bool m_expectBaseUrl = false;
	bool m_expectScript = false;
};

class CRobloxApp : public CommandTarget
{
public:
	CRobloxApp();
	~CRobloxApp() override;

	int run(int argc, char** argv);

	static QString baseUrl();
	static void setBaseUrl(const QString& url);

	CMainFrame* mainFrame() const { return m_mainFrame; }
	const CRobloxCommandLineInfo* commandLineInfo() const { return m_commandLineInfo; }
	bool showMainFrame(bool gameLayout);

	const std::string& version() const { return m_version; }
	MainLogManager& logManager() { return *m_logManager; }
	ErrorUploader& errorUploader() { return *m_errorUploader; }

	void openStartPage();
	void openScriptDocument(std::shared_ptr<RBX::Script> script);
	CRobloxDoc* playDocument();
	bool executeScripts(const std::vector<std::string>& scripts);

	bool promptFileName(
		QString& fileName, unsigned titleId, unsigned flags, bool open, DocumentTemplate* documentTemplate
	);
	void addToRecentFileList(const QString& pathName);
	std::vector<QString> recentFileDisplayNames() const;
	bool saveAllModified();
	void closeAllDocuments();

	bool clipboardHasFormat(const char* format);
	bool readClipboard(const char* format, std::string& text);
	void writeClipboard(const char* format, const std::string& text);

	void scheduleFrame();

private:
	bool initInstance(int argc, char** argv);
	void loadSettings();
	void loadRecentFileList(int size);
	void openRecentFile(unsigned index);
	bool processShellCommand(const CRobloxCommandLineInfo& commandLine);
	bool executeScript(const std::string& script);
	void reportCrashes();
	void idle();
	void renderFrame();

	std::unique_ptr<QApplication> m_application;
	std::unique_ptr<MainLogManager> m_logManager;
	std::unique_ptr<ErrorUploader> m_errorUploader;
	std::unique_ptr<IDEDocumentManager> m_documentManager;
	std::unique_ptr<QTimer> m_frameTimer;
	CMainFrame* m_mainFrame = nullptr;
	const CRobloxCommandLineInfo* m_commandLineInfo = nullptr;
	std::string m_version;
	std::vector<QString> m_recentFiles;
	int m_recentFileListSize = 0;
	bool m_idle = false;
};

extern CRobloxApp theApp;

#endif // ROBLOX_H
