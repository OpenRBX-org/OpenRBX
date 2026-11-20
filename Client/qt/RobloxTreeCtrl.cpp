#include "RobloxTreeCtrl.h"

#include "ReflectionMetadata.h"
#include "Resources.h"
#include "Roblox.h"
#include "RobloxTreeCtrlNode.h"
#include "util/scopedassign.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QCursor>
#include <QFocusEvent>
#include <QHeaderView>
#include <QImage>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QRubberBand>
#include <QScrollBar>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTimer>
#include <algorithm>
#include <cstdlib>
#include <iterator>

static const unsigned explorerImagesResourceId = 133;
static const int explorerImageSize = 16;
static const int labelTextLimit = 32;
static const int labelEditInset = 3;
static const int labelEditExtraWidth = 16;
static const QColor labelEditFrame(100, 100, 100);
static const int bandScrollTimerPeriod = 75;
static const int pendingChildrenPerPass = 10;

class LabelEdit : public QLineEdit
{
public:
	explicit LabelEdit(QWidget* parent) : QLineEdit(parent)
	{
		setFrame(false);
		setTextMargins(labelEditInset, 0, 0, 0);
	}

protected:
	void paintEvent(QPaintEvent* event) override
	{
		QLineEdit::paintEvent(event);

		QPainter painter(this);
		painter.setPen(labelEditFrame);
		painter.drawRect(rect().adjusted(0, 0, -1, -1));
	}
};

class CRobloxTreeCtrlDelegate : public TreeItemDelegate
{
public:
	explicit CRobloxTreeCtrlDelegate(CRobloxTreeCtrl* tree) : TreeItemDelegate(tree), m_tree(tree) {}

	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		QLineEdit* editor = new LabelEdit(parent);
		editor->setMaxLength(labelTextLimit);
		return editor;
	}

	void updateEditorGeometry(
		QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index
	) const override
	{
		editor->setGeometry(
			option.rect.left() + labelOffset - labelEditInset,
			option.rect.top() - 1,
			labelWidth(option.font, index.data().toString()) + labelEditExtraWidth,
			option.rect.height() + 1
		);
	}

protected:
	void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
	{
		TreeItemDelegate::initStyleOption(option, index);

		QModelIndex target = m_tree->dropTargetIndex();
		if (target.isValid()) {
			if (index == target) {
				option->state |= QStyle::State_Selected | QStyle::State_Active;
			}
			else {
				option->state &= ~QStyle::State_Selected;
			}
		}
	}

private:
	CRobloxTreeCtrl* m_tree;
};

static RBX::Reflection::Metadata::Class* metadataFor(const RBX::Reflection::ClassDescriptor* descriptor)
{
	return RBX::Reflection::Metadata::Reflection::singleton()->getClass(descriptor, true);
}

static int explorerOrder(RBX::Instance* instance)
{
	RBX::Reflection::Metadata::Class* metadata = metadataFor(&instance->getDescriptor());
	return metadata ? metadata->getExplorerOrder() : 0;
}

static int explorerImageIndex(const RBX::Reflection::ClassDescriptor* descriptor)
{
	RBX::Reflection::Metadata::Class* metadata = metadataFor(descriptor);
	return metadata ? metadata->getExplorerImageIndex() : 0;
}

static void appendVisibleItems(QTreeWidgetItem* parent, QList<QTreeWidgetItem*>& items)
{
	for (int i = 0; i < parent->childCount(); i++) {
		QTreeWidgetItem* child = parent->child(i);
		items.append(child);
		if (child->isExpanded()) {
			appendVisibleItems(child, items);
		}
	}
}

CRobloxTreeCtrlItem::CRobloxTreeCtrlItem(CRobloxTreeCtrl* tree, CRobloxTreeCtrlNode* node)
	: QTreeWidgetItem(QTreeWidgetItem::UserType), m_tree(tree), m_node(node)
{
}

CRobloxTreeCtrlItem::~CRobloxTreeCtrlItem()
{
	while (childCount() > 0) {
		delete child(childCount() - 1);
	}

	m_tree->itemDeleted(this);

	RBX::ScopedAssign<bool> deleting(m_tree->m_deleting, true);
	delete m_node;
}

QVariant CRobloxTreeCtrlItem::data(int column, int role) const
{
	if (column == 0 && (role == Qt::DisplayRole || role == Qt::EditRole)) {
		return m_node->text();
	}

	return QTreeWidgetItem::data(column, role);
}

void CRobloxTreeCtrlItem::setData(int column, int role, const QVariant& value)
{
	if (column == 0 && role == Qt::EditRole) {
		m_node->rename(value.toString());
		return;
	}

	QTreeWidgetItem::setData(column, role, value);
}

void CRobloxTreeCtrlNode::repaintIfNameChanged()
{
	if (m_nameChanged) {
		if (m_item && m_item != m_tree->invisibleRootItem()) {
			static_cast<CRobloxTreeCtrlItem*>(m_item)->invalidate();
		}

		m_nameChanged = false;
	}
}

void CRobloxTreeCtrlNode::queueChild(const std::shared_ptr<RBX::Instance>& instance)
{
	{
		std::scoped_lock lock(m_tree->m_mutex);
		m_pendingChildren.insert(instance);
	}
	m_tree->queueNode(this);
}

CRobloxTreeCtrlNode::~CRobloxTreeCtrlNode()
{
	{
		std::scoped_lock lock(m_tree->m_mutex);

		if (m_parent) {
			m_parent->m_children.erase(m_instance.get());
		}

		m_tree->dequeueNode(this);
		m_tree->m_nodes.erase(m_instance.get());
	}

	RBX::DataModel::Lock lock(m_tree->dataModel());
	m_instance->RBX::Notifier<RBX::Instance, RBX::PropertyChanged>::removeListener(this);
	m_instance->RBX::Notifier<RBX::Instance, RBX::ChildAdded>::removeListener(this);
	m_instance->RBX::Notifier<RBX::Instance, RBX::ChildRemoved>::removeListener(this);
}

void CRobloxTreeCtrlNode::onExpanded()
{
	m_populated = !m_expandingForSelection;
}

