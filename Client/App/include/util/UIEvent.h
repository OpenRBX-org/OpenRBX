#ifndef UTIL_UIEVENT_H
#define UTIL_UIEVENT_H

#include "util/KeyCode.h"

#include <G3D/Vector2int16.h>
#include <bit>
#include <type_traits>

namespace RBX {

class UserInputBase;

class UIEvent
{
public:
	enum EventType
	{
		NO_EVENT = 0,
		MOUSE_RIGHT_BUTTON_DOWN = 1,
		MOUSE_RIGHT_BUTTON_UP = 2,
		MOUSE_LEFT_BUTTON_DOWN = 3,
		MOUSE_LEFT_BUTTON_UP = 4,
		MOUSE_MOVE = 5,
		MOUSE_DELTA = 6,
		MOUSE_IDLE = 7,
		MOUSE_WHEEL_FORWARD = 8,
		MOUSE_WHEEL_BACKWARD = 9,
		KEY_DOWN = 10,
		KEY_UP = 11
	};

	EventType eventType;
	UserInputBase* userInput;
	G3D::Vector2int16 mousePosition;
	G3D::Vector2int16 mouseDelta;
	G3D::Vector2int16 windowSize;

	UIEvent(UserInputBase* userInput, EventType eventType, SDLKey key, SDLMod mod)
		: eventType(eventType), userInput(userInput),
		  mousePosition(std::bit_cast<G3D::Vector2int16>(static_cast<std::underlying_type_t<SDLKey>>(key))),
		  mouseDelta(std::bit_cast<G3D::Vector2int16>(static_cast<std::underlying_type_t<SDLMod>>(mod))),
		  windowSize(0, 0)
	{
	}
	UIEvent(UserInputBase* userInput, EventType eventType, G3D::Vector2int16 position, G3D::Vector2int16 size)
		: eventType(eventType), userInput(userInput), mousePosition(position), mouseDelta(0, 0), windowSize(size)
	{
	}
	UIEvent() : eventType(NO_EVENT), userInput(nullptr) {}

	bool isMouseEvent() const
	{
		return eventType == MOUSE_RIGHT_BUTTON_DOWN || eventType == MOUSE_RIGHT_BUTTON_UP ||
			   eventType == MOUSE_LEFT_BUTTON_DOWN || eventType == MOUSE_LEFT_BUTTON_UP || eventType == MOUSE_MOVE ||
			   eventType == MOUSE_DELTA || eventType == MOUSE_IDLE;
	}

	SDLKey key() const { return static_cast<SDLKey>(std::bit_cast<std::underlying_type_t<SDLKey>>(mousePosition)); }
	SDLMod mod() const { return static_cast<SDLMod>(std::bit_cast<std::underlying_type_t<SDLMod>>(mouseDelta)); }

	bool isKeyPressedEvent(SDLKey key) const { return eventType == KEY_DOWN && this->key() == key; }
	bool isKeyUpEvent(SDLKey key) const;
	bool isKeyPressedWithShiftEvent(SDLKey key) const;
	bool isKeyPressedWithShiftEvent() const;
};

} // namespace RBX

#endif // UTIL_UIEVENT_H
