#ifndef EXPLORERPANE_H
#define EXPLORERPANE_H

#include "Command.h"

#include <QWidget>

class CSelectionTreeCtrl;

class ExplorerPane : public QWidget, public CommandTarget
{
public:
	explicit ExplorerPane(QWidget* parent = nullptr);
	~ExplorerPane() override;

	bool hasCommand(unsigned id) const override;
	bool executeCommand(unsigned id) override;
	bool updateCommand(unsigned id, CommandState& state) override;

	CSelectionTreeCtrl* tree() const { return m_tree; }

protected:
	void resizeEvent(QResizeEvent* event) override;

private:
	CSelectionTreeCtrl* m_tree;
};

#endif // EXPLORERPANE_H
