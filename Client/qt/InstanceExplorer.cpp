#include "InstanceExplorer.h"

#include "FileDialog.h"
#include "InsertObjectDialog.h"
#include "MainFrm.h"
#include "Profile.h"
#include "Resources.h"
#include "Roblox.h"
#include "SystemMessageBox.h"
#include "util/Http.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Decal.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/Selection.h"
#include "v8datamodel/custommesh.h"
#include "v8tree/Service.h"

#include <G3D/GImage.h>
#include <G3D/format.h>
#include <QFileInfo>
#include <fstream>
#include <vector>

bool isTrustedUrl(const char* url)
{
	return RBX::Http::trustCheck(url);
}

const RBX::Name& imageMimeType(const char* path)
{
	switch (G3D::GImage::resolveFormat(path)) {
	case G3D::GImage::JPEG:
		return RBX::Name::declare("image/jpeg", -1);
	case G3D::GImage::BMP:
		return RBX::Name::declare("image/bmp", -1);
	case G3D::GImage::ICO:
		return RBX::Name::declare("image/vnd.microsoft.icon", -1);
	case G3D::GImage::PNG:
		return RBX::Name::declare("image/png", -1);
	default:
		return RBX::Name::getNullName();
	}
}

InsertDecal::InsertDecal(RBX::DataModel* dataModel, const char* name) : RBX::EditSelectionVerb(name, dataModel)
{
}

RBX::ContentId registerImageFile(const char* path)
{
	std::ifstream stream(path, std::ios_base::in | std::ios_base::binary);
	return RBX::ContentProvider::singleton().registerContent(stream, imageMimeType(path));
}

std::shared_ptr<RBX::Decal> InsertDecal::createDecal()
{
	return RBX::Creatable<RBX::Instance>::create<RBX::Decal>();
}

std::shared_ptr<RBX::Decal> InsertTexture::createDecal()
{
	return RBX::Creatable<RBX::Instance>::create<RBX::Texture>();
}

RBX::Instance* InsertDecal::getDecalParent() const
{
	RBX::Selection* selection = dataModel->create<RBX::Selection>();
	if (selection->size() == 1) {
		RBX::Instance* instance = selection->front().get();
		if (dynamic_cast<RBX::PartInstance*>(instance)) {
			return instance;
		}

		if (dynamic_cast<RBX::SpecialShape*>(instance)) {
			return instance;
		}
	}

	return nullptr;
}

void InsertDecal::doIt(RBX::IDataState* dataState)
{
	CFileDialog dialog(true, QString(), QString(), 0, Resources::string(132));
	if (dialog.DoModal() == IDOK) {
		QString pathName = dialog.GetPathName();
		std::string path = pathName.toStdString();
		RBX::ContentId id = registerImageFile(path.c_str());
		std::shared_ptr<RBX::Decal> decal = createDecal();
		decal->setTexture(id);
		decal->setName(QFileInfo(pathName).completeBaseName().toStdString());
		decal->setParent(getDecalParent());

		RBX::Selection* selection = dataModel->create<RBX::Selection>();
		selection->setSelection(decal.get());

		dataState->clearStack();
	}
}

bool InsertDecal::isEnabled() const
{
	return RBX::EditSelectionVerb::isEnabled() && getDecalParent();
}

InsertObject::InsertObject(RBX::DataModel* dataModel) : RBX::RunStateVerb("Insert Object", dataModel)
{
}

RBX::Instance* InsertObject::getInsertParent() const
{
	RBX::Selection* selection = dataModel->create<RBX::Selection>();
	size_t count = selection->size();
	if (count == 1) {
		return selection->front().get();
	}

	if (count == 0) {
		return dataModel->getWorkspace();
	}

	return nullptr;
}

void InsertObject::doIt(RBX::IDataState* dataState)
{
	CInsertObjectDialog dialog(theApp.mainFrame());
	dialog.m_className = Profile::GetProfileString("InsertObject", "ClassName");

	if (dialog.DoModal() == IDOK) {
		Profile::WriteProfileString("InsertObject", "ClassName", dialog.m_className);

		std::string name = dialog.m_className.toStdString();
		const RBX::Name& className = RBX::Name::lookup(name.c_str());
		std::shared_ptr<RBX::Instance> instance = RBX::AbstractFactoryProduct<RBX::Instance>::create(className);
		if (!instance) {
			SystemMessageBox::warning(
				theApp.mainFrame(),
				Resources::string(appTitleStringId),
				Resources::string(133).replace("%s", dialog.m_className)
			);
		}
		else if (dynamic_cast<RBX::Service*>(instance.get())) {
			if (!RBX::ServiceProvider::create(dataModel, className)) {
				SystemMessageBox::warning(
					theApp.mainFrame(),
					Resources::string(appTitleStringId),
					Resources::string(137).replace("%s", dialog.m_className)
				);
			}
		}
		else {
			if (!instance->canSetParent(getInsertParent())) {
				throw std::runtime_error(
					G3D::format(
						"%s cannot be a child of %s",
						instance->getClassNameStr().c_str(),
						getInsertParent()->getName().c_str()
					)
				);
			}

			instance->setParent(getInsertParent());

			RBX::Selection* selection = dataModel->create<RBX::Selection>();
			selection->setSelection(instance.get());

			dataState->clearStack();
		}
	}
}

bool InsertObject::isEnabled() const
{
	return getInsertParent() != nullptr;
}
