#include "UserInputUtil.h"

#include "util/Rect.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

void moveCursorFree(const G3D::Vector2& mouseDelta, G3D::Vector2& wrapMouseDelta, G3D::Vector2& position)
{
	wrapMouseDelta = G3D::Vector2::zero();
	position += mouseDelta;
}

void wrapCursorCenter(const G3D::Vector2& mouseDelta, G3D::Vector2& wrapMouseDelta, G3D::Vector2& position)
{
	wrapMouseDelta += mouseDelta;
}

static SDLKey scanCodeToKey[0x100];
static bool scanCodeToKeyReady;

SDLKey keyFromScanCode(unsigned scanCode)
{
	if (!scanCodeToKeyReady) {
		for (int i = 0; i < 0x100; i++) {
			scanCodeToKey[i] = SDLK_UNKNOWN;
		}

		scanCodeToKey[SCAN_ESCAPE] = SDLK_ESCAPE;
		scanCodeToKey[SCAN_1] = SDLK_1;
		scanCodeToKey[SCAN_2] = SDLK_2;
		scanCodeToKey[SCAN_3] = SDLK_3;
		scanCodeToKey[SCAN_4] = SDLK_4;
		scanCodeToKey[SCAN_5] = SDLK_5;
		scanCodeToKey[SCAN_6] = SDLK_6;
		scanCodeToKey[SCAN_7] = SDLK_7;
		scanCodeToKey[SCAN_8] = SDLK_8;
		scanCodeToKey[SCAN_9] = SDLK_9;
		scanCodeToKey[SCAN_0] = SDLK_0;
		scanCodeToKey[SCAN_MINUS] = SDLK_MINUS;
		scanCodeToKey[SCAN_EQUALS] = SDLK_EQUALS;
		scanCodeToKey[SCAN_BACK] = SDLK_BACKSPACE;
		scanCodeToKey[SCAN_TAB] = SDLK_TAB;
		scanCodeToKey[SCAN_Q] = SDLK_q;
		scanCodeToKey[SCAN_W] = SDLK_w;
		scanCodeToKey[SCAN_E] = SDLK_e;
		scanCodeToKey[SCAN_R] = SDLK_r;
		scanCodeToKey[SCAN_T] = SDLK_t;
		scanCodeToKey[SCAN_Y] = SDLK_y;
		scanCodeToKey[SCAN_U] = SDLK_u;
		scanCodeToKey[SCAN_I] = SDLK_i;
		scanCodeToKey[SCAN_O] = SDLK_o;
		scanCodeToKey[SCAN_P] = SDLK_p;
		scanCodeToKey[SCAN_LBRACKET] = SDLK_LEFTBRACKET;
		scanCodeToKey[SCAN_RBRACKET] = SDLK_RIGHTBRACKET;
		scanCodeToKey[SCAN_RETURN] = SDLK_RETURN;
		scanCodeToKey[SCAN_LCONTROL] = SDLK_LCTRL;
		scanCodeToKey[SCAN_A] = SDLK_a;
		scanCodeToKey[SCAN_S] = SDLK_s;
		scanCodeToKey[SCAN_D] = SDLK_d;
		scanCodeToKey[SCAN_F] = SDLK_f;
		scanCodeToKey[SCAN_G] = SDLK_g;
		scanCodeToKey[SCAN_H] = SDLK_h;
		scanCodeToKey[SCAN_J] = SDLK_j;
		scanCodeToKey[SCAN_K] = SDLK_k;
		scanCodeToKey[SCAN_L] = SDLK_l;
		scanCodeToKey[SCAN_SEMICOLON] = SDLK_SEMICOLON;
		scanCodeToKey[SCAN_APOSTROPHE] = SDLK_QUOTE;
		scanCodeToKey[SCAN_GRAVE] = SDLK_BACKQUOTE;
		scanCodeToKey[SCAN_LSHIFT] = SDLK_LSHIFT;
		scanCodeToKey[SCAN_BACKSLASH] = SDLK_BACKSLASH;
		scanCodeToKey[SCAN_OEM_102] = SDLK_BACKSLASH;
		scanCodeToKey[SCAN_Z] = SDLK_z;
		scanCodeToKey[SCAN_X] = SDLK_x;
		scanCodeToKey[SCAN_C] = SDLK_c;
		scanCodeToKey[SCAN_V] = SDLK_v;
		scanCodeToKey[SCAN_B] = SDLK_b;
		scanCodeToKey[SCAN_N] = SDLK_n;
		scanCodeToKey[SCAN_M] = SDLK_m;
		scanCodeToKey[SCAN_COMMA] = SDLK_COMMA;
		scanCodeToKey[SCAN_PERIOD] = SDLK_PERIOD;
		scanCodeToKey[SCAN_SLASH] = SDLK_SLASH;
		scanCodeToKey[SCAN_RSHIFT] = SDLK_RSHIFT;
		scanCodeToKey[SCAN_MULTIPLY] = SDLK_KP_MULTIPLY;
		scanCodeToKey[SCAN_LMENU] = SDLK_LALT;
		scanCodeToKey[SCAN_SPACE] = SDLK_SPACE;
		scanCodeToKey[SCAN_CAPITAL] = SDLK_CAPSLOCK;
		scanCodeToKey[SCAN_F1] = SDLK_F1;
		scanCodeToKey[SCAN_F2] = SDLK_F2;
		scanCodeToKey[SCAN_F3] = SDLK_F3;
		scanCodeToKey[SCAN_F4] = SDLK_F4;
		scanCodeToKey[SCAN_F5] = SDLK_F5;
		scanCodeToKey[SCAN_F6] = SDLK_F6;
		scanCodeToKey[SCAN_F7] = SDLK_F7;
		scanCodeToKey[SCAN_F8] = SDLK_F8;
		scanCodeToKey[SCAN_F9] = SDLK_F9;
		scanCodeToKey[SCAN_F10] = SDLK_F10;
		scanCodeToKey[SCAN_NUMLOCK] = SDLK_NUMLOCK;
		scanCodeToKey[SCAN_SCROLL] = SDLK_SCROLLOCK;
		scanCodeToKey[SCAN_NUMPAD7] = SDLK_KP7;
		scanCodeToKey[SCAN_NUMPAD8] = SDLK_KP8;
		scanCodeToKey[SCAN_NUMPAD9] = SDLK_KP9;
		scanCodeToKey[SCAN_SUBTRACT] = SDLK_KP_MINUS;
		scanCodeToKey[SCAN_NUMPAD4] = SDLK_KP4;
		scanCodeToKey[SCAN_NUMPAD5] = SDLK_KP5;
		scanCodeToKey[SCAN_NUMPAD6] = SDLK_KP6;
		scanCodeToKey[SCAN_ADD] = SDLK_KP_PLUS;
		scanCodeToKey[SCAN_NUMPAD1] = SDLK_KP1;
		scanCodeToKey[SCAN_NUMPAD2] = SDLK_KP2;
		scanCodeToKey[SCAN_NUMPAD3] = SDLK_KP3;
		scanCodeToKey[SCAN_NUMPAD0] = SDLK_KP0;
		scanCodeToKey[SCAN_DECIMAL] = SDLK_KP_PERIOD;
		scanCodeToKey[SCAN_F11] = SDLK_F11;
		scanCodeToKey[SCAN_F12] = SDLK_F12;
		scanCodeToKey[SCAN_F13] = SDLK_F13;
		scanCodeToKey[SCAN_F14] = SDLK_F14;
		scanCodeToKey[SCAN_F15] = SDLK_F15;
		scanCodeToKey[SCAN_NUMPADEQUALS] = SDLK_KP_EQUALS;
		scanCodeToKey[SCAN_NUMPADENTER] = SDLK_KP_ENTER;
		scanCodeToKey[SCAN_RCONTROL] = SDLK_RCTRL;
		scanCodeToKey[SCAN_DIVIDE] = SDLK_KP_DIVIDE;
		scanCodeToKey[SCAN_SYSRQ] = SDLK_SYSREQ;
		scanCodeToKey[SCAN_RMENU] = SDLK_RALT;
		scanCodeToKey[SCAN_PAUSE] = SDLK_PAUSE;
		scanCodeToKey[SCAN_HOME] = SDLK_HOME;
		scanCodeToKey[SCAN_UP] = SDLK_UP;
		scanCodeToKey[SCAN_PRIOR] = SDLK_PAGEUP;
		scanCodeToKey[SCAN_LEFT] = SDLK_LEFT;
		scanCodeToKey[SCAN_RIGHT] = SDLK_RIGHT;
		scanCodeToKey[SCAN_END] = SDLK_END;
		scanCodeToKey[SCAN_DOWN] = SDLK_DOWN;
		scanCodeToKey[SCAN_NEXT] = SDLK_PAGEDOWN;
		scanCodeToKey[SCAN_INSERT] = SDLK_INSERT;
		scanCodeToKey[SCAN_DELETE] = SDLK_DELETE;
		scanCodeToKey[SCAN_LWIN] = SDLK_LMETA;
		scanCodeToKey[SCAN_RWIN] = SDLK_RMETA;
		scanCodeToKey[SCAN_APPS] = SDLK_MENU;
		scanCodeToKeyReady = true;
	}

	return scanCodeToKey[scanCode];
}

