#ifndef BROWSERVIEW_H
#define BROWSERVIEW_H

#include "HtmlView.h"
#include "NullDoc.h"

#include <list>
#include <map>
#include <memory>
#include <string>

class QAction;
class QComboBox;
class QToolBar;

class CBrowserView : public CHtmlView
{
public:
	CBrowserView();

	static CBrowserView* createPopup(
		QRect rect, QWidget* parent, const QString& url, std::shared_ptr<CExternal> external
	);

	bool create() override;
	void initialUpdate() override;
	void updateCommandUI() override;

protected:
	std::shared_ptr<CExternal> OnGetExternal() override;
	void OnCommandStateChange(int command, bool enable) override;
	void OnNavigateComplete2(const QString& url) override;
	void OnBeforeNavigate2(const QString& url, const QString& targetFrameName, bool* cancel) override;
	void OnTitleChange(const QString& text) override;

	void resizeEvent(QResizeEvent* event) override;

	QString m_homeUrl;

private:
	void goHome();
	void navigateToAddress();

	std::list<std::string> m_popupUrls;
	QComboBox* m_address = nullptr;
	QToolBar* m_toolBar = nullptr;
	std::map<unsigned, QAction*> m_actions;
	std::shared_ptr<CExternal> m_external;
	bool m_canGoBack = false;
	bool m_canGoForward = false;
};

class CIDEBrowserView : public CBrowserView
{
public:
	CIDEBrowserView();
};

class CPlayBrowserView : public CBrowserView
{
public:
	CPlayBrowserView();
};

class CBrowserDoc : public CNullDoc
{
public:
	CBrowserView* view = nullptr;
};

#endif // BROWSERVIEW_H
