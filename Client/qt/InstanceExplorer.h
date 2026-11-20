#ifndef INSTANCEEXPLORER_H
#define INSTANCEEXPLORER_H

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
	virtual std::shared_ptr<RBX::Decal> createDecal();

private:
	RBX::Instance* getDecalParent() const;
};

class InsertTexture : public InsertDecal
{
public:
	InsertTexture(RBX::DataModel* dataModel) : InsertDecal(dataModel, "InsertTexture") {}

	std::shared_ptr<RBX::Decal> createDecal() override;
};

class InsertObject : public RBX::RunStateVerb
{
public:
	InsertObject(RBX::DataModel* dataModel);

	bool isEnabled() const override;
	void doIt(RBX::IDataState* dataState) override;

private:
	RBX::Instance* getInsertParent() const;
};

bool isTrustedUrl(const char* url);
RBX::ContentId registerImageFile(const char* path);
const RBX::Name& imageMimeType(const char* path);

#endif // INSTANCEEXPLORER_H
