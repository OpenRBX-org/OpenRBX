#ifndef PROPGRID_H
#define PROPGRID_H

#include "ActiveDocView.h"
#include "util/Events.h"
#include "util/Sound.h"
#include "util/TextureId.h"
#include "v8datamodel/Selection.h"
#include "v8tree/Instance.h"

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

class CPropGrid : public CXTPPropertyGrid, public RBX::Listener<RBX::Instance, RBX::PropertyChanged>
{
	DECLARE_DYNAMIC(CPropGrid)

public:
	CPropGrid();
	~CPropGrid() override;

	class Item
	{
	public:
		Item(const RBX::Reflection::PropertyDescriptor& descriptor, CPropGrid* grid);

		virtual void VTable0x00(
			const std::map<const RBX::Reflection::ClassDescriptor*, std::vector<std::shared_ptr<RBX::Instance>>>&
				instances
		) = 0;

	protected:
		void FUN_0043e120(const RBX::Reflection::ClassDescriptor* classDescriptor);

		std::vector<const RBX::Reflection::ClassDescriptor*> m_unk0x04;
		CPropGrid* m_unk0x14;
	};

	class Unk0x1d0
		: public std::map<const RBX::Reflection::ClassDescriptor*, std::vector<std::shared_ptr<RBX::Instance>>>
	{
	public:
		void FUN_0043e170(RBX::Instance* instance);
		void FUN_004452d0(RBX::Instance* instance);
	};

	virtual void FUN_00442e20(
		const RBX::Reflection::ClassDescriptor* classDescriptor, const RBX::Reflection::PropertyDescriptor* descriptor
	);
	virtual RBX::Selection* VTable0x164() = 0;

protected:
	void onEvent(const RBX::Instance* source, RBX::PropertyChanged event) override;

	void FUN_0043db00(const RBX::Reflection::ClassDescriptor* classDescriptor);
	void FUN_0043e7f0(std::shared_ptr<RBX::Instance> instance);
	void FUN_00445380(std::shared_ptr<RBX::Instance> instance);

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg LRESULT FUN_0043e8a0(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

private:
	template <class T, class Base, class Descriptor>
	friend class XItem;
	friend class Vector3ComponentItem;
	friend class ScriptItem;

	CXTPPropertyGridItem* FUN_0043cba0(const RBX::Name& category);

	template <class T>
	void FUN_0043e6b0(T* item);

protected:
	std::recursive_mutex m_unk0x188;
	RBX::DataModel* m_unk0x194;
	std::vector<Item*> m_unk0x198;
	std::map<const RBX::Reflection::PropertyDescriptor*, Item*> m_unk0x1a8;
	bool m_unk0x1c0;
	bool m_unk0x1c1;
	std::set<Item*> m_unk0x1c4;
	Unk0x1d0 m_unk0x1d0;
};

class CPropertyGridItemBrickColor : public CXTPPropertyGridItemColor
{
public:
	CPropertyGridItemBrickColor(CString strCaption, COLORREF clr = 0);

	void SetValue(CString strValue) override;
	void SetColor(COLORREF clr) override;

protected:
	void OnInplaceButtonDown(CXTPPropertyGridInplaceButton* pButton) override;
};

template <class T, class Base, class Descriptor>
class XItem : public Base, public CPropGrid::Item
{
public:
	XItem(const Descriptor& descriptor, CPropGrid* grid)
		: Base(descriptor.name.c_str()), Item(descriptor, grid), m_unk0x124(&descriptor)
	{
		m_bHidden = TRUE;
		SetReadOnly(descriptor.isReadOnly());
	}

	void OnValueChanged(CString strValue) override
	{
		Base::OnValueChanged(strValue);
		FUN_00443670(VTable0xe4());
	}

	void VTable0x00(
		const std::map<const RBX::Reflection::ClassDescriptor*, std::vector<std::shared_ptr<RBX::Instance>>>& instances
	) override;

