#ifndef CLIPBOARDVERBS_H
#define CLIPBOARDVERBS_H

#include "v8datamodel/Commands.h"
#include "v8datamodel/DataModel.h"
#include "v8tree/Verb.h"

class PasteVerb : public RBX::Verb
{
public:
	PasteVerb(RBX::DataModel* dataModel, bool pasteInto);

	bool isEnabled() const override;
	void doIt(RBX::IDataState* dataState) override;

private:
	RBX::DataModel* m_unk0x0c;
	bool m_unk0x10;
};

class CutVerb : public RBX::DeleteSelectionVerb
{
public:
	CutVerb(RBX::DataModel* dataModel) : RBX::DeleteSelectionVerb(dataModel, dataModel, "Cut") {}

	void doIt(RBX::IDataState* dataState) override;
};

class CopyVerb : public RBX::EditSelectionVerb
{
public:
	CopyVerb(RBX::DataModel* dataModel);

	~CopyVerb() override = default;

	void doIt(RBX::IDataState* dataState) override;
};

#endif // CLIPBOARDVERBS_H
