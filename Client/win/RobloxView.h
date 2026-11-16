#ifndef ROBLOXVIEW_H
#define ROBLOXVIEW_H

#include "MultiPlayerPane.h"
#include "RobloxWnd.h"
#include "VerbBinder.h"
#include "WebToolbox.h"
#include "network/Players.h"
#include "network/SuperSafeChanged.h"
#include "util/Events.h"
#include "v8datamodel/DataModel.h"
#include "v8tree/Service.h"
#include "v8tree/Verb.h"

#include <memory>

class CRobloxView;

class InsertModelFromRobloxVerb : public RBX::Verb
{
public:
	InsertModelFromRobloxVerb(CRobloxView* view, RBX::DataModel* dataModel);

	bool isEnabled() const override;
	bool isChecked() const override;
	void doIt(RBX::IDataState* dataState) override;

private:
	CRobloxView* m_unk0x0c;
	RBX::DataModel* m_unk0x10;
	double m_unk0x18;
};

class ToggleIDEModeVerb : public RBX::Verb
{
public:
	ToggleIDEModeVerb(RBX::VerbContainer* container);

	bool isEnabled() const override;
	bool isChecked() const override;
	void doIt(RBX::IDataState* dataState) override;

private:
	double m_unk0x10;
};

class LockPlayModeVerb : public RBX::Verb
{
public:
	LockPlayModeVerb(RBX::VerbContainer* container) : RBX::Verb(container, "LockPlayMode") {}

	void doIt(RBX::IDataState* dataState) override;
};

class ToggleFullscreenVerb : public RBX::Verb
{
public:
	ToggleFullscreenVerb(RBX::VerbContainer* container) : RBX::Verb(container, "ToggleFullScreen") {}

	bool isChecked() const override;
	void doIt(RBX::IDataState* dataState) override;
};

class HelpCommand : public RBX::Verb
{
public:
	HelpCommand(RBX::VerbContainer* container, CRobloxView* view) : RBX::Verb(container, "Help"), m_unk0x0c(view) {}

	void doIt(RBX::IDataState* dataState) override;

private:
	CRobloxView* m_unk0x0c;
};

class CRobloxView : public VerbBinder<CRobloxView, CView>,
					public RBX::VerbContainer,
					public RBX::Listener<RBX::ServiceProvider, RBX::ServiceAdded>,
					public RBX::Listener<RBX::ServiceProvider, RBX::ServiceRemoving>,
					public RBX::Listener<RBX::Network::Players, RBX::SuperSafeChanged>
{
	DECLARE_DYNCREATE(CRobloxView)
	DECLARE_MESSAGE_MAP()

protected:
	CRobloxView();

public:
	~CRobloxView() override;

	void OnDraw(CDC* pDC) override;
	void OnInitialUpdate() override;

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDestroy();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void FUN_0045d8c0(CCmdUI* pCmdUI);

	void FUN_0045d920();
	void FUN_0045da90(int p_unk0x04);
	afx_msg void FUN_0045dbe0();
	afx_msg void FUN_0045dbf0();
	afx_msg void FUN_0045dc00();
	void FUN_0045dd10(bool show, bool activate);
	afx_msg LRESULT FUN_0045de20(WPARAM wParam, LPARAM lParam);
	afx_msg void FUN_0045de50();
	afx_msg void FUN_0045de70();
	afx_msg LRESULT FUN_0045e740(WPARAM wParam, LPARAM lParam);
	void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) override;
	void OnActivateFrame(UINT nState, CFrameWnd* pFrameWnd) override;
	BOOL OnPreparePrinting(CPrintInfo* pInfo) override;
	void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo) override;
	void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo) override;

	std::shared_ptr<RBX::DataModel> FUN_00403290() const;
	void FUN_0045e900();
	CRobloxView* FUN_004558a0() { return this; }

protected:
	void onEvent(const RBX::ServiceProvider* source, RBX::ServiceAdded event) override;
	void onEvent(const RBX::ServiceProvider* source, RBX::ServiceRemoving event) override;
	void onEvent(const RBX::Network::Players* source, RBX::SuperSafeChanged event) override;

private:
	friend class InsertModelFromRobloxVerb;

	CRobloxWnd m_unk0x88;
	std::shared_ptr<RBX::DataModel> m_unk0x140;
	CMenu m_unk0x148;
	CMultiPlayerPane m_unk0x150;
	CWebToolbox* m_unk0x1a8;
	std::unique_ptr<RBX::Verb> m_unk0x1ac;
	std::unique_ptr<RBX::Verb> m_unk0x1b0;
	std::unique_ptr<RBX::Verb> m_unk0x1b4;
	std::unique_ptr<RBX::Verb> m_unk0x1b8;
	std::unique_ptr<RBX::Verb> m_unk0x1bc;
};

#endif // ROBLOXVIEW_H