	virtual T VTable0xe4()
	{
		T result;
		if (RBX::StringConverter<T>::convertToValue((LPCTSTR) GetValue(), result)) {
			return result;
		}
		return T();
	}
	virtual void VTable0xe8(const T& value);
	virtual bool VTable0xec(const T& first, const T& second) { return first == second; }
	virtual void VTable0xf0(RBX::Instance* instance, const T& value)
	{
		RBX::DataModel::Lock lock(m_unk0x14->m_unk0x194);
		m_unk0x124->setValue(instance, value);
	}
	virtual bool VTable0xf4() { return GetValue().IsEmpty(); }

	const Descriptor* getDescriptor() const { return m_unk0x124; }

protected:
	T FUN_0043f1f0(RBX::Instance* instance);
	void FUN_00443670(const T& value);

	friend class CPropGrid;

	const Descriptor* m_unk0x124;
};

template <>
std::string XItem<std::string, CXTPPropertyGridItem, RBX::Reflection::PropertyDescriptor>::VTable0xe4();
template <>
void XItem<std::string, CXTPPropertyGridItem, RBX::Reflection::PropertyDescriptor>::VTable0xe8(
	const std::string& value
);
template <>
void XItem<std::string, CXTPPropertyGridItem, RBX::Reflection::PropertyDescriptor>::VTable0xf0(
	RBX::Instance* instance, const std::string& value
);
template <>
int XItem<int, CXTPPropertyGridItemEnum, RBX::Reflection::EnumPropertyDescriptor>::VTable0xe4();
template <>
void XItem<int, CXTPPropertyGridItemEnum, RBX::Reflection::EnumPropertyDescriptor>::VTable0xe8(const int& value);
template <>
void XItem<int, CXTPPropertyGridItemEnum, RBX::Reflection::EnumPropertyDescriptor>::VTable0xf0(
	RBX::Instance* instance, const int& value
);

class XBoolItem : public XItem<bool, CXTPPropertyGridItemBool, RBX::Reflection::TypedPropertyDescriptor<bool>>
{
public:
	XBoolItem(const RBX::Reflection::TypedPropertyDescriptor<bool>& descriptor, CPropGrid* grid);

	BOOL OnDrawItemValue(CDC& dc, CRect rcValue) override;
};

class XBrickColorItem : public XItem<
							RBX::BrickColor,
							CPropertyGridItemBrickColor,
							RBX::Reflection::TypedPropertyDescriptor<RBX::BrickColor>>
{
public:
	XBrickColorItem(const RBX::Reflection::TypedPropertyDescriptor<RBX::BrickColor>& descriptor, CPropGrid* grid);

	BOOL OnDrawItemValue(CDC& dc, CRect rcValue) override;
};

class Vector3ComponentItem
	: public XItem<G3D::Vector3, CXTPPropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>>
{
public:
	Vector3ComponentItem(
		const RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>& descriptor, CPropGrid* grid, int component
	);

	G3D::Vector3 VTable0xe4() override;
	void VTable0xe8(const G3D::Vector3& value) override;
	bool VTable0xec(const G3D::Vector3& first, const G3D::Vector3& second) override;
	void VTable0xf0(RBX::Instance* instance, const G3D::Vector3& value) override;

private:
	int m_unk0x128;
	G3D::Vector3 m_unk0x12c;
};

class Vector3Item
	: public XItem<G3D::Vector3, CXTPPropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>>
{
public:
	Vector3Item(const RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3>& descriptor, CPropGrid* grid);

	BOOL OnAfterEdit(CString& strValue) override;
	void VTable0x00(
		const std::map<const RBX::Reflection::ClassDescriptor*, std::vector<std::shared_ptr<RBX::Instance>>>& instances
	) override;

protected:
	void OnAddChildItem() override;

private:
	Vector3ComponentItem* m_unk0x128[3];
};

class RefItem : public XItem<std::string, CXTPPropertyGridItem, RBX::Reflection::PropertyDescriptor>
{
public:
	RefItem(const RBX::Reflection::RefPropertyDescriptor& descriptor, CPropGrid* grid);

