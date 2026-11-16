#ifndef INSTANCEEXPLORER_H
#define INSTANCEEXPLORER_H

#include "MarshaledListener.h"
#include "util/ContentProvider.h"
#include "v8datamodel/Commands.h"
#include "v8datamodel/Decal.h"
#include "v8tree/Instance.h"

#include <memory>

class InsertDecal : public RBX::EditSelectionVerb
{
public:
	InsertDecal(RBX::DataModel* dataModel, const char* name);

	bool isEnabled() const override;
	void doIt(RBX::IDataState* dataState) override;
	virtual std::shared_ptr<RBX::Decal> VTable0x14();

private:
	RBX::Instance* FUN_0041f500() const;
};

class InsertTexture : public InsertDecal
{
public:
	InsertTexture(RBX::DataModel* dataModel) : InsertDecal(dataModel, "InsertTexture") {}

	std::shared_ptr<RBX::Decal> VTable0x14() override;
};

class InsertObject : public RBX::RunStateVerb
{
public:
	InsertObject(RBX::DataModel* dataModel);

	bool isEnabled() const override;
	void doIt(RBX::IDataState* dataState) override;

private:
	RBX::Instance* FUN_0041fb40() const;
};

class CInstanceRecord : public CXTPReportRecord
{
public:
	class CNameItem : public CXTPReportRecordItem
	{
	public:
		CNameItem(RBX::Instance* instance) : m_unk0x7c(instance) {}

		CString GetCaption(CXTPReportColumn* pColumn) override;

	private:
		RBX::Instance* m_unk0x7c;
	};

	CInstanceRecord(std::shared_ptr<RBX::Instance> instance);
	~CInstanceRecord() override;

	std::shared_ptr<RBX::Instance> m_unk0x54;
};

class CInstanceExplorer
	: public CXTPReportView,
	  public RBX::MarshaledListener<RBX::Instance, RBX::DescendentAdded, RBX::Instance, RBX::DescendentAdded>,
	  public RBX::MarshaledListener<RBX::Instance, RBX::DescendentRemoving, RBX::Instance, RBX::DescendentRemoving>
{
	DECLARE_DYNCREATE(CInstanceExplorer)

protected:
	CInstanceExplorer();

public:
	~CInstanceExplorer() override;

	void FUN_004212d0(RBX::Instance* root);

protected:
	void VTable0x08(const RBX::Instance* source, RBX::DescendentRemoving event) override;
	void VTable0x08(const RBX::Instance* source, RBX::DescendentAdded event) override;

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void FUN_00421580(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()

private:
	void populate()
	{
		if (!m_unk0x3ac) {
			GetReportCtrl().Populate();
		}
	}

	std::shared_ptr<RBX::Instance> m_unk0x3a4;
	bool m_unk0x3ac;
};

HRESULT FUN_0041e610(LPCTSTR url);
HRESULT FUN_0041e630(BSTR url);
HRESULT FUN_0041e6b0(BSTR url, DWORD zone);
RBX::ContentId FUN_0041e900(const char* path);
const RBX::Name& FUN_0043c120(const char* path);

#endif // INSTANCEEXPLORER_H
