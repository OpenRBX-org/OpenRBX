#ifndef CUSTOMIZESHEET_H
#define CUSTOMIZESHEET_H

#include "CommandBarsOptions.h"
#include "Dialog.h"
#include "PropertySheet.h"

#include <QListWidget>
#include <functional>
#include <vector>

class QToolBar;

struct CommandBarEntry
{
	QString title;
	QWidget* bar;
	bool builtIn;
	bool closeable;
	std::function<void()> reset;
};

class CustomizeCheckList : public QListWidget
{
public:
	explicit CustomizeCheckList(QWidget* parent);

	std::function<void(int)> onCheckChanged;

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
};

struct CustomToolBarActions
{
	std::function<CommandBarEntry(const QString&)> add;
	std::function<void(QWidget*)> remove;
	std::function<void(unsigned, const QString&, bool, unsigned, int, QWidget*, QPoint)> dragCommand;
};

class NewToolbarDialog : public CDialog
{
public:
	NewToolbarDialog(std::vector<QString> titles, const QString* current, QWidget* parent);

	QString name() const { return m_name; }

protected:
	bool OnInitDialog() override;
	void OnOK() override;

private:
	std::vector<QString> m_titles;
	QString m_current;
	bool m_rename;
	QString m_name;
};

class CustomizeToolbarsPage : public CDialog
{
public:
	CustomizeToolbarsPage(std::vector<CommandBarEntry> bars, CustomToolBarActions actions, QWidget* parent);

private:
	void refreshList(int select = -1);
	void selectionChanged();
	void checkChanged(int row);
	void newToolbar();
	void renameToolbar();
	void deleteToolbar();
	void resetToolbar();
	std::vector<QString> titles() const;

	std::vector<CommandBarEntry> m_bars;
	CustomToolBarActions m_actions;
	CustomizeCheckList* m_list;
};

struct CustomizeCommand
{
	QString caption;
	unsigned id;
	bool popup;
	unsigned menuResource = 0;
	int menuIndex = -1;
};

class CustomizeCommandList : public QListWidget
{
public:
	explicit CustomizeCommandList(QWidget* parent);

	void setCommands(const std::vector<CustomizeCommand>& commands);

	std::function<void(unsigned, const QString&, bool, unsigned, int, QWidget*, QPoint)> dragCommand;

protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
};

class CustomizeCommandsPage : public CDialog
{
public:
	CustomizeCommandsPage(
		std::function<void(unsigned, const QString&, bool, unsigned, int, QWidget*, QPoint)> dragCommand,
		QWidget* parent
	);

private:
	void addCategories(unsigned menuId);
	void insertAllCommandsCategory();
	void insertBuiltInMenus(unsigned menuId);
	void insertNewMenuCategory();
	void categoryChanged();

	std::vector<std::pair<QString, std::vector<CustomizeCommand>>> m_categories;
	QListWidget* m_categoryList;
	CustomizeCommandList* m_commandList;
};

class CustomizeGroupLine : public QWidget
{
public:
	CustomizeGroupLine(const QString& text, QWidget* parent);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	QString m_text;
};

class CustomizeOptionsPage : public CDialog
{
public:
	CustomizeOptionsPage(CommandBarsOptions& options, std::function<void()> changed, QWidget* parent);

private:
	void replaceGroup(int id);
	void updateEnabled();
	void resetUsageData();

	CommandBarsOptions& m_options;
	std::function<void()> m_changed;
};

class CustomizeSheet : public CPropertySheet
{
public:
	CustomizeSheet(
		std::vector<CommandBarEntry> bars,
		CustomToolBarActions actions,
		CommandBarsOptions& options,
		std::function<void()> optionsChanged,
		QWidget* parent
	);
};

#endif // CUSTOMIZESHEET_H