	void OnValueChanged(CString strValue) override;
	void VTable0x00(
		const std::map<const RBX::Reflection::ClassDescriptor*, std::vector<std::shared_ptr<RBX::Instance>>>& instances
	) override;

private:
	const RBX::Reflection::RefPropertyDescriptor* m_unk0x128;
};

class ScriptItem : public XItem<std::string, CXTPPropertyGridItem, RBX::Reflection::PropertyDescriptor>
{
public:
	ScriptItem(const RBX::Reflection::PropertyDescriptor& descriptor, CPropGrid* grid);

	void OnValueChanged(CString strValue) override;
	void VTable0x00(
		const std::map<const RBX::Reflection::ClassDescriptor*, std::vector<std::shared_ptr<RBX::Instance>>>& instances
	) override;

protected:
	void OnInplaceButtonDown(CXTPPropertyGridInplaceButton* pButton) override;
};

class EnumItem : public XItem<int, CXTPPropertyGridItemEnum, RBX::Reflection::EnumPropertyDescriptor>
{
public:
	EnumItem(const RBX::Reflection::EnumPropertyDescriptor& descriptor, CPropGrid* grid);
};

template <class T>
class ContentItem : public XItem<T, CXTPPropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<T>>
{
public:
	ContentItem(const RBX::Reflection::TypedPropertyDescriptor<T>& descriptor, CPropGrid* grid)
		: XItem<T, CXTPPropertyGridItem, RBX::Reflection::TypedPropertyDescriptor<T>>(descriptor, grid)
	{
		this->SetFlags(this->GetFlags() | xtpGridItemHasComboButton);
		this->GetConstraints()->AddConstraint("Browse...", 0, ID_FILE_OPEN);
		this->GetConstraints()->AddConstraint("Clear", 1, ID_EDIT_CLEAR);
	}

	void OnValueChanged(CString strValue) override
	{
		switch (this->GetConstraints()->GetCurrent()) {
		case 0:
			VTable0xf8();
			break;
		default:
			this->FUN_00443670(T((LPCTSTR) strValue));
			break;
		case 1:
			this->FUN_00443670(T());
			break;
		}
	}

	virtual void VTable0xf8() = 0;
};

class SoundItem : public ContentItem<RBX::Soundscape::SoundId>
{
public:
	SoundItem(const RBX::Reflection::TypedPropertyDescriptor<RBX::Soundscape::SoundId>& descriptor, CPropGrid* grid);

	void VTable0xf8() override;
};

class TextureItem : public ContentItem<RBX::TextureId>
{
public:
	TextureItem(const RBX::Reflection::TypedPropertyDescriptor<RBX::TextureId>& descriptor, CPropGrid* grid);

	void VTable0xf8() override;
};

class CSelectionPropGrid : public CPropGrid, public RBX::Listener<RBX::Selection, RBX::SelectionChanged>
{
	DECLARE_DYNAMIC(CSelectionPropGrid)

public:
	CSelectionPropGrid() {}

	~CSelectionPropGrid() override { FUN_00445580(nullptr); }

	RBX::Selection* VTable0x164() override { return m_unk0x1e0.get(); }

	void FUN_00445580(RBX::Selection* selection);

protected:
	void onEvent(const RBX::Selection* source, RBX::SelectionChanged event) override;

private:
	std::shared_ptr<RBX::Selection> m_unk0x1e0;
};

class CDataModelPropGrid : public CSelectionPropGrid, public ActiveDocView
{
	DECLARE_DYNAMIC(CDataModelPropGrid)

public:
	CDataModelPropGrid() {}

protected:
	void VTable0x00(
		std::shared_ptr<RBX::DataModel> newDataModel, std::shared_ptr<RBX::DataModel> oldDataModel
	) override;
};

#endif // PROPGRID_H
