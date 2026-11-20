#ifndef MAINFRM_H
#define MAINFRM_H

#include "Command.h"
#include "CommandBarsOptions.h"
#include "CommandIds.h"
#include "util/Events.h"
#include "v8datamodel/BrickColor.h"

#include <QElapsedTimer>
#include <QIcon>
#include <QMainWindow>
#include <QRect>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <vector>

class CChildFrame;
struct ImportedLayout;
class QAbstractNativeEventFilter;
class QAction;
class QComboBox;
class QDockWidget;
class QLabel;
class QMenu;
class QTabWidget;
class QToolBar;
class QToolButton;
class PopupToolBar;
class WorkspaceTabButtons;
class View;

namespace Resources {
struct MenuEntry;
}

namespace RBX {
class FillToolColor;
}

struct ToolBarControl
{
	unsigned id = 0;
	std::vector<QAction*> actions;
	bool beginGroup = false;
	QAction* separator = nullptr;
	QString caption;
	QString customCaption;
	QIcon customIcon;
	bool customIconAlpha = false;
	std::optional<Qt::ToolButtonStyle> customStyle;
};

class CMainFrame : public QMainWindow, public CommandTarget, public RBX::Listener<RBX::FillToolColor, RBX::BrickColor>
{
public:
	void setMessageText(const QString& text);
	CMainFrame();
	~CMainFrame() override;

	bool create();
	void restorePlacement(bool show);

	void enterGameLayout();
	void leaveGameLayout();
	void setFullScreen(bool fullScreen);

	bool isGameLayout() const { return m_gameLayout; }
	bool isGameLayoutLocked() const { return m_gameLayoutLocked; }
	bool isFullScreenMode() const { return m_fullScreen; }

	CommandRoute commandRoute();
	bool sendCommand(unsigned id);
	void postCommand(unsigned id);
	void updateCommandUI();

	void addChildFrame(CChildFrame* frame);
	void activateChildFrame(CChildFrame* frame);
	void childFrameTitleChanged(CChildFrame* frame);
	void childFrameDestroyed(CChildFrame* frame);
	void childFrameViewChanged(CChildFrame* frame);
	CChildFrame* activeChildFrame() const;
	View* activeView() const { return m_activeView; }

	QComboBox* commandLine() const { return m_commandLine; }

protected:
	void onEvent(const RBX::FillToolColor* source, RBX::BrickColor event) override;

	void closeEvent(QCloseEvent* event) override;
	void contextMenuEvent(QContextMenuEvent* event) override;
	void changeEvent(QEvent* event) override;
	bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	using MenuIterator = std::span<const Resources::MenuEntry>::iterator;

	void createStatusBar();
	void createMenuBar();
	void createToolBars();
	void createAccelerators();
	QAction* toolBarAction(unsigned id);
	void buildMenu(QMenu* menu, MenuIterator& it, MenuIterator end);
	QToolBar* addCommandToolBar(const QString& caption, unsigned id, Qt::ToolBarArea area);
	QToolButton* createFillColorButton(QToolBar* toolBar);
	void resetCommandBars();
	void showCustomize();
	void applyCommandBarsOptions();
	void setCustomizeMode(bool customize, bool quick = false);
	QToolBar* addCustomToolBar(const QString& title, unsigned id = 0);
	void removeCustomToolBar(QToolBar* toolBar);
	bool isCommandHidden(unsigned id) const;
	bool isActionHidden(QAction* action) const;
	void markCommandUsed(unsigned id);
	void applyPersonalizedMenu(QMenu* menu);
	void expandMenu(QMenu* menu);
	void showToolBarOptions(QToolBar* toolBar);
	void getHiddenControls(QToolBar* toolBar, PopupToolBar* expandBar, QMenu* popup);
	void buildAddOrRemoveMenu(QMenu* menu, QToolBar* toolBar);
	void buildCustomizeMenu(QMenu* menu, QToolBar* toolBar);
	bool toggleCustomizeItem(QMenu* menu, QAction* action);
	void setToolBarItemVisible(QToolBar* toolBar, unsigned id, bool visible);
	ToolBarControl* toolBarControl(QWidget* widget, QToolBar** toolBar);
	QToolButton* controlButton(QToolBar* toolBar, const ToolBarControl& control) const;
	QWidget* controlWidget(QToolBar* toolBar, const ToolBarControl& control) const;
	ToolBarControl* controlForWidget(QWidget* widget, QToolBar** toolBar, QWidget** owner);
	void selectControl(QToolBar* toolBar, const ToolBarControl& control);
	QString controlCaption(const ToolBarControl& control) const;
	void applyToolBarControl(QToolBar* toolBar, ToolBarControl& control);
	void setControlBeginGroup(QToolBar* toolBar, ToolBarControl& control, bool beginGroup);
	void deleteToolBarControl(QToolBar* toolBar, ToolBarControl* control);
	void resetToolBar(QToolBar* toolBar);
	bool isToolBarCustomized(QToolBar* toolBar);
	void restoreToolBarControls(QToolBar* toolBar, const QVariantList& entries);
	QAction* menuCommandAction(QObject* parent, unsigned id, const QString& text);
	void restoreMenuControls(QMenu* menu, const QVariantList& entries);
	QVariantList saveControls(const std::vector<ToolBarControl>& controls);
	void ensureMenuControls(QMenu* menu);
	ToolBarControl* selectedBarControl(QWidget* bar);
	void deleteBarControl(QWidget* bar, QAction* identity);
	bool isDefaultToolBarAction(QAction* action) const;
	void setBarControlBeginGroup(QWidget* bar, QAction* identity, bool beginGroup);
	void setupMenu(QMenu* menu);
	QMenu* menuByKey(const QString& key) const;
	QMenu* createPopupMenu(const QString& caption, unsigned resource, int index, QString key);
	QMenu* clonePopupMenu(QMenu* source);
	void reparentPopups(QWidget* bar);
	void restoreMenuBar(const QVariantList& entries);
	void applyMenuControl(QMenu* menu, ToolBarControl& control);
	void showCustomizeContextMenu(const QPoint& pos);
	QRect controlRect(QToolBar* toolBar, const ToolBarControl& control) const;
	bool isCustomizeBar(QWidget* widget) const;
	QWidget* dropTarget(QPoint globalPos) const;
	std::vector<ToolBarControl>& barControls(QWidget* bar);
	QRect barControlRect(QWidget* bar, const ToolBarControl& control) const;
	bool barVertical(QWidget* bar) const;
	void findDropIndex(
		QWidget* bar,
		QPoint point,
		const ToolBarControl* dragged,
		QRect& marker,
		int& index,
		bool& after,
		bool& vertical
	);
	void showDropMarker(QWidget* bar, QRect marker, bool vertical);
	void openDragPopup(QWidget* bar, QPoint globalPos);
	void closeDragPopups(QWidget* keep);
	void applyBarControl(QWidget* bar, ToolBarControl& control);
	QAction* newBarAction(QWidget* bar, const ToolBarControl& control);
	void rebuildBar(QWidget* bar, std::vector<ToolBarControl> controls);
	void selectBarControl(QWidget* bar, const ToolBarControl& control);
	void ensureMenuBarControls();
	void rebuildToolBar(QToolBar* toolBar, std::vector<ToolBarControl> controls);
	struct DragOutcome
	{
		bool dropped = false;
		bool moved = false;
		bool move = false;
		QWidget* target = nullptr;
		QPoint last;
	};
	DragOutcome runDragLoop(
		QWidget* grabber, QPoint start, bool copyOnly, const std::function<const ToolBarControl*(QWidget*)>& dragged
	);
	bool dragControl(QWidget* button, QPoint start);
	bool dragBarControl(QWidget* bar, QAction* identity, QWidget* grabber, QPoint start);
	void dragCommand(
		unsigned id,
		const QString& caption,
		bool popup,
		unsigned menuResource,
		int menuIndex,
		QWidget* grabber,
		QPoint start
	);
	QString acceleratorText(unsigned id) const;

