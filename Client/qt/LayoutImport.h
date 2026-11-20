#ifndef LAYOUTIMPORT_H
#define LAYOUTIMPORT_H

#include "CommandBarsOptions.h"

#include <QPoint>
#include <QString>
#include <QStringList>
#include <map>
#include <optional>
#include <vector>

struct ImportedPaneNode
{
	enum Type
	{
		Pane = 0,
		Tabbed = 1,
		Splitter = 2,
		Client = 4,
	};

	int type = Pane;
	unsigned paneId = 0;
	bool horizontal = false;
	int dockingWidth = 0;
	int dockingHeight = 0;
	int selected = 0;
	int dockingHolder = 0;
	std::vector<int> children;
};

struct ImportedLayout
{
	std::map<int, std::vector<std::vector<unsigned>>> dockBars;
	std::map<int, ImportedPaneNode> panes;
	int topContainer = 0;
	int client = 0;
	QStringList commandHistory;
	std::map<unsigned, QPoint> barPositions;
	std::optional<CommandBarsOptions> options;

	bool containsClient(int node) const;
};

bool importLayout(const QString& fileName, ImportedLayout& layout);

#endif // LAYOUTIMPORT_H
