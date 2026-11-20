#ifndef ROBLOXTREECTRLNODE_H
#define ROBLOXTREECTRLNODE_H

#include "util/Events.h"
#include "v8tree/Instance.h"

#include <QString>
#include <map>
#include <memory>
#include <set>

class CRobloxTreeCtrl;
class QTreeWidgetItem;

class CRobloxTreeCtrlNode : public RBX::Listener<RBX::Instance, RBX::PropertyChanged>,
							public RBX::Listener<RBX::Instance, RBX::ChildAdded>,
							public RBX::Listener<RBX::Instance, RBX::ChildRemoved>
{
public:
	CRobloxTreeCtrlNode(CRobloxTreeCtrlNode* parent, std::shared_ptr<RBX::Instance> instance, CRobloxTreeCtrl* tree);
	~CRobloxTreeCtrlNode() override;

	virtual void syncSelection();
	virtual void create();
	virtual void onExpanded();
	virtual QString text();
	virtual QTreeWidgetItem* insertItem();

	void repaintIfNameChanged();
	void queueChild(const std::shared_ptr<RBX::Instance>& instance);
	bool hasVisibleChildren();
	void updateChildIndicator();
	void rename(const QString& name);
	void deleteRemovedChildren();
	CRobloxTreeCtrlNode* findOrCreateChild(const std::shared_ptr<RBX::Instance>& instance);
	void queueAllChildren();
	void insertPendingChildren();

	const std::shared_ptr<RBX::Instance>& instance() const { return m_instance; }
	QTreeWidgetItem* item() const { return m_item; }
	bool isExpandingForSelection() const { return m_expandingForSelection; }

protected:
	friend class CRobloxTreeCtrl;
	friend class CSelectionTreeCtrl;

	void onEvent(const RBX::Instance* source, RBX::PropertyChanged event) override;
	void onEvent(const RBX::Instance* source, RBX::ChildAdded event) override;
	void onEvent(const RBX::Instance* source, RBX::ChildRemoved event) override;

	std::shared_ptr<RBX::Instance> m_instance;
	std::map<RBX::Instance*, CRobloxTreeCtrlNode*> m_children;
	std::set<CRobloxTreeCtrlNode*> m_removedChildren;
	bool m_nameChanged;
	bool m_populated;
	bool m_expandingForSelection;
	CRobloxTreeCtrl* m_tree;
	CRobloxTreeCtrlNode* m_parent;
	std::set<std::shared_ptr<RBX::Instance>, std::owner_less<std::shared_ptr<RBX::Instance>>> m_pendingChildren;
	QTreeWidgetItem* m_item;
	int m_explorerOrder;
	const RBX::Reflection::ClassDescriptor* m_descriptor;
};

class RootNode : public CRobloxTreeCtrlNode
{
public:
	RootNode(const std::shared_ptr<RBX::Instance>& instance, CRobloxTreeCtrl* tree);

	void syncSelection() override;
	QString text() override;
	QTreeWidgetItem* insertItem() override;
};

#endif // ROBLOXTREECTRLNODE_H
