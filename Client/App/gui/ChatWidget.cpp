#include "gui/ChatWidget.h"

#include "network/Players.h"
#include "util/Sound.h"
#include "v8tree/Service.h"

namespace RBX {

void UnifiedImageWidget::render2dMe(Adorn* adorn)
{
	if (isVisible()) {
		guiImageDraw.render2d(adorn, true, const_cast<Rect&>(static_cast<const Rect&>(getMyRect())), getWidgetState());
	}
}

void ChatWidget::onMenuStateChanged()
{
	if (getMenuState() == HOVER) {
		setMenuState(SHOWN);
	}
}

ChatWidget::ChatWidget(const std::string& name, std::string chatCode)
{
	setName(name);
	code = chatCode;
}

bool ChatButton::isVisible() const
{
	Network::Players* players = ServiceProvider::find<Network::Players>(this);
	return players != nullptr && players->getLocalPlayer() != nullptr && Network::Players::clientIsPresent(this, true);
}

GuiResponse ChatWidget::process(const GuiEvent& event)
{
	if (isVisible() && event.isMouseEvent() && event.eventType == UIEvent::MOUSE_LEFT_BUTTON_UP &&
		getMyRect().pointInRect(event.mousePosition)) {
		if (Network::Players* players = ServiceProvider::find<Network::Players>(this)) {
			ServiceProvider::create<Soundscape::SoundService>(this)->playSound(CLICK_SOUND);
			std::string message = "/sc " + code;
			players->chat(message);
			return GuiResponse::finished();
		}
	}

	return UnifiedWidget::process(event);
}

} // namespace RBX
