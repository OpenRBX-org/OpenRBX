#include "NativeInput.h"

#include "MainFrm.h"
#include "Roblox.h"
#include "UserInputUtil.h"

#include <QCoreApplication>
#include <QWidget>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <mutex>
#endif

namespace NativeInput {

#ifdef _WIN32

const unsigned focusViewportMessage = WM_USER + 0xbb;

static HWND handle(WId window)
{
	return reinterpret_cast<HWND>(window);
}

WId rootWindow(WId window)
{
	return reinterpret_cast<WId>(GetAncestor(handle(window), GA_ROOT));
}

QSize clientSize(WId window)
{
	RECT rect;
	GetClientRect(handle(window), &rect);
	return QSize(rect.right - rect.left, rect.bottom - rect.top);
}

QPoint clientToScreen(WId window, QPoint point)
{
	POINT screen = {point.x(), point.y()};
	ClientToScreen(handle(window), &screen);
	return QPoint(screen.x, screen.y);
}

QPoint mapToWindow(WId from, WId to, QPoint point)
{
	POINT mapped = {point.x(), point.y()};
	MapWindowPoints(handle(from), handle(to), &mapped, 1);
	return QPoint(mapped.x, mapped.y);
}

bool isForeground(WId frame)
{
	return GetForegroundWindow() == handle(frame);
}

bool isLastActivePopupForeground(WId frame)
{
	return GetLastActivePopup(handle(frame)) == GetForegroundWindow();
}

QPoint cursorPosition()
{
	POINT point;
	GetCursorPos(&point);
	return QPoint(point.x, point.y);
}

void setCursorPosition(QPoint point)
{
	SetCursorPos(point.x(), point.y());
}

void hideCursor(WId window)
{
	SetCursor(nullptr);
}

void showArrowCursor(WId window)
{
	static HCURSOR cursor = LoadCursor(nullptr, IDC_ARROW);
	SetCursor(cursor);
}

void invalidate(WId window)
{
	InvalidateRect(handle(window), nullptr, TRUE);
}

bool isPointVisible(WId window, QPoint point)
{
	HDC dc = GetDC(handle(window));
	bool visible = PtVisible(dc, point.x(), point.y()) != FALSE;
	ReleaseDC(handle(window), dc);
	return visible;
}

void postCommand(WId frame, unsigned id)
{
	PostMessage(handle(frame), WM_COMMAND, MAKEWPARAM(id, 1), 0);
}

void postFocus(WId window)
{
	PostMessage(handle(window), focusViewportMessage, 0, 0);
}

unsigned virtualKey(unsigned scanCode, SDLKey key)
{
	static HKL layout = GetKeyboardLayout(0);
	UINT code = MapVirtualKeyEx(scanCode, 1, layout);
	if (code == 0) {
		code = virtualKeyFromKey(key);
	}
	return code;
}

#else

static std::recursive_mutex displayMutex;

static Display* display()
{
	static Display* connection = XOpenDisplay(nullptr);
	return connection;
}

static Window rootOf(Display* connection)
{
	return DefaultRootWindow(connection);
}

static Window windowProperty(Display* connection, Window window, const char* name)
{
	Atom property = XInternAtom(connection, name, True);
	if (property == None) {
		return None;
	}

	Atom type;
	int format;
	unsigned long count;
	unsigned long remaining;
	unsigned char* data = nullptr;
	Window result = None;
	if (XGetWindowProperty(
			connection, window, property, 0, 1, False, XA_WINDOW, &type, &format, &count, &remaining, &data
		) == Success &&
		data) {
		if (type == XA_WINDOW && format == 32 && count == 1) {
			result = *reinterpret_cast<Window*>(data);
		}
		XFree(data);
	}
	return result;
}

static Window topLevel(Display* connection, Window window)
{
	Window root = rootOf(connection);
	while (window != None && window != root) {
		Window parent;
		Window* children = nullptr;
		unsigned int count;
		if (!XQueryTree(connection, window, &root, &parent, &children, &count)) {
			return None;
		}
		if (children) {
			XFree(children);
		}
		if (parent == root) {
			return window;
		}
		window = parent;
	}
	return None;
}

static Window activeWindow(Display* connection)
{
	Window active = windowProperty(connection, rootOf(connection), "_NET_ACTIVE_WINDOW");
	if (active != None) {
		return active;
	}

	int revert;
	XGetInputFocus(connection, &active, &revert);
	if (active == PointerRoot) {
		Window root;
		int rootX;
		int rootY;
		int windowX;
		int windowY;
		unsigned int mask;
		XQueryPointer(connection, rootOf(connection), &root, &active, &rootX, &rootY, &windowX, &windowY, &mask);
	}
	return topLevel(connection, active);
}

WId rootWindow(WId window)
{
	QWidget* widget = QWidget::find(window);
	return widget ? widget->window()->winId() : window;
}

QSize clientSize(WId window)
{
	std::scoped_lock lock(displayMutex);
	XWindowAttributes attributes;
	if (!display() || !XGetWindowAttributes(display(), window, &attributes)) {
		return QSize();
	}
	return QSize(attributes.width, attributes.height);
}

QPoint clientToScreen(WId window, QPoint point)
{
	std::scoped_lock lock(displayMutex);
	int x = point.x();
	int y = point.y();
	Window child;
	if (display()) {
		XTranslateCoordinates(display(), window, rootOf(display()), point.x(), point.y(), &x, &y, &child);
	}
	return QPoint(x, y);
}

QPoint mapToWindow(WId from, WId to, QPoint point)
{
	std::scoped_lock lock(displayMutex);
	int x = point.x();
	int y = point.y();
	Window child;
	if (display()) {
		XTranslateCoordinates(display(), from, to, point.x(), point.y(), &x, &y, &child);
	}
	return QPoint(x, y);
}

bool isForeground(WId frame)
{
	std::scoped_lock lock(displayMutex);
	return display() && activeWindow(display()) == frame;
}

bool isLastActivePopupForeground(WId frame)
{
	std::scoped_lock lock(displayMutex);
	if (!display()) {
		return false;
	}
	Window active = activeWindow(display());
	if (active == frame) {
		return true;
	}
	Window owner = None;
	return active != None && XGetTransientForHint(display(), active, &owner) && owner == frame;
}

QPoint cursorPosition()
{
	std::scoped_lock lock(displayMutex);
	Window root;
	Window child;
	int rootX = 0;
	int rootY = 0;
	int windowX;
	int windowY;
	unsigned int mask;
	if (display()) {
		XQueryPointer(display(), rootOf(display()), &root, &child, &rootX, &rootY, &windowX, &windowY, &mask);
	}
	return QPoint(rootX, rootY);
}

void setCursorPosition(QPoint point)
{
	std::scoped_lock lock(displayMutex);
	if (display()) {
		XWarpPointer(display(), None, rootOf(display()), 0, 0, 0, 0, point.x(), point.y());
		XFlush(display());
	}
}

void hideCursor(WId window)
{
	std::scoped_lock lock(displayMutex);
	if (!display()) {
		return;
	}
	static Cursor blank = None;
	if (blank == None) {
		static const char empty[1] = {0};
		Pixmap pixmap = XCreateBitmapFromData(display(), rootOf(display()), empty, 1, 1);
		XColor black = {};
		blank = XCreatePixmapCursor(display(), pixmap, pixmap, &black, &black, 0, 0);
		XFreePixmap(display(), pixmap);
	}
	XDefineCursor(display(), window, blank);
	XFlush(display());
}

void showArrowCursor(WId window)
{
	std::scoped_lock lock(displayMutex);
	if (display()) {
		XUndefineCursor(display(), window);
		XFlush(display());
	}
}

void invalidate(WId window)
{
	QMetaObject::invokeMethod(
		QCoreApplication::instance(),
		[window]() {
			if (QWidget* widget = QWidget::find(window)) {
				widget->update();
			}
		},
		Qt::QueuedConnection
	);
}

bool isPointVisible(WId window, QPoint point)
{
	std::scoped_lock lock(displayMutex);
	XWindowAttributes attributes;
	if (!display() || !XGetWindowAttributes(display(), window, &attributes)) {
		return false;
	}
	return attributes.map_state == IsViewable && point.x() >= 0 && point.y() >= 0 && point.x() < attributes.width &&
		   point.y() < attributes.height;
}

void postCommand(WId frame, unsigned id)
{
	QMetaObject::invokeMethod(
		theApp.mainFrame(), [id]() { theApp.mainFrame()->sendCommand(id); }, Qt::QueuedConnection
	);
}

void postFocus(WId window)
{
	QMetaObject::invokeMethod(
		QCoreApplication::instance(),
		[window]() {
			if (QWidget* widget = QWidget::find(window)) {
				widget->setFocus();
			}
		},
		Qt::QueuedConnection
	);
}

unsigned virtualKey(unsigned scanCode, SDLKey key)
{
	if (key >= SDLK_a && key <= SDLK_z) {
		return 'A' + (key - SDLK_a);
	}
	if (key >= SDLK_0 && key <= SDLK_9) {
		return '0' + (key - SDLK_0);
	}
	return virtualKeyFromKey(key);
}

void forwardKey(WId window, bool press, unsigned keyCode, unsigned state, unsigned long time, QPoint root)
{
	std::scoped_lock lock(displayMutex);
	if (!display()) {
		return;
	}

	XEvent event = {};
	event.xkey.type = press ? KeyPress : KeyRelease;
	event.xkey.display = display();
	event.xkey.window = window;
	event.xkey.root = rootOf(display());
	event.xkey.time = time;
	event.xkey.x_root = root.x();
	event.xkey.y_root = root.y();
	event.xkey.state = state;
	event.xkey.keycode = keyCode;
	event.xkey.same_screen = True;
	XSendEvent(display(), window, False, press ? KeyPressMask : KeyReleaseMask, &event);
	XFlush(display());
}

#endif

} // namespace NativeInput
