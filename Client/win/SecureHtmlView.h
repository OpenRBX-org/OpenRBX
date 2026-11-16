#ifndef SECUREHTMLVIEW_H
#define SECUREHTMLVIEW_H

#include <afxhtml.h>

class CSecureHtmlView : public CHtmlView
{
	DECLARE_DYNAMIC(CSecureHtmlView)

protected:
	CSecureHtmlView();

public:
	~CSecureHtmlView() override;

protected:
	BOOL OnAmbientProperty(COleControlSite* pSite, DISPID dispid, VARIANT* pvar) override;
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;

public:
	void OnBeforeNavigate2(
		LPCTSTR lpszURL,
		DWORD nFlags,
		LPCTSTR lpszTargetFrameName,
		CByteArray& baPostedData,
		LPCTSTR lpszHeaders,
		BOOL* pbCancel
	) override;

protected:
	bool m_unk0xf0;
};

#endif // SECUREHTMLVIEW_H
