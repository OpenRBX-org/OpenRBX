#include "SelectionTreeCtrl.h"

#include "ImageManager.h"
#include "MainFrm.h"
#include "Resources.h"
#include "Roblox.h"
#include "RobloxDoc.h"
#include "RobloxTreeCtrlNode.h"
#include "util/InsertMode.h"
#include "util/scopedassign.h"
#include "v8datamodel/Workspace.h"

#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <algorithm>
#include <map>
#include <span>
#include <vector>

static const unsigned selectionMenuResourceId = 103;
static const int updateTimerPeriod = 200;
static const int pendingSelectionPerPass = 10;

static void reparent(RBX::Instance* parent, std::shared_ptr<RBX::Instance> child)
{
	if (child->getParent() == parent) {
		return;
	}

	RBX::Workspace* workspace = RBX::Workspace::findWorkspace(parent);
	if (workspace) {
		bool descendent = child->isDescendentOf(workspace);
		if (!descendent) {
			std::vector<std::shared_ptr<RBX::Instance>> instances;
			instances.push_back(child);
			workspace->insertInstances(instances, workspace, RBX::INSERT_TO_TREE, RBX::SUPPRESS_PROMPTS);
		}
	}

	child->setParent(parent);
}

CSelectionTreeCtrl::CSelectionTreeCtrl(QWidget* parent)
	: CRobloxTreeCtrl(parent), m_selectionChangeBlocked(false), m_updatingFromModel(false), m_updatingSelection(false)
{
	refresh();

	m_timer = new QTimer(this);
	connect(m_timer, &QTimer::timeout, this, [this]() { onTimer(); });
	m_timer->start(updateTimerPeriod);
}

CSelectionTreeCtrl::~CSelectionTreeCtrl()
{
	m_timer->stop();
	CRobloxTreeCtrl::resetTree();
	setDataModel(std::shared_ptr<RBX::DataModel>());
}

bool CSelectionTreeCtrl::hasCommand(unsigned id) const
{
	if (CommandTarget::hasCommand(id)) {
		return true;
	}

	CRobloxDoc* document = ActiveDocView::activeDocument();
	return document && document->hasCommand(id);
}

bool CSelectionTreeCtrl::executeCommand(unsigned id)
{
	if (CommandTarget::executeCommand(id)) {
		return true;
	}

	CRobloxDoc* document = ActiveDocView::activeDocument();
	if (!document) {
		return false;
	}

	return document->executeCommand(id);
}

bool CSelectionTreeCtrl::updateCommand(unsigned id, CommandState& state)
{
	if (CommandTarget::updateCommand(id, state)) {
		return true;
	}

	CRobloxDoc* document = ActiveDocView::activeDocument();
	if (!document) {
		return false;
	}

	return document->updateCommand(id, state);
}

bool CSelectionTreeCtrl::onSelectionChanging(const TreeSelectionChange& change)
{
	return m_selectionChangeBlocked;
}

CommandRoute CSelectionTreeCtrl::contextMenuRoute()
{
	CommandRoute route;
	route.push_back(this);
	if (CMainFrame* mainFrame = theApp.mainFrame()) {
		for (CommandTarget* target : mainFrame->commandRoute()) {
			route.push_back(target);
		}
	}
	else {
		route.push_back(&theApp);
	}
	return route;
}

bool CSelectionTreeCtrl::isCommandEnabled(unsigned id)
{
	CommandState state;
	routeCommandUpdate(contextMenuRoute(), id, state);
	return state.enabled;
}

void CSelectionTreeCtrl::showContextMenu(const QPoint& globalPoint)
{
	std::span<const Resources::MenuEntry> entries = Resources::menu(selectionMenuResourceId);
	if (entries.empty()) {
		return;
	}

	QMenu popup(this);
	for (auto it = entries.begin() + 1; it != entries.end() && it->kind != Resources::MENU_END; ++it) {
		if (it->kind == Resources::MENU_SEPARATOR) {
			popup.addSeparator();
			continue;
		}
		if (it->kind != Resources::MENU_ITEM) {
			continue;
		}

		unsigned id = it->id;
		QAction* action = popup.addAction(QString::fromLatin1(it->text));
		action->setIcon(ImageManager::instance().icon(id));
		action->setEnabled(isCommandEnabled(id));
		connect(action, &QAction::triggered, this, [this, id]() { routeCommand(contextMenuRoute(), id); });
	}

	if (globalPoint.x() == -1 && globalPoint.y() == -1) {
		QPoint center = viewport()->mapToGlobal(viewport()->rect().center());
		QSize size = popup.sizeHint();
		popup.exec(center - QPoint(size.width() / 2, size.height() / 2));
	}
	else {
		popup.exec(globalPoint);
	}
}

void CSelectionTreeCtrl::onContextMenu(const QPoint& globalPoint)
{
	showContextMenu(globalPoint);
}

void CSelectionTreeCtrl::onRightButtonDown(const QPoint& point)
{
	showContextMenu(viewport()->mapToGlobal(point));
}

