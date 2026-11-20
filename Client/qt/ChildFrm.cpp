#include "ChildFrm.h"

#include "DocView.h"
#include "MainFrm.h"
#ifdef _WIN32
#include "PreviewView.h"
#endif
#include "Roblox.h"

#include <QCloseEvent>
#include <QVBoxLayout>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

CChildFrame::CChildFrame(View* view) : m_view(view), m_activeView(view)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(view);
	setFocusProxy(view);

#ifdef _WIN32
	RECT frame = {0, 0, 0, 0};
	AdjustWindowRectEx(&frame, WS_CHILD | WS_OVERLAPPEDWINDOW, FALSE, WS_EX_MDICHILD);
	setMinimumSize(
		std::max(0, static_cast<int>(GetSystemMetrics(SM_CXMINTRACK) - (frame.right - frame.left))),
		std::max(0, static_cast<int>(GetSystemMetrics(SM_CYMINTRACK) - (frame.bottom - frame.top)))
	);
#endif
}

CChildFrame::~CChildFrame()
{
	if (CMainFrame* frame = theApp.mainFrame()) {
		frame->childFrameDestroyed(this);
	}
}

void CChildFrame::activateFrame()
{
	theApp.mainFrame()->activateChildFrame(this);
}

void CChildFrame::updateTitle()
{
	if (m_view->document()) {
		setWindowTitle(m_view->document()->title());
	}
	theApp.mainFrame()->childFrameTitleChanged(this);
}

void CChildFrame::setActiveView(View* view)
{
	View* old = m_activeView;
	if (view == old) {
		return;
	}
	m_activeView = nullptr;
	if (old) {
		old->activate(false, view);
	}
	m_activeView = view;
	setFocusProxy(view);
	if (view) {
		view->activate(true, view);
	}
	theApp.mainFrame()->childFrameViewChanged(this);
}

#ifdef _WIN32
void CChildFrame::setPreviewMode(bool preview, CPreviewView* view)
{
	QVBoxLayout* frameLayout = static_cast<QVBoxLayout*>(layout());
	if (preview) {
		m_preview = view;
		m_view->hide();
		frameLayout->insertWidget(0, view->toolBar());
		frameLayout->insertWidget(1, view);
		view->toolBar()->show();
		view->show();
	}
	else {
		m_preview = nullptr;
		frameLayout->removeWidget(view->toolBar());
		frameLayout->removeWidget(view);
		view->toolBar()->hide();
		view->hide();
		m_view->show();
	}
}
#endif

void CChildFrame::requestClose()
{
#ifdef _WIN32
	if (m_preview) {
		m_preview->onPreviewClose();
	}
#endif
	Document* document = m_view->document();
	if (document && document->saveModified()) {
		document->close();
	}
}

void CChildFrame::closeEvent(QCloseEvent* event)
{
	event->ignore();
	QMetaObject::invokeMethod(this, [this]() { requestClose(); }, Qt::QueuedConnection);
}