QTreeWidgetItem* CRobloxTreeCtrlNode::insertItem()
{
	QTreeWidgetItem* parentItem = m_parent ? m_parent->m_item : m_tree->invisibleRootItem();

	int position = 0;
	for (int i = 0; i < parentItem->childCount(); i++) {
		CRobloxTreeCtrlNode* sibling = static_cast<CRobloxTreeCtrlItem*>(parentItem->child(i))->node();
		if (m_explorerOrder > sibling->m_explorerOrder) {
			position = i + 1;
		}
		else if (m_explorerOrder == sibling->m_explorerOrder) {
			RBX::DataModel::Lock lock(m_tree->dataModel());
			if (!(m_instance->getName() >= sibling->m_instance->getName())) {
				break;
			}
			position = i + 1;
		}
		else {
			break;
		}
	}

	CRobloxTreeCtrlItem* item = new CRobloxTreeCtrlItem(m_tree, this);
	item->setData(0, Qt::DecorationRole, m_tree->image(explorerImageIndex(m_descriptor)));
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	item->setChildIndicatorPolicy(
		hasVisibleChildren() ? QTreeWidgetItem::ShowIndicator : QTreeWidgetItem::DontShowIndicator
	);
	parentItem->insertChild(position, item);
	return item;
}

void CRobloxTreeCtrlNode::onEvent(const RBX::Instance* source, RBX::PropertyChanged event)
{
	bool isName;
	{
		RBX::DataModel::Lock lock(m_tree->dataModel());
		isName = event.getDescriptor() == RBX::Instance::desc_Name;
	}

	if (isName) {
		m_nameChanged = true;
		m_tree->queueNode(this);
	}
}

bool CRobloxTreeCtrlNode::hasVisibleChildren()
{
	RBX::DataModel::Lock lock(m_tree->dataModel());
	std::shared_ptr<const std::vector<std::shared_ptr<RBX::Instance>>> children = m_instance->getChildren().read();
	if (!children) {
		return false;
	}

	for (const std::shared_ptr<RBX::Instance>& child : *children) {
		RBX::Reflection::Metadata::Class* metadata = metadataFor(&child->getDescriptor());
		if (metadata && metadata->getExplorerOrder() >= 0) {
			return true;
		}
	}
	return false;
}

void CRobloxTreeCtrlNode::updateChildIndicator()
{
	if (m_item && m_item != m_tree->invisibleRootItem()) {
		m_item->setChildIndicatorPolicy(
			hasVisibleChildren() ? QTreeWidgetItem::ShowIndicator : QTreeWidgetItem::DontShowIndicator
		);
	}
}

QString CRobloxTreeCtrlNode::text()
{
	try {
		RBX::DataModel::Lock lock(m_tree->dataModel());
		return QString::fromStdString(m_instance->getName());
	}
	catch (std::exception&) {
		return QString();
	}
}

void CRobloxTreeCtrlNode::rename(const QString& name)
{
	RBX::DataModel::Lock lock(m_tree->dataModel());
	m_instance->setName(name.toStdString());
}

void CRobloxTreeCtrlNode::syncSelection()
{
	if (m_item) {
		if (m_tree->selection() && m_tree->selection()->isSelected(m_instance.get())) {
			for (CRobloxTreeCtrlNode* node = m_parent; node; node = node->m_parent) {
				RBX::ScopedAssign<bool> assign(node->m_expandingForSelection, true);
				node->m_tree->expandTreeItem(node->m_item, EXPAND_EXPAND | EXPAND_PARTIAL);
			}

			m_tree->selectItems(m_item, m_item, false);
			m_tree->ensureItemVisible(m_item);
		}
		else {
			m_tree->setItemState(m_item, 0, ITEM_SELECTED);
		}
	}
}

void CRobloxTreeCtrlNode::onEvent(const RBX::Instance* source, RBX::ChildAdded event)
{
	if (m_populated) {
		std::scoped_lock lock(m_tree->m_mutex);
		m_pendingChildren.insert(event.child);
	}

	m_tree->queueNode(this);
}

void CRobloxTreeCtrlNode::deleteRemovedChildren()
{
	std::vector<CRobloxTreeCtrlNode*> nodes;

	{
		std::scoped_lock lock(m_tree->m_mutex);
		std::copy(m_removedChildren.begin(), m_removedChildren.end(), std::back_inserter(nodes));
		m_removedChildren.clear();
	}

	for (CRobloxTreeCtrlNode* node : nodes) {
		m_tree->deleteItem(node->m_item);
	}
}

void CRobloxTreeCtrlNode::onEvent(const RBX::Instance* source, RBX::ChildRemoved event)
{
	{
		std::scoped_lock lock(m_tree->m_mutex);
		m_tree->m_nodes.erase(event.child.get());

		std::map<RBX::Instance*, CRobloxTreeCtrlNode*>::iterator it = m_children.find(event.child.get());
		if (it != m_children.end()) {
			m_removedChildren.insert(it->second);
		}

		m_pendingChildren.erase(event.child);
	}

	m_tree->queueNode(this);
}

CRobloxTreeCtrlNode::CRobloxTreeCtrlNode(
	CRobloxTreeCtrlNode* parent, std::shared_ptr<RBX::Instance> instance, CRobloxTreeCtrl* tree
)
	: m_instance(instance), m_nameChanged(false), m_populated(false), m_expandingForSelection(false), m_tree(tree),
	  m_parent(parent), m_item(nullptr), m_explorerOrder(explorerOrder(instance.get())),
	  m_descriptor(&instance->getDescriptor())
{
	{
		std::scoped_lock lock(tree->m_mutex);
		tree->m_nodes[instance.get()] = this;

		if (parent) {
			parent->m_children[instance.get()] = this;
		}
	}

	RBX::DataModel::Lock lock(tree->dataModel());
	instance->RBX::Notifier<RBX::Instance, RBX::PropertyChanged>::addListener(this);
}

void CRobloxTreeCtrlNode::create()
{
	m_item = insertItem();
	syncSelection();

	RBX::DataModel::Lock lock(m_tree->dataModel());
	m_instance->RBX::Notifier<RBX::Instance, RBX::ChildAdded>::addListener(this);
	m_instance->RBX::Notifier<RBX::Instance, RBX::ChildRemoved>::addListener(this);
}

