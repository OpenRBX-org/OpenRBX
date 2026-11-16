#ifndef GUI_HEALTHHUD_H
#define GUI_HEALTHHUD_H

#include "gui/GUI.h"

#include <memory>

namespace RBX {

class Visit;

class HealthHud : public GuiItem
{
private:
	std::shared_ptr<Visit> visit;

	void render2d(Adorn* adorn) override;

public:
	HealthHud();
};

} // namespace RBX

#endif // GUI_HEALTHHUD_H
