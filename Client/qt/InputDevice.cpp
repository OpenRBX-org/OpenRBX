#include "InputDevice.h"

#include "UserInputUtil.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_video.h>
#include <algorithm>
#include <cstdint>
#include <stdexcept>

#ifndef _WIN32
#include <QSocketNotifier>
#include <X11/Xlib.h>
#endif

struct ScanCodeEntry
{
	SDL_Scancode scancode;
	KeyboardScanCode scanCode;
};

static const ScanCodeEntry scanCodes[] = {
	{SDL_SCANCODE_ESCAPE, SCAN_ESCAPE},
	{SDL_SCANCODE_1, SCAN_1},
	{SDL_SCANCODE_2, SCAN_2},
	{SDL_SCANCODE_3, SCAN_3},
	{SDL_SCANCODE_4, SCAN_4},
	{SDL_SCANCODE_5, SCAN_5},
	{SDL_SCANCODE_6, SCAN_6},
	{SDL_SCANCODE_7, SCAN_7},
	{SDL_SCANCODE_8, SCAN_8},
	{SDL_SCANCODE_9, SCAN_9},
	{SDL_SCANCODE_0, SCAN_0},
	{SDL_SCANCODE_MINUS, SCAN_MINUS},
	{SDL_SCANCODE_EQUALS, SCAN_EQUALS},
	{SDL_SCANCODE_BACKSPACE, SCAN_BACK},
	{SDL_SCANCODE_TAB, SCAN_TAB},
	{SDL_SCANCODE_Q, SCAN_Q},
	{SDL_SCANCODE_W, SCAN_W},
	{SDL_SCANCODE_E, SCAN_E},
	{SDL_SCANCODE_R, SCAN_R},
	{SDL_SCANCODE_T, SCAN_T},
	{SDL_SCANCODE_Y, SCAN_Y},
	{SDL_SCANCODE_U, SCAN_U},
	{SDL_SCANCODE_I, SCAN_I},
	{SDL_SCANCODE_O, SCAN_O},
	{SDL_SCANCODE_P, SCAN_P},
	{SDL_SCANCODE_LEFTBRACKET, SCAN_LBRACKET},
	{SDL_SCANCODE_RIGHTBRACKET, SCAN_RBRACKET},
	{SDL_SCANCODE_RETURN, SCAN_RETURN},
	{SDL_SCANCODE_LCTRL, SCAN_LCONTROL},
	{SDL_SCANCODE_A, SCAN_A},
	{SDL_SCANCODE_S, SCAN_S},
	{SDL_SCANCODE_D, SCAN_D},
	{SDL_SCANCODE_F, SCAN_F},
	{SDL_SCANCODE_G, SCAN_G},
	{SDL_SCANCODE_H, SCAN_H},
	{SDL_SCANCODE_J, SCAN_J},
	{SDL_SCANCODE_K, SCAN_K},
	{SDL_SCANCODE_L, SCAN_L},
	{SDL_SCANCODE_SEMICOLON, SCAN_SEMICOLON},
	{SDL_SCANCODE_APOSTROPHE, SCAN_APOSTROPHE},
	{SDL_SCANCODE_GRAVE, SCAN_GRAVE},
	{SDL_SCANCODE_LSHIFT, SCAN_LSHIFT},
	{SDL_SCANCODE_BACKSLASH, SCAN_BACKSLASH},
	{SDL_SCANCODE_Z, SCAN_Z},
	{SDL_SCANCODE_X, SCAN_X},
	{SDL_SCANCODE_C, SCAN_C},
	{SDL_SCANCODE_V, SCAN_V},
	{SDL_SCANCODE_B, SCAN_B},
	{SDL_SCANCODE_N, SCAN_N},
	{SDL_SCANCODE_M, SCAN_M},
	{SDL_SCANCODE_COMMA, SCAN_COMMA},
	{SDL_SCANCODE_PERIOD, SCAN_PERIOD},
	{SDL_SCANCODE_SLASH, SCAN_SLASH},
	{SDL_SCANCODE_RSHIFT, SCAN_RSHIFT},
	{SDL_SCANCODE_KP_MULTIPLY, SCAN_MULTIPLY},
	{SDL_SCANCODE_LALT, SCAN_LMENU},
	{SDL_SCANCODE_SPACE, SCAN_SPACE},
	{SDL_SCANCODE_CAPSLOCK, SCAN_CAPITAL},
	{SDL_SCANCODE_F1, SCAN_F1},
	{SDL_SCANCODE_F2, SCAN_F2},
	{SDL_SCANCODE_F3, SCAN_F3},
	{SDL_SCANCODE_F4, SCAN_F4},
	{SDL_SCANCODE_F5, SCAN_F5},
	{SDL_SCANCODE_F6, SCAN_F6},
	{SDL_SCANCODE_F7, SCAN_F7},
	{SDL_SCANCODE_F8, SCAN_F8},
	{SDL_SCANCODE_F9, SCAN_F9},
	{SDL_SCANCODE_F10, SCAN_F10},
	{SDL_SCANCODE_NUMLOCKCLEAR, SCAN_NUMLOCK},
	{SDL_SCANCODE_SCROLLLOCK, SCAN_SCROLL},
	{SDL_SCANCODE_KP_7, SCAN_NUMPAD7},
	{SDL_SCANCODE_KP_8, SCAN_NUMPAD8},
	{SDL_SCANCODE_KP_9, SCAN_NUMPAD9},
	{SDL_SCANCODE_KP_MINUS, SCAN_SUBTRACT},
	{SDL_SCANCODE_KP_4, SCAN_NUMPAD4},
	{SDL_SCANCODE_KP_5, SCAN_NUMPAD5},
	{SDL_SCANCODE_KP_6, SCAN_NUMPAD6},
	{SDL_SCANCODE_KP_PLUS, SCAN_ADD},
	{SDL_SCANCODE_KP_1, SCAN_NUMPAD1},
	{SDL_SCANCODE_KP_2, SCAN_NUMPAD2},
	{SDL_SCANCODE_KP_3, SCAN_NUMPAD3},
	{SDL_SCANCODE_KP_0, SCAN_NUMPAD0},
	{SDL_SCANCODE_KP_PERIOD, SCAN_DECIMAL},
	{SDL_SCANCODE_NONUSBACKSLASH, SCAN_OEM_102},
	{SDL_SCANCODE_F11, SCAN_F11},
	{SDL_SCANCODE_F12, SCAN_F12},
	{SDL_SCANCODE_F13, SCAN_F13},
	{SDL_SCANCODE_F14, SCAN_F14},
	{SDL_SCANCODE_F15, SCAN_F15},
	{SDL_SCANCODE_KP_EQUALS, SCAN_NUMPADEQUALS},
	{SDL_SCANCODE_KP_ENTER, SCAN_NUMPADENTER},
	{SDL_SCANCODE_RCTRL, SCAN_RCONTROL},
	{SDL_SCANCODE_KP_DIVIDE, SCAN_DIVIDE},
	{SDL_SCANCODE_PRINTSCREEN, SCAN_SYSRQ},
	{SDL_SCANCODE_RALT, SCAN_RMENU},
	{SDL_SCANCODE_PAUSE, SCAN_PAUSE},
	{SDL_SCANCODE_HOME, SCAN_HOME},
	{SDL_SCANCODE_UP, SCAN_UP},
	{SDL_SCANCODE_PAGEUP, SCAN_PRIOR},
	{SDL_SCANCODE_LEFT, SCAN_LEFT},
	{SDL_SCANCODE_RIGHT, SCAN_RIGHT},
	{SDL_SCANCODE_END, SCAN_END},
	{SDL_SCANCODE_DOWN, SCAN_DOWN},
	{SDL_SCANCODE_PAGEDOWN, SCAN_NEXT},
	{SDL_SCANCODE_INSERT, SCAN_INSERT},
	{SDL_SCANCODE_DELETE, SCAN_DELETE},
	{SDL_SCANCODE_LGUI, SCAN_LWIN},
	{SDL_SCANCODE_RGUI, SCAN_RWIN},
	{SDL_SCANCODE_APPLICATION, SCAN_APPS},
};

