#ifndef WEBTOOLBOX_H
#define WEBTOOLBOX_H

#include "Roblox_h.h"
#include "SecureHtmlView.h"

#include <atlbase.h>

class CRobloxWnd;

class CWebToolbox : public CSecureHtmlView
{
	DECLARE_DYNCREATE(CWebToolbox)

protected:
	CWebToolbox() {}

public:
	~CWebToolbox() override = default;

	void FUN_0046ea00(int category);

	void OnInitialUpdate() override;
	HRESULT OnGetExternal(LPDISPATCH* lppDispatch) override;
	HRESULT OnDocWindowActivate(BOOL fActivate) override;
	HRESULT OnFrameWindowActivate(BOOL fActivate) override;
	HRESULT OnGetHostInfo(DOCHOSTUIINFO* pInfo) override;

protected:
	void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) override;

	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);

	DECLARE_MESSAGE_MAP()

private:
	friend class CRobloxView;

	CRobloxWnd* m_unk0xf4;
	CComPtr<IWorkspace> m_unk0xf8;
};

#endif // WEBTOOLBOX_H
