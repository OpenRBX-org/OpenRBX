#include "gui/MenuItem.h"

#include "v8tree/Verb.h"

namespace RBX {

MenuItem::MenuItem(Verb* verb, const std::string& name) : VerbWidget(verb)
{
	setName(name);
	setGuiSize(G3D::Vector2(200, 24));
}

void MenuItem::render2d(Adorn* adorn)
{
	if (widgetState == HOVER || widgetState == DOWN_OVER) {
		adorn->rect2d(getMyRect2D(), menuSelect());
	}

	bool enabled = isEnabled();
	const G3D::Color4& fontColor = isEnabled() ? menuEnabledFill() : menuDisabledFill();

	label2d(
		adorn,
		getVerb() != nullptr && getVerb()->isChecked() ? "x " + getTitle() : getTitle(),
		fontColor,
		G3D::Color4::clear(),
		Adorn::XALIGN_LEFT
	);

	if (accelerator.size() > 0) {
		label2d(
			adorn,
			accelerator,
			isEnabled() ? menuEnabledFill() : menuDisabledFill(),
			G3D::Color4::clear(),
			Adorn::XALIGN_RIGHT
		);
	}
}

} // namespace RBX
