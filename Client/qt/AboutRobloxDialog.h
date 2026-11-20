#ifndef ABOUTROBLOXDIALOG_H
#define ABOUTROBLOXDIALOG_H

#include "HtmlDialog.h"

class CAboutRobloxDialog : public CRobloxDHtmlDialog
{
public:
	explicit CAboutRobloxDialog(QWidget* parent = nullptr);

protected:
	void OnDocumentComplete() override;
};

#endif // ABOUTROBLOXDIALOG_H