CRobloxTreeCtrlNode* CRobloxTreeCtrlNode::findOrCreateChild(const std::shared_ptr<RBX::Instance>& instance)
{
	RBX::Reflection::Metadata::Class* metadata = metadataFor(&instance->getDescriptor());
	if (!metadata || metadata->getExplorerOrder() < 0) {
		return nullptr;
	}

	{
		std::scoped_lock lock(m_tree->m_mutex);
		std::map<RBX::Instance*, CRobloxTreeCtrlNode*>::iterator it = m_children.find(instance.get());
		if (it != m_children.end()) {
			return it->second;
		}
	}

	CRobloxTreeCtrlNode* node = new CRobloxTreeCtrlNode(this, instance, m_tree);
	node->create();
	return node;
}

void CRobloxTreeCtrlNode::queueAllChildren()
{
	std::vector<std::shared_ptr<RBX::Instance>> children;

	{
		RBX::DataModel::Lock lock(m_tree->dataModel());
		if (m_instance->getChildren()) {
			children = *m_instance->getChildren();
		}
	}

	for (const std::shared_ptr<RBX::Instance>& child : children) {
		queueChild(child);
	}
}

void CRobloxTreeCtrlNode::insertPendingChildren()
{
	for (int i = 0; i < pendingChildrenPerPass; i++) {
		std::shared_ptr<RBX::Instance> instance;
		{
			std::scoped_lock lock(m_tree->m_mutex);
			auto it = m_pendingChildren.begin();
			if (it == m_pendingChildren.end()) {
				return;
			}
			instance = *it;
			m_pendingChildren.erase(it);
		}
		findOrCreateChild(instance);
	}
}

RootNode::RootNode(const std::shared_ptr<RBX::Instance>& instance, CRobloxTreeCtrl* tree)
	: CRobloxTreeCtrlNode(nullptr, instance, tree)
{
	m_populated = true;
}

QTreeWidgetItem* RootNode::insertItem()
{
	return m_tree->invisibleRootItem();
}

void RootNode::syncSelection()
{
}

QString RootNode::text()
{
	return QString();
}

CRobloxTreeCtrl::CRobloxTreeCtrl(QWidget* parent) : TreeWidget(parent), m_rootNode(nullptr), m_deleting(false)
{
	setColumnCount(1);
	setHeaderHidden(true);
	setRootIsDecorated(true);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setExpandsOnDoubleClick(false);
	setAutoScroll(false);
	setIconSize(QSize(explorerImageSize, explorerImageSize));
	setTextElideMode(Qt::ElideNone);
	header()->setStretchLastSection(false);
	header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	setItemDelegate(new CRobloxTreeCtrlDelegate(this));

	loadImages();

	QPalette colors = palette();
	QColor face = colors.color(QPalette::Active, QPalette::Button);
	QColor highlight = colors.color(QPalette::Active, QPalette::Highlight);
	colors.setColor(
		QPalette::Inactive,
		QPalette::Highlight,
		QColor(
			(face.red() + highlight.red()) / 2,
			(face.green() + highlight.green()) / 2,
			(face.blue() + highlight.blue()) / 2
		)
	);
	colors.setColor(
		QPalette::Inactive, QPalette::HighlightedText, colors.color(QPalette::Active, QPalette::WindowText)
	);
	setPalette(colors);

	m_labelEditTimer = new QTimer(this);
	m_labelEditTimer->setSingleShot(true);
	connect(m_labelEditTimer, &QTimer::timeout, this, [this]() {
		if (m_labelEditItem) {
			editItem(m_labelEditItem, 0);
		}
	});

	m_rubberBand = new QRubberBand(QRubberBand::Rectangle, viewport());
	m_rubberBand->hide();

	m_bandTimer = new QTimer(this);
	connect(m_bandTimer, &QTimer::timeout, this, [this]() {
		QPoint point = viewport()->mapFromGlobal(QCursor::pos());
		if (!viewport()->rect().contains(point)) {
			updateBanding(point);
		}
	});
}

CRobloxTreeCtrl::~CRobloxTreeCtrl() = default;

void CRobloxTreeCtrl::loadImages()
{
	QImage strip(Resources::pngPath(QStringLiteral("png_%1").arg(explorerImagesResourceId)));
	if (strip.isNull()) {
		return;
	}

	bool masked = !strip.hasAlphaChannel();
	strip = strip.convertToFormat(QImage::Format_ARGB32);
	if (masked) {
		QRgb mask = qRgb(255, 0, 255);
		for (int y = 0; y < strip.height(); y++) {
			QRgb* line = reinterpret_cast<QRgb*>(strip.scanLine(y));
			for (int x = 0; x < strip.width(); x++) {
				if (line[x] == mask) {
					line[x] = qRgba(0, 0, 0, 0);
				}
			}
		}
	}

	for (int x = 0; x + explorerImageSize <= strip.width(); x += explorerImageSize) {
		m_images.push_back(strip.copy(x, 0, explorerImageSize, explorerImageSize));
	}
}

QImage CRobloxTreeCtrl::image(int index) const
{
	if (index < 0 || index >= static_cast<int>(m_images.size())) {
		return QImage();
	}
	return m_images[index];
}

QModelIndex CRobloxTreeCtrl::dropTargetIndex() const
{
	return m_dropTargetItem ? indexFromItem(m_dropTargetItem) : QModelIndex();
}

void CRobloxTreeCtrl::deleteAllItems()
{
	m_mouseMode = MOUSE_IDLE;
	m_labelEditTimer->stop();
	m_bandTimer->stop();
	m_rubberBand->hide();

	m_caretItem = nullptr;
	setCurrentItem(nullptr, 0, QItemSelectionModel::NoUpdate);

	while (topLevelItemCount() > 0) {
		delete topLevelItem(topLevelItemCount() - 1);
	}
}

void CRobloxTreeCtrl::resetTree()
{
	RBX::ScopedAssign<bool> clearing(m_deleting, true);
	deleteAllItems();
	if (m_rootNode) {
		delete m_rootNode;
		m_rootNode = nullptr;
	}
}

