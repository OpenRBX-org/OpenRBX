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
	MenuItem(Verb* p_unk0x04, const std::string& p_unk0x08, std::string p_unk0x0c);
	MenuItem(Verb* p_unk0x04, const std::string& p_unk0x08);

	void render2d(Adorn* adorn) override;
};

} // namespace RBX

#endif // GUI_MENUITEM_H
