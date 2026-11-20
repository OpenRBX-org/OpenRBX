#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "Dialog.h"
#include "PropGrid.h"
#include "ReportControl.h"
#include "v8tree/Instance.h"

#include <memory>

class QSplitter;

class CSettingsPropGrid : public CSelectionPropGrid
{
public:
	using CSelectionPropGrid::CSelectionPropGrid;

	void createItem(
		const RBX::Reflection::ClassDescriptor* classDescriptor, const RBX::Reflection::PropertyDescriptor* descriptor
	) override;
};

class CSettingsExplorer : public ReportControl,
						  public RBX::Listener<RBX::Instance, RBX::DescendentAdded>,
						  public RBX::Listener<RBX::Instance, RBX::DescendentRemoving>
{
public:
	explicit CSettingsExplorer(QWidget* parent = nullptr);
	~CSettingsExplorer() override;

	void setRoot(RBX::Instance* root);

protected:
	void onEvent(const RBX::Instance* source, RBX::DescendentAdded event) override;
	void onEvent(const RBX::Instance* source, RBX::DescendentRemoving event) override;

private:
	void addRecord(std::shared_ptr<RBX::Instance> instance);
	void removeRecord(RBX::Instance* instance);
	void selectionChanged();

	std::shared_ptr<RBX::Instance> m_root;
};

class CSettingsDialog : public CDialog
{
public:
	explicit CSettingsDialog(QWidget* parent = nullptr);

	static void show(QWidget* parent);

protected:
	bool OnInitDialog() override;
	void OnCommand(int id) override;

private:
	void resetAllSettings();

	QSplitter* m_splitter = nullptr;
	CSettingsExplorer* m_explorer = nullptr;
	CSettingsPropGrid* m_grid = nullptr;
};

#endif // SETTINGSDIALOG_H
