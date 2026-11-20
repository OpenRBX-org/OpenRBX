#ifndef GUI_MENUITEM_H
#define GUI_MENUITEM_H

#include "gui/Widget.h"

#include <string>

namespace RBX {

class Verb;

class MenuItem : public VerbWidget
{
private:
	std::string accelerator;

protected:
	bool canLoseFocus() override { return true; }

public:
	MenuItem(Verb* verb, const std::string& name, std::string label);
	MenuItem(Verb* verb, const std::string& name);

	void render2d(Adorn* adorn) override;
};

} // namespace RBX

#endif // GUI_MENUITEM_H
