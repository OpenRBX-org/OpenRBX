#ifndef INSERTOBJECTDIALOG_H
#define INSERTOBJECTDIALOG_H

class CInsertObjectDialog : public CDialog
{
	DECLARE_DYNAMIC(CInsertObjectDialog)

public:
	CInsertObjectDialog(CWnd* pParent = NULL);
	virtual ~CInsertObjectDialog();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()

public:
	CString m_unk0x74;
};

#endif // INSERTOBJECTDIALOG_H
