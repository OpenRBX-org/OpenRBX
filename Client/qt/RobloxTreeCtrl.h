#ifndef ROBLOXTREECTRL_H
#define ROBLOXTREECTRL_H

#include "TreeWidget.h"
#include "script/Script.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Selection.h"
#include "v8tree/Instance.h"

#include <QImage>
#include <QList>
#include <QPoint>
#include <QRect>
#include <QTreeWidget>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

class CRobloxTreeCtrl;
class CRobloxTreeCtrlNode;
class QRubberBand;
class QTimer;

enum TreeSelectionAction
{
	SELECTION_UNKNOWN,
	SELECTION_BY_MOUSE,
	SELECTION_BY_KEYBOARD,
	SELECTION_EMULATED,
};

struct TreeSelectionChange
{
	QTreeWidgetItem* oldItem = nullptr;
	QTreeWidgetItem* newItem = nullptr;
	TreeSelectionAction action = SELECTION_UNKNOWN;
};

enum TreeItemState
{
	ITEM_FOCUSED = 0x1,
	ITEM_SELECTED = 0x2,
};

enum TreeExpandCode
{
	EXPAND_COLLAPSE = 0x1,
	EXPAND_EXPAND = 0x2,
	EXPAND_TOGGLE = 0x3,
	EXPAND_PARTIAL = 0x4000,
};

enum TreeHitFlags
{
	HIT_NOWHERE = 0x1,
	HIT_ON_ITEM_ICON = 0x2,
	HIT_ON_ITEM_LABEL = 0x4,
	HIT_ON_ITEM = HIT_ON_ITEM_ICON | HIT_ON_ITEM_LABEL,
	HIT_ON_ITEM_INDENT = 0x8,
	HIT_ON_ITEM_BUTTON = 0x10,
	HIT_ON_ITEM_RIGHT = 0x20,
};

struct TreeHitTest
{
	QTreeWidgetItem* item = nullptr;
	unsigned flags = 0;
};

class CRobloxTreeCtrlItem : public QTreeWidgetItem
{
public:
	CRobloxTreeCtrlItem(CRobloxTreeCtrl* tree, CRobloxTreeCtrlNode* node);
	~CRobloxTreeCtrlItem() override;

	QVariant data(int column, int role) const override;
	void setData(int column, int role, const QVariant& value) override;

	CRobloxTreeCtrlNode* node() const { return m_node; }
	bool isPartiallyExpanded() const { return m_partiallyExpanded; }
	void invalidate() { emitDataChanged(); }

private:
	friend class CRobloxTreeCtrl;

	CRobloxTreeCtrl* m_tree;
	CRobloxTreeCtrlNode* m_node;
	bool m_partiallyExpanded = false;
	bool m_expandedOnce = false;
};

class CRobloxTreeCtrl : public TreeWidget
{
public:
	explicit CRobloxTreeCtrl(QWidget* parent = nullptr);
	~CRobloxTreeCtrl() override;

	virtual RBX::Selection* selection() = 0;
	virtual std::shared_ptr<RBX::DataModel> dataModel() const = 0;
	virtual void queueNode(CRobloxTreeCtrlNode* node) = 0;
	virtual void dequeueNode(CRobloxTreeCtrlNode* node) = 0;
	virtual void resetTree();

	CRobloxTreeCtrlNode* findOrCreateNode(const std::shared_ptr<RBX::Instance>& instance);

	QTreeWidgetItem* caretItem() const { return m_caretItem; }
	QTreeWidgetItem* dropTargetItem() const { return m_dropTargetItem; }
	QModelIndex dropTargetIndex() const;
	QImage image(int index) const;

