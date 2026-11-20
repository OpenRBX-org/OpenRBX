#include "gui/Button.h"

#include "util/UserInputBase.h"
#include "v8tree/Verb.h"

namespace RBX {

GuiResponse KeyButton::process(const GuiEvent& event)
{
	if (event.eventType == UIEvent::KEY_DOWN && event.key() == keyCode) {
		keyIsDown = true;
	}

	if (event.eventType == UIEvent::KEY_UP && event.key() == keyCode) {
		keyIsDown = false;
	}

	return Widget::process(event);
}

void KeyButton::onDown(const UIEvent& event)
{
	event.userInput->setKeyState(keyCode, true);
}

void KeyButton::onUp(const UIEvent& event)
{
	event.userInput->setKeyState(keyCode, false);
}

void ImageKeyButton::render2d(Adorn* adorn)
{
	if (isVisible()) {
		imageDraw.render2d(
			adorn,
			isEnabled(),
			const_cast<Rect&>(static_cast<const Rect&>(getMyRect())),
			keyIsDown ? DOWN_OVER : widgetState
		);
	}
}

void ImageWidget::render2d(Adorn* adorn)
{
	if (isVisible()) {
		guiImageDraw.render2d(adorn, isEnabled(), const_cast<Rect&>(static_cast<const Rect&>(getMyRect())), NOTHING);
	}
}

void ImageButton::render2d(Adorn* adorn)
{
	if (isVisible()) {
		guiImageDraw.render2d(
			adorn, isEnabled(), const_cast<Rect&>(static_cast<const Rect&>(getMyRect())), widgetState
		);
	}
}

KeyButton::KeyButton(const std::string& name, SDLKey keyCode) : keyCode(keyCode), keyIsDown(false)
{
	setName(name);
}

ImageWidget::ImageWidget(Adorn* adorn, const std::string& name) : guiImageDraw(adorn, name)
{
	setName(name);
}

ImageButton::ImageButton(Adorn* adorn, Verb* verb, const std::string& name)
	: VerbWidget(verb), guiImageDraw(adorn, verb->getName().toString())
{
	setName(name);
}

ImageKeyButton::ImageKeyButton(Adorn* adorn, const std::string& name, SDLKey keyCode)
	: KeyButton(name, keyCode), imageDraw(adorn, name)
{
}

} // namespace RBX