void CRobloxTreeCtrl::itemDeleted(QTreeWidgetItem* item)
{
	if (m_caretItem == item) {
		m_caretItem = nullptr;
	}
	if (m_shiftAnchorItem == item) {
		m_shiftAnchorItem = nullptr;
	}
	if (m_dropTargetItem == item) {
		m_dropTargetItem = nullptr;
	}
	if (m_pressItem == item) {
		m_pressItem = nullptr;
	}
	if (m_labelEditItem == item) {
		m_labelEditItem = nullptr;
	}
	m_bandLockedItems.removeAll(item);
}

void CRobloxTreeCtrl::deleteItem(QTreeWidgetItem* item)
{
	if (!item) {
		return;
	}

	for (QTreeWidgetItem* caret = m_caretItem; caret; caret = caret->parent()) {
		if (caret == item) {
			m_caretItem = nullptr;
			break;
		}
	}
	for (QTreeWidgetItem* current = currentItem(); current; current = current->parent()) {
		if (current == item) {
			setCurrentItem(nullptr, 0, QItemSelectionModel::NoUpdate);
			break;
		}
	}

	delete item;
	syncCurrentItem();
}

void CRobloxTreeCtrl::syncCurrentItem()
{
	if (currentItem() != m_caretItem) {
		setCurrentItem(m_caretItem, 0, QItemSelectionModel::NoUpdate);
	}
}

CRobloxTreeCtrlNode* CRobloxTreeCtrl::findOrCreateNode(const std::shared_ptr<RBX::Instance>& instance)
{
	if (!instance) {
		return nullptr;
	}

	{
		std::scoped_lock lock(m_mutex);
		std::map<RBX::Instance*, CRobloxTreeCtrlNode*>::iterator it = m_nodes.find(instance.get());
		if (it != m_nodes.end()) {
			return it->second;
		}
	}

	CRobloxTreeCtrlNode* parent = findOrCreateNode(RBX::shared_from(instance->getParent()));
	if (!parent) {
		return nullptr;
	}
	return parent->findOrCreateChild(instance);
}

QItemSelectionModel::SelectionFlags CRobloxTreeCtrl::selectionCommand(
	const QModelIndex& index, const QEvent* event
) const
{
	return QItemSelectionModel::NoUpdate;
}

bool CRobloxTreeCtrl::onSelectionChanging(const TreeSelectionChange& change)
{
	return false;
}

void CRobloxTreeCtrl::onSelectionChanged(const TreeSelectionChange& change)
{
}

void CRobloxTreeCtrl::onBeginDrag(QTreeWidgetItem* item, const QPoint& point)
{
}

void CRobloxTreeCtrl::onContextMenu(const QPoint& globalPoint)
{
}

void CRobloxTreeCtrl::onRightButtonDown(const QPoint& point)
{
}

bool CRobloxTreeCtrl::isItemSelected(const QTreeWidgetItem* item) const
{
	return item && item->isSelected();
}

bool CRobloxTreeCtrl::selectCaretItem(QTreeWidgetItem* item, TreeSelectionAction action)
{
	if (item == m_caretItem) {
		return true;
	}

	TreeSelectionChange change;
	change.oldItem = m_caretItem;
	change.newItem = item;
	change.action = action;

	if (onSelectionChanging(change)) {
		return false;
	}

	if (m_caretItem) {
		m_caretItem->setSelected(false);
	}

	m_caretItem = item;
	if (item) {
		item->setSelected(true);
		ensureItemVisible(item);
	}
	syncCurrentItem();

	onSelectionChanged(change);
	return true;
}

bool CRobloxTreeCtrl::setItemState(QTreeWidgetItem* item, unsigned state, unsigned mask)
{
	if (!item) {
		return false;
	}

	QTreeWidgetItem* focus = m_caretItem;
	bool wasFocus = focus == item;
	bool focusWasSelected = focus && isItemSelected(focus);
	bool wasSelected = isItemSelected(item);

	unsigned newState = state & ~ITEM_FOCUSED;
	unsigned newMask = mask & ~ITEM_FOCUSED;

	if (mask & ITEM_FOCUSED) {
		if (state & ITEM_FOCUSED) {
			if (!wasFocus && focus) {
				selectCaretItem(nullptr, SELECTION_UNKNOWN);
				focus->setSelected(true);
			}

			if (!selectCaretItem(item, SELECTION_UNKNOWN)) {
				return false;
			}

			if (!wasFocus && focus) {
				focus->setSelected(focusWasSelected);
			}

			if (!(mask & ITEM_SELECTED)) {
				if (!wasSelected) {
					newState &= ~ITEM_SELECTED;
					newMask |= ITEM_SELECTED;
				}
			}
		}
		else if (wasFocus) {
			selectCaretItem(nullptr, SELECTION_UNKNOWN);
			if (!(mask & ITEM_SELECTED)) {
				if (wasSelected) {
					item->setSelected(true);
				}
			}
			else if (state & ITEM_SELECTED) {
				if (wasSelected) {
					item->setSelected(true);
				}
				newState &= ~ITEM_SELECTED;
				newMask &= ~ITEM_SELECTED;
			}
		}
	}

	if (!newMask) {
		return true;
	}

	if (newMask & ITEM_SELECTED) {
		TreeSelectionChange change;
		change.action = SELECTION_EMULATED;
		if (newState & ITEM_SELECTED) {
			change.newItem = item;
		}
		else {
			change.oldItem = item;
		}

		if (onSelectionChanging(change)) {
			return false;
		}

		item->setSelected(newState & ITEM_SELECTED);
		onSelectionChanged(change);
	}

	return true;
}

bool CRobloxTreeCtrl::selectItem(QTreeWidgetItem* item)
{
	return setItemState(item, ITEM_SELECTED | ITEM_FOCUSED, ITEM_SELECTED | ITEM_FOCUSED);
}

QList<QTreeWidgetItem*> CRobloxTreeCtrl::visibleItems() const
{
	QList<QTreeWidgetItem*> items;
	appendVisibleItems(invisibleRootItem(), items);
	return items;
}