static SDL_Window* window;
static int users;
#ifndef _WIN32
static QSocketNotifier* connectionNotifier;
#endif

static unsigned scanCodeFromSDL(SDL_Scancode scancode)
{
	for (const ScanCodeEntry& entry : scanCodes) {
		if (entry.scancode == scancode) {
			return entry.scanCode;
		}
	}
	return 0;
}

static void convertMouseEvent(const SDL_Event& source, InputEvent& event)
{
	event = {};
	switch (source.type) {
	case SDL_EVENT_MOUSE_MOTION:
		event.type = InputEvent::MouseMotion;
		event.x = source.motion.xrel;
		event.y = source.motion.yrel;
		break;
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP:
		event.type = InputEvent::MouseButton;
		event.button = source.button.button == SDL_BUTTON_LEFT    ? InputEvent::LeftButton
					   : source.button.button == SDL_BUTTON_RIGHT ? InputEvent::RightButton
																  : InputEvent::OtherButton;
		event.down = source.button.down;
		break;
	default:
		event.type = InputEvent::MouseWheel;
		event.y = source.wheel.y;
		break;
	}
}

namespace InputDevice {

void open(void* frame)
{
	if (users++ != 0) {
		return;
	}

	SDL_SetHint(SDL_HINT_WINDOWS_ENABLE_MESSAGELOOP, "0");
	SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER, "0");
	SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");
	if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		users = 0;
		throw std::runtime_error(SDL_GetError());
	}

	SDL_PropertiesID properties = SDL_CreateProperties();
#ifdef _WIN32
	SDL_SetPointerProperty(properties, SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER, frame);
#else
	SDL_SetNumberProperty(
		properties,
		SDL_PROP_WINDOW_CREATE_X11_WINDOW_NUMBER,
		static_cast<Sint64>(reinterpret_cast<std::uintptr_t>(frame))
	);
