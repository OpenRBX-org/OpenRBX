#include "InsertObjectDialog.h"

CInsertObjectDialog::CInsertObjectDialog(QWidget* pParent) : CDialog(107, pParent), m_className("")
{
}

CInsertObjectDialog::~CInsertObjectDialog() = default;

bool CInsertObjectDialog::OnInitDialog()
{
	SetDlgItemText(1000, m_className);
	return CDialog::OnInitDialog();
}

void CInsertObjectDialog::OnOK()
{
	m_className = GetDlgItemText(1000);
	CDialog::OnOK();
}