QList<QTreeWidgetItem*> CRobloxTreeCtrl::selectedVisibleItems() const
{
	QList<QTreeWidgetItem*> selected;
	for (QTreeWidgetItem* item : visibleItems()) {
		if (isItemSelected(item)) {
			selected.append(item);
		}
	}
	return selected;
}

unsigned CRobloxTreeCtrl::selectedCount() const
{
	return static_cast<unsigned>(selectedVisibleItems().size());
}

void CRobloxTreeCtrl::selectAllItems(bool select)
{
	for (QTreeWidgetItem* item : visibleItems()) {
		if (isItemSelected(item) != select) {
			setItemState(item, select ? ITEM_SELECTED : 0, ITEM_SELECTED);
		}
	}
}

void CRobloxTreeCtrl::selectAllIgnore(bool select, QTreeWidgetItem* ignore)
{
	for (QTreeWidgetItem* item : visibleItems()) {
		if (item != ignore) {
			if (isItemSelected(item) != select) {
				setItemState(item, select ? ITEM_SELECTED : 0, ITEM_SELECTED);
			}
		}
	}
}

void CRobloxTreeCtrl::selectItems(QTreeWidgetItem* first, QTreeWidgetItem* last, bool only)
{
	QList<QTreeWidgetItem*> items = visibleItems();
	qsizetype index = 0;

	while (index < items.size()) {
		QTreeWidgetItem* item = items[index];
		if (item == first || item == last) {
			if (first != last) {
				if (!isItemSelected(item)) {
					setItemState(item, ITEM_SELECTED, ITEM_SELECTED);
				}
				index++;
			}
			break;
		}

		if (only && isItemSelected(item)) {
			setItemState(item, 0, ITEM_SELECTED);
		}

		index++;
	}

	while (index < items.size()) {
		QTreeWidgetItem* item = items[index];
		if (!isItemSelected(item)) {
			setItemState(item, ITEM_SELECTED, ITEM_SELECTED);
		}
		index++;
		if (item == first || item == last) {
			break;
		}
	}

	if (!only) {
		return;
	}

	while (index < items.size()) {
		if (isItemSelected(items[index])) {
			setItemState(items[index], 0, ITEM_SELECTED);
		}
		index++;
	}
}

void CRobloxTreeCtrl::notifyItemExpanding(CRobloxTreeCtrlItem* item, unsigned code)
{
	CRobloxTreeCtrlNode* node = item->node();
	if (code & EXPAND_EXPAND) {
		if (!node->isExpandingForSelection()) {
			node->queueAllChildren();
			node->insertPendingChildren();
		}
	}
}

void CRobloxTreeCtrl::notifyItemExpanded(CRobloxTreeCtrlItem* item)
{
	item->node()->onExpanded();
}

bool CRobloxTreeCtrl::expandTreeItem(QTreeWidgetItem* item, unsigned code, bool byUser)
{
	if (!item || item == invisibleRootItem()) {
		return false;
	}

	CRobloxTreeCtrlItem* treeItem = static_cast<CRobloxTreeCtrlItem*>(item);
	bool partial = code & EXPAND_PARTIAL;
	unsigned action = code & EXPAND_TOGGLE;
	if (action == EXPAND_TOGGLE) {
		action = !item->isExpanded() || treeItem->m_partiallyExpanded ? EXPAND_EXPAND : EXPAND_COLLAPSE;
	}

	bool notify = byUser || !treeItem->m_expandedOnce;

	if (action == EXPAND_EXPAND) {
		if (item->isExpanded() && (!treeItem->m_partiallyExpanded || partial)) {
			return false;
		}

		if (notify) {
			notifyItemExpanding(treeItem, EXPAND_EXPAND | (partial ? EXPAND_PARTIAL : 0));
		}

		if (item->childCount() == 0) {
			return false;
		}

		treeItem->m_partiallyExpanded = partial;
		item->setExpanded(true);
		viewport()->update();

		if (notify) {
			notifyItemExpanded(treeItem);
			treeItem->m_expandedOnce = true;
		}
		return true;
	}

	if (!item->isExpanded()) {
		return false;
	}

	if (notify) {
		notifyItemExpanding(treeItem, EXPAND_COLLAPSE);
	}

	for (QTreeWidgetItem* caret = m_caretItem ? m_caretItem->parent() : nullptr; caret; caret = caret->parent()) {
		if (caret == item) {
			selectCaretItem(item, SELECTION_UNKNOWN);
			break;
		}
	}

	treeItem->m_partiallyExpanded = false;
	item->setExpanded(false);
	viewport()->update();

	if (notify) {
		notifyItemExpanded(treeItem);
	}
	return true;
}

void CRobloxTreeCtrl::expandWholeTree(QTreeWidgetItem* item)
{
	expandTreeItem(item, EXPAND_EXPAND, true);
	for (int i = 0; i < item->childCount(); i++) {
		expandWholeTree(item->child(i));
	}
}

void CRobloxTreeCtrl::ensureItemVisible(QTreeWidgetItem* item)
{
	if (!item || item == invisibleRootItem()) {
		return;
	}

	std::vector<QTreeWidgetItem*> ancestors;
	for (QTreeWidgetItem* ancestor = item->parent(); ancestor; ancestor = ancestor->parent()) {
		ancestors.push_back(ancestor);
	}
	for (std::vector<QTreeWidgetItem*>::reverse_iterator it = ancestors.rbegin(); it != ancestors.rend(); ++it) {
		if (!(*it)->isExpanded()) {
			expandTreeItem(*it, EXPAND_EXPAND);
		}
	}

	scrollToItem(item, QAbstractItemView::EnsureVisible);
}

void CRobloxTreeCtrl::selectDropTarget(QTreeWidgetItem* item)
{
	if (m_dropTargetItem != item) {
		m_dropTargetItem = item;
		viewport()->update();
	}
}

