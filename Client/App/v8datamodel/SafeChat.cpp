#include "v8datamodel/SafeChat.h"

#include "util/ContentProvider.h"
#include "util/Name.h"
#include "util/Utilities.h"
#include "v8xml/XmlElement.h"
#include "v8xml/XmlSerializer.h"

#include <algorithm>
#include <fstream>
#include <mutex>

static const RBX::Name& utterance = RBX::Name::declare("utterance", -1);

static std::unique_ptr<RBX::SafeChat> sing;

static std::once_flag once_SafeChat_singleton;

namespace RBX {

static void deleteChatOption(ChatOption* option)
{
	delete option;
}

static std::string FUN_00660930()
{
	std::string file = ContentProvider::singleton().getAssetFile("Fonts\\safechat.xml");
	return file;
}

ChatOption::ChatOption()
{
}

ChatOption::ChatOption(std::string text)
{
	this->text = text;
}

ChatOption::~ChatOption()
{
	std::for_each(children.begin(), children.end(), deleteChatOption);
}

void SafeChat::loadChildren(ChatOption* parent, XmlElement* element)
{
	for (const XmlElement* child = element->findFirstChildByTag(utterance); child != NULL;
		 child = element->findNextChildWithSameTag(child)) {
		std::string text;
		child->getValue(text);
		trim(text);

		ChatOption* option = new ChatOption(text);
		parent->children.push_back(option);
		loadChildren(option, const_cast<XmlElement*>(child));
	}
}

std::string SafeChat::getMessage(std::vector<std::string> path)
{
	ChatOption* option = chatRoot.get();

	for (unsigned int i = 1; i < path.size(); i++) {
		int index = lexicalCast<int>(path[i]);

		if (index >= option->children.size()) {
			return "";
		}

		option = option->children[index];
	}

	return option->text;
}

void SafeChat::loadChatTree()
{
	std::string fileName = FUN_00660930();
	std::ifstream stream(fileName.c_str(), std::ios_base::in | std::ios_base::binary);
	TextXmlParser parser(stream.rdbuf());
	std::unique_ptr<XmlElement> root = parser.parse();

	chatRoot.reset(new ChatOption());
	chatRoot->text = "ROOT";
	loadChildren(chatRoot.get(), root.get());
}

SafeChat::SafeChat()
{
	loadChatTree();
}

static void SafeChat_singleton()
{
	sing.reset(new SafeChat());
}

SafeChat& SafeChat::singleton()
{
	std::call_once(once_SafeChat_singleton, SafeChat_singleton);
	return *sing;
}

} // namespace RBX
