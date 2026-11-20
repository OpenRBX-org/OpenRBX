#include "SystemColorDialog.h"

#include <QWidget>

#ifdef _WIN32
#include "NativeWindow.h"

#define NOMINMAX
#include <windows.h>
#else
#include <QColorDialog>
#endif

namespace SystemColorDialog {

#ifdef _WIN32

static UINT_PTR CALLBACK colorDialogHook(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message != WM_INITDIALOG) {
		return FALSE;
	}
	centerWindowOverOwner(window);
	return TRUE;
}

std::optional<QColor> getColor(QColor initial, QWidget* parent)
{
	static COLORREF customColors[16] = {
		RGB(255, 255, 255),
		RGB(255, 255, 255),
		RGB(255, 255, 255),
		RGB(255, 255, 255),
		RGB(255, 255, 255),
		RGB(255, 255, 255),
		RGB(255, 255, 255),
		RGB(255, 255, 255),
		RGB(255, 255, 255),
		RGB(255, 255, 255),
		RGB(255, 255, 255),
		RGB(255, 255, 255),
		RGB(255, 255, 255),
		RGB(255, 255, 255),
		RGB(255, 255, 255),
		RGB(255, 255, 255),
	};
	CHOOSECOLORW chooser = {};
	chooser.lStructSize = sizeof(chooser);
	chooser.hwndOwner = parent ? reinterpret_cast<HWND>(parent->window()->winId()) : nullptr;
	chooser.lpCustColors = customColors;
	chooser.Flags = CC_ENABLEHOOK;
	chooser.lpfnHook = colorDialogHook;
	chooser.rgbResult = RGB(initial.red(), initial.green(), initial.blue());
	if (chooser.rgbResult != 0) {
		chooser.Flags |= CC_RGBINIT;
	}
	if (!ChooseColorW(&chooser)) {
		return std::nullopt;
	}
	return QColor(GetRValue(chooser.rgbResult), GetGValue(chooser.rgbResult), GetBValue(chooser.rgbResult));
}

#else

std::optional<QColor> getColor(QColor initial, QWidget* parent)
{
	QColor color = QColorDialog::getColor(initial, parent);
	if (!color.isValid()) {
		return std::nullopt;
	}
	return color;
}

#endif

} // namespace SystemColorDialog