void CSelectionTreeCtrl::updateSelection(QTreeWidgetItem* item, bool select)
{
	if (item && static_cast<CRobloxTreeCtrlItem*>(item)->node()) {
		std::shared_ptr<RBX::Instance> instance = static_cast<CRobloxTreeCtrlItem*>(item)->node()->instance();
		if (instance) {
			RBX::DataModel::Lock lock(dataModel());
			RBX::Selection* currentSelection = selection();
			RBX::ScopedAssign<bool> updating(m_updatingSelection, true);
			if (select) {
				currentSelection->addToSelection(instance.get());
			}
			else {
				currentSelection->removeFromSelection(instance.get());
			}
		}
	}
}

void CSelectionTreeCtrl::onSelectionChanged(const TreeSelectionChange& change)
{
	if (!m_updatingFromModel && !m_deleting && m_dataModel) {
		if (change.newItem) {
			updateSelection(change.newItem, true);
		}
		if (change.oldItem && change.action != SELECTION_UNKNOWN) {
			updateSelection(change.oldItem, false);
		}
	}
}

QLabel* CSelectionTreeCtrl::createDragImage(QTreeWidgetItem* item)
{
	QRect cell = visualItemRect(item);
	QRect label = itemLabelRect(item);
	QSize size(label.left() + label.width() - cell.left(), cell.height());
	if (size.isEmpty()) {
		return nullptr;
	}

	QPixmap pixmap(size);
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);
	painter.setFont(font());
	int iconMargin = (label.left() - cell.left() - iconSize().width()) / 2;
	painter.drawImage(
		QPoint(iconMargin, (size.height() - iconSize().height()) / 2), item->data(0, Qt::DecorationRole).value<QImage>()
	);
	painter.setPen(palette().color(QPalette::Text));
	painter.drawText(
		QRect(label.left() - cell.left(), 0, label.width(), size.height()),
		Qt::AlignCenter | Qt::TextSingleLine,
		item->text(0)
	);
	painter.end();

	QLabel* image = new QLabel(
		nullptr, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus
	);
	image->setAttribute(Qt::WA_TranslucentBackground);
	image->setAttribute(Qt::WA_ShowWithoutActivating);
	image->setPixmap(pixmap);
	image->resize(size);
	image->setWindowOpacity(0.5);
	return image;
}

void CSelectionTreeCtrl::onBeginDrag(QTreeWidgetItem* item, const QPoint& point)
{
	m_dragImage.reset(createDragImage(item));
	if (m_dragImage.get()) {
		m_dragHotSpot = point - visualItemRect(item).topLeft();
		m_dragImage->move(viewport()->mapToGlobal(point) - m_dragHotSpot);
		m_dragImage->show();
	}
}

void CSelectionTreeCtrl::selectAllIgnore(bool select, QTreeWidgetItem* ignore)
{
	if (!select) {
		CRobloxTreeCtrlNode* node = ignore ? static_cast<CRobloxTreeCtrlItem*>(ignore)->node() : nullptr;
		std::shared_ptr<RBX::Instance> instance;
		if (node) {
			instance = node->instance();
		}

		{
			RBX::DataModel::Lock lock(dataModel());
			RBX::Selection* currentSelection = selection();
			RBX::ScopedAssign<bool> updating(m_updatingSelection, true);
			currentSelection->setSelection(instance.get());
		}

		{
			RBX::ScopedAssign<bool> selecting(m_updatingFromModel, true);
			CRobloxTreeCtrl::selectAllIgnore(false, ignore);
		}
	}
	else {
		CRobloxTreeCtrl::selectAllIgnore(select, ignore);
	}
}

QTreeWidgetItem* CSelectionTreeCtrl::dropTarget(QTreeWidgetItem* item)
{
	if (item) {
		CRobloxTreeCtrlNode* node = static_cast<CRobloxTreeCtrlItem*>(item)->node();
		if (node) {
			std::shared_ptr<RBX::Instance> instance = node->instance();
			if (instance && instance->canAddChildren(selection()->begin(), selection()->end())) {
				return item;
			}
		}
	}
	return nullptr;
}

void CSelectionTreeCtrl::mouseMoveEvent(QMouseEvent* event)
{
	if (m_dragImage.get()) {
		QPoint point = event->position().toPoint();
		m_dragImage->move(viewport()->mapToGlobal(point) - m_dragHotSpot);

		TreeHitTest hit = hitTest(point);
		QTreeWidgetItem* target = hit.item;
		if (target) {
			if (hit.flags & HIT_ON_ITEM) {
				target = dropTarget(target);
			}
			else {
				target = nullptr;
			}
		}

		selectDropTarget(target);
	}
	CRobloxTreeCtrl::mouseMoveEvent(event);
}

void CSelectionTreeCtrl::queueNode(CRobloxTreeCtrlNode* node)
{
	std::scoped_lock lock(m_mutex);
	m_pendingNodes.insert(node);
}

