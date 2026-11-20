#ifndef NATIVEINPUT_H
#define NATIVEINPUT_H

#include "util/KeyCode.h"

#include <QPoint>
#include <QSize>
#include <qwindowdefs.h>

namespace NativeInput {

#ifdef _WIN32
extern const unsigned focusViewportMessage;
#endif
WId rootWindow(WId window);
QSize clientSize(WId window);
QPoint clientToScreen(WId window, QPoint point);
QPoint mapToWindow(WId from, WId to, QPoint point);
bool isForeground(WId frame);
bool isLastActivePopupForeground(WId frame);
QPoint cursorPosition();
void setCursorPosition(QPoint point);
void hideCursor(WId window);
void showArrowCursor(WId window);
void invalidate(WId window);
bool isPointVisible(WId window, QPoint point);
void postCommand(WId frame, unsigned id);
void postFocus(WId window);
unsigned virtualKey(unsigned scanCode, SDLKey key);
#ifndef _WIN32
void forwardKey(WId window, bool press, unsigned keyCode, unsigned state, unsigned long time, QPoint root);
#endif

} // namespace NativeInput

#endif // NATIVEINPUT_H
