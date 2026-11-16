#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "InstanceExplorer.h"
#include "SettingsPropGrid.h"

class CSettingsExplorer : public CInstanceExplorer
{
	DECLARE_DYNCREATE(CSettingsExplorer)

protected:
	CSettingsExplorer() {}

	void onEvent(const RBX::Instance* source, RBX::DescendentAdded event) noexcept override;
	void onEvent(const RBX::Instance* source, RBX::DescendentRemoving event) noexcept override;
};

class CSettingsDialog : public CDialog
{
	DECLARE_DYNCREATE(CSettingsDialog)

public:
	enum
	{
		IDD = 122
	};

	CSettingsDialog(CWnd* pParent = nullptr);
	~CSettingsDialog() override;

	static void FUN_0046b3f0(CWnd* p_unk0x04);

protected:
	BOOL OnInitDialog() override;

	afx_msg void FUN_0046b120();

	DECLARE_MESSAGE_MAP()

private:
	CXTSplitterWnd m_unk0x74;
	CSettingsExplorer* m_unk0x18c;
	CSettingsPropGrid m_unk0x190;
};

#endif // SETTINGSDIALOG_H
