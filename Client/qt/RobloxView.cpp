#include "RobloxView.h"

#include "ActiveDocView.h"
#include "BrowserView.h"
#include "CommandIds.h"
#include "FileDialog.h"
#include "MainFrm.h"
#include "MultiPlayerPane.h"
#include "Printing.h"
#include "Resources.h"
#include "Roblox.h"
#include "RobloxDoc.h"
#include "WebToolbox.h"

#include <G3D/System.h>
#include <QResizeEvent>
#include <algorithm>

static const int toolboxWidth = 225;
static const int multiPlayerPaneHeight = 20;
static const double verbEnableDelay = 3.0;
static const unsigned modelFilterStringId = 184;
static const int toolsCategory = 9;

InsertModelFromRobloxVerb::InsertModelFromRobloxVerb(CRobloxView* view, RBX::DataModel* dataModel)
	: RBX::Verb(dataModel, "ToggleToolbox"), m_view(view), m_dataModel(dataModel), m_created(G3D::System::getTick())
{
}

bool InsertModelFromRobloxVerb::isEnabled() const
{
	if (G3D::System::getTick() < m_created + verbEnableDelay) {
		return false;
	}
	return !RBX::Network::Players::clientIsPresent(m_dataModel, true);
}

bool InsertModelFromRobloxVerb::isChecked() const
{
	return m_view->isToolboxVisible();
}

void InsertModelFromRobloxVerb::doIt(RBX::IDataState* dataState)
{
	m_view->postCommand(ID_VIEW_TOOLBOX);
}

ToggleIDEModeVerb::ToggleIDEModeVerb(RBX::VerbContainer* container)
	: RBX::Verb(container, "TogglePlayMode"), m_created(G3D::System::getTick())
{
}

bool ToggleIDEModeVerb::isEnabled() const
{
	if (G3D::System::getTick() < m_created + verbEnableDelay) {
		return false;
	}
	return !theApp.mainFrame()->isGameLayoutLocked();
}

bool ToggleIDEModeVerb::isChecked() const
{
	return !theApp.mainFrame()->isGameLayout();
}

void ToggleIDEModeVerb::doIt(RBX::IDataState* dataState)
{
	if (isEnabled()) {
		theApp.mainFrame()->postCommand(ID_VIEW_GAME_LAYOUT);
	}
}

void LockPlayModeVerb::doIt(RBX::IDataState* dataState)
{
	theApp.mainFrame()->postCommand(ID_LOCK_GAME_LAYOUT);
}

bool ToggleFullscreenVerb::isChecked() const
{
	return theApp.mainFrame()->isFullScreenMode();
}

void ToggleFullscreenVerb::doIt(RBX::IDataState* dataState)
{
	theApp.mainFrame()->postCommand(ID_VIEW_FULL_SCREEN);
}

void HelpCommand::doIt(RBX::IDataState* dataState)
{
	m_view->postCommand(ID_GAME_HELP);
}

CRobloxView::CRobloxView()
	: RBX::VerbContainer(nullptr), m_verbs([this]() { return m_dataModel; }, [this]() { return this; }),
	  m_window(false, this)
{
	setFocusPolicy(Qt::StrongFocus);
	setAttribute(Qt::WA_OpaquePaintEvent);

	addCommand(ID_INSERT_MODEL, [this]() { insertModel(); });
	addCommand(ID_TOOLS_INSERT_MODEL_RAW, [this]() { insertModel(); });
	addCommand(ID_GAME_HELP, [this]() { showHelp(); });
	addCommand(ID_VIEW_TOOLBOX, [this]() { showToolbox(m_toolbox == nullptr); });
	addCommand(ID_INSERT_TOOL, [this]() {
		showToolbox(true);
		m_toolbox->showCategory(toolsCategory);
	});
	addCommandUpdate(ID_VIEW_TOOLBOX, [this](CommandState& state) { state.checked = m_toolbox != nullptr; });
	addCommand(ID_FILE_PRINT, [this]() { filePrint(); });
	addCommand(ID_FILE_PRINT_DIRECT, [this]() { filePrint(true); });
	addCommand(ID_FILE_PRINT_PREVIEW, [this]() { filePrintPreview(); });
}

bool CRobloxView::onPreparePrinting(PrintInfo& info)
{
	return doPreparePrinting(info);
}

CRobloxView::~CRobloxView()
{
	releaseDataModel();
}

CRobloxDoc* CRobloxView::robloxDocument() const
{
	return static_cast<CRobloxDoc*>(document());
}

void CRobloxView::postCommand(unsigned id)
{
	QMetaObject::invokeMethod(this, [this, id]() { routeCommand({this, document()}, id); }, Qt::QueuedConnection);
}

bool CRobloxView::hasCommand(unsigned id) const
{
	return m_verbs.has(id) || View::hasCommand(id);
}

bool CRobloxView::executeCommand(unsigned id)
{
	return m_verbs.execute(id) || View::executeCommand(id);
}

