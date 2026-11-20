#include "BrowserView.h"

#include "CommandIds.h"
#include "External.h"
#include "ImageManager.h"
#include "Resources.h"
#include "Roblox.h"
#include "util/Http.h"

#include <QAction>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QResizeEvent>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

static const unsigned browserToolBarId = 147;
static const int addressHistorySize = 10;

CBrowserView::CBrowserView()
{
	ImageManager::instance().loadBrowserImages();

	addCommand(ID_BROWSER_STOP, [this]() { Stop(); });
	addCommand(ID_BROWSER_HOME, [this]() { goHome(); });
	addCommand(ID_BROWSER_REFRESH, [this]() { Refresh(); });
	addCommand(ID_BROWSER_BACK, [this]() { GoBack(); });
	addCommand(ID_BROWSER_FORWARD, [this]() { GoForward(); });
	addCommand(ID_BROWSER_GO, [this]() { navigateToAddress(); });

	addCommandUpdate(ID_BROWSER_STOP, [this](CommandState& state) { state.enabled = GetBusy(); });
	addCommandUpdate(ID_BROWSER_HOME, [this](CommandState& state) { state.enabled = !m_homeUrl.isEmpty(); });
	addCommandUpdate(ID_BROWSER_REFRESH, [](CommandState& state) { state.enabled = true; });
	addCommandUpdate(ID_BROWSER_BACK, [this](CommandState& state) { state.enabled = m_canGoBack; });
	addCommandUpdate(ID_BROWSER_FORWARD, [this](CommandState& state) { state.enabled = m_canGoForward; });
	addCommandUpdate(ID_BROWSER_GO, [this](CommandState& state) { state.enabled = m_address != nullptr; });
}

void CBrowserView::OnTitleChange(const QString& text)
{
	CHtmlView::OnTitleChange(text);

	if (Document* doc = document()) {
		doc->setTitle(text);
	}
	else {
		window()->setWindowTitle(text);
	}
}

void CBrowserView::OnCommandStateChange(int command, bool enable)
{
	CHtmlView::OnCommandStateChange(command, enable);

	switch (command) {
	case CSC_NAVIGATEBACK:
		m_canGoBack = enable;
		break;
	case CSC_NAVIGATEFORWARD:
		m_canGoForward = enable;
		break;
	case CSC_UPDATECOMMANDS:
		updateCommandUI();
		break;
	}
}

void CBrowserView::resizeEvent(QResizeEvent* event)
{
	CHtmlView::resizeEvent(event);

	int barHeight = 0;
	if (m_toolBar) {
		barHeight = m_toolBar->sizeHint().height();
		m_toolBar->setGeometry(0, 0, event->size().width(), barHeight);
	}
	SetBrowserGeometry(QRect(0, barHeight, event->size().width(), event->size().height() - barHeight));
}

bool CBrowserView::create()
{
	m_toolBar = new QToolBar(this);
	m_toolBar->setIconSize(QSize(16, 16));

	const Resources::ToolBar* resource = Resources::toolBar(browserToolBarId);
	for (unsigned button : resource->buttons) {
		if (button == 0) {
			m_toolBar->addSeparator();
			continue;
		}

		if (button == ID_BROWSER_ADDRESS) {
			m_toolBar->addWidget(new QLabel(QStringLiteral("Address"), m_toolBar));
			m_address = new QComboBox(m_toolBar);
			m_address->setEditable(true);
			m_address->setInsertPolicy(QComboBox::NoInsert);
			m_address->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
			connect(m_address->lineEdit(), &QLineEdit::returnPressed, this, [this]() { navigateToAddress(); });
			m_toolBar->addWidget(m_address);
			continue;
		}

		QString prompt = Resources::string(button);
		QString caption = prompt.section('\n', 1, 1);
		QAction* action = new QAction(caption, m_toolBar);
		action->setToolTip(caption);
		action->setStatusTip(prompt.section('\n', 0, 0));
		action->setIcon(ImageManager::instance().icon(button));
		connect(action, &QAction::triggered, this, [this, button]() { routeCommand({this}, button); });
		m_toolBar->addAction(action);
		m_actions[button] = action;
	}

	m_toolBar->show();
	updateCommandUI();

	QResizeEvent event(size(), size());
	resizeEvent(&event);
	if (document()) {
		QMetaObject::invokeMethod(this, [this]() { CHtmlView::create(); }, Qt::QueuedConnection);
		return true;
	}
	return CHtmlView::create();
}

