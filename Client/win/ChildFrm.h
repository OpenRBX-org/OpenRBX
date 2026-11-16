#ifndef CHILDFRM_H
#define CHILDFRM_H

class CChildFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CChildFrame)

public:
	CChildFrame();
	~CChildFrame() override;

	BOOL PreCreateWindow(CREATESTRUCT& cs) override;
	void ActivateFrame(int nCmdShow = -1) override;

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	DECLARE_MESSAGE_MAP()
};

class CRbxChildFrame : public CChildFrame
{
	DECLARE_DYNCREATE(CRbxChildFrame)

protected:
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);

	DECLARE_MESSAGE_MAP()
};

#endif // CHILDFRM_H
