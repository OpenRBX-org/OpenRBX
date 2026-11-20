#include "LayoutImport.h"

#include <QFile>
#include <QXmlStreamReader>
#include <algorithm>

bool ImportedLayout::containsClient(int node) const
{
	if (node == client) {
		return true;
	}

	auto it = panes.find(node);
	if (it == panes.end()) {
		return false;
	}
	return std::any_of(it->second.children.begin(), it->second.children.end(), [this](int child) {
		return containsClient(child);
	});
}

static void readDockBar(const QXmlStreamAttributes& attributes, std::vector<std::vector<unsigned>>& rows)
{
	int count = attributes.value("Count").toInt();
	std::vector<unsigned> row;
	for (int i = 1; i <= count; i++) {
		QString name = QStringLiteral("Id%1").arg(i);
		if (attributes.hasAttribute(name)) {
			row.push_back(attributes.value(name).toUInt());
		}
		else if (!row.empty()) {
			rows.push_back(row);
			row.clear();
		}
	}
	if (!row.empty()) {
		rows.push_back(row);
	}
}

static void readPane(const QXmlStreamAttributes& attributes, ImportedPaneNode& node)
{
	node.type = attributes.value("Type").toInt();
	node.paneId = attributes.value("ID").toUInt();
	node.horizontal = attributes.value("Horiz").toInt() != 0;
	node.dockingWidth = attributes.value("DockingCX").toInt();
	node.dockingHolder = attributes.value("DockingHolder").toInt();
	node.dockingHeight = attributes.value("DockingCY").toInt();
	node.selected = attributes.value("Selected").toInt();

	int count = attributes.value("Panes").toInt();
	for (int i = 1; i <= count; i++) {
		node.children.push_back(attributes.value(QStringLiteral("Pane-%1").arg(i)).toInt());
	}
}

bool importLayout(const QString& fileName, ImportedLayout& layout)
{
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) {
		return false;
	}

	QXmlStreamReader reader(&file);
	bool inDockingPane = false;
	bool inComboList = false;
	bool historyRead = false;
	while (!reader.atEnd()) {
		QXmlStreamReader::TokenType token = reader.readNext();
		if (token == QXmlStreamReader::EndElement && reader.name() == QStringLiteral("DockingPane")) {
			inDockingPane = false;
		}
		if (token != QXmlStreamReader::StartElement) {
			continue;
		}

		QStringView name = reader.name();
		QXmlStreamAttributes attributes = reader.attributes();
		if (name == QStringLiteral("DockingPane")) {
			inDockingPane = true;
		}
		else if (name == QStringLiteral("CommandBar")) {
			inComboList = attributes.value("Class") == QStringLiteral("CXTPControlComboBoxList");
		}
		else if (inComboList && !historyRead && name == QStringLiteral("Items")) {
			int count = attributes.value("Count").toInt();
			for (int i = 0; i < count; i++) {
				layout.commandHistory.append(attributes.value(QStringLiteral("Item%1").arg(i)).toString());
			}
			historyRead = true;
		}
		else if (name == QStringLiteral("Options") && !layout.options) {
			CommandBarsOptions options;
			auto flag = [&attributes](const char* key, bool fallback) {
				return attributes.hasAttribute(key) ? attributes.value(key).toInt() != 0 : fallback;
			};
			options.alwaysShowFullMenus = flag("AlwaysShowFullMenus", options.alwaysShowFullMenus);
			options.showFullAfterDelay = flag("ShowFullAfterDelay", options.showFullAfterDelay);
			options.toolBarScreenTips = flag("ToolBarScreenTips", options.toolBarScreenTips);
			options.toolBarAccelTips = flag("ToolBarAccelTips", options.toolBarAccelTips);
			options.largeIcons = flag("LargeIcons", options.largeIcons);
			if (attributes.hasAttribute("Animation")) {
				options.animation = attributes.value("Animation").toInt();
			}
			layout.options = options;
		}
		else if (name == QStringLiteral("UsedCommands") && layout.options) {
			int count = attributes.value("Count").toInt();
			for (int i = 0; i < count; i++) {
				layout.options->usedCommands.insert(attributes.value(QStringLiteral("Item%1").arg(i)).toUInt());
			}
		}
		else if (name.startsWith(QStringLiteral("BarInfo"))) {
			QString position = attributes.value("MRUDockPos").toString();
			if (position.isEmpty()) {
				position = attributes.value("PointPos").toString();
			}
			QStringList parts = position.split(',');
			if (parts.size() >= 2) {
				layout.barPositions[attributes.value("BarId").toUInt()] =
					QPoint(parts[0].trimmed().toInt(), parts[1].trimmed().toInt());
			}
		}
		else if (name.startsWith(QStringLiteral("DockBar")) && name != QStringLiteral("DockBars")) {
			readDockBar(attributes, layout.dockBars[name.mid(7).toInt()]);
		}
		else if (inDockingPane && name == QStringLiteral("Summary")) {
			layout.topContainer = attributes.value("TopContainer").toInt();
			layout.client = attributes.value("Client").toInt();
		}
		else if (inDockingPane && name.startsWith(QStringLiteral("Pane-"))) {
			readPane(attributes, layout.panes[name.mid(5).toInt()]);
		}
	}

	return !reader.hasError() && (!layout.dockBars.empty() || layout.topContainer != 0);
}
