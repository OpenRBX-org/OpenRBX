#ifndef INPUTDEVICE_H
#define INPUTDEVICE_H

struct InputEvent
{
	enum Type
	{
		MouseMotion,
		MouseButton,
		MouseWheel,
		Key,
	};

	enum Button
	{
		LeftButton,
		RightButton,
		OtherButton,
	};

	Type type;
	float x;
	float y;
	Button button;
	bool down;
	unsigned scanCode;
};

namespace InputDevice {

constexpr int eventBufferSize = 0x800;

void open(void* frame);
void close();
void pump();

bool captureMouse(int x, int y);
void releaseMouse();
int takeMouseEvents(InputEvent* events, int capacity);
int peekMouseMotion(InputEvent* events, int capacity);
void flushMouseEvents();

int takeKeyEvents(InputEvent* events, int capacity);
void flushKeyEvents();
void readKeyboardState(unsigned char* keys);

void flushOtherEvents();

#ifndef _WIN32
void postMouseButton(InputEvent::Button button, bool down);
void postMouseWheel(float y);
#endif

} // namespace InputDevice

#endif // INPUTDEVICE_H
