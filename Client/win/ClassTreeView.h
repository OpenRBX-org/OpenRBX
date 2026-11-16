#ifndef CLASSTREEVIEW_H
#define CLASSTREEVIEW_H

#include "reflection/EnumConverter.h"
#include "reflection/object.h"

namespace RBX::Reflection::Metadata {
class Item;
}

class CDeclarationView;
class CMemberTreeView;

class CClassTreeView : public CXTTreeView
{
	DECLARE_DYNCREATE(CClassTreeView)

protected:
	CClassTreeView();

public:
	~CClassTreeView() override;

	void OnInitialUpdate() override;

	void FUN_00437f90(const RBX::Reflection::EnumDescriptor* descriptor);
	void FUN_00438130(HTREEITEM item, RBX::Reflection::Metadata::Item* metadata);
	void FUN_00439690(const RBX::Reflection::ClassDescriptor* descriptor);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void FUN_0043a280(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()

private:
	CImageList m_unk0xa0;

public:
	CMemberTreeView* m_unk0xa8;
	CDeclarationView* m_unk0xac;
};

#endif // CLASSTREEVIEW_H
