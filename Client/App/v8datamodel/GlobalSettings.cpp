#include "v8datamodel/GlobalSettings.h"

#include "util/FileSystem.h"
#include "util/Name.h"
#include "util/standardout.h"
#include "v8xml/SerializerV2.h"
#include "v8xml/XmlSerializer.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <system_error>

namespace RBX {

const char sGlobalSettings[] = "GlobalSettings";

std::recursive_mutex GlobalSettings::mutex;

static std::string settingsFileName()
{
	std::string fileName = FileSystem::getUserAppDataDirectory(true, nullptr);

	if (fileName.size() > 0) {
		fileName += "GlobalSettings7.xml";
	}

	return fileName;
}

void GlobalSettings::eraseSettingsStore()
{
	std::error_code error;
	std::filesystem::remove(settingsFileName(), error);
}

void GlobalSettings::loadState()
{
	std::string fileName = settingsFileName();

	if (fileName.size() == 0) {
		return;
	}

	try {
		std::ifstream stream(fileName.c_str(), std::ios_base::in | std::ios_base::binary);
		TextXmlParser parser(stream.rdbuf());
		std::unique_ptr<XmlElement> root(parser.parse());

		std::scoped_lock lock(mutex);
		MergeBinder binder;
		readChildren(root.get(), binder);
		binder.resolveRefs();
	}
	catch (std::exception& e) {
		StandardOut::singleton()->print(MESSAGE_INFO, "Exception thrown in loadState %s", e.what());
	}
}

void GlobalSettings::saveState()
{
	std::string fileName = settingsFileName();

	if (fileName.size() == 0) {
		return;
	}

	std::unique_ptr<XmlElement> root(SerializerV2::newRootElement());
	writeChildren(root.get());

	std::ofstream stream(fileName.c_str(), std::ios_base::out | std::ios_base::binary);
	TextXmlWriter writer(stream);
	writer.serialize(root.get());
}

GlobalSettings::GlobalSettings()
{
	setName("Global Settings");
}

std::shared_ptr<GlobalSettings> GlobalSettings::singleton()
{
	std::scoped_lock lock(mutex);

	static std::shared_ptr<GlobalSettings> sing = Creatable<Instance>::create<GlobalSettings>();

	return sing;
}

} // namespace RBX

template const RBX::Name& RBX::Name::doDeclare<RBX::sGlobalSettings>();
template void RBX::Name::callDoDeclare<RBX::sGlobalSettings>();
