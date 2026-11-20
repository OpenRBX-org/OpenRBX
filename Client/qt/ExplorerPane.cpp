#include "ExplorerPane.h"

#include "SelectionTreeCtrl.h"

#include <QResizeEvent>

ExplorerPane::ExplorerPane(QWidget* parent) : QWidget(parent)
{
	m_tree = new CSelectionTreeCtrl(this);
	m_tree->setGeometry(0, 0, 100, 100);
}

ExplorerPane::~ExplorerPane() = default;

bool ExplorerPane::hasCommand(unsigned id) const
{
	if (CommandTarget::hasCommand(id)) {
		return true;
	}

	return m_tree->hasCommand(id);
}

bool ExplorerPane::executeCommand(unsigned id)
{
	if (CommandTarget::executeCommand(id)) {
		return true;
	}

	return m_tree->executeCommand(id);
}

bool ExplorerPane::updateCommand(unsigned id, CommandState& state)
{
	if (CommandTarget::updateCommand(id, state)) {
		return true;
	}

	return m_tree->updateCommand(id, state);
}

void ExplorerPane::resizeEvent(QResizeEvent* event)
{
	m_tree->setGeometry(0, 0, event->size().width(), event->size().height());
	QWidget::resizeEvent(event);
}
