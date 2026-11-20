#include "DisplayMode.h"

#include "util/standardout.h"

#include <QWidget>

#ifdef _WIN32
#include <cmath>
#include <cstdlib>
#include <windows.h>

static bool monitorDevice(QWidget* window, MONITORINFOEXA& info)
{
	HMONITOR monitor = MonitorFromWindow(reinterpret_cast<HWND>(window->winId()), MONITOR_DEFAULTTONEAREST);
	if (monitor == nullptr) {
		RBX::StandardOut::singleton()->print(
			RBX::MESSAGE_WARNING, "CMainFrameFrameWnd::getMonitorInfo MonitorFromWindow returned NULL"
		);
		return false;
	}

	info.cbSize = sizeof(info);
	return GetMonitorInfoA(monitor, &info) != FALSE;
}

static DEVMODEA closestMode(LPCSTR device, int pixels)
{
	DEVMODEA mode;
	memset(&mode, 0, sizeof(mode));
	mode.dmSize = sizeof(mode);

	if (!EnumDisplaySettingsExA(device, ENUM_CURRENT_SETTINGS, &mode, 0)) {
		RBX::StandardOut::singleton()->print(
			RBX::MESSAGE_ERROR, "EnumDisplaySettings failed. GetLastError() == %d", GetLastError()
		);
	}

	double aspect = (double) mode.dmPelsWidth / (double) mode.dmPelsHeight;
	DEVMODEA best = mode;

	DWORD i = 0;
	while (EnumDisplaySettingsExA(device, i++, &mode, 0)) {
		double ratio = (double) mode.dmPelsWidth / (double) mode.dmPelsHeight;
		if (std::fabs(aspect - ratio) / aspect <= 0.1 && best.dmBitsPerPel == mode.dmBitsPerPel &&
			best.dmDisplayFrequency == mode.dmDisplayFrequency &&
			std::abs(pixels - (int) (mode.dmPelsWidth * mode.dmPelsHeight)) <
				std::abs(pixels - (int) (best.dmPelsWidth * best.dmPelsHeight))) {
			best = mode;
		}
	}

	best.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
	return best;
}

bool changeDisplayMode(QWidget* window, QSize size)
{
	MONITORINFOEXA info;
	if (!monitorDevice(window, info)) {
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "GetMonitorInfo failed");
		return false;
	}

	DEVMODEA mode = closestMode(info.szDevice, size.width() * size.height());
	LONG result = ChangeDisplaySettingsExA(info.szDevice, &mode, nullptr, CDS_FULLSCREEN, nullptr);
	if (result != DISP_CHANGE_SUCCESSFUL) {
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "ChangeDisplaySettings == %d", result);
		return false;
	}
	return true;
}

void restoreDisplayMode(QWidget* window)
{
	MONITORINFOEXA info;
	if (!monitorDevice(window, info)) {
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "CMainFrame::RestoreResolution hm is NULL");
		return;
	}

	LONG result = ChangeDisplaySettingsExA(info.szDevice, nullptr, nullptr, 0, nullptr);
	if (result != DISP_CHANGE_SUCCESSFUL) {
		RBX::StandardOut::singleton()->print(
			RBX::MESSAGE_ERROR, "CMainFrame::RestoreResolution ChangeDisplaySettings == %d", result
		);
	}
}
#else
#include <QGuiApplication>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <cmath>
#include <cstdlib>
#include <vector>

struct SavedMode
{
	RRCrtc crtc = None;
	RRMode mode = None;
	int x = 0;
	int y = 0;
	Rotation rotation = RR_Rotate_0;
	std::vector<RROutput> outputs;
};

static SavedMode savedMode;

static Display* display()
{
	QNativeInterface::QX11Application* application = qApp->nativeInterface<QNativeInterface::QX11Application>();
	return application ? application->display() : nullptr;
}

static const XRRModeInfo* modeInfo(const XRRScreenResources* resources, RRMode id)
{
	for (int i = 0; i < resources->nmode; i++) {
		if (resources->modes[i].id == id) {
			return &resources->modes[i];
		}
	}
	return nullptr;
}

static int refreshRate(const XRRModeInfo* mode)
{
	if (mode->hTotal == 0 || mode->vTotal == 0) {
		return 0;
	}
	return static_cast<int>(std::lround(static_cast<double>(mode->dotClock) / (mode->hTotal * mode->vTotal)));
}

