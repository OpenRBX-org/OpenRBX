#ifndef SELECTIONCAPTION_H
#define SELECTIONCAPTION_H

#include "ActiveDocView.h"
#include "FilteredListener.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Selection.h"

#include <memory>

class CSelectionCaption
	: public CXTCaption,
	  public ActiveDocView,
	  public RBX::FilteredListener<RBX::Selection, RBX::SelectionChanged, RBX::Selection, RBX::SelectionChanged>
{
	DECLARE_DYNAMIC(CSelectionCaption)

public:
	CSelectionCaption();
	~CSelectionCaption() override;

protected:
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	DECLARE_MESSAGE_MAP()

	void VTable0x00(
		std::shared_ptr<RBX::DataModel> newDataModel, std::shared_ptr<RBX::DataModel> oldDataModel
	) override;
	void VTable0x08(const RBX::Selection* source, RBX::SelectionChanged event) override;
	bool VTable0x10(
		const RBX::Selection* source,
		const RBX::SelectionChanged& event,
		const RBX::Selection* oldSource,
		const RBX::SelectionChanged& oldEvent
	) override;

private:
	void FUN_0046a290();

	std::shared_ptr<RBX::DataModel> m_unk0x1b0;
	std::shared_ptr<RBX::Selection> m_unk0x1b8;
};

#endif // SELECTIONCAPTION_H
