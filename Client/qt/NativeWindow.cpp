#include "NativeWindow.h"

#include <QWidget>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

void removeWindowIcon(QWidget* window)
{
#ifdef _WIN32
	HWND handle = reinterpret_cast<HWND>(window->winId());
	SetWindowLongPtrW(handle, GWL_EXSTYLE, GetWindowLongPtrW(handle, GWL_EXSTYLE) | WS_EX_DLGMODALFRAME);
	SetWindowPos(
		handle, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED
	);
	SetClassLongPtrW(handle, GCLP_HICON, 0);
	SetClassLongPtrW(handle, GCLP_HICONSM, 0);
	SendMessageW(handle, WM_SETICON, ICON_SMALL, 0);
	SendMessageW(handle, WM_SETICON, ICON_BIG, 0);
#endif
}

QRect windowFrameRect(const QWidget* window)
{
#ifdef _WIN32
	RECT rect;
	if (GetWindowRect(reinterpret_cast<HWND>(window->winId()), &rect)) {
		return QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
	}
#endif
	return window->frameGeometry();
}

#ifdef _WIN32
void centerWindowOverOwner(HWND window)
{
	HWND center = GetWindow(window, GW_OWNER);
	if (center) {
		LONG_PTR style = GetWindowLongPtrW(center, GWL_STYLE);
		if (!(style & WS_VISIBLE) || (style & WS_MINIMIZE)) {
			center = nullptr;
		}
	}

	RECT rect;
	GetWindowRect(window, &rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	MONITORINFO monitor;
	monitor.cbSize = sizeof(monitor);
	RECT area;
	RECT target;
	if (center) {
		GetWindowRect(center, &target);
		GetMonitorInfoW(MonitorFromWindow(center, MONITOR_DEFAULTTONEAREST), &monitor);
		area = monitor.rcWork;
	}
	else {
		GetMonitorInfoW(MonitorFromWindow(nullptr, MONITOR_DEFAULTTOPRIMARY), &monitor);
		target = monitor.rcWork;
		area = monitor.rcWork;
	}

	int left = (target.left + target.right) / 2 - width / 2;
	int top = (target.top + target.bottom) / 2 - height / 2;
	if (left + width > area.right) {
		left = area.right - width;
	}
	if (left < area.left) {
		left = area.left;
	}
	if (top + height > area.bottom) {
		top = area.bottom - height;
	}
	if (top < area.top) {
		top = area.top;
	}
	SetWindowPos(window, nullptr, left, top, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}
#endif

void centerWindowOverOwner(QWidget* window)
{
#ifdef _WIN32
	centerWindowOverOwner(reinterpret_cast<HWND>(window->winId()));
#endif
}

void moveWindowFrame(QWidget* window, const QPoint& position)
{
#ifdef _WIN32
	SetWindowPos(
		reinterpret_cast<HWND>(window->winId()),
		nullptr,
		position.x(),
		position.y(),
		-1,
		-1,
		SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
	);
#else
	window->move(position);
#endif
}

void applyDialogTemplateStyle(QWidget* window, unsigned style, unsigned exStyle)
{
#ifdef _WIN32
	HWND handle = reinterpret_cast<HWND>(window->winId());
	LONG_PTR current = GetWindowLongPtrW(handle, GWL_EXSTYLE);
	LONG_PTR wanted = current | (exStyle & WS_EX_TOOLWINDOW);
	if (style & DS_CONTEXTHELP) {
		wanted |= WS_EX_CONTEXTHELP;
	}
	if (wanted != current) {
		SetWindowLongPtrW(handle, GWL_EXSTYLE, wanted);
		SetWindowPos(
			handle, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED
		);
	}
#endif
}