bool CRobloxView::updateCommand(unsigned id, CommandState& state)
{
	return m_verbs.update(id, state) || View::updateCommand(id, state);
}

bool CRobloxView::create()
{
	m_window.setGeometry(rect());
	return m_window.create();
}

void CRobloxView::focusInEvent(QFocusEvent* event)
{
	View::focusInEvent(event);
	m_window.setFocus();
}

void CRobloxView::layoutChildren()
{
	int viewWidth = width();
	int viewHeight = height();
	if (m_toolbox) {
		viewWidth -= toolboxWidth;
	}
	if (m_multiPlayerPane) {
		viewHeight -= multiPlayerPaneHeight;
	}

	int cx = std::max(viewWidth, 0);
	int cy = std::max(viewHeight, 0);

	m_window.setGeometry(0, 0, cx, cy);
	if (m_toolbox) {
		m_toolbox->setGeometry(cx, 0, toolboxWidth, height());
	}
	if (m_multiPlayerPane) {
		m_multiPlayerPane->setGeometry(0, cy, cx, multiPlayerPaneHeight);
	}
}

void CRobloxView::resizeEvent(QResizeEvent* event)
{
	View::resizeEvent(event);
	layoutChildren();
}

void CRobloxView::activate(bool active, View* other)
{
	if (active) {
		ActiveDocView::activate(true, robloxDocument());
		m_window.setActive(true);
	}
	else if (other != this) {
		ActiveDocView::activate(false, robloxDocument());
		m_window.setActive(false);
	}
}

void CRobloxView::activateFrame(bool active)
{
	m_window.setActive(active);
}

void CRobloxView::insertModel()
{
	CFileDialog dialog(
		true,
		QStringLiteral("rbxm"),
		QString(),
		CFileDialog::HideReadOnly | CFileDialog::OverwritePrompt,
		Resources::string(modelFilterStringId),
		this
	);
	if (dialog.DoModal() == IDOK) {
		robloxDocument()->workspace()->insertContent(
			RBX::ContentId::fromFile(dialog.GetPathName().toStdString().c_str()), false
		);
	}
}

void CRobloxView::showHelp()
{
	QRect rect = m_window.rect().adjusted(12, 12, -12, -12);
	rect.setRight(rect.width() / 2 - 1);
	rect.setBottom(std::min(480, rect.height()) - rect.top() - 1);
	CBrowserView::createPopup(rect, &m_window, CRobloxApp::baseUrl() + "/Game/Help.aspx", nullptr);
}

void CRobloxView::showToolbox(bool show)
{
	bool shown = m_toolbox != nullptr;
	if (show == shown) {
		return;
	}

	if (show) {
		m_toolbox = new CWebToolbox(this, &m_window, robloxDocument()->workspace());
		m_toolbox->show();
		if (!m_toolbox->create()) {
			delete m_toolbox;
			m_toolbox = nullptr;
			return;
		}

		m_toolbox->initialUpdate();
	}
	else {
		delete m_toolbox;
		m_toolbox = nullptr;
	}

	layoutChildren();
}

void CRobloxView::onEvent(const RBX::Network::Players* source, RBX::SuperSafeChanged event)
{
	QMetaObject::invokeMethod(this, [this]() { updateMultiPlayerPane(); }, Qt::QueuedConnection);
}

void CRobloxView::onEvent(const RBX::ServiceProvider* source, RBX::ServiceRemoving event)
{
	if (event.service->getClassName() == "NetworkClient") {
		QMetaObject::invokeMethod(this, [this]() { showToolbox(false); }, Qt::QueuedConnection);
	}

	if (event.service->getClassName() == "NetworkClient" || event.service->getClassName() == "Players") {
		QMetaObject::invokeMethod(this, [this]() { updateMultiPlayerPane(); }, Qt::QueuedConnection);
	}

	if (event.service->getClassName() == "Players") {
		static_cast<RBX::Network::Players*>(event.service.get())
			->RBX::Notifier<RBX::Network::Players, RBX::SuperSafeChanged>::removeListener(this);
	}
}

void CRobloxView::onEvent(const RBX::ServiceProvider* source, RBX::ServiceAdded event)
{
	if (event.service->getClassName() == "NetworkClient" || event.service->getClassName() == "Players") {
		QMetaObject::invokeMethod(this, [this]() { updateMultiPlayerPane(); }, Qt::QueuedConnection);
	}

	if (event.service->getClassName() == "Players") {
		static_cast<RBX::Network::Players*>(event.service.get())
			->RBX::Notifier<RBX::Network::Players, RBX::SuperSafeChanged>::addListener(this);
	}
}