void CBrowserView::updateCommandUI()
{
	for (auto& [id, action] : m_actions) {
		CommandState state;
		routeCommandUpdate({this}, id, state);
		action->setEnabled(state.enabled);
	}
}

void CBrowserView::goHome()
{
	if (!m_homeUrl.isEmpty()) {
		Navigate(m_homeUrl);
	}
}

void CBrowserView::navigateToAddress()
{
	QString url = m_address->currentText();

	if (m_address->findText(url) == -1) {
		m_address->insertItem(0, url);

		if (m_address->count() > addressHistorySize) {
			m_address->removeItem(m_address->count() - 1);
		}
	}

	Navigate(url);
	m_address->lineEdit()->selectAll();
}

void CBrowserView::initialUpdate()
{
	CHtmlView::initialUpdate();

	if (document()) {
		static_cast<CBrowserDoc*>(document())->view = this;
	}

	if (!m_homeUrl.isEmpty()) {
		Navigate(m_homeUrl);
	}
}

CBrowserView* CBrowserView::createPopup(
	QRect rect, QWidget* parent, const QString& url, std::shared_ptr<CExternal> external
)
{
	QWidget* frame = new QWidget(parent, Qt::Tool);
	frame->setAttribute(Qt::WA_DeleteOnClose);
	QVBoxLayout* layout = new QVBoxLayout(frame);
	layout->setContentsMargins(0, 0, 0, 0);

	CBrowserView* view = new CBrowserView;
	view->setParent(frame);
	view->m_external = std::move(external);
	view->m_homeUrl = url;
	layout->addWidget(view);

	frame->setGeometry(QRect(parent->mapToGlobal(rect.topLeft()), rect.size()));
	frame->show();
	if (view->create()) {
		view->Navigate(url);
	}
	return view;
}

std::shared_ptr<CExternal> CBrowserView::OnGetExternal()
{
	if (!m_external) {
		m_external = std::make_shared<CBrowserViewExternal>(dynamic_cast<CIDEBrowserView*>(this) != nullptr);
	}

	return m_external;
}

void CBrowserView::OnBeforeNavigate2(const QString& url, const QString& targetFrameName, bool* cancel)
{
	CHtmlView::OnBeforeNavigate2(url, targetFrameName, cancel);
	if (*cancel) {
		return;
	}

	std::string address = url.toStdString();
	if (!targetFrameName.isEmpty()) {
		m_popupUrls.push_back(address);
		if (m_popupUrls.size() > 10) {
			m_popupUrls.pop_front();
		}
	}
	else if (
		url != "about:blank" && !RBX::Http::isScript(address.c_str()) && !RBX::Http::isRobloxSite(address.c_str())
	) {
		Navigate(url, QStringLiteral("_blank"));
		*cancel = true;
	}
}

void CBrowserView::OnNavigateComplete2(const QString& url)
{
	CHtmlView::OnNavigateComplete2(url);

	if (m_address) {
		if (std::find(m_popupUrls.begin(), m_popupUrls.end(), url.toStdString()) == m_popupUrls.end()) {
			m_address->setEditText(url);
		}
	}
}

CIDEBrowserView::CIDEBrowserView()
{
	m_homeUrl = CRobloxApp::baseUrl() + "/IDE/Start.aspx";
}

CPlayBrowserView::CPlayBrowserView()
{
	m_homeUrl = CRobloxApp::baseUrl() + "/Start.aspx";
}