	bool isItemSelected(const QTreeWidgetItem* item) const;
	bool setItemState(QTreeWidgetItem* item, unsigned state, unsigned mask);
	bool selectItem(QTreeWidgetItem* item);
	void selectAllItems(bool select);
	void selectItems(QTreeWidgetItem* first, QTreeWidgetItem* last, bool only);
	unsigned selectedCount() const;
	QList<QTreeWidgetItem*> visibleItems() const;
	QList<QTreeWidgetItem*> selectedVisibleItems() const;
	bool expandTreeItem(QTreeWidgetItem* item, unsigned code, bool byUser = false);
	void ensureItemVisible(QTreeWidgetItem* item);
	void selectDropTarget(QTreeWidgetItem* item);
	void deleteItem(QTreeWidgetItem* item);
	TreeHitTest hitTest(const QPoint& point) const;

protected:
	virtual void selectAllIgnore(bool select, QTreeWidgetItem* ignore);
	virtual bool onSelectionChanging(const TreeSelectionChange& change);
	virtual void onSelectionChanged(const TreeSelectionChange& change);
	virtual void onBeginDrag(QTreeWidgetItem* item, const QPoint& point);
	virtual void onContextMenu(const QPoint& globalPoint);
	virtual void onRightButtonDown(const QPoint& point);

	void deleteAllItems();

	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void contextMenuEvent(QContextMenuEvent* event) override;
	void focusInEvent(QFocusEvent* event) override;
	void drawBranches(QPainter* painter, const QRect& branches, const QModelIndex& index) const override;
	QItemSelectionModel::SelectionFlags selectionCommand(
		const QModelIndex& index, const QEvent* event = nullptr
	) const override;

	friend class CRobloxTreeCtrlNode;
	friend class CRobloxTreeCtrlItem;

	CRobloxTreeCtrlNode* m_rootNode;
	std::map<RBX::Instance*, CRobloxTreeCtrlNode*> m_nodes;
	bool m_deleting;
	std::vector<QImage> m_images;
	std::recursive_mutex m_mutex;

private:
	enum MouseMode
	{
		MOUSE_IDLE,
		MOUSE_ACTION,
		MOUSE_NATIVE_LEFT,
		MOUSE_NATIVE_RIGHT,
		MOUSE_BANDING,
	};

	void loadImages();
	void itemDeleted(QTreeWidgetItem* item);
	void syncCurrentItem();
	bool selectCaretItem(QTreeWidgetItem* item, TreeSelectionAction action);
	void notifyItemExpanding(CRobloxTreeCtrlItem* item, unsigned code);
	void notifyItemExpanded(CRobloxTreeCtrlItem* item);
	void expandWholeTree(QTreeWidgetItem* item);

	void leftButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers, bool hadFocus);
	void rightButtonDown(const QPoint& point);
	void buttonDown(Qt::KeyboardModifiers modifiers, const QPoint& point);
	void preSelect(QTreeWidgetItem* item, Qt::KeyboardModifiers modifiers);
	void finishAction(bool drag);
	bool exceedsDragDistance(const QPoint& point) const;
	void beginBanding(Qt::KeyboardModifiers modifiers, const QPoint& point);
	void updateBanding(const QPoint& point);
	void updateSelectionForRect(const QRect& rect);
	void endBanding();
	void defaultKeyDown(QKeyEvent* event);
	void moveCaret(CursorAction action);

	QTreeWidgetItem* m_caretItem = nullptr;
	QTreeWidgetItem* m_shiftAnchorItem = nullptr;
	QTreeWidgetItem* m_dropTargetItem = nullptr;
	bool m_okToEdit = true;
	bool m_focusedByMouse = false;

	MouseMode m_mouseMode = MOUSE_IDLE;
	QTreeWidgetItem* m_pressItem = nullptr;
	QPoint m_pressPoint;
	Qt::KeyboardModifiers m_pressModifiers;
	bool m_pressHadFocus = false;

	QTimer* m_labelEditTimer;
	QTreeWidgetItem* m_labelEditItem = nullptr;

	QPoint m_bandOrigin;
	bool m_bandDragging = false;
	QList<QTreeWidgetItem*> m_bandLockedItems;
	QRubberBand* m_rubberBand;
	QTimer* m_bandTimer;
};

#endif // ROBLOXTREECTRL_H
