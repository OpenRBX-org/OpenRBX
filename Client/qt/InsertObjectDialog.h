#ifndef INSERTOBJECTDIALOG_H
#define INSERTOBJECTDIALOG_H

#include "Dialog.h"

class CInsertObjectDialog : public CDialog
{
public:
	CInsertObjectDialog(QWidget* pParent = nullptr);
	~CInsertObjectDialog() override;

	QString m_className;

protected:
	bool OnInitDialog() override;
	void OnOK() override;
};

#endif // INSERTOBJECTDIALOG_H
