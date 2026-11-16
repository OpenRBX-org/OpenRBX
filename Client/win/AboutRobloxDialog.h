#ifndef ABOUTROBLOXDIALOG_H
#define ABOUTROBLOXDIALOG_H

#include "Roblox.h"

class CAboutRobloxDialog : public CRobloxDHtmlDialog
{
	DECLARE_DYNCREATE(CAboutRobloxDialog)

public:
	CAboutRobloxDialog(CWnd* pParentWnd = nullptr);
	~CAboutRobloxDialog() override;

protected:
	void DoDataExchange(CDataExchange* pDX) override;
	BOOL OnInitDialog() override;
	void OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl) override;

	HRESULT OnButtonOK(IHTMLElement* pElement);

	DECLARE_MESSAGE_MAP()
	DECLARE_DHTML_EVENT_MAP()
};

#endif // ABOUTROBLOXDIALOG_H
