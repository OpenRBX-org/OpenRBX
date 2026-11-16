#ifndef V8DATAMODEL_GUIBUILDER_H
#define V8DATAMODEL_GUIBUILDER_H

#include "v8datamodel/Workspace.h"

#include <memory>
#include <string>

namespace RBX {

class Adorn;
class ChatOption;
class DataModel;
class GuiRoot;
class TopMenuBar;
class UnifiedWidget;
class Verb;

class GuiBuilder
{
private:
	std::shared_ptr<TopMenuBar> buildMenu();
	std::shared_ptr<TopMenuBar> buildRightPalette(Adorn* adorn);
	std::shared_ptr<TopMenuBar> buildPanelControlPalette(Adorn* adorn, GuiRoot* guiRoot);
	std::shared_ptr<TopMenuBar> buildScoreHud();
	std::shared_ptr<TopMenuBar> buildScoreHud(int offset);
	std::shared_ptr<TopMenuBar> buildControlHud(Adorn* adorn, int offset);
	std::shared_ptr<TopMenuBar> buildMessageHud(int offset);
	std::shared_ptr<TopMenuBar> buildChatHud();
	void buildChatMenu(ChatOption* option, std::string prefix, std::shared_ptr<UnifiedWidget> parent);
	std::shared_ptr<TopMenuBar> buildChatMenu(Adorn* adorn);
	std::shared_ptr<TopMenuBar> buildHealthHud();
	std::shared_ptr<TopMenuBar> buildStatsHud1();
	std::shared_ptr<TopMenuBar> buildStatsHud2();
	void buildRightPaletteItems(Adorn* adorn, std::shared_ptr<TopMenuBar> palette);
	TopMenuBar* buildColorPanel(Adorn* adorn, GuiRoot* guiRoot);

	DataModel* dataModel;
	Workspace* workspace;

	Verb* getVerb(const std::string& name) { return workspace->getVerb(name); }

public:
	void buildGui(Adorn* adorn, DataModel* dataModel, Workspace* workspace);
};

} // namespace RBX

#endif // V8DATAMODEL_GUIBUILDER_H
