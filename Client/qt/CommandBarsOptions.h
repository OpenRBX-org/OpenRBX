#ifndef COMMANDBARSOPTIONS_H
#define COMMANDBARSOPTIONS_H

#include <set>

enum MenuAnimation
{
	MenuAnimationWindowsDefault,
	MenuAnimationRandom,
	MenuAnimationUnfold,
	MenuAnimationSlide,
	MenuAnimationFade,
	MenuAnimationNone,
};

struct CommandBarsOptions
{
	bool alwaysShowFullMenus = true;
	bool showFullAfterDelay = true;
	bool toolBarScreenTips = true;
	bool toolBarAccelTips = true;
	bool largeIcons = false;
	int animation = MenuAnimationWindowsDefault;
	std::set<unsigned> usedCommands;
};

#endif // COMMANDBARSOPTIONS_H
