#ifndef SELECTIONTREECTRL_H
#define SELECTIONTREECTRL_H

#include "ActiveDocView.h"
#include "Command.h"
#include "RobloxTreeCtrl.h"
#include "util/Events.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Selection.h"

#include <QPoint>
#include <memory>
#include <set>

class QLabel;
class QTimer;

class CSelectionTreeCtrl : public CRobloxTreeCtrl,
						   public CommandTarget,
						   public ActiveDocView,
						   public RBX::Listener<RBX::Selection, RBX::SelectionChanged>
{
public:
	explicit CSelectionTreeCtrl(QWidget* parent = nullptr);
	~CSelectionTreeCtrl() override;

	bool hasCommand(unsigned id) const override;
	bool executeCommand(unsigned id) override;
	bool updateCommand(unsigned id, CommandState& state) override;

	RBX::Selection* selection() override;
	std::shared_ptr<RBX::DataModel> dataModel() const override;
	void queueNode(CRobloxTreeCtrlNode* node) override;
	void dequeueNode(CRobloxTreeCtrlNode* node) override;
	void resetTree() override;

protected:
	void activeDataModelChanged(
		std::shared_ptr<RBX::DataModel> newDataModel, std::shared_ptr<RBX::DataModel> oldDataModel
	) override;
	void onEvent(const RBX::Selection* source, RBX::SelectionChanged event) override;

	void selectAllIgnore(bool select, QTreeWidgetItem* ignore) override;
	bool onSelectionChanging(const TreeSelectionChange& change) override;
	void onSelectionChanged(const TreeSelectionChange& change) override;
	void onBeginDrag(QTreeWidgetItem* item, const QPoint& point) override;
	void onContextMenu(const QPoint& globalPoint) override;
	void onRightButtonDown(const QPoint& point) override;

	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	void showContextMenu(const QPoint& globalPoint);
	CommandRoute contextMenuRoute();
	bool isCommandEnabled(unsigned id);
	void updateSelection(QTreeWidgetItem* item, bool select);
	QTreeWidgetItem* dropTarget(QTreeWidgetItem* item);
	void insertPendingSelection();
	void setDataModel(std::shared_ptr<RBX::DataModel> dataModel);
	void onTimer();
	QLabel* createDragImage(QTreeWidgetItem* item);

	bool m_selectionChangeBlocked;
	bool m_updatingFromModel;
	bool m_updatingSelection;
	std::set<std::shared_ptr<RBX::Instance>, std::owner_less<std::shared_ptr<RBX::Instance>>> m_pendingSelection;
	std::set<CRobloxTreeCtrlNode*> m_pendingNodes;
	std::shared_ptr<RBX::DataModel> m_dataModel;
	std::unique_ptr<QLabel> m_dragImage;
	QPoint m_dragHotSpot;
	QTimer* m_timer;
};

#endif // SELECTIONTREECTRL_H
