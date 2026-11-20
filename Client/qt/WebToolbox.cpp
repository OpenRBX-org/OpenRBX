#include "WebToolbox.h"

#include "External.h"
#include "Roblox.h"
#include "RobloxWnd.h"

#ifdef _WIN32
#include <windows.h>
#endif

CWebToolbox::CWebToolbox(QWidget* parent, CRobloxWnd* viewport, std::shared_ptr<CWorkspace> workspace)
	: CHtmlView(parent), m_viewport(viewport), m_workspace(std::move(workspace))
{
	setFocusPolicy(Qt::NoFocus);
}

void CWebToolbox::focusViewport()
{
	if (m_viewport) {
#ifdef _WIN32
		SetFocus(reinterpret_cast<HWND>(m_viewport->window()->winId()));
#endif
		m_viewport->setFocus();
	}
}

void CWebToolbox::showCategory(int category)
{
	Navigate(QStringLiteral("%1/IDE/ClientToolbox.aspx?Category=%2").arg(CRobloxApp::baseUrl()).arg(category));
}

void CWebToolbox::setWorkspace(std::shared_ptr<CWorkspace> workspace)
{
	m_workspace = std::move(workspace);
	ResetExternal();
}

void CWebToolbox::initialUpdate()
{
	CHtmlView::initialUpdate();
	Navigate(CRobloxApp::baseUrl() + "/IDE/ClientToolbox.aspx");
}

void CWebToolbox::activate(bool active, View* other)
{
	CHtmlView::activate(active, other);
	focusViewport();
}

std::shared_ptr<CExternal> CWebToolbox::OnGetExternal()
{
	focusViewport();
	return std::make_shared<CWorkspaceExternal>(m_workspace);
}

void CWebToolbox::OnBrowserFocus()
{
	focusViewport();
}

void CWebToolbox::focusInEvent(QFocusEvent* event)
{
	CHtmlView::focusInEvent(event);
	focusViewport();
}

void CWebToolbox::mousePressEvent(QMouseEvent* event)
{
	CHtmlView::mousePressEvent(event);
	focusViewport();
}