TreeHitTest CRobloxTreeCtrl::hitTest(const QPoint& point) const
{
	TreeHitTest hit;
	hit.item = itemAt(point);
	if (!hit.item) {
		hit.flags = HIT_NOWHERE;
		return hit;
	}

	QRect cell = visualItemRect(hit.item);
	QRect label = itemLabelRect(hit.item);

	if (point.x() < cell.left()) {
		bool hasButton = hit.item->childIndicatorPolicy() == QTreeWidgetItem::ShowIndicator;
		if (hasButton && point.x() >= cell.left() - indentation()) {
			hit.flags = HIT_ON_ITEM_BUTTON;
		}
		else {
			hit.flags = HIT_ON_ITEM_INDENT;
		}
	}
	else if (point.x() < label.left()) {
		hit.flags = HIT_ON_ITEM_ICON;
	}
	else if (point.x() < label.left() + label.width()) {
		hit.flags = HIT_ON_ITEM_LABEL;
	}
	else {
		hit.flags = HIT_ON_ITEM_RIGHT;
	}
	return hit;
}

bool CRobloxTreeCtrl::exceedsDragDistance(const QPoint& point) const
{
	int distance = QApplication::startDragDistance();
	return std::abs(point.x() - m_pressPoint.x()) > distance || std::abs(point.y() - m_pressPoint.y()) > distance;
}

void CRobloxTreeCtrl::focusInEvent(QFocusEvent* event)
{
	if (event->reason() == Qt::MouseFocusReason) {
		m_focusedByMouse = true;
	}

	TreeWidget::focusInEvent(event);
	syncCurrentItem();
}

void CRobloxTreeCtrl::mousePressEvent(QMouseEvent* event)
{
	bool hadFocus = !m_focusedByMouse;
	m_focusedByMouse = false;

	if (m_mouseMode != MOUSE_IDLE) {
		return;
	}

	m_labelEditTimer->stop();

	QPoint point = event->position().toPoint();
	if (event->button() == Qt::LeftButton) {
		leftButtonDown(point, event->modifiers(), hadFocus);
	}
	else if (event->button() == Qt::RightButton) {
		rightButtonDown(point);
	}
}

void CRobloxTreeCtrl::leftButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers, bool hadFocus)
{
	TreeHitTest hit = hitTest(point);
	QTreeWidgetItem* caret = m_caretItem;

	if (hit.flags & HIT_ON_ITEM_BUTTON) {
		expandTreeItem(hit.item, EXPAND_TOGGLE, true);
		return;
	}

	if (!(modifiers & (Qt::ControlModifier | Qt::ShiftModifier)) && (hit.flags & HIT_ON_ITEM_LABEL)) {
		if (m_okToEdit && hit.item == caret) {
			selectAllItems(false);
			selectItem(hit.item);

			m_mouseMode = MOUSE_NATIVE_LEFT;
			m_pressItem = hit.item;
			m_pressPoint = point;
			m_pressModifiers = modifiers;
			m_pressHadFocus = hadFocus;
			return;
		}
	}

	m_okToEdit = hit.flags & HIT_ON_ITEM;

	buttonDown(modifiers, point);
}

void CRobloxTreeCtrl::rightButtonDown(const QPoint& point)
{
	TreeHitTest hit = hitTest(point);
	if (hit.item && (hit.flags & HIT_ON_ITEM)) {
		if (!isItemSelected(hit.item)) {
			selectAllItems(false);
			selectItem(hit.item);
		}
	}
	else {
		selectAllItems(false);
	}

	m_mouseMode = MOUSE_NATIVE_RIGHT;
	m_pressItem = hit.item;
	m_pressPoint = point;
}

void CRobloxTreeCtrl::buttonDown(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
	TreeHitTest hit = hitTest(point);
	if (hit.item && (hit.flags & HIT_ON_ITEM_BUTTON)) {
		return;
	}

	if (!hit.item || (hit.flags & (HIT_ON_ITEM_RIGHT | HIT_NOWHERE | HIT_ON_ITEM_INDENT))) {
		beginBanding(modifiers, point);
		return;
	}

	preSelect(hit.item, modifiers);

	m_mouseMode = MOUSE_ACTION;
	m_pressItem = hit.item;
	m_pressPoint = point;
	m_pressModifiers = modifiers;
}

void CRobloxTreeCtrl::preSelect(QTreeWidgetItem* item, Qt::KeyboardModifiers modifiers)
{
	if (modifiers & Qt::ShiftModifier) {
		if (!m_shiftAnchorItem) {
			m_shiftAnchorItem = m_caretItem;
		}

		setItemState(item, ITEM_FOCUSED, ITEM_FOCUSED);
		selectItems(m_shiftAnchorItem, item, !modifiers.testFlag(Qt::ControlModifier));
	}
	else {
		if (!(modifiers & Qt::ControlModifier)) {
			if (!isItemSelected(item)) {
				selectAllIgnore(false, item);
			}
			setItemState(item, ITEM_SELECTED | ITEM_FOCUSED, ITEM_SELECTED | ITEM_FOCUSED);
		}
		m_shiftAnchorItem = nullptr;
	}
}

void CRobloxTreeCtrl::finishAction(bool drag)
{
	QTreeWidgetItem* item = m_pressItem;
	Qt::KeyboardModifiers modifiers = m_pressModifiers;
	QPoint point = m_pressPoint;
	m_mouseMode = MOUSE_IDLE;

	if (!item) {
		return;
	}

	if (!drag) {
		if (!(modifiers & Qt::ShiftModifier)) {
			if (modifiers & Qt::ControlModifier) {
				unsigned state = isItemSelected(item) ? 0 : ITEM_SELECTED;
				setItemState(item, ITEM_FOCUSED | state, ITEM_FOCUSED | ITEM_SELECTED);
			}
			else {
				selectAllIgnore(false, item);
				setItemState(item, ITEM_FOCUSED | ITEM_SELECTED, ITEM_FOCUSED | ITEM_SELECTED);
			}
		}

		if (!hasFocus()) {
			setFocus();
		}
	}
	else {
		setItemState(item, ITEM_FOCUSED | ITEM_SELECTED, ITEM_FOCUSED | ITEM_SELECTED);
		onBeginDrag(item, point);
	}
}