static int keyToScanCode[SDLK_LAST];
static bool keyToScanCodeReady;

int scanCodeFromKey(SDLKey key)
{
	if (!keyToScanCodeReady) {
		for (int i = 0; i < SDLK_LAST; i++) {
			keyToScanCode[i] = 0;
		}

		keyToScanCode[SDLK_ESCAPE] = SCAN_ESCAPE;
		keyToScanCode[SDLK_1] = SCAN_1;
		keyToScanCode[SDLK_2] = SCAN_2;
		keyToScanCode[SDLK_3] = SCAN_3;
		keyToScanCode[SDLK_4] = SCAN_4;
		keyToScanCode[SDLK_5] = SCAN_5;
		keyToScanCode[SDLK_6] = SCAN_6;
		keyToScanCode[SDLK_7] = SCAN_7;
		keyToScanCode[SDLK_8] = SCAN_8;
		keyToScanCode[SDLK_9] = SCAN_9;
		keyToScanCode[SDLK_0] = SCAN_0;
		keyToScanCode[SDLK_MINUS] = SCAN_MINUS;
		keyToScanCode[SDLK_EQUALS] = SCAN_EQUALS;
		keyToScanCode[SDLK_BACKSPACE] = SCAN_BACK;
		keyToScanCode[SDLK_TAB] = SCAN_TAB;
		keyToScanCode[SDLK_q] = SCAN_Q;
		keyToScanCode[SDLK_w] = SCAN_W;
		keyToScanCode[SDLK_e] = SCAN_E;
		keyToScanCode[SDLK_r] = SCAN_R;
		keyToScanCode[SDLK_t] = SCAN_T;
		keyToScanCode[SDLK_y] = SCAN_Y;
		keyToScanCode[SDLK_u] = SCAN_U;
		keyToScanCode[SDLK_i] = SCAN_I;
		keyToScanCode[SDLK_o] = SCAN_O;
		keyToScanCode[SDLK_p] = SCAN_P;
		keyToScanCode[SDLK_LEFTBRACKET] = SCAN_LBRACKET;
		keyToScanCode[SDLK_RIGHTBRACKET] = SCAN_RBRACKET;
		keyToScanCode[SDLK_RETURN] = SCAN_RETURN;
		keyToScanCode[SDLK_LCTRL] = SCAN_LCONTROL;
		keyToScanCode[SDLK_a] = SCAN_A;
		keyToScanCode[SDLK_s] = SCAN_S;
		keyToScanCode[SDLK_d] = SCAN_D;
		keyToScanCode[SDLK_f] = SCAN_F;
		keyToScanCode[SDLK_g] = SCAN_G;
		keyToScanCode[SDLK_h] = SCAN_H;
		keyToScanCode[SDLK_j] = SCAN_J;
		keyToScanCode[SDLK_k] = SCAN_K;
		keyToScanCode[SDLK_l] = SCAN_L;
		keyToScanCode[SDLK_SEMICOLON] = SCAN_SEMICOLON;
		keyToScanCode[SDLK_QUOTE] = SCAN_APOSTROPHE;
		keyToScanCode[SDLK_BACKQUOTE] = SCAN_GRAVE;
		keyToScanCode[SDLK_LSHIFT] = SCAN_LSHIFT;
		keyToScanCode[SDLK_BACKSLASH] = SCAN_OEM_102;
		keyToScanCode[SDLK_z] = SCAN_Z;
		keyToScanCode[SDLK_x] = SCAN_X;
		keyToScanCode[SDLK_c] = SCAN_C;
		keyToScanCode[SDLK_v] = SCAN_V;
		keyToScanCode[SDLK_b] = SCAN_B;
		keyToScanCode[SDLK_n] = SCAN_N;
		keyToScanCode[SDLK_m] = SCAN_M;
		keyToScanCode[SDLK_COMMA] = SCAN_COMMA;
		keyToScanCode[SDLK_PERIOD] = SCAN_PERIOD;
		keyToScanCode[SDLK_SLASH] = SCAN_SLASH;
		keyToScanCode[SDLK_RSHIFT] = SCAN_RSHIFT;
		keyToScanCode[SDLK_KP_MULTIPLY] = SCAN_MULTIPLY;
		keyToScanCode[SDLK_LALT] = SCAN_LMENU;
		keyToScanCode[SDLK_SPACE] = SCAN_SPACE;
		keyToScanCode[SDLK_CAPSLOCK] = SCAN_CAPITAL;
		keyToScanCode[SDLK_F1] = SCAN_F1;
		keyToScanCode[SDLK_F2] = SCAN_F2;
		keyToScanCode[SDLK_F3] = SCAN_F3;
		keyToScanCode[SDLK_F4] = SCAN_F4;
		keyToScanCode[SDLK_F5] = SCAN_F5;
		keyToScanCode[SDLK_F6] = SCAN_F6;
		keyToScanCode[SDLK_F7] = SCAN_F7;
		keyToScanCode[SDLK_F8] = SCAN_F8;
		keyToScanCode[SDLK_F9] = SCAN_F9;
		keyToScanCode[SDLK_F10] = SCAN_F10;
		keyToScanCode[SDLK_NUMLOCK] = SCAN_NUMLOCK;
		keyToScanCode[SDLK_SCROLLOCK] = SCAN_SCROLL;
		keyToScanCode[SDLK_KP7] = SCAN_NUMPAD7;
		keyToScanCode[SDLK_KP8] = SCAN_NUMPAD8;
		keyToScanCode[SDLK_KP9] = SCAN_NUMPAD9;
		keyToScanCode[SDLK_KP_MINUS] = SCAN_SUBTRACT;
		keyToScanCode[SDLK_KP4] = SCAN_NUMPAD4;
		keyToScanCode[SDLK_KP5] = SCAN_NUMPAD5;
		keyToScanCode[SDLK_KP6] = SCAN_NUMPAD6;
		keyToScanCode[SDLK_KP_PLUS] = SCAN_ADD;
		keyToScanCode[SDLK_KP1] = SCAN_NUMPAD1;
		keyToScanCode[SDLK_KP2] = SCAN_NUMPAD2;
		keyToScanCode[SDLK_KP3] = SCAN_NUMPAD3;
		keyToScanCode[SDLK_KP0] = SCAN_NUMPAD0;
		keyToScanCode[SDLK_KP_PERIOD] = SCAN_DECIMAL;
		keyToScanCode[SDLK_F11] = SCAN_F11;
		keyToScanCode[SDLK_F12] = SCAN_F12;
		keyToScanCode[SDLK_F13] = SCAN_F13;
		keyToScanCode[SDLK_F14] = SCAN_F14;
		keyToScanCode[SDLK_F15] = SCAN_F15;
		keyToScanCode[SDLK_KP_EQUALS] = SCAN_NUMPADEQUALS;
		keyToScanCode[SDLK_KP_ENTER] = SCAN_NUMPADENTER;
		keyToScanCode[SDLK_RCTRL] = SCAN_RCONTROL;
		keyToScanCode[SDLK_KP_DIVIDE] = SCAN_DIVIDE;
		keyToScanCode[SDLK_SYSREQ] = SCAN_SYSRQ;
		keyToScanCode[SDLK_RALT] = SCAN_RMENU;
		keyToScanCode[SDLK_PAUSE] = SCAN_PAUSE;
		keyToScanCode[SDLK_HOME] = SCAN_HOME;
		keyToScanCode[SDLK_UP] = SCAN_UP;
		keyToScanCode[SDLK_PAGEUP] = SCAN_PRIOR;
		keyToScanCode[SDLK_LEFT] = SCAN_LEFT;
		keyToScanCode[SDLK_RIGHT] = SCAN_RIGHT;
		keyToScanCode[SDLK_END] = SCAN_END;
		keyToScanCode[SDLK_DOWN] = SCAN_DOWN;
		keyToScanCode[SDLK_PAGEDOWN] = SCAN_NEXT;
		keyToScanCode[SDLK_INSERT] = SCAN_INSERT;
		keyToScanCode[SDLK_DELETE] = SCAN_DELETE;
		keyToScanCode[SDLK_LMETA] = SCAN_LWIN;
		keyToScanCode[SDLK_RMETA] = SCAN_RWIN;
		keyToScanCode[SDLK_MENU] = SCAN_APPS;

		keyToScanCodeReady = true;
	}

	return keyToScanCode[key];
}

