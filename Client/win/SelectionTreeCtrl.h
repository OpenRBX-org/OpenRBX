#ifndef SELECTIONTREECTRL_H
#define SELECTIONTREECTRL_H

#include "ActiveDocView.h"
#include "RobloxTreeCtrl.h"
#include "util/Events.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Selection.h"

#include <memory>
#include <set>

class CSelectionTreeCtrl : public CRobloxTreeCtrl,
						   public ActiveDocView,
						   public RBX::Listener<RBX::Selection, RBX::SelectionChanged>
{
	DECLARE_DYNAMIC(CSelectionTreeCtrl)

public:
	CSelectionTreeCtrl();
	~CSelectionTreeCtrl() override;

	BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) override;

	RBX::Selection* VTable0x148() override;
	std::shared_ptr<RBX::DataModel> VTable0x14c() const override;
	void VTable0x150(RBX::Instance* instance) override;
	void VTable0x154(CRobloxTreeCtrlNode* node) override;
	void VTable0x158(CRobloxTreeCtrlNode* node) override;
	void VTable0x15c() override;

protected:
	void VTable0x00(
		std::shared_ptr<RBX::DataModel> newDataModel, std::shared_ptr<RBX::DataModel> oldDataModel
	) override;
	void onEvent(const RBX::Selection* source, RBX::SelectionChanged event) override;

	void SelectAllIgnore(BOOL bSelect, HTREEITEM hIgnore) override;
	BOOL OnButtonDown(BOOL bLeft, UINT nFlags, CPoint point) override;
	void DoPreSelection(HTREEITEM hItem, BOOL bLeft, UINT nFlags) override;
	void DoAction(HTREEITEM hItem, BOOL bLeft, UINT nFlags, CPoint point) override;
	void DoBanding(UINT nFlags, CPoint point) override;
	void UpdateSelectionForRect(LPCRECT pRect, UINT nFlags, CTypedPtrList<CPtrList, HTREEITEM>& list) override;

	afx_msg void FUN_00422230(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void FUN_00423630(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void FUN_004232d0(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	DECLARE_MESSAGE_MAP()

private:
	void FUN_00422270(CPoint point);
	void FUN_00423170(TVITEM* item, bool select);
	HTREEITEM FUN_00423cc0(HTREEITEM item);
	void FUN_00426f50();
	void FUN_004285a0(std::shared_ptr<RBX::DataModel> dataModel);

	CMenu m_unk0xc4;
	bool m_unk0xcc;
	bool m_unk0xcd;
	bool m_unk0xce;
	std::set<std::shared_ptr<RBX::Instance>, std::owner_less<std::shared_ptr<RBX::Instance>>> m_unk0xd0;
	std::set<CRobloxTreeCtrlNode*> m_unk0xdc;
	std::shared_ptr<RBX::DataModel> m_unk0xe8;
	std::unique_ptr<CImageList> m_unk0xf0;
};

#endif // SELECTIONTREECTRL_H