#endif
	window = SDL_CreateWindowWithProperties(properties);
	SDL_DestroyProperties(properties);
	if (!window) {
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		users = 0;
		throw std::runtime_error(SDL_GetError());
	}
#ifndef _WIN32
	Display* display = static_cast<Display*>(
		SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr)
	);
	connectionNotifier = new QSocketNotifier(XConnectionNumber(display), QSocketNotifier::Read);
	QObject::connect(connectionNotifier, &QSocketNotifier::activated, SDL_PumpEvents);
#endif
}

void close()
{
	if (--users == 0) {
#ifndef _WIN32
		delete connectionNotifier;
		connectionNotifier = nullptr;
#endif
		SDL_DestroyWindow(window);
		window = nullptr;
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}
}

void pump()
{
	SDL_PumpEvents();
}

bool captureMouse(int x, int y)
{
#ifdef _WIN32
	SDL_Rect pin = {x, y, 1, 1};
	SDL_FlushEvents(SDL_EVENT_MOUSE_MOTION, SDL_EVENT_MOUSE_WHEEL);
	SDL_SetWindowMouseRect(window, &pin);
	if (!SDL_SetWindowRelativeMouseMode(window, true)) {
		SDL_SetWindowMouseRect(window, nullptr);
		return false;
	}
	return true;
#else
	SDL_PumpEvents();
	SDL_FlushEvents(SDL_EVENT_MOUSE_MOTION, SDL_EVENT_MOUSE_WHEEL);
	return SDL_SetWindowRelativeMouseMode(window, true);
#endif
}

void releaseMouse()
{
	SDL_SetWindowRelativeMouseMode(window, false);
#ifdef _WIN32
	SDL_SetWindowMouseRect(window, nullptr);
#endif
}

int takeMouseEvents(InputEvent* events, int capacity)
{
	int count = 0;
	SDL_Event event;
	while (count < capacity &&
		   SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENT_MOUSE_MOTION, SDL_EVENT_MOUSE_WHEEL) == 1) {
		convertMouseEvent(event, events[count++]);
	}
	return count;
}

int peekMouseMotion(InputEvent* events, int capacity)
{
	static thread_local SDL_Event pending[eventBufferSize];
	int count = SDL_PeepEvents(
		pending, std::min(capacity, eventBufferSize), SDL_PEEKEVENT, SDL_EVENT_MOUSE_MOTION, SDL_EVENT_MOUSE_MOTION
	);
	for (int i = 0; i < count; i++) {
		convertMouseEvent(pending[i], events[i]);
	}
	return count < 0 ? 0 : count;
}

void flushMouseEvents()
{
	SDL_FlushEvents(SDL_EVENT_MOUSE_MOTION, SDL_EVENT_MOUSE_WHEEL);
}

int takeKeyEvents(InputEvent* events, int capacity)
{
	int count = 0;
	SDL_Event event;
	while (count < capacity && SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENT_KEY_DOWN, SDL_EVENT_KEY_UP) == 1) {
		unsigned scanCode = scanCodeFromSDL(event.key.scancode);
		if (event.key.repeat || scanCode == 0) {
			continue;
		}

		events[count] = {};
		events[count].type = InputEvent::Key;
		events[count].down = event.key.down;
		events[count].scanCode = scanCode;
		count++;
	}
	return count;
}

void flushKeyEvents()
{
	SDL_FlushEvents(SDL_EVENT_KEY_DOWN, SDL_EVENT_KEY_UP);
}

void readKeyboardState(unsigned char* keys)
{
	const bool* state = SDL_GetKeyboardState(nullptr);
	for (const ScanCodeEntry& entry : scanCodes) {
		keys[entry.scanCode] = state[entry.scancode] ? 0x80 : 0;
	}
}

void flushOtherEvents()
{
	SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_KEY_DOWN - 1);
	SDL_FlushEvents(SDL_EVENT_KEY_UP + 1, SDL_EVENT_MOUSE_MOTION - 1);
	SDL_FlushEvents(SDL_EVENT_MOUSE_WHEEL + 1, SDL_EVENT_LAST);
}

#ifndef _WIN32
void postMouseButton(InputEvent::Button button, bool down)
{
	SDL_Event event = {};
	event.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
	event.button.windowID = SDL_GetWindowID(window);
	event.button.button = button == InputEvent::LeftButton    ? SDL_BUTTON_LEFT
						  : button == InputEvent::RightButton ? SDL_BUTTON_RIGHT
															  : SDL_BUTTON_MIDDLE;
	event.button.down = down;
	SDL_PushEvent(&event);
}

void postMouseWheel(float y)
{
	SDL_Event event = {};
	event.type = SDL_EVENT_MOUSE_WHEEL;
	event.wheel.windowID = SDL_GetWindowID(window);
	event.wheel.y = y;
	SDL_PushEvent(&event);
}
#endif

} // namespace InputDevice