void CRobloxTreeCtrl::mouseMoveEvent(QMouseEvent* event)
{
	QPoint point = event->position().toPoint();

	switch (m_mouseMode) {
	case MOUSE_ACTION:
		if (exceedsDragDistance(point)) {
			finishAction(true);
		}
		break;
	case MOUSE_NATIVE_LEFT:
		if (exceedsDragDistance(point)) {
			QTreeWidgetItem* item = m_pressItem;
			QPoint pressPoint = m_pressPoint;
			m_mouseMode = MOUSE_IDLE;
			if (item) {
				onBeginDrag(item, pressPoint);
			}
		}
		break;
	case MOUSE_NATIVE_RIGHT:
		if (exceedsDragDistance(point)) {
			QPoint pressPoint = m_pressPoint;
			m_mouseMode = MOUSE_IDLE;
			onRightButtonDown(pressPoint);
		}
		break;
	case MOUSE_BANDING:
		updateBanding(point);
		break;
	default:
		break;
	}
}

void CRobloxTreeCtrl::mouseReleaseEvent(QMouseEvent* event)
{
	m_focusedByMouse = false;

	switch (m_mouseMode) {
	case MOUSE_ACTION:
		finishAction(false);
		break;
	case MOUSE_NATIVE_LEFT:
		m_mouseMode = MOUSE_IDLE;
		if (m_pressHadFocus && m_pressItem) {
			m_labelEditItem = m_pressItem;
			m_labelEditTimer->start(QApplication::doubleClickInterval());
		}
		break;
	case MOUSE_NATIVE_RIGHT: {
		QPoint pressPoint = m_pressPoint;
		m_mouseMode = MOUSE_IDLE;
		onContextMenu(event->globalPosition().toPoint());
		onRightButtonDown(pressPoint);
		break;
	}
	case MOUSE_BANDING:
		endBanding();
		break;
	default:
		break;
	}
}

void CRobloxTreeCtrl::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton || m_mouseMode != MOUSE_IDLE) {
		return;
	}

	m_labelEditTimer->stop();

	QPoint point = event->position().toPoint();
	TreeHitTest hit = hitTest(point);
	if (hit.item && (hit.flags & (HIT_ON_ITEM_BUTTON | HIT_ON_ITEM))) {
		expandTreeItem(hit.item, EXPAND_TOGGLE, true);
	}

	if (event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier)) {
		return;
	}

	hit = hitTest(point);
	if (hit.item && (hit.flags & HIT_ON_ITEM)) {
		CRobloxTreeCtrlNode* node = static_cast<CRobloxTreeCtrlItem*>(hit.item)->node();
		if (node) {
			std::shared_ptr<RBX::Script> script = std::dynamic_pointer_cast<RBX::Script>(node->instance());
			if (script) {
				RBX::DataModel::Lock lock(dataModel());
				theApp.openScriptDocument(script);
			}
		}
	}
}

void CRobloxTreeCtrl::contextMenuEvent(QContextMenuEvent* event)
{
	if (event->reason() == QContextMenuEvent::Keyboard) {
		onContextMenu(QPoint(-1, -1));
	}
	event->accept();
}

void CRobloxTreeCtrl::beginBanding(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
	if (!hasFocus()) {
		setFocus();
	}

	m_bandLockedItems.clear();
	if (modifiers & (Qt::ShiftModifier | Qt::ControlModifier)) {
		m_bandLockedItems = selectedVisibleItems();
	}

	m_mouseMode = MOUSE_BANDING;
	m_pressItem = nullptr;
	m_pressModifiers = modifiers;
	m_bandDragging = false;
	m_bandOrigin = point + QPoint(horizontalOffset(), verticalOffset());
	m_bandTimer->start(bandScrollTimerPeriod);
}

void CRobloxTreeCtrl::updateBanding(const QPoint& point)
{
	if (!m_bandDragging) {
		int distance = QApplication::startDragDistance();
		if (std::abs(point.x() - m_bandOrigin.x()) <= distance && std::abs(point.y() - m_bandOrigin.y()) <= distance) {
			return;
		}

		m_bandDragging = true;
		if (!(m_pressModifiers & (Qt::ControlModifier | Qt::ShiftModifier))) {
			selectAllItems(false);
		}
	}

	QRect client = viewport()->rect();
	if (point.y() < client.top()) {
		verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
	}
	else if (point.y() > client.bottom()) {
		verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
	}
	if (point.x() < client.left()) {
		horizontalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
	}
	else if (point.x() > client.right()) {
		horizontalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
	}

	QPoint origin = m_bandOrigin - QPoint(horizontalOffset(), verticalOffset());
	QRect rect = QRect(origin, point).normalized();
	updateSelectionForRect(rect);
	m_rubberBand->setGeometry(rect);
	m_rubberBand->show();
}

void CRobloxTreeCtrl::updateSelectionForRect(const QRect& rect)
{
	for (QTreeWidgetItem* item : visibleItems()) {
		bool selected = isItemSelected(item);
		bool locked = m_bandLockedItems.contains(item);
		if (itemLabelRect(item).intersects(rect)) {
			if (!selected && !locked) {
				setItemState(item, ITEM_SELECTED, ITEM_SELECTED);
			}
			else if ((m_pressModifiers & Qt::ControlModifier) && locked) {
				setItemState(item, 0, ITEM_SELECTED);
			}
			else if ((m_pressModifiers & Qt::ShiftModifier) && locked) {
				m_bandLockedItems.removeOne(item);
			}
		}
		else {
			if (selected && !locked) {
				setItemState(item, 0, ITEM_SELECTED);
			}
			else if (locked) {
				setItemState(item, ITEM_SELECTED, ITEM_SELECTED);
			}
		}
	}
}

void CRobloxTreeCtrl::endBanding()
{
	m_bandTimer->stop();
	m_rubberBand->hide();
	m_mouseMode = MOUSE_IDLE;

	if (!m_bandDragging) {
		if (!(m_pressModifiers & (Qt::ControlModifier | Qt::ShiftModifier))) {
			selectAllItems(false);
		}
	}
}

void CRobloxTreeCtrl::moveCaret(CursorAction action)
{
	QTreeWidgetItem* item = itemFromIndex(moveCursor(action, Qt::NoModifier));
	if (item) {
		selectCaretItem(item, SELECTION_BY_KEYBOARD);
	}
}

