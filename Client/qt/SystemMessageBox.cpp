#include "SystemMessageBox.h"

#include <algorithm>

#ifdef _WIN32
#include "MainFrm.h"
#include "NativeWindow.h"
#include "Roblox.h"

#include <QWidget>
#define NOMINMAX
#include <windows.h>
#endif

namespace SystemMessageBox {

#ifdef _WIN32

static HHOOK initDialogHook;

static LRESULT CALLBACK centerOnInitDialog(int code, WPARAM wParam, LPARAM lParam)
{
	if (code == HC_ACTION) {
		const CWPRETSTRUCT* message = reinterpret_cast<const CWPRETSTRUCT*>(lParam);
		if (message->message == WM_INITDIALOG) {
			centerWindowOverOwner(message->hwnd);
		}
	}
	return CallNextHookEx(initDialogHook, code, wParam, lParam);
}

static QMessageBox::StandardButton show(
	QWidget* parent,
	const QString& title,
	const QString& text,
	QMessageBox::StandardButtons buttons,
	UINT icon,
	QMessageBox::StandardButton defaultButton = QMessageBox::NoButton
)
{
	UINT type = MB_OK;
	if (buttons == (QMessageBox::Ok | QMessageBox::Cancel)) {
		type = MB_OKCANCEL;
	}
	else if (buttons == (QMessageBox::Yes | QMessageBox::No)) {
		type = MB_YESNO;
	}
	else if (buttons == (QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel)) {
		type = MB_YESNOCANCEL;
	}
	else if (buttons == (QMessageBox::Retry | QMessageBox::Cancel)) {
		type = MB_RETRYCANCEL;
	}

	if (defaultButton != QMessageBox::NoButton) {
		int position = 0;
		for (QMessageBox::StandardButton button :
			 {QMessageBox::Ok, QMessageBox::Yes, QMessageBox::No, QMessageBox::Retry, QMessageBox::Cancel}) {
			if (buttons & button) {
				if (button == defaultButton) {
					break;
				}
				position++;
			}
		}
		static const UINT defaultFlags[] = {MB_DEFBUTTON1, MB_DEFBUTTON2, MB_DEFBUTTON3};
		type |= defaultFlags[std::min(position, 2)];
	}

	QWidget* ownerWidget = parent ? parent->window() : theApp.mainFrame();
	HWND owner = ownerWidget ? reinterpret_cast<HWND>(ownerWidget->winId()) : nullptr;
	if (owner) {
		owner = GetLastActivePopup(owner);
	}
	HHOOK previous = initDialogHook;
	initDialogHook = SetWindowsHookExW(WH_CALLWNDPROCRET, centerOnInitDialog, nullptr, GetCurrentThreadId());
	int result = MessageBoxW(
		owner,
		reinterpret_cast<const wchar_t*>(text.utf16()),
		reinterpret_cast<const wchar_t*>(title.utf16()),
		type | icon
	);
	UnhookWindowsHookEx(initDialogHook);
	initDialogHook = previous;

	switch (result) {
	case IDOK:
		return QMessageBox::Ok;
	case IDYES:
		return QMessageBox::Yes;
	case IDNO:
		return QMessageBox::No;
	case IDRETRY:
		return QMessageBox::Retry;
	default:
		return QMessageBox::Cancel;
	}
}

QMessageBox::StandardButton information(
	QWidget* parent, const QString& title, const QString& text, QMessageBox::StandardButtons buttons
)
{
	return show(parent, title, text, buttons, MB_ICONINFORMATION);
}

QMessageBox::StandardButton question(
	QWidget* parent, const QString& title, const QString& text, QMessageBox::StandardButtons buttons
)
{
	return show(parent, title, text, buttons, MB_ICONQUESTION);
}

QMessageBox::StandardButton warning(
	QWidget* parent,
	const QString& title,
	const QString& text,
	QMessageBox::StandardButtons buttons,
	QMessageBox::StandardButton defaultButton
)
{
	return show(parent, title, text, buttons, MB_ICONWARNING, defaultButton);
}

QMessageBox::StandardButton critical(
	QWidget* parent, const QString& title, const QString& text, QMessageBox::StandardButtons buttons
)
{
	return show(parent, title, text, buttons, MB_ICONERROR);
}

#else

QMessageBox::StandardButton information(
	QWidget* parent, const QString& title, const QString& text, QMessageBox::StandardButtons buttons
)
{
	return QMessageBox::information(parent, title, text, buttons);
}

QMessageBox::StandardButton question(
	QWidget* parent, const QString& title, const QString& text, QMessageBox::StandardButtons buttons
)
{
	return QMessageBox::question(parent, title, text, buttons);
}

QMessageBox::StandardButton warning(
	QWidget* parent,
	const QString& title,
	const QString& text,
	QMessageBox::StandardButtons buttons,
	QMessageBox::StandardButton defaultButton
)
{
	return QMessageBox::warning(parent, title, text, buttons, defaultButton);
}

QMessageBox::StandardButton critical(
	QWidget* parent, const QString& title, const QString& text, QMessageBox::StandardButtons buttons
)
{
	return QMessageBox::critical(parent, title, text, buttons);
}

#endif

} // namespace SystemMessageBox