static RRCrtc windowCrtc(Display* connection, XRRScreenResources* resources, QWidget* window)
{
	QPoint center = window->mapToGlobal(window->rect().center());
	RRCrtc fallback = None;
	for (int i = 0; i < resources->ncrtc; i++) {
		XRRCrtcInfo* info = XRRGetCrtcInfo(connection, resources, resources->crtcs[i]);
		if (!info) {
			continue;
		}

		bool active = info->mode != None && info->noutput > 0;
		bool contains = center.x() >= info->x && center.y() >= info->y &&
						center.x() < info->x + static_cast<int>(info->width) &&
						center.y() < info->y + static_cast<int>(info->height);
		XRRFreeCrtcInfo(info);
		if (active && contains) {
			return resources->crtcs[i];
		}
		if (active && fallback == None) {
			fallback = resources->crtcs[i];
		}
	}
	return fallback;
}

bool changeDisplayMode(QWidget* window, QSize size)
{
	Display* connection = display();
	if (!connection) {
		return false;
	}

	XRRScreenResources* resources = XRRGetScreenResourcesCurrent(connection, DefaultRootWindow(connection));
	if (!resources) {
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "GetMonitorInfo failed");
		return false;
	}

	RRCrtc crtc = windowCrtc(connection, resources, window);
	XRRCrtcInfo* crtcInfo = crtc != None ? XRRGetCrtcInfo(connection, resources, crtc) : nullptr;
	const XRRModeInfo* current = crtcInfo ? modeInfo(resources, crtcInfo->mode) : nullptr;
	if (!current) {
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "GetMonitorInfo failed");
		if (crtcInfo) {
			XRRFreeCrtcInfo(crtcInfo);
		}
		XRRFreeScreenResources(resources);
		return false;
	}

	int pixels = size.width() * size.height();
	double aspect = static_cast<double>(current->width) / current->height;
	const XRRModeInfo* best = current;
	XRROutputInfo* output = XRRGetOutputInfo(connection, resources, crtcInfo->outputs[0]);
	for (int i = 0; output && i < output->nmode; i++) {
		const XRRModeInfo* mode = modeInfo(resources, output->modes[i]);
		if (!mode) {
			continue;
		}

		double ratio = static_cast<double>(mode->width) / mode->height;
		if (std::fabs(aspect - ratio) / aspect <= 0.1 && refreshRate(best) == refreshRate(mode) &&
			std::abs(pixels - static_cast<int>(mode->width * mode->height)) <
				std::abs(pixels - static_cast<int>(best->width * best->height))) {
			best = mode;
		}
	}
	if (output) {
		XRRFreeOutputInfo(output);
	}

	if (savedMode.crtc == None) {
		savedMode.crtc = crtc;
		savedMode.mode = crtcInfo->mode;
		savedMode.x = crtcInfo->x;
		savedMode.y = crtcInfo->y;
		savedMode.rotation = crtcInfo->rotation;
		savedMode.outputs.assign(crtcInfo->outputs, crtcInfo->outputs + crtcInfo->noutput);
	}

	Status result = XRRSetCrtcConfig(
		connection,
		resources,
		crtc,
		CurrentTime,
		crtcInfo->x,
		crtcInfo->y,
		best->id,
		crtcInfo->rotation,
		crtcInfo->outputs,
		crtcInfo->noutput
	);
	XRRFreeCrtcInfo(crtcInfo);
	XRRFreeScreenResources(resources);
	if (result != RRSetConfigSuccess) {
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "ChangeDisplaySettings == %d", result);
		return false;
	}
	return true;
}

void restoreDisplayMode(QWidget* window)
{
	Display* connection = display();
	if (!connection || savedMode.crtc == None) {
		return;
	}

	XRRScreenResources* resources = XRRGetScreenResourcesCurrent(connection, DefaultRootWindow(connection));
	if (!resources) {
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "CMainFrame::RestoreResolution hm is NULL");
		return;
	}

	Status result = XRRSetCrtcConfig(
		connection,
		resources,
		savedMode.crtc,
		CurrentTime,
		savedMode.x,
		savedMode.y,
		savedMode.mode,
		savedMode.rotation,
		savedMode.outputs.data(),
		static_cast<int>(savedMode.outputs.size())
	);
	XRRFreeScreenResources(resources);
	savedMode = SavedMode();
	if (result != RRSetConfigSuccess) {
		RBX::StandardOut::singleton()->print(
			RBX::MESSAGE_ERROR, "CMainFrame::RestoreResolution ChangeDisplaySettings == %d", result
		);
	}
}
#endif