void CRobloxTreeCtrl::defaultKeyDown(QKeyEvent* event)
{
	if (event->modifiers() & Qt::ControlModifier) {
		switch (event->key()) {
		case Qt::Key_Left:
			horizontalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
			break;
		case Qt::Key_Right:
			horizontalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
			break;
		case Qt::Key_PageUp:
			verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
			break;
		case Qt::Key_PageDown:
			verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
			break;
		case Qt::Key_Up:
			verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
			break;
		case Qt::Key_Down:
			verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
			break;
		case Qt::Key_Home:
			verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMinimum);
			break;
		case Qt::Key_End:
			verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMaximum);
			break;
		default:
			event->ignore();
			break;
		}
		return;
	}

	bool keypad = event->modifiers().testFlag(Qt::KeypadModifier);

	switch (event->key()) {
	case Qt::Key_Return:
	case Qt::Key_Enter:
	case Qt::Key_F2:
		break;
	case Qt::Key_Up:
		moveCaret(MoveUp);
		break;
	case Qt::Key_Down:
		moveCaret(MoveDown);
		break;
	case Qt::Key_PageUp:
		moveCaret(MovePageUp);
		break;
	case Qt::Key_PageDown:
		moveCaret(MovePageDown);
		break;
	case Qt::Key_Home:
		moveCaret(MoveHome);
		break;
	case Qt::Key_End:
		moveCaret(MoveEnd);
		break;
	case Qt::Key_Left:
		if (m_caretItem && m_caretItem->isExpanded()) {
			expandTreeItem(m_caretItem, EXPAND_COLLAPSE, true);
			break;
		}
		[[fallthrough]];
	case Qt::Key_Backspace:
		if (m_caretItem && m_caretItem->parent()) {
			selectCaretItem(m_caretItem->parent(), SELECTION_BY_KEYBOARD);
		}
		break;
	case Qt::Key_Right:
		if (!m_caretItem) {
			break;
		}
		if (m_caretItem->isExpanded()) {
			if (m_caretItem->childCount() > 0) {
				selectCaretItem(m_caretItem->child(0), SELECTION_BY_KEYBOARD);
			}
		}
		else {
			expandTreeItem(m_caretItem, EXPAND_EXPAND, true);
		}
		break;
	default:
		if (keypad && m_caretItem && event->key() == Qt::Key_Plus) {
			expandTreeItem(m_caretItem, EXPAND_EXPAND, true);
		}
		else if (keypad && m_caretItem && event->key() == Qt::Key_Minus) {
			expandTreeItem(m_caretItem, EXPAND_COLLAPSE, true);
		}
		else if (keypad && m_caretItem && event->key() == Qt::Key_Asterisk) {
			expandWholeTree(m_caretItem);
		}
		else if (!event->text().isEmpty() && event->text().at(0).isPrint() && !(event->modifiers() & Qt::AltModifier)) {
			QTreeWidget::keyboardSearch(event->text());
			QTreeWidgetItem* found = currentItem();
			if (found != m_caretItem) {
				if (!found || !selectCaretItem(found, SELECTION_BY_KEYBOARD)) {
					syncCurrentItem();
				}
			}
		}
		else {
			event->ignore();
		}
		break;
	}
}

void CRobloxTreeCtrl::keyPressEvent(QKeyEvent* event)
{
	int key = event->key();

	if (m_mouseMode == MOUSE_BANDING && key == Qt::Key_Escape) {
		selectAllItems(false);
		endBanding();
		return;
	}

	if (key == Qt::Key_Return || key == Qt::Key_Enter) {
		if (m_caretItem) {
			expandTreeItem(m_caretItem, EXPAND_TOGGLE);
		}
	}

	if (key == Qt::Key_F2) {
		if (m_caretItem) {
			editItem(m_caretItem, 0);
		}
	}

	bool control = event->modifiers().testFlag(Qt::ControlModifier);
	bool shift = event->modifiers().testFlag(Qt::ShiftModifier);

	if (!control && !shift) {
		unsigned count = selectedCount();
		if (count > 1 || (count == 1 && !isItemSelected(m_caretItem))) {
			selectAllItems(false);
		}
	}

	QTreeWidgetItem* selected = nullptr;
	switch (key) {
	case Qt::Key_Up:
	case Qt::Key_Down:
	case Qt::Key_PageUp:
	case Qt::Key_PageDown:
		selected = m_caretItem;
		if (!m_shiftAnchorItem) {
			m_shiftAnchorItem = selected;
		}
		if (!control && !shift) {
			m_shiftAnchorItem = nullptr;
		}
		break;
	default:
		break;
	}

	bool upward = key == Qt::Key_Up || key == Qt::Key_PageUp;

	defaultKeyDown(event);
	if (!selected || (!control && !shift)) {
		return;
	}

	QTreeWidgetItem* next = key == Qt::Key_PageDown || key == Qt::Key_PageUp ? m_caretItem
							: upward                                         ? itemAbove(selected)
																			 : itemBelow(selected);
	if (!next) {
		next = selected;
	}

	if (shift) {
		selectItems(m_shiftAnchorItem, next, true);
	}
	else if (control) {
		setItemState(next, ITEM_FOCUSED, ITEM_FOCUSED);
	}
}

void CRobloxTreeCtrl::drawBranches(QPainter* painter, const QRect& branches, const QModelIndex& index) const
{
	QRect rect = branches.translated(0, buttonDrop);
	TreeWidget::drawBranches(painter, branches, index);

	QTreeWidgetItem* item = itemFromIndex(index);
	if (!item || !static_cast<CRobloxTreeCtrlItem*>(item)->isPartiallyExpanded()) {
		return;
	}

	QStyleOptionViewItem option;
	initViewItemOption(&option);
	option.rect = QRect(rect.right() + 1 - indentation(), rect.top(), indentation(), rect.height());
	option.state =
		(option.state & (QStyle::State_Enabled | QStyle::State_Active)) | QStyle::State_Item | QStyle::State_Children;

	painter->fillRect(option.rect, palette().brush(QPalette::Base));
	style()->drawPrimitive(QStyle::PE_IndicatorBranch, &option, painter, this);
}