static int keyToVirtualKey[SDLK_LAST];
static bool keyToVirtualKeyReady;

int virtualKeyFromKey(SDLKey key)
{
	if (!keyToVirtualKeyReady) {
		for (int i = 0; i < SDLK_LAST; i++) {
			keyToVirtualKey[i] = 0;
		}

		keyToVirtualKey[SDLK_RETURN] = VKEY_RETURN;
		keyToVirtualKey[SDLK_KP_ENTER] = VKEY_RETURN;
		keyToVirtualKey[SDLK_PRINT] = VKEY_PRINT;
		keyToVirtualKey[SDLK_SYSREQ] = VKEY_SNAPSHOT;
		keyToVirtualKey[SDLK_ESCAPE] = VKEY_ESCAPE;
		keyToVirtualKey[SDLK_BACKSPACE] = VKEY_BACK;
		keyToVirtualKey[SDLK_TAB] = VKEY_TAB;
		keyToVirtualKey[SDLK_LCTRL] = VKEY_LCONTROL;
		keyToVirtualKey[SDLK_LSHIFT] = VKEY_LSHIFT;
		keyToVirtualKey[SDLK_BACKSLASH] = VKEY_OEM_102;
		keyToVirtualKey[SDLK_RSHIFT] = VKEY_RSHIFT;
		keyToVirtualKey[SDLK_KP_MULTIPLY] = VKEY_MULTIPLY;
		keyToVirtualKey[SDLK_LALT] = VKEY_LMENU;
		keyToVirtualKey[SDLK_SPACE] = VKEY_SPACE;
		keyToVirtualKey[SDLK_CAPSLOCK] = VKEY_CAPITAL;
		keyToVirtualKey[SDLK_F1] = VKEY_F1;
		keyToVirtualKey[SDLK_F2] = VKEY_F2;
		keyToVirtualKey[SDLK_F3] = VKEY_F3;
		keyToVirtualKey[SDLK_F4] = VKEY_F4;
		keyToVirtualKey[SDLK_F5] = VKEY_F5;
		keyToVirtualKey[SDLK_F6] = VKEY_F6;
		keyToVirtualKey[SDLK_F7] = VKEY_F7;
		keyToVirtualKey[SDLK_F8] = VKEY_F8;
		keyToVirtualKey[SDLK_F9] = VKEY_F9;
		keyToVirtualKey[SDLK_F10] = VKEY_F10;
		keyToVirtualKey[SDLK_NUMLOCK] = VKEY_NUMLOCK;
		keyToVirtualKey[SDLK_SCROLLOCK] = VKEY_SCROLL;
		keyToVirtualKey[SDLK_KP7] = VKEY_NUMPAD7;
		keyToVirtualKey[SDLK_KP8] = VKEY_NUMPAD8;
		keyToVirtualKey[SDLK_KP9] = VKEY_NUMPAD9;
		keyToVirtualKey[SDLK_KP_MINUS] = VKEY_SUBTRACT;
		keyToVirtualKey[SDLK_KP4] = VKEY_NUMPAD4;
		keyToVirtualKey[SDLK_KP5] = VKEY_NUMPAD5;
		keyToVirtualKey[SDLK_KP6] = VKEY_NUMPAD6;
		keyToVirtualKey[SDLK_KP_PLUS] = VKEY_ADD;
		keyToVirtualKey[SDLK_KP1] = VKEY_NUMPAD1;
		keyToVirtualKey[SDLK_KP2] = VKEY_NUMPAD2;
		keyToVirtualKey[SDLK_KP3] = VKEY_NUMPAD3;
		keyToVirtualKey[SDLK_KP0] = VKEY_NUMPAD0;
		keyToVirtualKey[SDLK_KP_PERIOD] = VKEY_DECIMAL;
		keyToVirtualKey[SDLK_F11] = VKEY_F11;
		keyToVirtualKey[SDLK_F12] = VKEY_F12;
		keyToVirtualKey[SDLK_F13] = VKEY_F13;
		keyToVirtualKey[SDLK_F14] = VKEY_F14;
		keyToVirtualKey[SDLK_F15] = VKEY_F15;
		keyToVirtualKey[SDLK_RCTRL] = VKEY_RCONTROL;
		keyToVirtualKey[SDLK_KP_DIVIDE] = VKEY_DIVIDE;
		keyToVirtualKey[SDLK_RALT] = VKEY_RMENU;
		keyToVirtualKey[SDLK_HOME] = VKEY_HOME;
		keyToVirtualKey[SDLK_UP] = VKEY_UP;
		keyToVirtualKey[SDLK_PAGEUP] = VKEY_PRIOR;
		keyToVirtualKey[SDLK_LEFT] = VKEY_LEFT;
		keyToVirtualKey[SDLK_RIGHT] = VKEY_RIGHT;
		keyToVirtualKey[SDLK_END] = VKEY_END;
		keyToVirtualKey[SDLK_DOWN] = VKEY_DOWN;
		keyToVirtualKey[SDLK_PAGEDOWN] = VKEY_NEXT;
		keyToVirtualKey[SDLK_INSERT] = VKEY_INSERT;
		keyToVirtualKey[SDLK_DELETE] = VKEY_DELETE;
		keyToVirtualKey[SDLK_LMETA] = VKEY_LWIN;
		keyToVirtualKey[SDLK_RMETA] = VKEY_RWIN;
		keyToVirtualKey[SDLK_MENU] = VKEY_APPS;
		keyToVirtualKeyReady = true;
	}

	return keyToVirtualKey[key];
}

