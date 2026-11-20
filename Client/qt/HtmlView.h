#ifndef HTMLVIEW_H
#define HTMLVIEW_H

#include "DocView.h"

#include <QJsonValue>
#include <QString>
#include <memory>

class CExternal;
class CExternalObjects;

#ifdef _WIN32
// clang-format off
#include <windows.h>
#include <objbase.h>
#include <exdisp.h>
// clang-format on
#else
enum
{
	CSC_UPDATECOMMANDS = -1,
	CSC_NAVIGATEFORWARD = 1,
	CSC_NAVIGATEBACK = 2,
};
#endif

class CHtmlView : public View
{
public:
	explicit CHtmlView(QWidget* parent = nullptr);
	~CHtmlView() override;

	bool create() override;

	static QString quoteScriptString(const QString& text);

	void Navigate(const QString& url, const QString& targetFrameName = QString());
	void LoadFromResource(unsigned id);
	void ExecuteScript(const QString& script);
	void SetScriptsEnabled(bool enabled);
	void InstallBridge();
	void GoBack();
	void GoForward();
	void Refresh();
	void Stop();
	bool GetBusy() const { return m_busy; }
	QString GetLocationURL() const { return m_locationUrl; }

	QWidget* GetBrowserWindow() const { return m_host; }
	void SetBrowserGeometry(const QRect& rect);
	void ResetExternal();

protected:
	virtual std::shared_ptr<CExternal> OnGetExternal();
	virtual void OnBeforeNavigate2(const QString& url, const QString& targetFrameName, bool* cancel);
	virtual void OnNavigateComplete2(const QString& url);
	virtual void OnDocumentComplete(const QString& url);
	virtual void OnTitleChange(const QString& text);
	virtual void OnCommandStateChange(int command, bool enable);
	virtual void OnWindowClosing(const QString& dialogResult);
	virtual void OnBrowserFocus();
	virtual void OnHostEvent(const QString& name, const QJsonValue& value);

	void resizeEvent(QResizeEvent* event) override;
#ifndef _WIN32
	bool eventFilter(QObject* watched, QEvent* event) override;
#endif

private:
	struct Engine;
#ifndef _WIN32
	friend class XEmbedFilter;

	void destroyBrowser();
#endif

	QByteArray OnBridgeRequest(const QByteArray& request);

	QWidget* m_host;
	std::unique_ptr<Engine> m_engine;
	std::unique_ptr<CExternalObjects> m_externalObjects;
	QString m_locationUrl;
	QString m_pendingUrl;
	QString m_pendingHtml;
	bool m_busy;
	bool m_customGeometry;
	bool m_scriptsEnabled = true;
	bool m_ready = false;
};

#endif // HTMLVIEW_H
