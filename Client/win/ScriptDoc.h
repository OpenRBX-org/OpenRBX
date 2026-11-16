#ifndef SCRIPTDOC_H
#define SCRIPTDOC_H

#include "MarshaledListener.h"
#include "NullDoc.h"
#include "ScintillaDocView.h"
#include "util/RunStateOwner.h"
#include "v8tree/Instance.h"

#include <memory>
#include <mutex>

namespace RBX {
class Script;
} // namespace RBX

class CScriptDoc : public CNullDoc
{
	DECLARE_DYNCREATE(CScriptDoc)

protected:
	CScriptDoc();

public:
	void FUN_0044eaf0(std::shared_ptr<RBX::Script> script);

	friend class CScriptEditor;

private:
	std::shared_ptr<RBX::Script> m_unk0x54;
};

class CScriptEditor
	: public Scintilla::CScintillaView,
	  public RBX::Listener<RBX::Instance, RBX::AncestorChanged>,
	  public RBX::MarshaledListener<RBX::Instance, RBX::PropertyChanged, RBX::Instance, RBX::PropertyChanged>,
	  public RBX::MarshaledListener<RBX::RunService, RBX::RunState, RBX::RunService, RBX::RunState>
{
	DECLARE_DYNCREATE(CScriptEditor)

protected:
	CScriptEditor();

public:
	~CScriptEditor() override;

	void FUN_00468aa0();
	void FUN_00468db0();
	void FUN_004696d0();
	void FUN_00469b30(std::shared_ptr<RBX::Script> script);

protected:
	void onEvent(const RBX::Instance* source, RBX::AncestorChanged event) override;
	void VTable0x08(const RBX::RunService* source, RBX::RunState event) override;
	void VTable0x08(const RBX::Instance* source, RBX::PropertyChanged event) override;
	void VTable0x0c(const RBX::Instance* source, RBX::PropertyChanged event) noexcept override;

	BOOL PreCreateWindow(CREATESTRUCT& cs) override;
	void PostNcDestroy() override;
	void OnInitialUpdate() override;
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;
	void OnCharAdded(Scintilla::SCNotification* pSCNotification) override;
	void OnDwellStart(Scintilla::SCNotification* pSCNotification) override;
	void OnDwellEnd(Scintilla::SCNotification* pSCNotification) override;
	void OnKillFocus() override;

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnOptionsAddmarker();
	afx_msg void OnUpdateOptionsAddmarker(CCmdUI* pCmdUI);
	afx_msg void OnOptionsDeletemarker();
	afx_msg void OnUpdateOptionsDeletemarker(CCmdUI* pCmdUI);
	afx_msg void OnOptionsFindNextmarker();
	afx_msg void OnOptionsFindPrevmarker();
	afx_msg void OnOptionsFoldMargin();
	afx_msg void OnUpdateOptionsFoldMargin(CCmdUI* pCmdUI);
	afx_msg void OnOptionsSelectionMargin();
	afx_msg void OnUpdateOptionsSelectionMargin(CCmdUI* pCmdUI);
	afx_msg void OnOptionsViewLinenumbers();
	afx_msg void OnUpdateOptionsViewLinenumbers(CCmdUI* pCmdUI);
	afx_msg void OnUpdateInsert(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStyle(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFold(CCmdUI* pCmdUI);
	afx_msg void OnUpdateLine(CCmdUI* pCmdUI);
	afx_msg LRESULT FUN_00469a20(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

	void SetAStyle(
		int style, COLORREF fore, COLORREF back = RGB(0xff, 0xff, 0xff), int size = -1, const char* face = nullptr
	);
	void DefineMarker(int marker, int markerType, COLORREF fore, COLORREF back);

private:
	std::shared_ptr<std::recursive_mutex> m_unk0x128;
	CWnd* m_unk0x130;
	std::shared_ptr<RBX::Script> m_unk0x134;
	std::shared_ptr<RBX::RunService> m_unk0x13c;
	bool m_unk0x144;
	bool m_unk0x145;
	bool m_unk0x146;
	CFont m_unk0x148;
};

#endif // SCRIPTDOC_H