#ifdef _WIN32
RBX::UIEvent::EventType mouseEventType(unsigned message)
{
	switch (message) {
	case WM_MOUSEMOVE:
		return RBX::UIEvent::MOUSE_MOVE;
	case WM_LBUTTONDOWN:
		return RBX::UIEvent::MOUSE_LEFT_BUTTON_DOWN;
	case WM_RBUTTONDOWN:
		return RBX::UIEvent::MOUSE_RIGHT_BUTTON_DOWN;
	case WM_LBUTTONUP:
		return RBX::UIEvent::MOUSE_LEFT_BUTTON_UP;
	case WM_RBUTTONUP:
		return RBX::UIEvent::MOUSE_RIGHT_BUTTON_UP;
	default:
		return RBX::UIEvent::NO_EVENT;
	}
}

#endif

SDLMod modifiersFromKeyState(const unsigned char* keys)
{
	int mod = KMOD_NONE;
	if (keys[SCAN_LSHIFT] & 0x80) {
		mod = KMOD_LSHIFT;
	}
	if (keys[SCAN_RSHIFT] & 0x80) {
		mod |= KMOD_RSHIFT;
	}
	if (keys[SCAN_LCONTROL] & 0x80) {
		mod |= KMOD_LCTRL;
	}
	if (keys[SCAN_RCONTROL] & 0x80) {
		mod |= KMOD_RCTRL;
	}
	if (keys[SCAN_LMENU] & 0x80) {
		mod |= KMOD_LALT;
	}
	if (keys[SCAN_LMENU] & 0x80) {
		mod |= KMOD_RALT;
	}
	return static_cast<SDLMod>(mod);
}