	void updateMenu(QMenu* menu);
	void updateRecentFileMenu(QMenu* menu);
	void updateWindowListMenu(QMenu* menu);
	void updateToolBarListMenu(QMenu* menu);
	std::vector<QAction*> toolBarListActions(QObject* parent);
	void showToolBarsContextMenu(const QPoint& globalPos);
	void updateAction(QAction* action, unsigned id, bool setText);
	void updateTitle();
	void childFrameActivated(CChildFrame* frame);

	QDockWidget* createPane(PaneId id, Qt::DockWidgetArea area, QSize size);
	QWidget* createPaneContents(PaneId id);
	void showPane(PaneId id, Qt::DockWidgetArea area, QSize size);
	void nestDockArea(Qt::DockWidgetArea area, bool outermost);
	void showExplorerPane();
	void showPropertiesPane();

	void saveLayout();
	bool loadLayout();
	bool importOriginalLayout();
	void importPanes(const ImportedLayout& layout, int node, Qt::DockWidgetArea area, QDockWidget*& previous);
	void claimCorner(Qt::Corner corner, Qt::DockWidgetArea area);
	void savePlacement();

	void changeResolution();
	void restoreResolution();
	void updateMaximumSize();
	void applicationActivated(bool active);
	void onDestroy();

	bool m_fullScreen = false;
	bool m_changingResolution = false;
	bool m_gameLayout = false;
	bool m_gameLayoutLocked = false;
	bool m_destroyed = false;
	QString m_layoutFile;
	QRect m_normalGeometry;
	QTabWidget* m_workspace = nullptr;
	WorkspaceTabButtons* m_workspaceButtons = nullptr;
	std::map<PaneId, QDockWidget*> m_panes;
	std::multimap<unsigned, QAction*> m_toolBarActions;
	std::vector<QToolBar*> m_toolBars;
	std::map<unsigned, QToolBar*> m_toolBarsById;
	std::map<QToolBar*, std::vector<ToolBarControl>> m_toolBarItems;
	std::map<QMenu*, std::vector<ToolBarControl>> m_menuItems;
	std::set<QMenu*> m_customizedMenus;
	std::vector<ToolBarControl> m_menuBarItems;
	bool m_menuBarCustomized = false;
	struct CreatedMenu
	{
		QString caption;
		unsigned resource;
		int index;
	};
	std::map<QString, CreatedMenu> m_createdMenus;
	QWidget* m_dropMarkerBar = nullptr;
	QMenu* m_hiddenMenu = nullptr;
	QElapsedTimer m_hiddenMenuTimer;
	std::map<QToolBar*, std::vector<ToolBarControl>> m_toolBarDefaults;
	std::unique_ptr<QAbstractNativeEventFilter> m_acquiredKeyFilter;
	CommandBarsOptions m_options;
	bool m_menusExpanded = false;
	QToolBar* m_commandToolBar = nullptr;
	QWidget* m_customizeSheet = nullptr;
	std::vector<QLabel*> m_toolBarLabels;
	QLabel* m_commandLineLabel = nullptr;
	QComboBox* m_commandLine = nullptr;
	QToolButton* m_fillColorButton = nullptr;
	QLabel* m_indicators[5] = {};
	std::map<PaneId, Qt::DockWidgetArea> m_paneAreas;
	int m_claimedCorners = 0;
	View* m_activeView = nullptr;
};

extern bool g_overrideLockGameLayout;

#endif // MAINFRM_H