void CSelectionTreeCtrl::dequeueNode(CRobloxTreeCtrlNode* node)
{
	std::scoped_lock lock(m_mutex);
	m_pendingNodes.erase(node);
}

void CSelectionTreeCtrl::onEvent(const RBX::Selection* source, RBX::SelectionChanged event)
{
	if (!m_updatingSelection) {
		if (event.addedItem) {
			std::scoped_lock lock(m_mutex);
			std::map<RBX::Instance*, CRobloxTreeCtrlNode*>::iterator it = m_nodes.find(event.addedItem.get());
			if (it != m_nodes.end()) {
				queueNode(it->second);
			}
			else {
				m_pendingSelection.insert(RBX::shared_from(event.addedItem.get()));
			}
		}

		if (event.removedItem) {
			std::scoped_lock lock(m_mutex);
			m_pendingSelection.erase(event.removedItem);
			std::map<RBX::Instance*, CRobloxTreeCtrlNode*>::iterator it = m_nodes.find(event.removedItem.get());
			if (it != m_nodes.end()) {
				queueNode(it->second);
			}
		}
	}
}

std::shared_ptr<RBX::DataModel> CSelectionTreeCtrl::dataModel() const
{
	return m_dataModel;
}

void CSelectionTreeCtrl::resetTree()
{
	CRobloxTreeCtrl::resetTree();

	if (m_dataModel) {
		m_rootNode = new RootNode(m_dataModel, this);
		m_rootNode->create();
		m_rootNode->queueAllChildren();
		expandTreeItem(m_rootNode->item(), EXPAND_EXPAND);
	}
}

void CSelectionTreeCtrl::insertPendingSelection()
{
	for (int i = 0; i < pendingSelectionPerPass; i++) {
		std::shared_ptr<RBX::Instance> instance;
		{
			std::scoped_lock lock(m_mutex);
			auto it = m_pendingSelection.begin();
			if (it == m_pendingSelection.end()) {
				return;
			}
			instance = *it;
			m_pendingSelection.erase(it);
		}
		findOrCreateNode(instance);
	}
}

void CSelectionTreeCtrl::onTimer()
{
	CRobloxTreeCtrlNode* node = nullptr;
	{
		std::scoped_lock lock(m_mutex);
		if (m_pendingNodes.size() > 0) {
			node = *m_pendingNodes.begin();
		}
	}

	if (node) {
		RBX::ScopedAssign<bool> updating(m_updatingFromModel, true);
		node->repaintIfNameChanged();
		node->syncSelection();
		{
			RBX::ScopedAssign<bool> selecting(m_selectionChangeBlocked, true);
			node->deleteRemovedChildren();
		}
		node->insertPendingChildren();
		node->updateChildIndicator();

		std::scoped_lock lock(m_mutex);
		if (node->m_pendingChildren.empty()) {
			dequeueNode(node);
		}
	}

	insertPendingSelection();
}

RBX::Selection* CSelectionTreeCtrl::selection()
{
	return m_dataModel ? m_dataModel->create<RBX::Selection>() : nullptr;
}

void CSelectionTreeCtrl::setDataModel(std::shared_ptr<RBX::DataModel> dataModel)
{
	if (m_dataModel) {
		RBX::DataModel::Lock lock(m_dataModel);
		RBX::stopListening<RBX::Selection, RBX::SelectionChanged>(m_dataModel->find<RBX::Selection>(), this);
	}

	m_dataModel = dataModel;

	if (m_dataModel) {
		RBX::DataModel::Lock lock(m_dataModel);
		RBX::startListening<RBX::Selection, RBX::SelectionChanged>(m_dataModel->create<RBX::Selection>(), this);
	}
}

void CSelectionTreeCtrl::activeDataModelChanged(
	std::shared_ptr<RBX::DataModel> newDataModel, std::shared_ptr<RBX::DataModel> oldDataModel
)
{
	setDataModel(newDataModel);
	resetTree();
}

void CSelectionTreeCtrl::mouseReleaseEvent(QMouseEvent* event)
{
	CRobloxTreeCtrl::mouseReleaseEvent(event);

	if (event->button() == Qt::LeftButton && m_dragImage.get()) {
		m_dragImage.reset();

		QTreeWidgetItem* target = dropTargetItem();
		if (target) {
			CRobloxTreeCtrlNode* node = static_cast<CRobloxTreeCtrlItem*>(target)->node();
			if (node) {
				std::shared_ptr<RBX::Instance> instance = node->instance();
				RBX::DataModel::Lock lock(m_dataModel);
				if (instance) {
					std::for_each(
						selection()->begin(),
						selection()->end(),
						[parent = instance.get()](const std::shared_ptr<RBX::Instance>& child) {
							reparent(parent, child);
						}
					);
				}
				m_dataModel->clearStack();
				m_dataModel->setDirty(true);
			}
		}

		selectDropTarget(nullptr);
	}
}