void CRobloxView::updateMultiPlayerPane()
{
	if (!m_dataModel) {
		return;
	}

	RBX::DataModel::Lock lock(m_dataModel);

	bool showPane = false;
	RBX::Network::Players* players = RBX::ServiceProvider::find<RBX::Network::Players>(m_dataModel.get());
	if (players) {
		if (RBX::Network::Players::clientIsPresent(m_dataModel.get(), true) && !players->superSafeOn()) {
			showPane = true;
		}
	}

	bool paneShown = m_multiPlayerPane != nullptr;
	if (showPane != paneShown) {
		if (showPane) {
			m_multiPlayerPane = new CMultiPlayerPane(this);
			m_multiPlayerPane->create(&m_window, m_dataModel.get());
			m_multiPlayerPane->show();
		}
		else {
			delete m_multiPlayerPane;
			m_multiPlayerPane = nullptr;
		}

		layoutChildren();
	}
}

void CRobloxView::releaseDataModel()
{
	RBX::DataModel::Lock lock(m_dataModel);
	m_helpVerb.reset();
	m_toolboxVerb.reset();
	m_fullScreenVerb.reset();
	m_ideModeVerb.reset();
	m_lockPlayModeVerb.reset();
	RBX::stopListening<RBX::ServiceProvider, RBX::ServiceAdded>(m_dataModel, this);
	RBX::stopListening<RBX::ServiceProvider, RBX::ServiceRemoving>(m_dataModel, this);

	RBX::Network::Players* players = RBX::ServiceProvider::find<RBX::Network::Players>(m_dataModel.get());
	if (players) {
		players->RBX::Notifier<RBX::Network::Players, RBX::SuperSafeChanged>::removeListener(this);
	}
}

void CRobloxView::initialUpdate()
{
	try {
		releaseDataModel();
		m_window.setWorkspace(nullptr);
		m_dataModel.reset();
		View::initialUpdate();

		m_window.setWorkspace(robloxDocument()->workspace().get());
		m_dataModel = robloxDocument()->workspace()->dataModel();

		RBX::DataModel::Lock lock(m_dataModel);
		if (m_toolbox) {
			m_toolbox->setWorkspace(robloxDocument()->workspace());
		}
		RBX::startListening(m_dataModel, static_cast<RBX::Listener<RBX::ServiceProvider, RBX::ServiceAdded>*>(this));
		RBX::startListening(m_dataModel, static_cast<RBX::Listener<RBX::ServiceProvider, RBX::ServiceRemoving>*>(this));
		setVerbParent(m_dataModel->getWorkspace());

		m_toolboxVerb.reset(new InsertModelFromRobloxVerb(this, m_dataModel.get()));
		m_helpVerb.reset(new HelpCommand(m_dataModel.get(), this));
		m_fullScreenVerb.reset(new ToggleFullscreenVerb(m_dataModel.get()));
		m_ideModeVerb.reset(new ToggleIDEModeVerb(m_dataModel.get()));
		m_lockPlayModeVerb.reset(new LockPlayModeVerb(m_dataModel.get()));

		RBX::Workspace* workspace = m_dataModel->getWorkspace();
		m_verbs.bind(ID_CAMERA_PAN_LEFT, workspace->getVerb("CameraPanLeft"));
		m_verbs.bind(ID_CAMERA_PAN_RIGHT, workspace->getVerb("CameraPanRight"));
		m_verbs.bind(ID_CAMERA_TILT_UP, workspace->getVerb("CameraTiltUp"));
		m_verbs.bind(ID_CAMERA_TILT_DOWN, workspace->getVerb("CameraTiltDown"));
		m_verbs.bind(ID_CAMERA_ZOOM_IN, workspace->getVerb("CameraZoomIn"));
		m_verbs.bind(ID_CAMERA_ZOOM_OUT, workspace->getVerb("CameraZoomOut"));
		m_verbs.bind(ID_CAMERA_LOOK_AT, workspace->getVerb("CameraCenter"));
		m_verbs.bind(ID_CAMERA_ZOOM_EXTENTS, workspace->getVerb("CameraZoomExtents"));
		m_verbs.bind(ID_TOOLS_IMAGE_SERVER_VIEW, workspace->getVerb("CameraModelView"));
		m_verbs.bind(ID_CAMERA_FIXED, RBX::Name::lookup("FixedCamera"));
		m_verbs.bind(ID_CAMERA_ATTACHED, RBX::Name::lookup("AttachCamera"));
		m_verbs.bind(ID_CAMERA_TRACKING, RBX::Name::lookup("TrackCamera"));
		m_verbs.bind(ID_CAMERA_FOLLOWING, RBX::Name::lookup("FollowCamera"));
		m_verbs.bind(ID_CAMERA_WATCHING, RBX::Name::lookup("WatchCamera"));
		m_verbs.bind(ID_FORMAT_ROTATE, RBX::Name::lookup("SelectionRotate"));
		m_verbs.bind(ID_FORMAT_TILT, RBX::Name::lookup("SelectionTilt"));

		m_window.buildGui();
	}
	catch (std::exception& e) {
		deferException(e);
	}
}
