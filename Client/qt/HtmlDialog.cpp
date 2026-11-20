#include "HtmlDialog.h"

#include "External.h"
#include "HtmlView.h"
#include "MainFrm.h"
#include "Roblox.h"

#include <QResizeEvent>

static const unsigned saveToRobloxTemplateId = 113;

class CRobloxDHtmlDialog::Browser : public CHtmlView
{
public:
	explicit Browser(CRobloxDHtmlDialog* dialog) : CHtmlView(dialog), m_dialog(dialog) {}

	std::shared_ptr<CExternal> external;

protected:
	std::shared_ptr<CExternal> OnGetExternal() override { return external; }

	void OnTitleChange(const QString& text) override { m_dialog->setWindowTitle(text); }

	void OnDocumentComplete(const QString& url) override { m_dialog->OnDocumentComplete(); }

	void OnWindowClosing(const QString& dialogResult) override
	{
		if (dialogResult.isEmpty()) {
			m_dialog->EndDialog(IDCANCEL);
		}
		else {
			m_dialog->EndDialog(dialogResult.toInt());
		}
	}

private:
	CRobloxDHtmlDialog* m_dialog;
};

CRobloxDHtmlDialog::CRobloxDHtmlDialog(unsigned templateId, const QString& url, QWidget* parent)
	: CDialog(templateId, parent ? parent : theApp.mainFrame()), m_browser(new Browser(this)), m_url(url)
{
}

CRobloxDHtmlDialog::CRobloxDHtmlDialog(unsigned templateId, unsigned htmlResourceId, QWidget* parent)
	: CDialog(templateId, parent ? parent : theApp.mainFrame()), m_browser(new Browser(this)),
	  m_htmlResourceId(htmlResourceId)
{
}

void CRobloxDHtmlDialog::OnDocumentComplete()
{
}

void CRobloxDHtmlDialog::setElementHtml(const QString& id, const QString& html)
{
	m_browser->ExecuteScript(QStringLiteral(
								 "(function (element, html) {"
								 " if (element.tagName === \"TEXTAREA\") {"
								 " element.value = html.replace(/\\s+/g, \" \").trim();"
								 " } else {"
								 " element.innerHTML = html;"
								 " }"
								 "})(document.getElementById(%1), %2);"
	)
								 .arg(CHtmlView::quoteScriptString(id), CHtmlView::quoteScriptString(html)));
}

void CRobloxDHtmlDialog::hideElement(const QString& id)
{
	m_browser->ExecuteScript(
		QStringLiteral("document.getElementById(%1).style.display = \"none\";").arg(CHtmlView::quoteScriptString(id))
	);
}

void CRobloxDHtmlDialog::closeOnClick(const QString& id, int result)
{
	m_browser->ExecuteScript(QStringLiteral(
								 "document.getElementById(%1).onclick = function () {"
								 " var result = document.createElement(\"input\");"
								 " result.type = \"hidden\"; result.id = \"DialogResult\"; result.value = \"%2\";"
								 " document.body.appendChild(result); window.close(); };"
	)
								 .arg(CHtmlView::quoteScriptString(id))
								 .arg(result));
}

void CRobloxDHtmlDialog::setExternal(std::shared_ptr<CExternal> external)
{
	m_browser->external = std::move(external);
}

bool CRobloxDHtmlDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_browser->setGeometry(rect());
	if (!m_browser->create()) {
		return true;
	}

	if (!m_url.isEmpty()) {
		m_browser->Navigate(m_url);
		return false;
	}

	if (m_htmlResourceId != 0) {
		m_browser->LoadFromResource(m_htmlResourceId);
	}

	return true;
}

void CRobloxDHtmlDialog::resizeEvent(QResizeEvent* event)
{
	CDialog::resizeEvent(event);
	m_browser->setGeometry(rect());
}

CSaveToRobloxDialog::CSaveToRobloxDialog(std::shared_ptr<CWorkspace> workspace, const QString& url, QWidget* parent)
	: CRobloxDHtmlDialog(saveToRobloxTemplateId, url, parent)
{
	setExternal(std::make_shared<CWorkspaceExternal>(std::move(workspace)));
}

CPublishAsPlaceDialog::CPublishAsPlaceDialog(std::shared_ptr<CWorkspace> workspace, QWidget* parent)
	: CSaveToRobloxDialog(std::move(workspace), CRobloxApp::baseUrl() + "/IDE/Upload.aspx", parent)
{
}

CUploadPlaceDialog::CUploadPlaceDialog(std::shared_ptr<CWorkspace> workspace, QWidget* parent)
	: CSaveToRobloxDialog(std::move(workspace), CRobloxApp::baseUrl() + "/Game/Upload.aspx", parent)
{
}

CSaveModelToRobloxDialog::CSaveModelToRobloxDialog(std::shared_ptr<CWorkspace> workspace, QWidget* parent)
	: CSaveToRobloxDialog(std::move(workspace), CRobloxApp::baseUrl() + "/UI/Save.aspx?type=Model", parent)
{
}