void wrapCursorAtBorder(
	const G3D::Vector2& mouseDelta,
	G3D::Vector2& wrapMouseDelta,
	G3D::Vector2& position,
	const G3D::Vector2& size,
	int border,
	float stickiness
)
{
	G3D::Vector2 half(size.x * 0.5, size.y * 0.5);
	RBX::Rect bounds = RBX::Rect(-half, half).inset(border);

	G3D::Vector2 oldPosition = position;
	bounds.unionWith(RBX::Rect(oldPosition, oldPosition));

	G3D::Vector2 newPosition = mouseDelta + oldPosition;
	G3D::Vector2 clamped = newPosition.clamp(bounds.low, bounds.high);
	G3D::Vector2 overflow = newPosition - clamped;

	position = clamped + overflow * stickiness;
	wrapMouseDelta += overflow;
}

void wrapCursorAtEdges(
	const G3D::Vector2& mouseDelta, G3D::Vector2& wrapMouseDelta, G3D::Vector2& position, const G3D::Vector2& size
)
{
	wrapCursorAtBorder(mouseDelta, wrapMouseDelta, position, size, 6, 0.0f);
}

void wrapCursorHorizontalClampTop(
	const G3D::Vector2& mouseDelta, G3D::Vector2& wrapMouseDelta, G3D::Vector2& position, const G3D::Vector2& size
)
{
	float y = mouseDelta.y + position.y;
	wrapCursorAtBorder(mouseDelta, wrapMouseDelta, position, size, 6, 0.0f);
	wrapMouseDelta.y = 0.0f;
	position.y = y;

	float inset = size.y * 0.5 - 2.0;
	float top = -inset;
	position.y = std::max(top, position.y);
}

void wrapCursorHorizontalSticky(
	const G3D::Vector2& mouseDelta, G3D::Vector2& wrapMouseDelta, G3D::Vector2& position, const G3D::Vector2& size
)
{
	float y = mouseDelta.y + position.y;
	wrapCursorAtBorder(mouseDelta, wrapMouseDelta, position, size, 20, 0.05f);
	wrapMouseDelta.y = 0.0f;
	position.y = y;
}
