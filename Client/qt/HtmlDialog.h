#ifndef HTMLDIALOG_H
#define HTMLDIALOG_H

#include "Dialog.h"

#include <QString>
#include <memory>

class CExternal;
class CHtmlView;
class CWorkspace;

class CRobloxDHtmlDialog : public CDialog
{
public:
	CRobloxDHtmlDialog(unsigned templateId, const QString& url, QWidget* parent);
	CRobloxDHtmlDialog(unsigned templateId, unsigned htmlResourceId, QWidget* parent);

	void setExternal(std::shared_ptr<CExternal> external);

protected:
	bool OnInitDialog() override;
	virtual void OnDocumentComplete();

	void setElementHtml(const QString& id, const QString& html);
	void hideElement(const QString& id);
	void closeOnClick(const QString& id, int result);

	void resizeEvent(QResizeEvent* event) override;

private:
	class Browser;
	friend class Browser;

	Browser* m_browser;
	QString m_url;
	unsigned m_htmlResourceId = 0;
};

class CSaveToRobloxDialog : public CRobloxDHtmlDialog
{
public:
	CSaveToRobloxDialog(std::shared_ptr<CWorkspace> workspace, const QString& url, QWidget* parent);
};

class CPublishAsPlaceDialog : public CSaveToRobloxDialog
{
public:
	CPublishAsPlaceDialog(std::shared_ptr<CWorkspace> workspace, QWidget* parent);
};

class CUploadPlaceDialog : public CSaveToRobloxDialog
{
public:
	CUploadPlaceDialog(std::shared_ptr<CWorkspace> workspace, QWidget* parent);
};

class CSaveModelToRobloxDialog : public CSaveToRobloxDialog
{
public:
	CSaveModelToRobloxDialog(std::shared_ptr<CWorkspace> workspace, QWidget* parent);
};

#endif // HTMLDIALOG_H
