#include "WhidbeyStyle.h"

#include "CommandBarsOptions.h"
#include "NativeWindow.h"
#include "WorkspaceTabButtons.h"

#include <QAbstractScrollArea>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QCursor>
#include <QElapsedTimer>
#include <QEvent>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QListView>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPaintEngine>
#include <QPainter>
#include <QPointer>
#include <QRandomGenerator>
#include <QScreen>
#include <QStyleFactory>
#include <QStyleOption>
#include <QTabBar>
#include <QTabWidget>
#include <QTextLayout>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeView>
#include <QWidgetAction>
#include <QtMath>
#include <algorithm>
#include <climits>
#include <cmath>
#include <initializer_list>
#include <map>
#include <utility>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace {

struct Box
{
	int left = 0;
	int top = 0;
	int right = 0;
	int bottom = 0;

	Box() = default;
	Box(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}
	explicit Box(const QRect& rect)
		: left(rect.x()), top(rect.y()), right(rect.x() + rect.width()), bottom(rect.y() + rect.height())
	{
	}

	int width() const { return right - left; }
	int height() const { return bottom - top; }
	int centerX() const { return (left + right) / 2; }
	int centerY() const { return (top + bottom) / 2; }
	QRect toRect() const { return QRect(left, top, right - left, bottom - top); }
	Box deflated(int dx, int dy) const { return Box(left + dx, top + dy, right - dx, bottom - dy); }
};

enum ColorIndex
{
	ColorWindow,
	ColorFace,
	ColorShadow,
	ColorHighlight,
	ColorGrayText,
	ColorButtonText,
	ColorWindowText,
	ColorMenuText,
	ColorHighlightText,
	XpToolbarFace,
	XpHighlight,
	XpHighlightPushed,
	XpMenuBarFace,
	XpMenuBarGrayText,
	XpToolbarGripper,
	XpSeparator,
	XpMenuBarBorder,
	XpFloatBarBorder,
	XpDisabled,
	XpMenuBarExpanded,
	XpHighlightBorder,
	XpHighlightCheckedBorder,
	XpHighlightPushedBorder,
	XpMenuBarText,
	XpHighlightText,
	XpToolbarText,
	XpPushedText,
	ColorIndexCount
};

struct MuteColor
{
	ColorIndex index;
	ColorIndex main;
	ColorIndex sub;
	int distance;
};

enum SystemColorIndex
{
	SystemActiveCaption = 2,
	SystemInactiveCaption = 3,
	SystemWindow = 5,
	SystemMenuText = 7,
	SystemWindowText = 8,
	SystemCaptionText = 9,
	SystemAppWorkspace = 12,
	SystemHighlight = 13,
	SystemHighlightText = 14,
	SystemFace = 15,
	SystemShadow = 16,
	SystemGrayText = 17,
	SystemButtonText = 18,
	SystemInactiveCaptionText = 19,
	SystemButtonHighlight = 20,
	SystemDarkShadow = 21,
	SystemLight = 22,
	SystemInfoText = 23,
	SystemInfoBackground = 24,
	SystemHotLight = 26
};

const int gradientFractionBits = 24;
const int buttonLabelInset = 2;
const int toolBarPopupTextPadding = 16;
const int toolBarPopupItemHeight = 21;
const int dropDownGlyphGap = 5;
const QColor disabledButtonText(131, 131, 131);
const int popupShadowSize = 4;
const int splitDropDownWidth = 11;
const int toolBarExpandWidth = 12;
const int standaloneToolBarInset = 2;
const int toolBarLabelInset = 2;
const int comboThumbWidth = 15;
const int dropDownListTextShift = 1;
const int paneCaptionButtonSize = 14;
const int paneCaptionButtonGap = 2;

int menuAnimation = MenuAnimationWindowsDefault;
bool customizing = false;
QPointer<QWidget> customizeSelected;
const char* const animatePopupProperty = "animatePopup";
const int menuAnimationSteps = 10;
const int menuAnimationTime = 100;

struct MenuSegment
{
	int top;
	int bottom;
	bool expanded;
};

struct MenuAnimationState
{
	int type = MenuAnimationNone;
	bool expanding = false;
	bool expandDown = true;
	std::vector<MenuSegment> segments;
	QImage background;
	QPixmap content;
	QElapsedTimer elapsed;
	QTimer* timer = nullptr;
	bool grabbing = false;
};

std::map<QMenu*, MenuAnimationState*> menuAnimations;

#ifdef _WIN32
bool popupShadows = true;
#else
bool popupShadows = false;
#endif

QColor systemColor(int index, QRgb fallback)
{
#ifdef _WIN32
	COLORREF color = GetSysColor(index);
	return QColor(GetRValue(color), GetGValue(color), GetBValue(color));
#else
	return QColor(fallback);
#endif
}

QColor treeViewBorderColor(const QColor& fallback)
{
#ifdef _WIN32
	const int borderColorProperty = 3801;
	return WhidbeyStyle::themeColor(L"TREEVIEW", 0, 0, borderColorProperty, fallback);
#else
	return QColor(130, 135, 144);
#endif
}

QColor lightColor(const QColor& light, const QColor& dark, int delta)
{
	int param = delta < 100 ? 100 : 1000;
	return QColor(
		(dark.red() * (param - delta) + param / 2 + light.red() * delta) / param,
		(dark.green() * (param - delta) + param / 2 + light.green() * delta) / param,
		(dark.blue() * (param - delta) + param / 2 + light.blue() * delta) / param
	);
}

double redDelta(const QColor& color)
{
	return color.red() * (195.0 / 255.0);
}

double greenDelta(const QColor& color)
{
	return color.green() * (390.0 / 255.0);
}

double blueDelta(const QColor& color)
{
	return color.blue() * (65.0 / 255.0);
}

float colorWidth(int length, int width)
{
	if (length > 220) {
		length = 220;
	}
	return static_cast<float>((2.0 - static_cast<float>(length) / 220) * width);
}

float colorDelta(const QColor& a, const QColor& b)
{
	return static_cast<float>(
		std::pow(redDelta(a) - redDelta(b), 2) + std::pow(greenDelta(a) - greenDelta(b), 2) +
		std::pow(blueDelta(a) - blueDelta(b), 2)
	);
}

float colorLength(const QColor& a, const QColor& b)
{
	return std::sqrt(colorDelta(a, b));
}

bool longColor(const QColor& main, const QColor& sub, int distance)
{
	float width = colorWidth(static_cast<int>(colorLength(main, QColor(0, 0, 0))), distance);
	return std::pow(static_cast<double>(width), 2) < colorDelta(main, sub);
}

QColor scaledColor(float red, float green, float blue)
{
	red = std::clamp(red, 0.0f, 195.0f);
	green = std::clamp(green, 0.0f, 390.0f);
	blue = std::clamp(blue, 0.0f, 65.0f);

	int r = static_cast<int>(.5 + red * (255.0 / 195.0));
	int g = static_cast<int>(.5 + green * (255.0 / 390.0));
	int b = static_cast<int>(.5 + blue * (255.0 / 65.0));
	return QColor(std::clamp(r, 0, 255), std::clamp(g, 0, 255), std::clamp(b, 0, 255));
}

QColor mixColor(const QColor& main, const QColor& sub, int distance)
{
	QColor black(0, 0, 0);
	float mainLength = colorLength(main, black);
	float gate = colorWidth(static_cast<int>(mainLength), distance);

	bool reverse = true;
	if (colorLength(sub, black) > mainLength) {
		if (mainLength > 442 - gate) {
			reverse = false;
		}
	}
	else {
		if (mainLength > gate) {
			reverse = false;
		}
	}

	float subRed = static_cast<float>(redDelta(sub));
	float subGreen = static_cast<float>(greenDelta(sub));
	float subBlue = static_cast<float>(blueDelta(sub));
	float mainRed = static_cast<float>(redDelta(main));
	float mainGreen = static_cast<float>(greenDelta(main));
	float mainBlue = static_cast<float>(blueDelta(main));

	if (reverse) {
		subRed = static_cast<float>(195.0 - subRed);
		subGreen = static_cast<float>(390.0 - subGreen);
		subBlue = static_cast<float>(65.0 - subBlue);
		mainRed = static_cast<float>(195.0 - mainRed);
		mainGreen = static_cast<float>(390.0 - mainGreen);
		mainBlue = static_cast<float>(65.0 - mainBlue);
	}

	float a = static_cast<float>(
		std::max(0.001, std::sqrt(std::pow(subRed, 2) + std::pow(subGreen, 2) + std::pow(subBlue, 2)))
	);
	float b = mainRed * subRed + mainGreen * subGreen + mainBlue * subBlue;
	float x = static_cast<float>(b / std::pow(a, 2));
	float red = x * subRed - mainRed;
	float green = x * subGreen - mainGreen;
	float blue = x * subBlue - mainBlue;
	float y =
		static_cast<float>(std::sqrt(std::pow(gate, 2) - (std::pow(red, 2) + std::pow(green, 2) + std::pow(blue, 2))));
	float z = b / a - y;
	red = z * subRed / a;
	green = z * subGreen / a;
	blue = z * subBlue / a;

	if (reverse) {
		red = static_cast<float>(195.0 - red);
		green = static_cast<float>(390.0 - green);
		blue = static_cast<float>(65.0 - blue);
	}
	return scaledColor(red, green, blue);
}

QColor adjustColor(const QColor& main, const QColor& sub, int distance)
{
	float z = static_cast<float>(std::sqrt(
		std::pow(redDelta(main) - redDelta(sub), 2) + std::pow(greenDelta(main) - greenDelta(sub), 2) +
		std::pow(blueDelta(main) - blueDelta(sub), 2)
	));
	float q = (z - colorWidth(static_cast<int>(colorLength(QColor(0, 0, 0), main)), distance)) / z;
	float red = static_cast<float>(q * (redDelta(main) - redDelta(sub)) + redDelta(sub));
	float green = static_cast<float>(q * (greenDelta(main) - greenDelta(sub)) + greenDelta(sub));
	float blue = static_cast<float>(q * (blueDelta(main) - blueDelta(sub)) + blueDelta(sub));
	return scaledColor(red, green, blue);
}

WhidbeyColors computeColors()
{
	WhidbeyColors c;
	c.window = systemColor(SystemWindow, qRgb(255, 255, 255));
	c.windowText = systemColor(SystemWindowText, qRgb(0, 0, 0));
	c.menuText = systemColor(SystemMenuText, qRgb(0, 0, 0));
	c.face = systemColor(SystemFace, qRgb(240, 240, 240));
	c.shadow = systemColor(SystemShadow, qRgb(160, 160, 160));
	c.darkShadow = systemColor(SystemDarkShadow, qRgb(105, 105, 105));
	c.buttonHighlight = systemColor(SystemButtonHighlight, qRgb(255, 255, 255));
	c.buttonLight = systemColor(SystemLight, qRgb(227, 227, 227));
	c.buttonText = systemColor(SystemButtonText, qRgb(0, 0, 0));
	c.grayText = systemColor(SystemGrayText, qRgb(109, 109, 109));
	c.highlight = systemColor(SystemHighlight, qRgb(0, 120, 215));
	c.highlightText = systemColor(SystemHighlightText, qRgb(255, 255, 255));
	c.activeCaption = systemColor(SystemActiveCaption, qRgb(153, 180, 209));
	c.inactiveCaption = systemColor(SystemInactiveCaption, qRgb(191, 205, 219));
	c.captionText = systemColor(SystemCaptionText, qRgb(0, 0, 0));
	c.inactiveCaptionText = systemColor(SystemInactiveCaptionText, qRgb(0, 0, 0));
	c.infoBackground = systemColor(SystemInfoBackground, qRgb(255, 255, 225));
	c.infoText = systemColor(SystemInfoText, qRgb(0, 0, 0));
	c.hotLight = systemColor(SystemHotLight, qRgb(0, 102, 204));
	c.appWorkspace = systemColor(SystemAppWorkspace, qRgb(171, 171, 171));
	c.light =
		QColor(std::min(c.face.red() + 15, 255), std::min(c.face.green() + 15, 255), std::min(c.face.blue() + 15, 255));

	QColor table[ColorIndexCount];
	table[ColorWindow] = c.window;
	table[ColorFace] = c.face;
	table[ColorShadow] = c.shadow;
	table[ColorHighlight] = c.highlight;
	table[ColorGrayText] = c.grayText;
	table[ColorButtonText] = c.buttonText;
	table[ColorWindowText] = c.windowText;
	table[ColorMenuText] = c.menuText;
	table[ColorHighlightText] = c.highlightText;

	table[XpToolbarFace] = c.face;
	table[XpHighlight] = c.window;
	table[XpHighlightPushed] = c.highlight;
	table[XpMenuBarFace] = c.window;
	table[XpMenuBarGrayText] = c.grayText;
	table[XpToolbarGripper] = c.shadow;
	table[XpSeparator] = c.shadow;
	table[XpMenuBarBorder] = c.shadow;
	table[XpFloatBarBorder] = c.shadow;
	table[XpDisabled] = c.shadow;
	table[XpMenuBarExpanded] = c.shadow;
	table[XpHighlightBorder] = c.highlight;
	table[XpHighlightCheckedBorder] = c.highlight;
	table[XpHighlightPushedBorder] = c.highlight;
	table[XpMenuBarText] = c.windowText;
	table[XpHighlightText] = c.menuText;
	table[XpToolbarText] = c.buttonText;
	table[XpPushedText] = c.highlightText;

	static const MuteColor indexTable[] = {
		{XpToolbarFace, ColorWindow, ColorFace, 165},
		{XpHighlight, ColorHighlight, ColorWindow, 0x1E},
		{XpHighlightPushed, ColorHighlight, ColorWindow, 0x32},
		{XpMenuBarFace, ColorFace, ColorWindow, 0x8f},
		{XpMenuBarGrayText, ColorGrayText, ColorWindow, 0x46},
		{XpToolbarGripper, ColorShadow, ColorWindow, 0x4b},
		{XpSeparator, ColorShadow, ColorWindow, 0x46},
		{XpMenuBarBorder, XpToolbarText, ColorShadow, 0x14},
		{XpFloatBarBorder, XpToolbarText, ColorShadow, 0xF},
		{XpDisabled, ColorShadow, ColorWindow, 0x5A},
		{XpMenuBarExpanded, ColorFace, ColorShadow, 90},
	};
	for (const MuteColor& entry : indexTable) {
		table[entry.index] = lightColor(table[entry.main], table[entry.sub], entry.distance);
	}

	static const MuteColor longTable[] = {
		{XpHighlight, XpToolbarFace, XpHighlight, 50},
		{XpHighlightBorder, XpToolbarFace, ColorHighlight, 100},
		{XpHighlightPushed, XpToolbarFace, XpHighlightPushed, 30},
		{XpMenuBarGrayText, XpMenuBarFace, XpMenuBarGrayText, 80},
		{XpHighlightCheckedBorder, XpHighlightPushed, XpHighlightCheckedBorder, 100},
		{XpToolbarGripper, XpToolbarFace, XpToolbarGripper, 85},
		{XpSeparator, XpToolbarFace, XpSeparator, 50},
		{XpMenuBarBorder, XpMenuBarFace, XpMenuBarBorder, 100},
		{XpFloatBarBorder, XpToolbarFace, XpFloatBarBorder, 100},
		{XpDisabled, XpToolbarFace, XpDisabled, 80},
		{XpMenuBarText, XpMenuBarFace, XpMenuBarText, 180},
		{XpHighlightText, XpHighlight, XpHighlightText, 180},
		{XpToolbarText, XpToolbarFace, XpToolbarText, 180},
		{XpPushedText, XpHighlightPushed, XpPushedText, 180},
	};

	if (longColor(table[ColorFace], table[XpToolbarFace], 35)) {
		table[XpToolbarFace] = adjustColor(table[ColorFace], table[XpToolbarFace], 35);
	}
	for (const MuteColor& entry : longTable) {
		if (!longColor(table[entry.main], table[entry.sub], entry.distance)) {
			table[entry.index] = mixColor(table[entry.main], table[entry.sub], entry.distance);
		}
	}
	if (longColor(table[XpMenuBarFace], table[XpMenuBarGrayText], 145)) {
		table[XpMenuBarGrayText] = adjustColor(table[XpMenuBarFace], table[XpMenuBarGrayText], 145);
	}

	c.toolbarFace = table[XpToolbarFace];
	c.toolbarGripper = table[XpToolbarGripper];
	c.toolbarText = table[XpToolbarText];
	c.separator = table[XpSeparator];
	c.disabled = table[XpDisabled];
	c.toolbarGrayText = c.disabled;
	c.menuBarFace = table[XpMenuBarFace];
	c.menuBarBorder = table[XpMenuBarBorder];
	c.menuBarText = table[XpMenuBarText];
	c.menuBarGrayText = table[XpMenuBarGrayText];
	c.highlightFace = table[XpHighlight];
	c.highlightBorder = table[XpHighlightBorder];
	c.highlightPushed = table[XpHighlightPushed];
	c.highlightPushedBorder = table[XpHighlightPushedBorder];
	c.highlightCheckedBorder = table[XpHighlightCheckedBorder];
	c.highlightDisabledBorder = c.disabled;
	c.highlightedText = table[XpHighlightText];
	c.pushedText = table[XpPushedText];
	c.highlightChecked = QColor(
		c.window.red() * 40 / 100 + c.highlight.red() * 10 / 100 + c.toolbarFace.red() * 50 / 100,
		c.window.green() * 40 / 100 + c.highlight.green() * 10 / 100 + c.toolbarFace.green() * 50 / 100,
		c.window.blue() * 40 / 100 + c.highlight.blue() * 10 / 100 + c.toolbarFace.blue() * 50 / 100
	);
	c.staticFrame = treeViewBorderColor(c.shadow);

	QColor lightFace = lightColor(c.face, c.window, 0xcd);
	c.commandBarLight = lightFace;
	c.commandBarDark = c.face;
	c.dockBarLight = c.face;
	c.dockBarDark = lightFace;
	c.popupControlLight = c.menuBarFace;
	c.menuExpandDark = lightColor(c.window, c.face, 550);
	c.menuExpandedLight = lightColor(c.window, table[XpMenuBarExpanded], 550);
	c.menuExpandedDark = table[XpMenuBarExpanded];
	c.popupControlDark = lightColor(c.window, c.face, 550);
	c.toolbarExpandLight = lightColor(c.window, c.face, 550);
	c.toolbarExpandDark = c.shadow;
	c.toolbarShadow = c.toolbarFace;
	c.statusPane = lightColor(c.face, c.window, 70);
	c.disabledIconLight = c.face;
	c.disabledIconDark = c.shadow;

	c.paneCaptionActive = c.activeCaption;
	c.paneCaptionInactive = c.inactiveCaption;
	c.paneCaptionActiveText = c.captionText;
	c.paneCaptionInactiveText = c.inactiveCaptionText;
	c.paneCaptionBorder = c.shadow;

	c.tabFrameBorder = c.staticFrame;
	c.tabHeaderFace = c.face;
	c.tabButtonNormal = c.face;
	c.tabButtonSelected = c.window;
	c.tabNormalText = c.buttonText;
	c.tabSelectedText = c.windowText;
	c.tabBorderShadow = c.shadow;
	c.tabBorderHighlight = c.face;
	return c;
}

WhidbeyColors& colorCache()
{
	static WhidbeyColors cache = computeColors();
	return cache;
}

QFont clampedFont(QFont font)
{
	if (font.pixelSize() > 0) {
		if (font.pixelSize() < 11) {
			font.setPixelSize(11);
		}
	}
	else if (font.pointSizeF() > 0 && font.pointSizeF() < 8.25) {
		font.setPointSizeF(8.25);
	}
	return font;
}

void fill(QPainter* painter, int x, int y, int width, int height, const QColor& color)
{
	if (width > 0 && height > 0) {
		painter->fillRect(QRect(x, y, width, height), color);
	}
}

void fill(QPainter* painter, const Box& box, const QColor& color)
{
	fill(painter, box.left, box.top, box.width(), box.height(), color);
}

void draw3dRect(QPainter* painter, const Box& box, const QColor& topLeft, const QColor& bottomRight)
{
	int width = box.width();
	int height = box.height();
	if (width <= 0 || height <= 0) {
		return;
	}
	fill(painter, box.left, box.top, width - 1, 1, topLeft);
	fill(painter, box.left, box.top, 1, height - 1, topLeft);
	fill(painter, box.right - 1, box.top, 1, height, bottomRight);
	fill(painter, box.left, box.bottom - 1, width, 1, bottomRight);
}

void rectangle(QPainter* painter, const Box& box, const QColor& border, const QColor& face)
{
	fill(painter, box, face);
	draw3dRect(painter, box, border, border);
}

int gradientLevel(int from, int to, int step, int count)
{
	qint64 delta = static_cast<qint64>(to - from) * (qint64(1) << gradientFractionBits);
	qint64 slope = delta / count;
	if (delta % count < 0) {
		slope--;
	}
	return static_cast<int>(
		(static_cast<qint64>(from) * (qint64(1) << gradientFractionBits) + slope * step) >> gradientFractionBits
	);
}

QColor blend(const QColor& from, const QColor& to, int step, int count)
{
	return QColor(
		gradientLevel(from.red(), to.red(), step, count),
		gradientLevel(from.green(), to.green(), step, count),
		gradientLevel(from.blue(), to.blue(), step, count)
	);
}

void gradientFill(QPainter* painter, const Box& box, const QColor& from, const QColor& to, bool horizontal)
{
	if (box.width() <= 0 || box.height() <= 0) {
		return;
	}
	if (from == to) {
		fill(painter, box, from);
		return;
	}

	int count = horizontal ? box.width() : box.height();
	for (int i = 0; i < count; i++) {
		QColor color = blend(from, to, i, count);
		if (horizontal) {
			fill(painter, box.left + i, box.top, 1, box.height(), color);
		}
		else {
			fill(painter, box.left, box.top + i, box.width(), 1, color);
		}
	}
}

const QWidget* dockFillWindow(const QWidget* widget)
{
	if (qobject_cast<const QToolBar*>(widget) && widget->parentWidget()) {
		return widget->parentWidget();
	}
	return widget->window();
}

QBrush dockBarBrush(const QWidget* widget)
{
	const WhidbeyColors& c = WhidbeyStyle::colors();
	int offset = 0;
	int frameWidth = 0;
	if (widget) {
		const QWidget* window = dockFillWindow(widget);
		offset = widget->mapTo(window, QPoint(0, 0)).x();
		frameWidth = window->width();
	}

	int halfScreen = 0;
	if (QScreen* screen = QGuiApplication::primaryScreen()) {
		halfScreen = screen->availableGeometry().width() / 2;
	}

	int length = std::max(std::max(frameWidth, halfScreen), 1);
	static QImage strip;
	static QColor stripFrom;
	static QColor stripTo;
	if (strip.width() != length || stripFrom != c.dockBarLight || stripTo != c.dockBarDark) {
		stripFrom = c.dockBarLight;
		stripTo = c.dockBarDark;
		strip = QImage(length, 1, QImage::Format_RGB32);
		QRgb* line = reinterpret_cast<QRgb*>(strip.scanLine(0));
		for (int i = 0; i < length; i++) {
			int step = std::max(2 * i - 1, 0);
			int count = 2 * length;
			line[i] = qRgb(
				stripFrom.red() + (stripTo.red() - stripFrom.red()) * step / count,
				stripFrom.green() + (stripTo.green() - stripFrom.green()) * step / count,
				stripFrom.blue() + (stripTo.blue() - stripFrom.blue()) * step / count
			);
		}
	}

	QBrush brush(strip);
	brush.setTransform(QTransform::fromTranslate(-offset, 0));
	return brush;
}

int divideRounded(int value, int divisor)
{
	return value >= 0 ? (2 * value + divisor) / (2 * divisor) : -((-2 * value + divisor) / (2 * divisor));
}

void drawLine(QPainter* painter, int x0, int y0, int x1, int y1, const QColor& color)
{
	int dx = x1 - x0;
	int dy = y1 - y0;
	int steps = std::max(std::abs(dx), std::abs(dy));
	for (int i = 0; i < steps; i++) {
		fill(painter, x0 + divideRounded(dx * i, steps), y0 + divideRounded(dy * i, steps), 1, 1, color);
	}
}

void drawPolyline(QPainter* painter, QPoint start, std::initializer_list<QPoint> offsets, const QColor& color)
{
	QPoint point = start;
	for (const QPoint& offset : offsets) {
		QPoint next = point + offset;
		drawLine(painter, point.x(), point.y(), next.x(), next.y(), color);
		point = next;
	}
}

void drawTriangle(QPainter* painter, QPoint a, QPoint b, QPoint c, const QColor& color, bool filled = true)
{
	const QPoint points[3] = {a, b, c};
	int top = std::min(a.y(), std::min(b.y(), c.y()));
	int bottom = std::max(a.y(), std::max(b.y(), c.y()));
	std::vector<std::pair<int, int>> rows(bottom - top + 1, std::pair<int, int>(INT_MAX, INT_MIN));

	for (int edge = 0; edge < 3; edge++) {
		QPoint from = points[edge];
		QPoint to = points[(edge + 1) % 3];
		int dx = to.x() - from.x();
		int dy = to.y() - from.y();
		int steps = std::max(std::abs(dx), std::abs(dy));
		for (int i = 0; i <= steps; i++) {
			int x = steps ? from.x() + divideRounded(dx * i, steps) : from.x();
			int y = steps ? from.y() + divideRounded(dy * i, steps) : from.y();
			if (filled) {
				std::pair<int, int>& row = rows[y - top];
				row.first = std::min(row.first, x);
				row.second = std::max(row.second, x);
			}
			else {
				fill(painter, x, y, 1, 1, color);
			}
		}
	}

	if (filled) {
		for (int y = top; y <= bottom; y++) {
			const std::pair<int, int>& row = rows[y - top];
			if (row.first <= row.second) {
				fill(painter, row.first, y, row.second - row.first + 1, 1, color);
			}
		}
	}
}

void drawRectangle(
	QPainter* painter,
	const Box& box,
	bool selected,
	bool pressed,
	bool enabled,
	bool checked,
	bool popuped,
	bool popupBar
)
{
	const WhidbeyColors& c = WhidbeyStyle::colors();
	if (popuped) {
		gradientFill(painter, box, c.popupControlLight, c.popupControlDark, false);
		draw3dRect(painter, box, c.menuBarBorder, c.menuBarBorder);
		return;
	}

	if (!enabled) {
		if (checked) {
			if (!popupBar) {
				fill(painter, box, c.highlightChecked);
			}
			draw3dRect(painter, box, c.highlightDisabledBorder, c.highlightDisabledBorder);
		}
		return;
	}

	if (checked && !selected && !pressed) {
		rectangle(painter, box, c.highlightCheckedBorder, c.highlightChecked);
	}
	else if (checked && selected && !pressed) {
		rectangle(painter, box, c.highlightPushedBorder, c.highlightPushed);
	}
	else if (selected && pressed) {
		rectangle(painter, box, c.highlightPushedBorder, c.highlightPushed);
	}
	else if (selected || pressed) {
		rectangle(painter, box, c.highlightBorder, c.highlightFace);
	}
}

void drawAttachedPopupControl(QPainter* painter, const Box& box)
{
	const WhidbeyColors& c = WhidbeyStyle::colors();
	gradientFill(painter, box, c.popupControlLight, c.popupControlDark, false);
	fill(painter, box.left, box.top, box.width(), 1, c.menuBarBorder);
	fill(painter, box.left, box.top, 1, box.height(), c.menuBarBorder);
	fill(painter, box.right - 1, box.top, 1, box.height(), c.menuBarBorder);
}

QColor rectangleTextColor(bool selected, bool pressed, bool enabled, bool checked, bool popuped, bool popupBar)
{
	const WhidbeyColors& c = WhidbeyStyle::colors();
	if (popupBar) {
		return !enabled ? c.menuBarGrayText : selected ? c.highlightedText : c.menuBarText;
	}
	if (!enabled) {
		return c.toolbarGrayText;
	}
	if (popuped) {
		return c.toolbarText;
	}
	if (checked && !selected && !pressed) {
		return c.highlightedText;
	}
	if (selected && (pressed || checked)) {
		return c.pushedText;
	}
	if (selected || pressed) {
		return c.highlightedText;
	}
	return c.toolbarText;
}

void drawDropDownGlyph(QPainter* painter, QPoint point, bool selected, bool popuped, bool enabled)
{
	QColor color = rectangleTextColor(selected, false, enabled, false, popuped, false);
	drawTriangle(
		painter,
		QPoint(point.x() - 2, point.y() - 1),
		QPoint(point.x() + 2, point.y() - 1),
		QPoint(point.x(), point.y() + 1),
		color
	);
}

void drawCheckMark(QPainter* painter, const Box& box, const QColor& color)
{
	int left = box.left + box.width() / 2 - 10;
	int top = box.top + box.height() / 2 - 10;
	drawPolyline(painter, QPoint(left + 6, top + 9), {QPoint(2, 2), QPoint(5, -5)}, color);
	drawPolyline(painter, QPoint(left + 6, top + 10), {QPoint(2, 2), QPoint(5, -5)}, color);
}

void drawRadioMark(QPainter* painter, const Box& box, const QColor& color)
{
	Box radio(box.centerX() - 3, box.centerY() - 3, box.centerX() + 3, box.centerY() + 3);
	double centerX = static_cast<double>(radio.left + radio.right - 1) / 2;
	double centerY = static_cast<double>(radio.top + radio.bottom - 1) / 2;
	double radius = static_cast<double>(radio.right - radio.left - 1) / 2 - 0.25;

	for (int x = radio.left; x < radio.right; x++) {
		for (int y = radio.top; y < radio.bottom; y++) {
			double distance = std::sqrt(std::pow(centerX - x, 2) + std::pow(centerY - y, 2));
			if (distance > radius + 1) {
				continue;
			}

			QColor pixel = color;
			if (distance > radius) {
				pixel.setAlphaF(static_cast<float>(1.0 - (distance - radius)));
			}
			fill(painter, x, y, 1, 1, pixel);
		}
	}
}

void drawGripperDot(QPainter* painter, int x, int y)
{
	const WhidbeyColors& c = WhidbeyStyle::colors();
	fill(painter, x + 1, y + 1, 2, 2, c.window);
	fill(painter, x, y, 2, 2, c.toolbarGripper);
}

void drawExpandSymbols(QPainter* painter, const Box& box, bool vertical, bool hiddenExists, const QColor& color)
{
	if (!vertical) {
		QPoint point(box.left + box.width() / 2, box.bottom - 6);
		drawLine(painter, point.x() - 2, point.y() - 4, point.x() + 3, point.y() - 4, color);
		drawTriangle(
			painter,
			QPoint(point.x() - 2, point.y() - 1),
			QPoint(point.x() + 2, point.y() - 1),
			QPoint(point.x(), point.y() + 1),
			color
		);

		if (hiddenExists) {
			drawTriangle(
				painter,
				QPoint(box.left + 3, box.top + 5),
				QPoint(box.left + 4, box.top + 6),
				QPoint(box.left + 3, box.top + 7),
				color
			);
			drawTriangle(
				painter,
				QPoint(box.left + 7, box.top + 5),
				QPoint(box.left + 8, box.top + 6),
				QPoint(box.left + 7, box.top + 7),
				color
			);
		}
	}
	else {
		QPoint point(box.right - 5, box.centerY());
		drawLine(painter, point.x() - 5, point.y() - 2, point.x() - 5, point.y() + 3, color);
		drawTriangle(
			painter,
			QPoint(point.x(), point.y()),
			QPoint(point.x() - 2, point.y() - 2),
			QPoint(point.x() - 2, point.y() + 2),
			color
		);

		if (hiddenExists) {
			drawTriangle(
				painter,
				QPoint(box.left + 5, box.top + 3),
				QPoint(box.left + 6, box.top + 4),
				QPoint(box.left + 7, box.top + 3),
				color
			);
			drawTriangle(
				painter,
				QPoint(box.left + 5, box.top + 7),
				QPoint(box.left + 6, box.top + 8),
				QPoint(box.left + 7, box.top + 7),
				color
			);
		}
	}
}

void drawToolBarExpand(QPainter* painter, const Box& box, bool vertical, bool hiddenExists, bool selected, bool popuped)
{
	const WhidbeyColors& c = WhidbeyStyle::colors();

	QRegion region(box.toRect());
	if (!vertical) {
		region -= QRect(box.right - 1, box.top, 1, 1);
		region -= QRect(box.right - 1, box.bottom - 1, 1, 1);
		region -= QRect(box.left, box.top + 1, 1, box.height() - 2);
	}
	else {
		region -= QRect(box.left, box.bottom - 1, 1, 1);
		region -= QRect(box.right - 1, box.bottom - 1, 1, 1);
		region -= QRect(box.left + 1, box.top, box.width() - 2, 1);
	}

	painter->save();
	painter->setClipRegion(region, Qt::IntersectClip);
	if (popuped) {
		fill(painter, box, c.highlightChecked);
	}
	else if (selected) {
		fill(painter, box, c.highlightFace);
	}
	else {
		gradientFill(painter, box, c.toolbarExpandLight, c.toolbarExpandDark, vertical);
	}

	QColor text = popuped ? c.toolbarText : selected ? c.highlightedText : c.toolbarText;
	Box shifted(box.left + 1, box.top + 1, box.right + 1, box.bottom + 1);
	drawExpandSymbols(painter, shifted, vertical, hiddenExists, c.buttonHighlight);
	drawExpandSymbols(painter, box, vertical, hiddenExists, text);
	painter->restore();
}

bool isToolBarExtension(const QWidget* widget)
{
	return widget && widget->inherits("QToolBarExtension");
}

QWidget* toolBarExtension(const QToolBar* toolBar)
{
	return toolBar->findChild<QWidget*>(QLatin1String("qt_toolbar_ext_button"), Qt::FindDirectChildrenOnly);
}

bool inToolBar(const QWidget* widget)
{
	const QWidget* parent = widget ? widget->parentWidget() : nullptr;
	return parent && (qobject_cast<const QToolBar*>(parent) || parent->property("popupToolBar").toBool());
}

bool isToolBarComboEdit(const QWidget* widget)
{
	return widget && qobject_cast<const QComboBox*>(widget->parentWidget()) && inToolBar(widget->parentWidget());
}

bool showsExpandAlways(const QToolBar* toolBar)
{
	return toolBar && qobject_cast<const QMainWindow*>(toolBar->parentWidget());
}

int toolBarExtent(const QToolBar* toolBar, bool horizontal, bool* extensionVisible)
{
	int length = horizontal ? toolBar->width() : toolBar->height();
	QWidget* extension = toolBarExtension(toolBar);
	*extensionVisible = extension && !extension->isHidden();
	if (!showsExpandAlways(toolBar)) {
		return length;
	}
	if (*extensionVisible) {
		return length - 1;
	}

	int end = -1;
	for (QObject* child : toolBar->children()) {
		if (!child->isWidgetType() || child == extension) {
			continue;
		}
		QWidget* item = static_cast<QWidget*>(child);
		if (item->isHidden() || item->isWindow() || item->testAttribute(Qt::WA_TransparentForMouseEvents)) {
			continue;
		}
		end = std::max(end, horizontal ? item->geometry().right() + 1 : item->geometry().bottom() + 1);
	}

	if (end < 0) {
		return std::min(length, WhidbeyStyle::emptyToolBarExtent);
	}
	return std::min(length, end + toolBarExpandWidth);
}

int topDockInset(const QToolBar* toolBar)
{
	return toolBar ? toolBar->property("dockOffset").toInt() : 0;
}

int blendChannel(int a, int b, float amountA)
{
	return std::min(255, static_cast<int>(a * amountA + b * (1.0f - amountA)));
}

QColor blendColors(const QColor& a, const QColor& b, float amountA)
{
	return QColor(
		blendChannel(a.red(), b.red(), amountA),
		blendChannel(a.green(), b.green(), amountA),
		blendChannel(a.blue(), b.blue(), amountA)
	);
}

int gradientChannel(int from, int to, int index, int count)
{
	return from + (to - from) * index / count;
}

QImage clippedDockBarStrip(const QWidget* widget, int left, int width)
{
	const WhidbeyColors& c = WhidbeyStyle::colors();
	const QWidget* window = dockFillWindow(widget);
	int clipLeft = widget->mapTo(window, QPoint(left, 0)).x();
	int clipRight = clipLeft + width;
	int halfScreen = 0;
	if (QScreen* screen = QGuiApplication::primaryScreen()) {
		halfScreen = screen->availableGeometry().width() / 2;
	}
	int length = std::max(std::max(window->width(), halfScreen), 1);

	QColor from = c.dockBarLight;
	QColor to = c.dockBarDark;
	if (clipLeft > 0) {
		from = blendColors(c.dockBarLight, c.dockBarDark, static_cast<float>(length - clipLeft) / length);
	}
	if (clipRight < length) {
		to = blendColors(c.dockBarLight, c.dockBarDark, static_cast<float>(length - clipRight) / length);
	}

	QImage strip(std::max(width, 1), 1, QImage::Format_RGB32);
	QRgb* line = reinterpret_cast<QRgb*>(strip.scanLine(0));
	for (int i = 0; i < width; i++) {
		line[i] = qRgb(
			gradientLevel(from.red(), to.red(), i, width),
			gradientLevel(from.green(), to.green(), i, width),
			gradientLevel(from.blue(), to.blue(), i, width)
		);
	}
	return strip;
}

void fillFramePart(QPainter* painter, const QRect& rect, const QWidget* widget)
{
	const WhidbeyColors& c = WhidbeyStyle::colors();
	const QWidget* window = widget->window();
	QRect frame = windowFrameRect(window);
	int frameLeft = frame.x() - window->mapToGlobal(QPoint(0, 0)).x();
	int left = widget->mapTo(window, rect.topLeft()).x() - frameLeft;
	int halfScreen = 0;
	if (QScreen* screen = QGuiApplication::primaryScreen()) {
		halfScreen = screen->availableGeometry().width() / 2;
	}
	float width = static_cast<float>(std::max(frame.width(), halfScreen));

	QColor from = blendColors(c.dockBarDark, c.dockBarLight, left / width);
	QColor to = blendColors(c.dockBarDark, c.dockBarLight, (left + rect.width()) / width);
	QImage strip(std::max(rect.width(), 1), 1, QImage::Format_RGB32);
	QRgb* line = reinterpret_cast<QRgb*>(strip.scanLine(0));
	for (int i = 0; i < rect.width(); i++) {
		line[i] = qRgb(
			gradientChannel(from.red(), to.red(), i, rect.width()),
			gradientChannel(from.green(), to.green(), i, rect.width()),
			gradientChannel(from.blue(), to.blue(), i, rect.width())
		);
	}
	painter->drawImage(rect, strip);
}

void drawToolBarBackground(QPainter* painter, const QStyleOption* option, const QWidget* widget)
{
	const WhidbeyColors& c = WhidbeyStyle::colors();
	const QToolBar* toolBar = qobject_cast<const QToolBar*>(widget);
	const QStyleOptionToolBar* toolBarOption = qstyleoption_cast<const QStyleOptionToolBar*>(option);
	Box rc(option->rect);
	bool horizontal = option->state & QStyle::State_Horizontal;
	bool movable = toolBarOption && (toolBarOption->features & QStyleOptionToolBar::Movable);
	bool rightArea = toolBarOption && toolBarOption->toolBarArea == Qt::RightToolBarArea;

	QBrush dock = rightArea ? QBrush(c.dockBarDark) : dockBarBrush(widget);
	painter->fillRect(option->rect, dock);

	bool extensionVisible = false;
	int extent = horizontal ? rc.right : rc.bottom;
	if (toolBar) {
		extent = (horizontal ? rc.left : rc.top) + toolBarExtent(toolBar, horizontal, &extensionVisible);
	}

	if (horizontal) {
		Box bar(rc.left + topDockInset(toolBar), rc.top, extent, rc.bottom);
		QImage strip;
		int stripLeft = bar.left;
		if (widget && toolBarOption && toolBarOption->toolBarArea == Qt::TopToolBarArea) {
			strip = clippedDockBarStrip(widget, bar.left, bar.width() + 1);
		}
		else if (toolBar && !showsExpandAlways(toolBar)) {
			stripLeft = 0;
			strip = clippedDockBarStrip(widget, 0, toolBar->width());
		}
		auto fillDock = [&](const QRect& rect) {
			if (strip.isNull()) {
				painter->fillRect(rect, dock);
			}
			else {
				painter->drawImage(rect, strip, QRect(rect.left() - stripLeft, 0, rect.width(), 1));
			}
		};
		fillDock(QRect(bar.left, bar.top, bar.width() + 1, bar.height()));
		gradientFill(
			painter,
			Box(bar.left + 3, bar.top + 1, bar.right, bar.bottom - 1),
			c.commandBarLight,
			c.commandBarDark,
			false
		);
		fillDock(QRect(bar.left + 3, bar.top + 1, 1, 1));
		fillDock(QRect(bar.right - 1, bar.top + 1, 1, 1));
		fillDock(QRect(bar.left + 3, bar.bottom - 2, 1, 1));
		fillDock(QRect(bar.right - 1, bar.bottom - 2, 1, 1));
		fill(painter, bar.left + 5, bar.bottom - 1, bar.width() - 7, 1, c.toolbarShadow);

		if (movable) {
			for (int y = 5; y < bar.height() - 7; y += 4) {
				drawGripperDot(painter, bar.left + 6, bar.top + y);
			}
		}
		if (showsExpandAlways(toolBar)) {
			drawToolBarExpand(
				painter,
				Box(bar.right - toolBarExpandWidth + 1, bar.top + 1, bar.right + 1, bar.bottom),
				false,
				extensionVisible,
				false,
				toolBar && toolBar->property("expandPopuped").toBool()
			);
		}
	}
	else {
		Box bar(rc.left, rc.top, rc.right, extent);
		gradientFill(
			painter,
			Box(bar.left + 1, bar.top + 3, bar.right - 1, bar.bottom),
			c.commandBarLight,
			c.commandBarDark,
			true
		);
		painter->fillRect(QRect(bar.left + 1, bar.top + 3, 1, 1), dock);
		painter->fillRect(QRect(bar.right - 2, bar.top + 3, 1, 1), dock);
		painter->fillRect(QRect(bar.left + 1, bar.bottom - 1, 1, 1), dock);
		painter->fillRect(QRect(bar.right - 2, bar.bottom - 1, 1, 1), dock);
		fill(painter, bar.right - 1, bar.top + 5, 1, bar.height() - 7, c.toolbarShadow);

		if (movable) {
			for (int x = 5; x < bar.width() - 7; x += 4) {
				drawGripperDot(painter, bar.left + x, bar.top + 6);
			}
		}
		if (showsExpandAlways(toolBar)) {
			drawToolBarExpand(
				painter,
				Box(bar.left, bar.bottom - toolBarExpandWidth, bar.right, bar.bottom),
				true,
				extensionVisible,
				false,
				toolBar && toolBar->property("expandPopuped").toBool()
			);
		}
	}
}

void updateToolBarMargins(QToolBar* toolBar)
{
	if (!showsExpandAlways(toolBar)) {
		if (!toolBar->property("standaloneInset").toBool()) {
			QMargins margins = toolBar->layout()->contentsMargins();
			margins.setLeft(margins.left() + standaloneToolBarInset);
			toolBar->layout()->setContentsMargins(margins);
			toolBar->setProperty("standaloneInset", true);
		}
		toolBar->setContentsMargins(0, 0, 0, 0);
		return;
	}
	int reserved = toolBarExpandWidth - 1;
	if (toolBar->orientation() == Qt::Horizontal) {
		toolBar->setContentsMargins(0, 0, reserved, 0);
	}
	else {
		toolBar->setContentsMargins(0, 0, 0, reserved);
	}
}

bool isWorkspaceTabWidget(const QWidget* widget)
{
	const QTabWidget* tabWidget = qobject_cast<const QTabWidget*>(widget);
	return tabWidget && tabWidget->documentMode();
}

bool isWorkspaceTabBar(const QWidget* widget)
{
	const QTabBar* bar = qobject_cast<const QTabBar*>(widget);
	return bar && bar->documentMode() && qobject_cast<const QTabWidget*>(bar->parentWidget());
}

bool hasWorkspaceButtons(const QWidget* tabBar)
{
	const QTabWidget* tabWidget = tabBar ? qobject_cast<const QTabWidget*>(tabBar->parentWidget()) : nullptr;
	return tabWidget && dynamic_cast<WorkspaceTabButtons*>(tabWidget->cornerWidget(Qt::TopRightCorner));
}

int workspaceButtonHeight(const QFontMetrics& metrics)
{
	return std::max(18, metrics.height() + 3);
}

int workspaceTabLead(const QFontMetrics& metrics)
{
	return 2 + workspaceButtonHeight(metrics);
}

int commandBarTextHeight(const QFontMetrics& metrics)
{
	return metrics.height() + 6;
}

int menuShortcutColumnWidth(const QMenu* menu)
{
	int width = 0;
	if (menu) {
		QFontMetrics metrics(menu->font());
		for (const QAction* action : menu->actions()) {
			if (action->isSeparator() || !action->isVisible()) {
				continue;
			}
			QString text = action->text();
			qsizetype tab = text.indexOf(QLatin1Char('\t'));
			if (tab != -1) {
				width = std::max(width, metrics.horizontalAdvance(text.mid(tab + 1)));
			}
			else if (!action->shortcut().isEmpty()) {
				width =
					std::max(width, metrics.horizontalAdvance(action->shortcut().toString(QKeySequence::NativeText)));
			}
		}
	}
	return width;
}

int popupImageWidth(const QFontMetrics& metrics)
{
	return std::max(16 + 4, commandBarTextHeight(metrics) + 4);
}

bool isToolBarPopup(const QWidget* widget)
{
	return widget && widget->property("toolBarPopup").toBool();
}

int gripperColumns(const QWidget* widget)
{
	return widget && widget->property("doubleGripper").toBool() ? 2 : 1;
}

bool menuHasGutter(const QMenu* menu)
{
	if (isToolBarPopup(menu)) {
		return false;
	}
	for (QAction* action : menu->actions()) {
		if (action->isVisible() && !qobject_cast<QWidgetAction*>(action)) {
			return true;
		}
	}
	return false;
}

const char* const popupControlShadowName = "popupControlShadow";

bool isContextMenu(const QWidget* widget)
{
	const QMenu* menu = qobject_cast<const QMenu*>(widget);
	while (menu) {
		const QWidget* parent = menu->parentWidget();
		if (qobject_cast<const QMenuBar*>(parent) || qobject_cast<const QToolButton*>(parent) ||
			qobject_cast<const QToolBar*>(parent)) {
			return false;
		}
		const QMenu* parentMenu = qobject_cast<const QMenu*>(parent);
		if (!parentMenu) {
			return true;
		}
		menu = parentMenu;
	}
	return false;
}

class PopupControlShadow : public QWidget
{
public:
	explicit PopupControlShadow(QWidget* popup)
		: QWidget(
			  popup,
			  Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowTransparentForInput |
				  Qt::WindowDoesNotAcceptFocus
		  )
	{
		setObjectName(QLatin1String(popupControlShadowName));
		setAttribute(Qt::WA_TranslucentBackground);
		setAttribute(Qt::WA_ShowWithoutActivating);
	}

protected:
	void paintEvent(QPaintEvent* event) override
	{
		QPainter painter(this);
		for (int column = 0; column < width(); column++) {
			for (int row = 0; row < height(); row++) {
				int alpha = row < 4 ? 3 * (4 - column) * (row + 1) : 0x0F * (4 - column);
				painter.fillRect(column, row, 1, 1, QColor(0, 0, 0, alpha));
			}
		}
	}
};

bool menuHasShadow(const QWidget* menu)
{
	return menu && menu->testAttribute(Qt::WA_TranslucentBackground);
}

QRect popupControlRect(const QMenu* menu)
{
	QVariant control = menu->property("popupControl");
	if (control.isValid()) {
		return control.toRect();
	}
	QWidget* parent = menu->parentWidget();
	if (QMenuBar* bar = qobject_cast<QMenuBar*>(parent)) {
		QRect rect = bar->actionGeometry(menu->menuAction());
		if (rect.isValid()) {
			rect.adjust(0, 1, 0, -1);
			return QRect(bar->mapToGlobal(rect.topLeft()), rect.size());
		}
	}
	if (QToolButton* button = qobject_cast<QToolButton*>(parent)) {
		if (button->menu() == menu) {
			QRect rect(button->mapToGlobal(QPoint(0, 0)), button->size());
			const QToolBar* toolBar = qobject_cast<const QToolBar*>(button->parentWidget());
			if (toolBar && toolBar->orientation() == Qt::Vertical) {
				return rect.adjusted(1, 0, -1, 0);
			}
			return rect.adjusted(0, 1, 0, -1);
		}
	}
	return QRect();
}

void drawPopupShadow(QPainter* painter, const Box& panel)
{
	int height = panel.height() - popupShadowSize;
	for (int column = 0; column < popupShadowSize; column++) {
		int x = panel.right + column;
		for (int row = 0; row < height; row++) {
			int alpha = row < 4 ? 3 * (4 - column) * (row + 1) : 0x0F * (4 - column);
			fill(painter, x, panel.top + popupShadowSize + row, 1, 1, QColor(0, 0, 0, alpha));
		}
	}

	int width = panel.width();
	for (int row = 0; row < popupShadowSize; row++) {
		int fromBottom = popupShadowSize - 1 - row;
		int y = panel.bottom + row;
		for (int column = 0; column < width; column++) {
			int alpha;
			if (column < 4) {
				alpha = 3 * (column + 1) * (fromBottom + 1);
			}
			else if (column >= width - 4) {
				alpha = 3 * (width - column) * (fromBottom + 1);
			}
			else {
				alpha = 0x0F * (fromBottom + 1);
			}
			fill(painter, panel.left + popupShadowSize + column, y, 1, 1, QColor(0, 0, 0, alpha));
		}
	}
}

QIcon::Mode iconMode(bool enabled, bool active)
{
	return !enabled ? QIcon::Disabled : active ? QIcon::Active : QIcon::Normal;
}

enum class StretchRule
{
	Popup,
	Centred,
};

int stretchSource(int target, int sourceExtent, int targetExtent, StretchRule rule = StretchRule::Popup)
{
	if (targetExtent > sourceExtent && rule == StretchRule::Popup) {
		return ((target + 1) * sourceExtent - 1) / targetExtent;
	}
	return std::min((2 * target + 1) * sourceExtent / (2 * targetExtent), sourceExtent - 1);
}

QImage stretchImage(const QImage& source, const QSize& size, StretchRule rule)
{
	QImage stretched(size, QImage::Format_ARGB32_Premultiplied);
	for (int y = 0; y < stretched.height(); y++) {
		const QRgb* from = reinterpret_cast<const QRgb*>(
			source.constScanLine(stretchSource(y, source.height(), stretched.height(), rule))
		);
		QRgb* to = reinterpret_cast<QRgb*>(stretched.scanLine(y));
		for (int x = 0; x < stretched.width(); x++) {
			to[x] = from[stretchSource(x, source.width(), stretched.width(), rule)];
		}
	}
	return stretched;
}

void drawStretchedIcon(
	QPainter* painter,
	const QIcon& icon,
	const QRect& target,
	QIcon::Mode mode,
	QIcon::State state,
	StretchRule rule = StretchRule::Popup
)
{
	QSize sourceSize = target.size();
	for (const QSize& available : icon.availableSizes(QIcon::Normal, state)) {
		if (available.width() >= target.width() && available.height() >= target.height()) {
			sourceSize = available;
			break;
		}
	}

	QPixmap pixmap = icon.pixmap(sourceSize, 1.0, mode, state);
	if (pixmap.size() == target.size()) {
		painter->drawPixmap(target.topLeft(), pixmap);
		return;
	}

	QImage source = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
	painter->drawImage(target.topLeft(), stretchImage(source, target.size(), rule));
}

} // namespace

#ifndef _WIN32
namespace {

class AltKeyTracker : public QObject
{
public:
	using QObject::QObject;

	bool eventFilter(QObject* object, QEvent* event) override
	{
		if (!object->isWidgetType()) {
			return false;
		}

		QWidget* widget = static_cast<QWidget*>(object);
		switch (event->type()) {
		case QEvent::KeyPress:
			if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Alt) {
				QWidget* window = widget->window();
				if (!seenAlt.contains(window)) {
					seenAlt.append(window);
				}
				altDown = true;
				for (QWidget* child : window->findChildren<QWidget*>()) {
					if (!child->isWindow() && child->isVisible()) {
						child->update();
					}
				}
			}
			break;
		case QEvent::KeyRelease:
			if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Alt) {
				altDown = false;
				for (QMenuBar* menuBar : widget->window()->findChildren<QMenuBar*>()) {
					menuBar->update();
				}
			}
			break;
		case QEvent::Close:
			seenAlt.removeAll(widget);
			seenAlt.removeAll(widget->window());
			break;
		default:
			break;
		}
		return false;
	}

	static bool altDown;
	static QList<const QWidget*> seenAlt;
};

bool AltKeyTracker::altDown = false;
QList<const QWidget*> AltKeyTracker::seenAlt;

} // namespace
#endif

WhidbeyStyle::WhidbeyStyle()
	: QProxyStyle(
		  QStyleFactory::keys().contains(QStringLiteral("windowsvista"), Qt::CaseInsensitive)
			  ? QStringLiteral("windowsvista")
			  : QStringLiteral("fusion")
	  )
{
}

const WhidbeyColors& WhidbeyStyle::colors()
{
	return colorCache();
}

QPalette WhidbeyStyle::systemPalette()
{
	const WhidbeyColors& c = colors();
	QPalette palette;
	palette.setColor(QPalette::Window, c.face);
	palette.setColor(QPalette::WindowText, c.windowText);
	palette.setColor(QPalette::Base, c.window);
	palette.setColor(QPalette::AlternateBase, c.face);
	palette.setColor(QPalette::Text, c.windowText);
	palette.setColor(QPalette::Button, c.face);
	palette.setColor(QPalette::ButtonText, c.buttonText);
	palette.setColor(QPalette::Light, c.buttonHighlight);
	palette.setColor(QPalette::Midlight, c.light);
	palette.setColor(QPalette::Mid, c.shadow);
	palette.setColor(QPalette::Dark, c.shadow);
	palette.setColor(QPalette::Shadow, c.darkShadow);
	palette.setColor(QPalette::Highlight, c.highlight);
	palette.setColor(QPalette::HighlightedText, c.highlightText);
	palette.setColor(QPalette::ToolTipBase, c.infoBackground);
	palette.setColor(QPalette::ToolTipText, c.infoText);
	palette.setColor(QPalette::Link, c.hotLight);
	palette.setColor(QPalette::Disabled, QPalette::WindowText, c.grayText);
	palette.setColor(QPalette::Disabled, QPalette::Text, c.grayText);
	palette.setColor(QPalette::Disabled, QPalette::ButtonText, c.grayText);
	return palette;
}

QFont WhidbeyStyle::commandBarFont()
{
	return clampedFont(QApplication::font("QMenu"));
}

QFont WhidbeyStyle::iconTitleFont()
{
	return QApplication::font("QAbstractItemView");
}

QFont WhidbeyStyle::paneCaptionFont()
{
	QFont font = QApplication::font("QDockWidgetTitle");
	font.setBold(false);
	return font;
}

static int alphaBlendTerm(int value, int weight)
{
	return ((value + (value >> 7)) * weight) >> 8;
}

void WhidbeyStyle::drawAlphaImage(QPainter* painter, const QPoint& position, const QImage& image)
{
	QImage* target = dynamic_cast<QImage*>(painter->paintEngine()->paintDevice());
	const QTransform& transform = painter->deviceTransform();
	if (!target || transform.type() > QTransform::TxTranslate || image.format() != QImage::Format_ARGB32) {
		painter->drawImage(position, image);
		return;
	}

	QPoint origin = transform.map(position);
	QImage blended(image.size(), QImage::Format_ARGB32);
	for (int y = 0; y < image.height(); y++) {
		const QRgb* source = reinterpret_cast<const QRgb*>(image.constScanLine(y));
		QRgb* result = reinterpret_cast<QRgb*>(blended.scanLine(y));
		for (int x = 0; x < image.width(); x++) {
			int alpha = qAlpha(source[x]);
			QPoint device(origin.x() + x, origin.y() + y);
			if (alpha == 0 || !target->rect().contains(device)) {
				result[x] = alpha == 0 ? 0 : source[x];
				continue;
			}
			QRgb under = target->pixel(device);
			result[x] = qRgb(
				alphaBlendTerm(qRed(source[x]), alpha) + alphaBlendTerm(qRed(under), 255 - alpha),
				alphaBlendTerm(qGreen(source[x]), alpha) + alphaBlendTerm(qGreen(under), 255 - alpha),
				alphaBlendTerm(qBlue(source[x]), alpha) + alphaBlendTerm(qBlue(under), 255 - alpha)
			);
		}
	}
	painter->drawImage(position, blended);
}

QPixmap WhidbeyStyle::premultipliedPixmap(const QImage& source)
{
	QImage straight = source.convertToFormat(QImage::Format_ARGB32);
	QImage premultiplied(straight.size(), QImage::Format_ARGB32_Premultiplied);
	for (int y = 0; y < straight.height(); y++) {
		const QRgb* from = reinterpret_cast<const QRgb*>(straight.constScanLine(y));
		QRgb* to = reinterpret_cast<QRgb*>(premultiplied.scanLine(y));
		for (int x = 0; x < straight.width(); x++) {
			int alpha = qAlpha(from[x]);
			to[x] =
				qRgba(qRed(from[x]) * alpha / 255, qGreen(from[x]) * alpha / 255, qBlue(from[x]) * alpha / 255, alpha);
		}
	}
	return QPixmap::fromImage(premultiplied);
}

QRect WhidbeyStyle::toolBarExpandRect(const QToolBar* toolBar)
{
	bool horizontal = toolBar->orientation() == Qt::Horizontal;
	bool extensionVisible = false;
	Box rc(toolBar->rect());
	int extent = (horizontal ? rc.left : rc.top) + toolBarExtent(toolBar, horizontal, &extensionVisible);
	if (!showsExpandAlways(toolBar)) {
		return QRect();
	}
	if (horizontal) {
		Box bar(rc.left + topDockInset(toolBar), rc.top, extent, rc.bottom);
		return Box(bar.right - toolBarExpandWidth + 1, bar.top + 1, bar.right + 1, bar.bottom).toRect();
	}
	Box bar(rc.left, rc.top, rc.right, extent);
	return Box(bar.left, bar.bottom - toolBarExpandWidth, bar.right, bar.bottom).toRect();
}

static qsizetype mnemonicIndex(const QString& text)
{
	for (qsizetype i = 0; i + 1 < text.size(); i++) {
		if (text[i] == QLatin1Char('&')) {
			if (text[i + 1] != QLatin1Char('&')) {
				return i;
			}
			i++;
		}
	}
	return -1;
}

static QString stripMnemonics(QString text)
{
	text.replace(QLatin1String("&&"), QString(QChar::Null));
	text.remove(QLatin1Char('&'));
	return text.replace(QChar(QChar::Null), QLatin1Char('&'));
}

static QString removeAmpersandEscapes(QString text)
{
	return text.replace(QStringLiteral("&&"), QStringLiteral("&"));
}

static void drawTextRuns(QPainter* painter, const QRect& line, int flags, const QString& text)
{
	qsizetype mnemonic = mnemonicIndex(text);
	if (!(flags & Qt::TextShowMnemonic) || (flags & Qt::TextHideMnemonic) || mnemonic < 0) {
		painter->drawText(line, flags, text);
		return;
	}
	QString before = removeAmpersandEscapes(text.left(mnemonic));
	QString plain = before + removeAmpersandEscapes(text.mid(mnemonic + 1));
	QRectF bounds;
	painter->drawText(QRectF(line), flags & ~(Qt::TextShowMnemonic | Qt::TextHideMnemonic), plain, &bounds);

	QTextLayout layout(plain, painter->font(), painter->device());
	layout.beginLayout();
	QTextLine textLine = layout.createLine();
	layout.endLayout();
	QFontMetricsF metrics(painter->font(), painter->device());
	QFontMetrics integerMetrics(painter->font(), painter->device());
	int left = qFloor(bounds.left() + textLine.cursorToX(before.size()));
	int right = qFloor(bounds.left() + textLine.cursorToX(before.size() + 1));
	int underline = qRound(bounds.top() + metrics.ascent()) + integerMetrics.underlinePos();
	painter->fillRect(
		QRect(left, underline, right - left, std::max(1, integerMetrics.lineWidth())), painter->pen().color()
	);
}

void WhidbeyStyle::drawControlRectangle(QPainter* painter, const QRect& rect, bool selected, bool pressed)
{
	drawRectangle(painter, Box(rect), selected, pressed, true, false, false, false);
}

void WhidbeyStyle::drawText(QPainter* painter, const QRect& rect, int flags, const QString& text)
{
	painter->save();
	painter->setClipRect(rect, Qt::IntersectClip);
	QRect line = rect;
	if (flags & Qt::AlignVCenter) {
		int height = painter->fontMetrics().height();
		line = QRect(rect.x(), rect.y() + (rect.height() - height) / 2, rect.width(), height);
		flags = (flags & ~Qt::AlignVCenter) | Qt::AlignTop;
	}
	drawTextRuns(painter, line, flags, text);
	painter->restore();
}

QColor WhidbeyStyle::themeColor(const wchar_t* themeClass, int part, int state, int property, const QColor& fallback)
{
#ifdef _WIN32
	using OpenThemeDataFunction = HANDLE(WINAPI*)(HWND, LPCWSTR);
	using CloseThemeDataFunction = HRESULT(WINAPI*)(HANDLE);
	using GetThemeColorFunction = HRESULT(WINAPI*)(HANDLE, int, int, int, COLORREF*);
	using IsAppThemedFunction = BOOL(WINAPI*)();

	static HMODULE module = LoadLibraryW(L"uxtheme.dll");
	if (module == nullptr) {
		return fallback;
	}
	static auto openThemeData = reinterpret_cast<OpenThemeDataFunction>(GetProcAddress(module, "OpenThemeData"));
	static auto closeThemeData = reinterpret_cast<CloseThemeDataFunction>(GetProcAddress(module, "CloseThemeData"));
	static auto getThemeColor = reinterpret_cast<GetThemeColorFunction>(GetProcAddress(module, "GetThemeColor"));
	static auto isAppThemed = reinterpret_cast<IsAppThemedFunction>(GetProcAddress(module, "IsAppThemed"));
	if (!openThemeData || !closeThemeData || !getThemeColor || !isAppThemed || !isAppThemed()) {
		return fallback;
	}
	QColor result = fallback;
	if (HANDLE theme = openThemeData(nullptr, themeClass)) {
		COLORREF color = 0;
		if (SUCCEEDED(getThemeColor(theme, part, state, property, &color))) {
			result = QColor(GetRValue(color), GetGValue(color), GetBValue(color));
		}
		closeThemeData(theme);
	}
	return result;
#else
	return fallback;
#endif
}

bool WhidbeyStyle::drawThemeBackground(
	QPainter* painter, const QRect& rect, const wchar_t* themeClass, int part, int state, const QColor& background
)
{
#ifdef _WIN32
	using OpenThemeDataFunction = HANDLE(WINAPI*)(HWND, LPCWSTR);
	using CloseThemeDataFunction = HRESULT(WINAPI*)(HANDLE);
	using DrawThemeBackgroundFunction = HRESULT(WINAPI*)(HANDLE, HDC, int, int, const RECT*, const RECT*);
	using IsAppThemedFunction = BOOL(WINAPI*)();

	static HMODULE module = LoadLibraryW(L"uxtheme.dll");
	if (module == nullptr || rect.isEmpty()) {
		return false;
	}
	static auto openThemeData = reinterpret_cast<OpenThemeDataFunction>(GetProcAddress(module, "OpenThemeData"));
	static auto closeThemeData = reinterpret_cast<CloseThemeDataFunction>(GetProcAddress(module, "CloseThemeData"));
	static auto drawBackground =
		reinterpret_cast<DrawThemeBackgroundFunction>(GetProcAddress(module, "DrawThemeBackground"));
	static auto isAppThemed = reinterpret_cast<IsAppThemedFunction>(GetProcAddress(module, "IsAppThemed"));
	if (!openThemeData || !closeThemeData || !drawBackground || !isAppThemed || !isAppThemed()) {
		return false;
	}
	HANDLE theme = openThemeData(nullptr, themeClass);
	if (theme == nullptr) {
		return false;
	}

	BITMAPINFO info = {};
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = rect.width();
	info.bmiHeader.biHeight = -rect.height();
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;
	void* bits = nullptr;
	HDC dc = CreateCompatibleDC(nullptr);
	HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
	bool drawn = false;
	if (bitmap != nullptr) {
		HGDIOBJ previous = SelectObject(dc, bitmap);
		QImage image(static_cast<uchar*>(bits), rect.width(), rect.height(), QImage::Format_RGB32);
		image.fill(background);
		RECT area = {0, 0, rect.width(), rect.height()};
		if (SUCCEEDED(drawBackground(theme, dc, part, state, &area, nullptr))) {
			GdiFlush();
			painter->drawImage(rect.topLeft(), image);
			drawn = true;
		}
		SelectObject(dc, previous);
		DeleteObject(bitmap);
	}
	DeleteDC(dc);
	closeThemeData(theme);
	return drawn;
#else
	return false;
#endif
}

QSize WhidbeyStyle::listBoxControlSize()
{
	QFontMetrics metrics(commandBarFont());
	return QSize(popupImageWidth(metrics), std::max(16 + 6, commandBarTextHeight(metrics)));
}

void WhidbeyStyle::drawListBoxControlBackground(QPainter* painter, const QRect& rect)
{
	const WhidbeyColors& c = colors();
	Box rc(rect);
	fill(painter, rc, c.menuBarFace);
	gradientFill(
		painter, Box(0, rc.top, listBoxControlSize().width() + 1, rc.bottom), c.commandBarLight, c.commandBarDark, true
	);
}

void WhidbeyStyle::drawListBoxControl(
	QPainter* painter, const QRect& rect, const QIcon& icon, const QString& text, bool popup, bool selected
)
{
	const WhidbeyColors& c = colors();
	QSize image = listBoxControlSize();
	Box rc(rect);
	drawListBoxControlBackground(painter, rect);
	if (selected) {
		rectangle(painter, rc, c.highlightBorder, c.highlightFace);
	}

	if (!icon.isNull() && !popup) {
		QRect iconRect(rc.left + image.width() / 2 - 8, rc.top + rc.height() / 2 - 8, 16, 16);
		drawStretchedIcon(painter, icon, iconRect, iconMode(true, selected), QIcon::Off);
	}

	QColor textColor = rectangleTextColor(selected, false, true, false, false, true);
	QRect label(
		rc.left + image.width() + popupTextPadding,
		rc.top,
		rc.right - (rc.left + image.width() + popupTextPadding),
		rc.height()
	);
	if (popup) {
		QPoint point(rc.right - 7, rc.centerY());
		drawTriangle(
			painter, point, QPoint(point.x() - 5, point.y() - 5), QPoint(point.x() - 5, point.y() + 5), textColor
		);
		label.setWidth(label.width() - 7);
	}

	QString caption = stripMnemonics(text);
	painter->save();
	painter->setFont(commandBarFont());
	painter->setPen(textColor);
	drawText(
		painter,
		label,
		Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
		endEllipsis(commandBarFont(), caption, label.width())
	);
	painter->restore();
}

QString WhidbeyStyle::endEllipsis(const QFont& font, const QString& text, int width)
{
	QFont measured = font;
	measured.setKerning(false);
	QFontMetrics metrics(measured);
	if (metrics.horizontalAdvance(text) <= width) {
		return text;
	}

	const QString ellipsis = QStringLiteral("...");
	int ellipsisWidth = metrics.horizontalAdvance(ellipsis);
	qsizetype length = text.size();
	while (length > 1 && metrics.horizontalAdvance(text.left(length)) + ellipsisWidth > width) {
		length--;
	}
	return text.left(length) + ellipsis;
}

void WhidbeyStyle::drawClientEdge(QPainter* painter, const QRect& rect)
{
	const WhidbeyColors& c = colors();
	Box outer(rect);
	draw3dRect(painter, outer, c.shadow, c.buttonHighlight);
	Box inner(rect.adjusted(1, 1, -1, -1));
	draw3dRect(painter, inner, c.darkShadow, c.buttonLight);
}

static int effectiveMenuAnimation()
{
	switch (menuAnimation) {
	case MenuAnimationWindowsDefault: {
#ifdef _WIN32
		BOOL enabled = FALSE;
		if (!SystemParametersInfoW(SPI_GETMENUANIMATION, 0, &enabled, 0) || !enabled) {
			return MenuAnimationNone;
		}
		BOOL fade = FALSE;
		if (!SystemParametersInfoW(SPI_GETMENUFADE, 0, &fade, 0)) {
			return MenuAnimationSlide;
		}
		return fade ? MenuAnimationFade : MenuAnimationSlide;
#else
		return MenuAnimationNone;
#endif
	}
	case MenuAnimationRandom: {
		static const int choices[] = {MenuAnimationFade, MenuAnimationSlide, MenuAnimationUnfold};
		return choices[QRandomGenerator::global()->bounded(3)];
	}
	default:
		return menuAnimation;
	}
}

void WhidbeyStyle::setMenuAnimation(int animation)
{
	menuAnimation = animation;
}

int WhidbeyStyle::popupGripperWidth(const QWidget* menu)
{
	return popupImageWidth(menu->fontMetrics()) * gripperColumns(menu);
}

void WhidbeyStyle::setCustomizeMode(bool customize)
{
	customizing = customize;
	setCustomizeSelection(nullptr);
}

bool WhidbeyStyle::customizeMode()
{
	return customizing;
}

class CustomizeSelectionFrame : public QWidget
{
public:
	explicit CustomizeSelectionFrame(QWidget* parent) : QWidget(parent)
	{
		setAttribute(Qt::WA_TransparentForMouseEvents);
		setObjectName(QStringLiteral("CustomizeSelectionFrame"));
	}

	QRect frame;

protected:
	void paintEvent(QPaintEvent* event) override
	{
		QPainter painter(this);
		painter.setPen(Qt::black);
		painter.setBrush(Qt::NoBrush);
		painter.drawRect(frame.adjusted(0, 0, -1, -1));
		painter.drawRect(frame.adjusted(1, 1, -2, -2));
	}
};

static QWidget* selectionHost(QWidget* control)
{
	return control->isWindow() ? control : control->parentWidget();
}

static void hideSelectionFrame(QWidget* control)
{
	if (control && selectionHost(control)) {
		if (QWidget* overlay = selectionHost(control)->findChild<QWidget*>(
				QStringLiteral("CustomizeSelectionFrame"), Qt::FindDirectChildrenOnly
			)) {
			overlay->hide();
		}
	}
}

void WhidbeyStyle::setCustomizeSelection(QWidget* control, QRect frame)
{
	if (customizeSelected == control && (!control || qobject_cast<QToolButton*>(control))) {
		return;
	}
	if (customizeSelected) {
		hideSelectionFrame(customizeSelected);
		if (customizeSelected != control) {
			customizeSelected->setProperty("customizeAction", QVariant());
		}
		customizeSelected->update();
	}
	customizeSelected = control;
	if (!control) {
		return;
	}
	control->update();
	if (!qobject_cast<QToolButton*>(control) && control->parentWidget()) {
		QWidget* parent = selectionHost(control);
		CustomizeSelectionFrame* overlay = static_cast<CustomizeSelectionFrame*>(
			parent->findChild<QWidget*>(QStringLiteral("CustomizeSelectionFrame"), Qt::FindDirectChildrenOnly)
		);
		if (!overlay) {
			overlay = new CustomizeSelectionFrame(parent);
		}
		overlay->frame = frame.isEmpty() ? control->geometry() : frame;
		overlay->setGeometry(parent->rect());
		overlay->raise();
		overlay->show();
		overlay->update();
	}
}

QWidget* WhidbeyStyle::customizeSelection()
{
	return customizeSelected;
}

static void stopMenuAnimation(QMenu* menu)
{
	if (auto it = menuAnimations.find(menu); it != menuAnimations.end()) {
		delete it->second->timer;
		delete it->second;
		menuAnimations.erase(it);
	}
}

static void runMenuAnimation(QMenu* menu, MenuAnimationState* state)
{
	stopMenuAnimation(menu);
	state->timer = new QTimer(menu);
	state->timer->setInterval(menuAnimationTime / menuAnimationSteps);
	QObject::connect(state->timer, &QTimer::timeout, menu, [menu, state]() {
		if (state->elapsed.elapsed() >= menuAnimationTime) {
			state->timer->stop();
			state->timer->deleteLater();
			menuAnimations.erase(menu);
			delete state;
		}
		menu->update();
	});
	state->elapsed.start();
	state->timer->start();
	menuAnimations[menu] = state;
}

void WhidbeyStyle::animateMenuExpansion(QMenu* menu, const QRect& previous)
{
	if (effectiveMenuAnimation() == MenuAnimationNone) {
		return;
	}

	MenuAnimationState* state = new MenuAnimationState;
	state->expanding = true;
	state->expandDown = previous.bottom() < menu->geometry().bottom();
	bool current = false;
	int top = 0;
	for (QAction* action : menu->actions()) {
		if (!action->isVisible()) {
			continue;
		}
		bool expanded = action->property("expandedItem").toBool();
		if (expanded != current) {
			int boundary = menu->actionGeometry(action).top();
			state->segments.push_back({top, boundary, current});
			top = boundary;
			current = expanded;
		}
	}
	state->segments.push_back({top, menu->height(), current});
	if (QScreen* screen = menu->screen()) {
		state->background = screen->grabWindow(0, menu->x(), menu->y(), menu->width(), menu->height()).toImage();
	}
	runMenuAnimation(menu, state);
}

void WhidbeyStyle::startMenuAnimation(QMenu* menu)
{
	QWidget* parent = menu->parentWidget();
	bool commandBarParent =
		qobject_cast<QMenuBar*>(parent) || qobject_cast<QMenu*>(parent) || qobject_cast<QToolButton*>(parent);
	bool animate = (!commandBarParent || parent->property(animatePopupProperty).toBool()) &&
				   !(customizing && menu->property("customizable").toBool());
	if (commandBarParent) {
		parent->setProperty(animatePopupProperty, false);
	}
	menu->setProperty(animatePopupProperty, menu->activeAction() == nullptr);

	int type = animate ? effectiveMenuAnimation() : MenuAnimationNone;
	if (type == MenuAnimationNone) {
		return;
	}

	MenuAnimationState* state = new MenuAnimationState;
	state->type = type;
	if (QScreen* screen = menu->screen()) {
		state->background = screen->grabWindow(0, menu->x(), menu->y(), menu->width(), menu->height()).toImage();
	}
	runMenuAnimation(menu, state);
}

bool WhidbeyStyle::paintMenuAnimation(QMenu* menu)
{
	auto it = menuAnimations.find(menu);
	if (it == menuAnimations.end() || it->second->grabbing) {
		return false;
	}
	MenuAnimationState* state = it->second;
	if (state->content.isNull()) {
		state->grabbing = true;
		state->content = menu->grab();
		state->grabbing = false;
	}

	int width = menu->width();
	int height = menu->height();
	int step = std::min(
		static_cast<int>(state->elapsed.elapsed() * menuAnimationSteps / menuAnimationTime), menuAnimationSteps - 1
	);
	QPainter painter(menu);
	if (!state->background.isNull()) {
		painter.drawImage(0, 0, state->background);
	}
	if (state->expanding) {
		double percent = static_cast<double>(step + 1) / menuAnimationSteps;
		int offset = 0;
		if (state->expandDown) {
			for (const MenuSegment& segment : state->segments) {
				int source = segment.bottom - segment.top;
				int target = static_cast<int>((segment.expanded ? percent : 1.0) * source);
				painter.drawPixmap(
					QRect(0, offset, width, target), state->content, QRect(0, segment.top, width, source)
				);
				offset += target;
			}
		}
		else {
			for (auto segment = state->segments.rbegin(); segment != state->segments.rend(); ++segment) {
				int source = segment->bottom - segment->top;
				int target = static_cast<int>((segment->expanded ? percent : 1.0) * source);
				painter.drawPixmap(
					QRect(0, height - offset - target, width, target),
					state->content,
					QRect(0, segment->top, width, source)
				);
				offset += target;
			}
		}
		return true;
	}
	switch (state->type) {
	case MenuAnimationSlide: {
		int visible = height * (step + 1) / menuAnimationSteps;
		painter.drawPixmap(0, 0, state->content, 0, height - visible, width, visible);
		break;
	}
	case MenuAnimationUnfold: {
		int visibleWidth = width * (step + 1) / menuAnimationSteps;
		int visibleHeight = height * (step + 1) / menuAnimationSteps;
		painter.drawPixmap(
			0, 0, state->content, width - visibleWidth, height - visibleHeight, visibleWidth, visibleHeight
		);
		break;
	}
	case MenuAnimationFade:
		painter.setOpacity(static_cast<qreal>(std::max(step, 1)) / menuAnimationSteps);
		painter.drawPixmap(0, 0, state->content);
		break;
	default:
		break;
	}
	return true;
}

void WhidbeyStyle::setPopupShadowsEnabled(bool enabled)
{
	popupShadows = enabled;
}

void WhidbeyStyle::fillHorizontalGradient(QPainter* painter, const QRect& rect, const QColor& left, const QColor& right)
{
	gradientFill(painter, Box(rect), left, right, true);
}

int WhidbeyStyle::paneCaptionHeight()
{
	return std::max(13, QFontMetrics(paneCaptionFont()).height()) + 5;
}

QRect WhidbeyStyle::paneCaptionButtonRect(const QRect& caption, int index)
{
	Box rc(caption.x(), caption.y(), caption.x() + caption.width(), caption.y() + paneCaptionHeight() + 1);
	int right = rc.right - 2 - index * (paneCaptionButtonSize + paneCaptionButtonGap);
	return QRect(
		right - paneCaptionButtonSize,
		rc.centerY() - paneCaptionButtonSize / 2,
		paneCaptionButtonSize,
		paneCaptionButtonSize
	);
}

void WhidbeyStyle::drawPaneCaption(
	QPainter* painter, const QRect& caption, const QString& title, bool active, bool enabled, int buttonCount
)
{
	const WhidbeyColors& c = colors();
	Box rc(caption.x(), caption.y(), caption.x() + caption.width(), caption.y() + paneCaptionHeight() + 1);

	painter->save();
	painter->setClipRect(caption, Qt::IntersectClip);
	fill(painter, rc, active ? c.paneCaptionActive : c.paneCaptionInactive);
	fill(painter, rc.left, rc.top + 1, 1, rc.height() - 1, c.paneCaptionBorder);
	fill(painter, rc.left, rc.top, rc.width() - 1, 1, c.paneCaptionBorder);
	fill(painter, rc.right - 1, rc.top, 1, rc.height() - 1, c.paneCaptionBorder);

	int textRight = rc.right - 2 - buttonCount * (paneCaptionButtonSize + paneCaptionButtonGap);
	QRect textRect(rc.left + 4, rc.top, textRight - rc.left - 4, rc.height() - 1);
	if (!title.isEmpty() && textRect.width() > 0) {
		QFont font = paneCaptionFont();
		painter->setFont(font);
		painter->setPen(!enabled ? c.grayText : active ? c.paneCaptionActiveText : c.paneCaptionInactiveText);
		painter->drawText(
			textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, endEllipsis(font, title, textRect.width())
		);
	}
	painter->restore();
}

void WhidbeyStyle::drawPaneCaptionButton(
	QPainter* painter, const QRect& rect, PaneCaptionButton button, bool active, bool hot, bool pressed, bool autoHidden
)
{
	const WhidbeyColors& c = colors();
	Box rc(rect);
	QPoint point(rc.centerX(), rc.centerY());
	QColor color = active ? c.paneCaptionActiveText : c.paneCaptionInactiveText;

	if (pressed) {
		draw3dRect(painter, rc, c.buttonText, c.buttonHighlight);
		point += QPoint(1, 1);
	}
	else if (hot) {
		draw3dRect(painter, rc, c.buttonHighlight, c.buttonText);
	}

	int x = point.x();
	int y = point.y();
	if (button == PaneCaptionClose) {
		drawLine(painter, x - 4, y - 4, x + 3, y + 3, color);
		drawLine(painter, x - 4, y + 2, x + 3, y - 5, color);
		drawLine(painter, x - 3, y - 4, x + 4, y + 3, color);
		drawLine(painter, x - 3, y + 2, x + 4, y - 5, color);
		return;
	}

	y--;
	if (autoHidden) {
		drawLine(painter, x - 4, y, x - 1, y, color);
		drawLine(painter, x - 1, y - 3, x - 1, y + 4, color);
		drawLine(painter, x - 1, y + 2, x + 3, y + 2, color);
		drawLine(painter, x + 3, y + 2, x + 3, y - 2, color);
		drawLine(painter, x + 3, y - 2, x - 1, y - 2, color);
		drawLine(painter, x - 1, y + 1, x + 3, y + 1, color);
	}
	else {
		y += 2;
		drawLine(painter, x - 3, y - 1, x + 4, y - 1, color);
		drawLine(painter, x, y - 1, x, y + 3, color);
		drawLine(painter, x - 2, y - 1, x - 2, y - 6, color);
		drawLine(painter, x - 2, y - 6, x + 2, y - 6, color);
		drawLine(painter, x + 2, y - 6, x + 2, y - 1, color);
		drawLine(painter, x + 1, y - 1, x + 1, y - 6, color);
	}
}

QSize WhidbeyStyle::tabNavigateButtonSize()
{
	return QSize(14, 15);
}

int WhidbeyStyle::workspaceHeaderHeight(const QFontMetrics& metrics)
{
	return workspaceButtonHeight(metrics) + 3;
}

void WhidbeyStyle::drawWorkspaceHeader(QPainter* painter, const QRect& rect)
{
	const WhidbeyColors& c = colors();
	Box rc(rect);
	fill(painter, rc, c.tabHeaderFace);
	fill(painter, rc.left, rc.bottom - 1, rc.width(), 1, c.tabFrameBorder);
}

void WhidbeyStyle::drawTabNavigateButton(
	QPainter* painter, const QRect& rect, TabNavigateButton button, bool enabled, bool hot, bool pressed
)
{
	const WhidbeyColors& c = colors();
	Box rc(rect);
	QColor color = c.buttonText;

	if (enabled) {
		if (hot && pressed) {
			rectangle(painter, rc, c.highlightPushedBorder, c.highlightPushed);
			color = c.pushedText;
		}
		else if (hot || pressed) {
			rectangle(painter, rc, c.highlightBorder, c.highlightFace);
			color = c.highlightedText;
		}
	}

	int x = rc.centerX();
	int y = rc.centerY();
	switch (button) {
	case TabNavigateClose:
		drawLine(painter, x - 4, y - 3, x + 3, y + 4, color);
		drawLine(painter, x - 3, y - 3, x + 4, y + 4, color);
		drawLine(painter, x - 4, y + 3, x + 3, y - 4, color);
		drawLine(painter, x - 3, y + 3, x + 4, y - 4, color);
		break;
	case TabNavigateRight:
		drawTriangle(painter, QPoint(x - 2, y - 5), QPoint(x + 2, y - 1), QPoint(x - 2, y + 3), color, enabled);
		break;
	case TabNavigateLeft:
		drawTriangle(painter, QPoint(x + 2, y - 5), QPoint(x - 2, y - 1), QPoint(x + 2, y + 3), color, enabled);
		break;
	}
}

#ifndef _WIN32
void WhidbeyStyle::polish(QApplication* application)
{
	QProxyStyle::polish(application);
	application->installEventFilter(new AltKeyTracker(application));
}
#endif

void WhidbeyStyle::polish(QWidget* widget)
{
	QProxyStyle::polish(widget);

	if (QToolBar* toolBar = qobject_cast<QToolBar*>(widget)) {
		updateToolBarMargins(toolBar);
		connect(toolBar, &QToolBar::orientationChanged, this, [toolBar](Qt::Orientation) {
			updateToolBarMargins(toolBar);
		});
		if (QWidget* extension = toolBarExtension(toolBar)) {
			extension->installEventFilter(this);
		}
	}
	else if (qobject_cast<QMenuBar*>(widget) || (qobject_cast<QToolButton*>(widget) && inToolBar(widget))) {
		widget->installEventFilter(this);
	}
	else if (QMenu* menu = qobject_cast<QMenu*>(widget)) {
		menu->installEventFilter(this);
		int extraTop = isToolBarPopup(menu) ? 2 : 0;
		if (popupShadows && !menu->testAttribute(Qt::WA_WState_Created)) {
			menu->setWindowFlag(Qt::NoDropShadowWindowHint, true);
			menu->setAttribute(Qt::WA_TranslucentBackground, true);
			menu->setContentsMargins(0, extraTop, popupShadowSize, extraTop + popupShadowSize);
		}
		else if (extraTop) {
			menu->setContentsMargins(0, extraTop, 0, extraTop);
		}
	}
	else if (QComboBox* combo = qobject_cast<QComboBox*>(widget)) {
		if (inToolBar(combo)) {
			combo->setAttribute(Qt::WA_Hover, true);
		}
	}
	else if (QLabel* label = qobject_cast<QLabel*>(widget)) {
		if (inToolBar(label)) {
			label->setContentsMargins(toolBarLabelInset, 0, 4, 0);
		}
	}
}

void WhidbeyStyle::unpolish(QWidget* widget)
{
	if (QToolBar* toolBar = qobject_cast<QToolBar*>(widget)) {
		disconnect(toolBar, nullptr, this, nullptr);
		toolBar->setContentsMargins(0, 0, 0, 0);
		if (QWidget* extension = toolBarExtension(toolBar)) {
			extension->removeEventFilter(this);
		}
	}
	else if (QMenu* menu = qobject_cast<QMenu*>(widget)) {
		if (menuHasShadow(menu)) {
			menu->setContentsMargins(0, 0, 0, 0);
			menu->setAttribute(Qt::WA_TranslucentBackground, false);
		}
	}
	else if (QLabel* label = qobject_cast<QLabel*>(widget)) {
		if (inToolBar(label)) {
			label->setContentsMargins(0, 0, 0, 0);
		}
	}

	QProxyStyle::unpolish(widget);
}

bool WhidbeyStyle::eventFilter(QObject* object, QEvent* event)
{
	switch (event->type()) {
	case QEvent::MouseButtonPress:
		if (QMenuBar* bar = qobject_cast<QMenuBar*>(object); bar && !QApplication::activePopupWidget()) {
			bar->setProperty(animatePopupProperty, true);
		}
		else if (QToolButton* button = qobject_cast<QToolButton*>(object)) {
			button->setProperty(animatePopupProperty, true);
		}
		break;
	case QEvent::Paint:
		if (QMenu* menu = qobject_cast<QMenu*>(object); menu && paintMenuAnimation(menu)) {
			return true;
		}
		break;
	case QEvent::Hide:
		if (QMenu* menu = qobject_cast<QMenu*>(object)) {
			stopMenuAnimation(menu);
			qDeleteAll(menu->findChildren<QWidget*>(QLatin1String(popupControlShadowName), Qt::FindDirectChildrenOnly));
		}
		break;
	case QEvent::Move:
	case QEvent::Resize:
	case QEvent::Show:
		if (QMenu* menu = qobject_cast<QMenu*>(object)) {
			QRect control = popupControlRect(menu);
			if (control.isValid() && menu->y() == control.bottom() + 2) {
				menu->move(menu->x(), control.bottom() + 1);
			}
			if (QMenu* parentMenu = qobject_cast<QMenu*>(menu->parentWidget())) {
				QRect itemRect = parentMenu->actionGeometry(menu->menuAction());
				int frame = pixelMetric(PM_MenuPanelWidth, nullptr, menu);
				int itemTop = parentMenu->mapToGlobal(itemRect.topLeft()).y();
				if (itemRect.isValid() && menu->y() == itemTop - frame) {
					menu->move(menu->x(), itemTop);
				}
			}
			if (event->type() == QEvent::Show) {
				startMenuAnimation(menu);
			}
			if (event->type() == QEvent::Show && control.isValid() && menuHasShadow(menu) &&
				qobject_cast<QMenuBar*>(menu->parentWidget())) {
				PopupControlShadow* shadow = new PopupControlShadow(menu);
				shadow->setGeometry(
					control.right() + 1, control.top() + popupShadowSize, popupShadowSize, control.height() - 3
				);
				shadow->show();
			}
			break;
		}
		if (object->isWidgetType()) {
			QWidget* extension = static_cast<QWidget*>(object);
			if (QToolBar* toolBar = qobject_cast<QToolBar*>(extension->parentWidget());
				toolBar && toolBarExtension(toolBar) == extension) {
				QRect wanted =
					toolBar->orientation() == Qt::Horizontal
						? QRect(toolBar->width() - toolBarExpandWidth, 1, toolBarExpandWidth, toolBar->height() - 1)
						: QRect(0, toolBar->height() - toolBarExpandWidth, toolBar->width(), toolBarExpandWidth);
				if (extension->geometry() != wanted) {
					extension->setGeometry(wanted);
				}
			}
		}
		break;
	default:
		break;
	}
	return QProxyStyle::eventFilter(object, event);
}

QPalette WhidbeyStyle::standardPalette() const
{
	return systemPalette();
}

int WhidbeyStyle::mnemonicFlags(const QStyleOption* option, const QWidget* widget) const
{
	int flags = Qt::TextShowMnemonic | Qt::TextSingleLine;
	if (!customizing && !isContextMenu(widget) && !proxy()->styleHint(SH_UnderlineShortcut, option, widget)) {
		flags |= Qt::TextHideMnemonic;
	}
	return flags;
}

int WhidbeyStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
	switch (metric) {
	case PM_DefaultFrameWidth:
		if (qobject_cast<const QAbstractScrollArea*>(widget)) {
			return clientEdgeWidth;
		}
		break;
	case PM_CheckBoxLabelSpacing:
	case PM_RadioButtonLabelSpacing:
		return QProxyStyle::pixelMetric(metric, option, widget) - buttonLabelInset;
	case PM_MenuBarPanelWidth:
	case PM_MenuBarItemSpacing:
		return 0;
	case PM_MenuBarHMargin:
		return 9 + (widget ? widget->property("dockOffset").toInt() : 0);
	case PM_MenuBarVMargin:
		return 1;
	case PM_MenuPanelWidth:
		return 2;
	case PM_MenuHMargin:
	case PM_MenuVMargin:
	case PM_MenuDesktopFrameWidth:
		return 0;
	case PM_SubMenuOverlap:
		return isToolBarPopup(widget) ? 0 : 2;
	case PM_SmallIconSize:
	case PM_ToolBarIconSize:
		return 16;
	case PM_ToolBarFrameWidth:
	case PM_ToolBarItemSpacing:
		return 0;
	case PM_ToolBarItemMargin:
		return 2;
	case PM_ToolBarHandleExtent:
		if (const QToolBar* toolBar = qobject_cast<const QToolBar*>(widget)) {
			QList<QAction*> actions = toolBar->actions();
			if (!actions.isEmpty() && qobject_cast<QLabel*>(toolBar->widgetForAction(actions.first()))) {
				return 8;
			}
			return 8 + topDockInset(toolBar);
		}
		return 8;
	case PM_ToolBarSeparatorExtent:
		return 6;
	case PM_ToolBarExtensionExtent:
		if (const QToolBar* toolBar = qobject_cast<const QToolBar*>(widget); toolBar && showsExpandAlways(toolBar)) {
			return 0;
		}
		return toolBarExpandWidth;
	case PM_MenuButtonIndicator:
		if (inToolBar(widget)) {
			return splitDropDownWidth;
		}
		break;
	case PM_DockWidgetSeparatorExtent:
		return 4;
	case PM_DockWidgetTitleMargin:
		return 3;
	case PM_ToolTipLabelFrameWidth:
		return 1;
	case PM_TabBarTabOverlap:
		if (isWorkspaceTabBar(widget)) {
			return workspaceButtonHeight(widget->fontMetrics());
		}
		break;
	case PM_TabBarBaseHeight:
	case PM_TabBarBaseOverlap:
	case PM_TabBarTabShiftHorizontal:
	case PM_TabBarTabShiftVertical:
		if (isWorkspaceTabBar(widget) || isWorkspaceTabWidget(widget)) {
			return 0;
		}
		break;
	case PM_TabBarScrollButtonWidth:
		if (isWorkspaceTabBar(widget) && hasWorkspaceButtons(widget)) {
			return 0;
		}
		break;
	default:
		break;
	}
	return QProxyStyle::pixelMetric(metric, option, widget);
}

int WhidbeyStyle::styleHint(
	StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData
) const
{
	switch (hint) {
	case SH_Menu_MouseTracking:
	case SH_MenuBar_MouseTracking:
		return 1;
	case SH_DrawMenuBarSeparator:
	case SH_MainWindow_SpaceBelowMenuBar:
	case SH_ComboBox_Popup:
	case SH_Menu_FlashTriggeredItem:
	case SH_Menu_FadeOutOnHide:
	case SH_Menu_SupportsSections:
	case SH_DockWidget_ButtonsHaveFrame:
		return 0;
	case SH_ToolTipLabel_Opacity:
		return 255;
	case SH_ToolTip_WakeUpDelay:
		return 500;
	case SH_ToolButtonStyle:
		return Qt::ToolButtonIconOnly;
	case SH_ItemView_ShowDecorationSelected:
		return qobject_cast<const QListView*>(widget) != nullptr;
	case SH_TabBar_Alignment:
		if (isWorkspaceTabBar(widget) || isWorkspaceTabWidget(widget)) {
			return Qt::AlignLeft;
		}
		break;
#ifdef _WIN32
	case SH_UnderlineShortcut:
		if (widget && !qobject_cast<const QMenuBar*>(widget) && !qobject_cast<const QMenu*>(widget) &&
			qobject_cast<const QMainWindow*>(widget->window()) && widget->window()->internalWinId()) {
			HWND window = reinterpret_cast<HWND>(widget->window()->internalWinId());
			return !(SendMessageW(window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEACCEL);
		}
		break;
#else
	case SH_UnderlineShortcut:
		if (widget) {
			const QMenuBar* menuBar = qobject_cast<const QMenuBar*>(widget);
			if (!menuBar && qobject_cast<const QMenu*>(widget)) {
				QWidget* window = QApplication::activeWindow();
				if (window && window != widget) {
					menuBar = window->findChild<QMenuBar*>();
				}
			}
			if (menuBar) {
				return QApplication::focusWidget() == menuBar || AltKeyTracker::altDown;
			}
			return AltKeyTracker::seenAlt.contains(widget->window());
		}
		return 0;
#endif
	default:
		break;
	}
	return QProxyStyle::styleHint(hint, option, widget, returnData);
}

QSize WhidbeyStyle::sizeFromContents(
	ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget
) const
{
	switch (type) {
	case CT_MenuBarItem:
		if (const QStyleOptionMenuItem* item = qstyleoption_cast<const QStyleOptionMenuItem*>(option)) {
			QFont font = item->font;
			font.setKerning(false);
			return QSize(
				QFontMetrics(font).size(Qt::TextShowMnemonic, item->text).width() + 16, std::max(22, size.height() + 6)
			);
		}
		return QSize(size.width() + 16, size.height() + 6);
	case CT_MenuBar:
		return QSize(size.width(), size.height() + 1);
	case CT_Menu:
		return size;
	case CT_MenuItem:
		if (const QStyleOptionMenuItem* item = qstyleoption_cast<const QStyleOptionMenuItem*>(option)) {
			if (item->menuItemType == QStyleOptionMenuItem::Separator) {
				return QSize(size.width(), 3);
			}
			QFontMetrics metrics(item->font);
			qsizetype tab = item->text.indexOf(QLatin1Char('\t'));
			QString caption = stripMnemonics(tab == -1 ? item->text : item->text.left(tab));
			int text = metrics.horizontalAdvance(caption);
			if (isToolBarPopup(widget)) {
				return QSize(text + toolBarPopupTextPadding, toolBarPopupItemHeight);
			}
			int shortcut = tab == -1 ? 0 : metrics.horizontalAdvance(item->text.mid(tab + 1));
			return QSize(
				popupImageWidth(metrics) * gripperColumns(widget) + popupTextPadding + text + shortcut + 32 -
					menuShortcutColumnWidth(qobject_cast<const QMenu*>(widget)),
				std::max(16 + 6, commandBarTextHeight(metrics))
			);
		}
		break;
	case CT_ToolButton:
		if (isToolBarExtension(widget)) {
			return QSize(toolBarExpandWidth, toolBarExpandWidth);
		}
		if (const QStyleOptionToolButton* button = qstyleoption_cast<const QStyleOptionToolButton*>(option);
			button && inToolBar(widget)) {
			bool arrow = button->features & (QStyleOptionToolButton::MenuButtonPopup | QStyleOptionToolButton::HasMenu);
			bool drawImage = !button->icon.isNull() && button->toolButtonStyle != Qt::ToolButtonTextOnly;
			QSize result;
			if (!drawImage) {
				result = QSize(
					button->fontMetrics.size(Qt::TextShowMnemonic, button->text).width() + 8,
					std::max(button->fontMetrics.height() + 6, 22)
				);
			}
			else if (button->toolButtonStyle == Qt::ToolButtonTextBesideIcon) {
				QSize image(button->iconSize.width() + 6, button->iconSize.height() + 6);
				result = QSize(
					3 + button->fontMetrics.size(Qt::TextShowMnemonic, button->text).width() + image.width(),
					std::max(button->fontMetrics.height() + 6, image.height())
				);
			}
			else {
				result = QSize(button->iconSize.width() + 6, button->iconSize.height() + 6);
			}
			if (arrow) {
				result.rwidth() += splitDropDownWidth;
			}
			return result;
		}
		break;
	case CT_ComboBox:
		if (inToolBar(widget)) {
			return QSize(size.width() + comboThumbWidth + 4, std::max(22, option->fontMetrics.height() + 6));
		}
		break;
	case CT_TabBarTab:
		if (const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>(option);
			tab && isWorkspaceTabBar(widget)) {
			QFont font = widget->font();
			if (tab->state & State_Selected) {
				font.setBold(true);
			}
			QFontMetrics metrics(font);
			int length = metrics.size(Qt::TextShowMnemonic, tab->text).width() + 12;
			if (tab->tabIndex == 0) {
				length += workspaceTabLead(widget->fontMetrics());
			}
			return QSize(length, workspaceHeaderHeight(widget->fontMetrics()));
		}
		break;
	default:
		break;
	}
	return QProxyStyle::sizeFromContents(type, option, size, widget);
}

QRect WhidbeyStyle::subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const
{
	switch (element) {
	case SE_PushButtonFocusRect: {
		int border = proxy()->pixelMetric(PM_DefaultFrameWidth, option, widget);
		return subElementRect(SE_PushButtonContents, option, widget).adjusted(-border, -border, border, border);
	}
	case SE_TabWidgetTabBar:
	case SE_TabWidgetTabPane:
	case SE_TabWidgetTabContents:
	case SE_TabWidgetLeftCorner:
	case SE_TabWidgetRightCorner:
		if (const QStyleOptionTabWidgetFrame* frame = qstyleoption_cast<const QStyleOptionTabWidgetFrame*>(option);
			frame && isWorkspaceTabWidget(widget)) {
			QRect rect = frame->rect;
			if (frame->tabBarSize.isEmpty()) {
				if (element == SE_TabWidgetTabPane || element == SE_TabWidgetTabContents) {
					return rect;
				}
				return QRect();
			}

			int header = frame->tabBarSize.height();
			int corner = frame->rightCornerWidgetSize.width();
			switch (element) {
			case SE_TabWidgetTabBar:
				return QRect(rect.x() + 2, rect.y() + 1, std::max(0, rect.width() - 4 - corner), header);
			case SE_TabWidgetRightCorner:
				return QRect(rect.right() + 1 - 2 - corner, rect.y() + 1, corner, header);
			case SE_TabWidgetLeftCorner:
				return QRect();
			case SE_TabWidgetTabPane:
				return rect;
			default:
				return QRect(
					rect.x() + 2,
					rect.y() + header + 2,
					std::max(0, rect.width() - 4),
					std::max(0, rect.height() - header - 4)
				);
			}
		}
		break;
	case SE_TabBarScrollLeftButton:
	case SE_TabBarScrollRightButton:
		if (isWorkspaceTabBar(widget) && hasWorkspaceButtons(widget)) {
			return QRect(option->rect.right() + 1, option->rect.y(), 0, option->rect.height());
		}
		break;
	case SE_TabBarTearIndicatorLeft:
	case SE_TabBarTearIndicatorRight:
		if (isWorkspaceTabBar(widget)) {
			return QRect();
		}
		break;
	default:
		break;
	}
	return QProxyStyle::subElementRect(element, option, widget);
}

QRect WhidbeyStyle::subControlRect(
	ComplexControl control, const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget
) const
{
	if (control == CC_ToolButton && inToolBar(widget) && !isToolBarExtension(widget)) {
		if (const QStyleOptionToolButton* button = qstyleoption_cast<const QStyleOptionToolButton*>(option)) {
			QRect rect = button->rect;
			bool split = button->features & QStyleOptionToolButton::MenuButtonPopup;
			int menuWidth = splitDropDownWidth + 1;
			switch (subControl) {
			case SC_ToolButton:
				return split ? rect.adjusted(0, 0, -menuWidth, 0) : rect;
			case SC_ToolButtonMenu:
				return split ? QRect(rect.right() + 1 - menuWidth, rect.y(), menuWidth, rect.height()) : QRect();
			default:
				break;
			}
		}
	}
	else if (control == CC_ComboBox && inToolBar(widget)) {
		Box rc(option->rect);
		switch (subControl) {
		case SC_ComboBoxFrame:
		case SC_ComboBoxListBoxPopup:
			return option->rect;
		case SC_ComboBoxArrow:
			return Box(rc.right - comboThumbWidth, rc.top, rc.right, rc.bottom).toRect();
		case SC_ComboBoxEditField:
			return Box(rc.left + 1, rc.top + 1, rc.right - comboThumbWidth, rc.bottom - 1).toRect();
		default:
			break;
		}
	}
	else if (control == CC_ComboBox && subControl == SC_ComboBoxEditField) {
		const QStyleOptionComboBox* combo = qstyleoption_cast<const QStyleOptionComboBox*>(option);
		QRect field = QProxyStyle::subControlRect(control, option, subControl, widget);
		if (combo && !combo->editable) {
			field.adjust(-dropDownListTextShift, 0, 0, 0);
		}
		return field;
	}
	return QProxyStyle::subControlRect(control, option, subControl, widget);
}

void WhidbeyStyle::drawMenuPanel(const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
	const WhidbeyColors& c = colors();
	const QMenu* menu = qobject_cast<const QMenu*>(widget);
	bool shadow = menuHasShadow(widget);

	Box panel(option->rect);
	if (shadow) {
		panel.right -= popupShadowSize;
		panel.bottom -= popupShadowSize;
	}

	rectangle(painter, panel, c.menuBarBorder, c.menuBarFace);

	if (menu && menuHasGutter(menu)) {
		int gutter = popupImageWidth(menu->fontMetrics()) * gripperColumns(menu) + 1;
		gradientFill(
			painter,
			Box(panel.left + 1, panel.top + 2, panel.left + 1 + gutter, panel.bottom - 2),
			c.commandBarLight,
			c.commandBarDark,
			true
		);
	}

	if (menu) {
		QRect controlRect = popupControlRect(menu);
		if (controlRect.isValid()) {
			QPoint origin = menu->mapToGlobal(QPoint(0, 0));
			Box global(
				panel.left + origin.x(), panel.top + origin.y(), panel.right + origin.x(), panel.bottom + origin.y()
			);
			Box controlBox(controlRect);
			int left = std::max(controlBox.left + 1, global.left + 1);
			int right = std::min(controlBox.right - 1, global.right - 1);
			if (right > left) {
				if (controlBox.bottom == global.top) {
					fill(painter, left - origin.x(), panel.top, right - left, 1, c.menuBarFace);
				}
				else if (controlBox.top == global.bottom) {
					fill(painter, left - origin.x(), panel.bottom - 1, right - left, 1, c.menuBarFace);
				}
			}
		}
	}

	if (shadow) {
		drawPopupShadow(painter, panel);
	}
}

void WhidbeyStyle::drawToolBarPopupItem(
	const QStyleOptionMenuItem* item, QPainter* painter, const QWidget* widget
) const
{
	Box rc(item->rect);
	bool enabled = item->state & State_Enabled;
	bool selected = enabled && (item->state & State_Selected);
	bool popuped = false;
	if (const QMenu* menu = qobject_cast<const QMenu*>(widget)) {
		if (QAction* action = menu->actionAt(item->rect.center())) {
			popuped = action->menu() && action->menu()->isVisible();
		}
	}
	drawRectangle(painter, rc, selected, false, enabled, false, popuped, false);

	QString caption = stripMnemonics(item->text);
	QFontMetrics metrics(item->font);
	int width = metrics.horizontalAdvance(caption) + dropDownGlyphGap;
	int left = rc.left + (rc.width() - width) / 2;
	QColor text = rectangleTextColor(selected, false, enabled, false, popuped, false);

	painter->save();
	painter->setFont(item->font);
	painter->setPen(text);
	drawText(
		painter,
		QRect(left, rc.top, rc.right - left, rc.height()),
		mnemonicFlags(item, widget) | Qt::AlignLeft | Qt::AlignVCenter,
		item->text
	);
	painter->restore();

	QPoint point(left + width, rc.top + rc.height() / 2);
	drawTriangle(
		painter,
		QPoint(point.x() - 2, point.y() - 1),
		QPoint(point.x() + 2, point.y() - 1),
		QPoint(point.x(), point.y() + 1),
		text
	);
}

void WhidbeyStyle::drawMenuItem(const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
	const QStyleOptionMenuItem* item = qstyleoption_cast<const QStyleOptionMenuItem*>(option);
	if (item == nullptr) {
		return;
	}

	const WhidbeyColors& c = colors();
	Box rc(item->rect);
	QFontMetrics metrics(item->font);
	int imageWidth = popupImageWidth(metrics);
	int columns = gripperColumns(widget);

	if (isToolBarPopup(widget)) {
		drawToolBarPopupItem(item, painter, widget);
		return;
	}

	if (const QMenu* menu = qobject_cast<const QMenu*>(widget)) {
		QAction* action = menu->actionAt(item->rect.center());
		if (action && action->property("expandedItem").toBool()) {
			painter->save();
			painter->setClipping(false);
			gradientFill(
				painter,
				Box(rc.left - 1, rc.top, rc.left + imageWidth * columns, rc.bottom),
				c.menuExpandedLight,
				c.menuExpandedDark,
				true
			);
			painter->restore();
		}
	}

	if (item->menuItemType == QStyleOptionMenuItem::Separator) {
		painter->save();
		painter->setClipRect(item->rect.adjusted(0, 0, 1, 0));
		fill(
			painter,
			rc.left + imageWidth * columns + popupTextPadding,
			rc.top + 1,
			rc.right + 1 - (rc.left + imageWidth * columns + popupTextPadding),
			1,
			c.separator
		);
		painter->restore();
		return;
	}

	bool enabled = item->state & State_Enabled;
	bool selected =
		(item->state & State_Selected) && !(customizing && widget && widget->property("customizable").toBool());
	bool checked = item->checked;

	if (enabled) {
		if (selected) {
			rectangle(painter, rc, c.highlightBorder, c.highlightFace);
		}
	}
	else if (selected) {
		bool underMouse = widget && item->rect.contains(widget->mapFromGlobal(QCursor::pos()));
		if (!underMouse) {
			rectangle(painter, rc, c.highlightBorder, c.menuBarFace);
		}
	}

	Box image(rc.left, rc.top, rc.left + imageWidth, rc.bottom);
	Box check(
		image.left + image.width() / 2 - 10,
		image.top + image.height() / 2 - 10,
		image.left + image.width() / 2 + 10,
		image.top + image.height() / 2 + 10
	);
	if (checked) {
		drawRectangle(painter, check, selected, false, enabled, true, false, true);
	}

	if (!item->icon.isNull()) {
		Box iconImage(
			image.left + imageWidth * (columns - 1), image.top, image.right + imageWidth * (columns - 1), image.bottom
		);
		QRect iconRect(iconImage.left + iconImage.width() / 2 - 8, iconImage.top + iconImage.height() / 2 - 8, 16, 16);
		drawStretchedIcon(
			painter, item->icon, iconRect, iconMode(enabled, selected || checked), checked ? QIcon::On : QIcon::Off
		);
	}
	if (checked && (columns > 1 || item->icon.isNull())) {
		QColor mark = enabled ? QColor(0, 0, 0) : c.disabled;
		if (item->checkType == QStyleOptionMenuItem::Exclusive) {
			drawRadioMark(painter, check, mark);
		}
		else {
			drawCheckMark(painter, check, mark);
		}
	}

	QColor textColor = rectangleTextColor(selected, false, enabled, false, false, true);
	QString text = item->text;
	QString shortcut;
	qsizetype tab = text.indexOf(QLatin1Char('\t'));
	if (tab >= 0) {
		shortcut = text.mid(tab + 1);
		text = text.left(tab);
	}

	painter->save();
	painter->setFont(item->font);
	painter->setPen(textColor);
	int flags = mnemonicFlags(option, widget) | Qt::AlignVCenter;
	QRect textRect(
		rc.left + imageWidth * columns + popupTextPadding,
		rc.top,
		rc.right - (rc.left + imageWidth * columns + popupTextPadding),
		rc.height()
	);
	if (!shortcut.isEmpty()) {
		textRect.setWidth(std::max(0, textRect.width() - metrics.horizontalAdvance(shortcut) - 19));
	}
	drawText(painter, textRect, flags | Qt::AlignLeft, text);
	if (!shortcut.isEmpty()) {
		QRect shortcutRect(
			rc.left + imageWidth * columns + 8,
			rc.top,
			rc.right - 19 - (rc.left + imageWidth * columns + 8),
			rc.height()
		);
		drawText(painter, shortcutRect, Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine, shortcut);
	}
	painter->restore();

	if (item->menuItemType == QStyleOptionMenuItem::SubMenu) {
		QPoint point(rc.right - 11, rc.centerY());
		drawTriangle(
			painter,
			QPoint(point.x(), point.y() - 3),
			QPoint(point.x() + 3, point.y()),
			QPoint(point.x(), point.y() + 3),
			textColor
		);
	}
}

void WhidbeyStyle::drawMenuBarItem(const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
	const QStyleOptionMenuItem* item = qstyleoption_cast<const QStyleOptionMenuItem*>(option);
	if (item == nullptr) {
		return;
	}

	painter->fillRect(item->rect, dockBarBrush(widget));

	Box rc(item->rect);
	bool enabled = item->state & State_Enabled;
	bool selected = enabled && (item->state & State_Selected);
	bool popuped = false;
	if (const QMenuBar* bar = qobject_cast<const QMenuBar*>(widget)) {
		if (QAction* action = bar->actionAt(item->rect.center())) {
			if (customizing && enabled && bar->property("customizeAction").value<QObject*>() == action) {
				selected = true;
			}
			popuped = selected && action->menu() && action->menu()->isVisible();
		}
	}

	Box control(rc.left, rc.top + 1, rc.right, rc.bottom - 1);
	if (popuped) {
		drawAttachedPopupControl(painter, control);
	}
	else {
		drawRectangle(painter, control, selected, false, enabled, false, false, false);
	}

	painter->save();
	painter->setPen(rectangleTextColor(selected, false, enabled, false, popuped, false));
	drawTextRuns(painter, item->rect, mnemonicFlags(option, widget) | Qt::AlignCenter, item->text);
	painter->restore();
}

void WhidbeyStyle::drawToolBarButton(const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const
{
	const QStyleOptionToolButton* button = qstyleoption_cast<const QStyleOptionToolButton*>(option);
	if (button == nullptr) {
		return;
	}

	Box rc(button->rect);
	bool enabled = button->state & State_Enabled;
	bool selected = enabled && !customizing && (button->state & State_MouseOver);
	bool sunken = button->state & State_Sunken;
	bool popuped = enabled && sunken && (button->activeSubControls & SC_ToolButtonMenu);
	bool pressed = enabled && sunken && !popuped && (button->activeSubControls & SC_ToolButton);
	bool checked = button->state & State_On;

	if (isToolBarExtension(widget)) {
		const QToolBar* toolBar = qobject_cast<const QToolBar*>(widget->parentWidget());
		bool vertical = toolBar && toolBar->orientation() == Qt::Vertical;
		bool expandPopuped = toolBar && toolBar->property("expandPopuped").toBool();
		drawToolBarExpand(painter, rc, vertical, true, selected, (enabled && sunken) || expandPopuped);
		return;
	}

	bool arrow = button->features & (QStyleOptionToolButton::MenuButtonPopup | QStyleOptionToolButton::HasMenu);
	if (arrow) {
		if (popuped) {
			drawAttachedPopupControl(painter, rc);
		}
		else {
			drawRectangle(
				painter,
				Box(rc.left, rc.top, rc.right - splitDropDownWidth, rc.bottom),
				selected,
				pressed,
				enabled,
				checked,
				false,
				false
			);
			if ((selected || pressed) && enabled) {
				drawRectangle(
					painter,
					Box(rc.right - splitDropDownWidth - 1, rc.top, rc.right, rc.bottom),
					true,
					false,
					true,
					false,
					false,
					false
				);
			}
		}
		drawDropDownGlyph(painter, QPoint(rc.right - 7, rc.centerY()), selected, popuped, enabled);
	}
	else {
		drawRectangle(painter, rc, selected, pressed, enabled, checked, false, false);
	}

	bool drawImage = !button->icon.isNull() && button->toolButtonStyle != Qt::ToolButtonTextOnly;
	bool drawCaption =
		!button->text.isEmpty() && (!drawImage || button->toolButtonStyle == Qt::ToolButtonTextBesideIcon);
	if (drawImage) {
		const QWidget* parent = widget ? widget->parentWidget() : nullptr;
		QSize size = button->icon.actualSize(button->iconSize);
		bool largeIcons = parent && parent->property("largeIcons").toBool();
		if (largeIcons) {
			size = QSize(button->iconSize.width(), size.height() * button->iconSize.width() / size.width());
		}
		else if (parent && parent->property("popupToolBar").toBool()) {
			size = button->iconSize;
		}
		int x = arrow || drawCaption ? rc.left + 3 : rc.centerX() - size.width() / 2;
		int y = rc.centerY() - size.height() / 2;
		drawStretchedIcon(
			painter,
			button->icon,
			QRect(QPoint(x, y), size),
			iconMode(enabled, selected || pressed || checked || popuped),
			checked ? QIcon::On : QIcon::Off,
			largeIcons ? StretchRule::Centred : StretchRule::Popup
		);
	}
	if (drawCaption) {
		QRect textRect = button->rect;
		if (arrow) {
			textRect.setWidth(textRect.width() - splitDropDownWidth);
		}
		int alignment = Qt::AlignCenter;
		if (drawImage) {
			textRect.setLeft(textRect.left() + button->iconSize.width() + 6 - 1);
			alignment = Qt::AlignLeft | Qt::AlignVCenter;
		}
		painter->save();
		QFont font = button->font;
		font.setKerning(false);
		painter->setFont(font);
		painter->setPen(rectangleTextColor(selected, pressed, enabled, checked, popuped, false));
		drawText(painter, textRect, mnemonicFlags(option, widget) | alignment, button->text);
		painter->restore();
	}

	if (customizing && widget && widget == customizeSelected) {
		painter->setPen(Qt::black);
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(button->rect.adjusted(0, 0, -1, -1));
		painter->drawRect(button->rect.adjusted(1, 1, -2, -2));
	}
}

void WhidbeyStyle::drawToolBarCombo(const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const
{
	const WhidbeyColors& c = colors();
	Box rc(option->rect);
	bool enabled = option->state & State_Enabled;
	bool dropped = option->state & State_On;
	bool selected = enabled && ((option->state & (State_MouseOver | State_HasFocus)) || dropped);
	Box buttonBox(rc.right - comboThumbWidth, rc.top, rc.right, rc.bottom);

	if (!enabled) {
		rectangle(painter, rc, c.disabled, c.face);
		buttonBox = buttonBox.deflated(1, 1);
	}
	else {
		fill(painter, Box(rc.left, rc.top + 1, rc.right - comboThumbWidth + 1, rc.bottom - 1), c.window);
		if (selected) {
			rectangle(painter, rc, c.highlightBorder, c.window);
			drawRectangle(painter, buttonBox, true, dropped, true, false, false, false);
			buttonBox = buttonBox.deflated(1, 1);
		}
		else {
			buttonBox = buttonBox.deflated(1, 1);
			draw3dRect(painter, buttonBox, c.window, c.window);
		}
	}

	QColor mark = !enabled ? c.disabled : dropped ? c.pushedText : selected ? c.highlightedText : c.toolbarText;
	QPoint point(buttonBox.centerX(), buttonBox.centerY());
	int width = std::max(2, buttonBox.width() / 2 - 4);
	drawTriangle(
		painter,
		QPoint(point.x() - width, point.y() - width / 2),
		QPoint(point.x() + width, point.y() - width / 2),
		QPoint(point.x(), point.y() - width / 2 + width),
		mark
	);
}

void WhidbeyStyle::drawWorkspaceTabShape(const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
	const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>(option);
	if (tab == nullptr) {
		return;
	}

	const WhidbeyColors& c = colors();
	bool selected = tab->state & State_Selected;
	Box rc(tab->rect);
	if (tab->tabIndex == 0) {
		rc.left += workspaceTabLead(widget->fontMetrics());
	}
	rc.top += 3;
	rc.bottom--;

	int width = rc.width();
	int height = rc.height();
	QColor face = selected ? c.tabButtonSelected : c.tabButtonNormal;
	QColor border = selected ? c.tabFrameBorder : c.tabBorderShadow;
	QColor highlight = selected ? QColor(255, 255, 255) : c.tabBorderHighlight;

	for (int y = rc.top + 2; y < rc.bottom; y++) {
		int left = rc.left + 4 - (y - (rc.top + 2));
		fill(painter, left, y, rc.right - 1 - left, 1, face);
	}

	drawPolyline(
		painter,
		QPoint(rc.right, rc.bottom - 1),
		{QPoint(0, 3 - height),
		 QPoint(-2, -2),
		 QPoint(9 - width, 0),
		 QPoint(-5, 2),
		 QPoint(3 - height, height - 3),
		 QPoint(-2, 0)},
		border
	);
	drawPolyline(painter, QPoint(rc.right - 1, rc.bottom - 1), {QPoint(0, 3 - height), QPoint(-2, -2)}, highlight);
	drawPolyline(
		painter,
		QPoint(rc.right - 3, rc.top + 1),
		{QPoint(9 - width, 0), QPoint(-5, 3), QPoint(4 - height, height - 4)},
		highlight
	);

	if (selected) {
		fill(painter, rc.left - height + 4, rc.bottom, width + height - 3, 1, face);
	}
}

void WhidbeyStyle::drawWorkspaceTabLabel(const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
	const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>(option);
	if (tab == nullptr) {
		return;
	}

	const WhidbeyColors& c = colors();
	bool selected = tab->state & State_Selected;
	bool enabled = tab->state & State_Enabled;
	Box rc(tab->rect);
	if (tab->tabIndex == 0) {
		rc.left += workspaceTabLead(widget->fontMetrics());
	}
	rc.top += 3;

	QFont font = widget->font();
	if (selected) {
		font.setBold(true);
	}

	painter->save();
	painter->setFont(font);
	painter->setPen(!enabled ? c.grayText : selected ? c.tabSelectedText : c.tabNormalText);
	int textHeight = QFontMetrics(font).height();
	painter->setClipRect(QRect(rc.left + 6, rc.top, std::max(0, rc.width() - 12), rc.height()), Qt::IntersectClip);
	painter->drawText(
		QRect(rc.left + 6, rc.top + (rc.height() - textHeight) / 2, std::max(0, rc.width() - 12), textHeight),
		Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine | Qt::TextShowMnemonic | Qt::TextHideMnemonic,
		tab->text
	);
	painter->restore();
}

void WhidbeyStyle::drawItemText(
	QPainter* painter,
	const QRect& rect,
	int flags,
	const QPalette& palette,
	bool enabled,
	const QString& text,
	QPalette::ColorRole textRole
) const
{
	const QWidget* widget = dynamic_cast<const QWidget*>(painter->device());
	if (!enabled && qobject_cast<const QLabel*>(widget) && inToolBar(widget)) {
		painter->save();
		painter->setPen(colors().toolbarGrayText);
		drawText(painter, rect, flags, text);
		painter->restore();
		return;
	}

	if ((flags & Qt::AlignVCenter) && !(flags & Qt::TextWordWrap) && !text.contains(QChar::LineFeed)) {
		int height = painter->fontMetrics().height();
		QProxyStyle::drawItemText(
			painter,
			QRect(rect.x(), rect.y() + (rect.height() - height) / 2, rect.width(), height),
			(flags & ~Qt::AlignVCenter) | Qt::AlignTop,
			palette,
			enabled,
			text,
			textRole
		);
		return;
	}

	QProxyStyle::drawItemText(painter, rect, flags, palette, enabled, text, textRole);
}

void WhidbeyStyle::drawPrimitive(
	PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget
) const
{
	const WhidbeyColors& c = colors();

	switch (element) {
	case PE_PanelMenu:
		drawMenuPanel(option, painter, widget);
		return;
	case PE_Frame:
		if (const QAbstractScrollArea* area = qobject_cast<const QAbstractScrollArea*>(widget)) {
			if (option->state & State_Sunken) {
				drawClientEdge(painter, option->rect);
				return;
			}
			if (area->frameShadow() == QFrame::Plain) {
				painter->setBrush(Qt::NoBrush);
				painter->setPen(c.staticFrame);
				painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
				painter->setPen(option->palette.color(QPalette::Base));
				painter->drawRect(option->rect.adjusted(1, 1, -2, -2));
				return;
			}
		}
		break;
	case PE_IndicatorBranch: {
		QRect rect = option->rect;
		int centerX = rect.center().x();
		int centerY = rect.center().y();
		const QTreeView* tree = qobject_cast<const QTreeView*>(widget);
		bool lines = tree && tree->property("treeLines").toBool() && rect.left() >= tree->indentation();

		if (lines) {
			if (option->state & (State_Item | State_Children)) {
				painter->fillRect(centerX, centerY, rect.right() - centerX + 1, 1, Qt::black);
			}
			if (option->state & State_Sibling) {
				painter->fillRect(centerX, rect.top(), 1, rect.height(), Qt::black);
			}
			else if (option->state & (State_Item | State_Children)) {
				painter->fillRect(centerX, rect.top(), 1, centerY - rect.top() + 1, Qt::black);
			}
		}

		if (option->state & State_Children) {
			static const QColor fills[] = {
				QColor(252, 252, 252),
				QColor(252, 252, 252),
				QColor(250, 251, 251),
				QColor(250, 251, 251),
				QColor(237, 237, 236),
				QColor(227, 227, 227),
				QColor(227, 227, 227),
			};
			QRect box(centerX - 4, centerY - 4, 9, 9);
			QColor border(145, 145, 145);
			QColor corner(186, 187, 188);
			painter->fillRect(box.left() + 1, box.top(), 7, 1, border);
			painter->fillRect(box.left() + 1, box.bottom(), 7, 1, border);
			painter->fillRect(box.left(), box.top() + 1, 1, 7, border);
			painter->fillRect(box.right(), box.top() + 1, 1, 7, border);
			for (const QPoint& point : {box.topLeft(), box.topRight(), box.bottomLeft(), box.bottomRight()}) {
				painter->fillRect(point.x(), point.y(), 1, 1, corner);
			}
			for (int row = 0; row < 7; row++) {
				painter->fillRect(box.left() + 1, box.top() + 1 + row, 7, 1, fills[row]);
			}
			if (!(option->state & State_Open)) {
				painter->fillRect(centerX, centerY - 2, 1, 5, QColor(41, 66, 114));
			}
			painter->fillRect(centerX - 2, centerY, 5, 1, QColor(75, 99, 167));
		}
		return;
	}
	case PE_FrameMenu:
	case PE_PanelMenuBar:
	case PE_IndicatorToolBarHandle:
		return;
	case PE_IndicatorToolBarSeparator: {
		Box rc(option->rect);
		if (option->state & State_Horizontal) {
			fill(painter, rc.left + 2, rc.top + 4, 1, rc.height() - 8, c.separator);
		}
		else {
			fill(painter, rc.left + 4, rc.top + 2, rc.width() - 8, 1, c.separator);
		}
		return;
	}
	case PE_PanelButtonTool:
		if (option->state & State_AutoRaise) {
			bool enabled = option->state & State_Enabled;
			drawRectangle(
				painter,
				Box(option->rect),
				enabled && (option->state & State_MouseOver),
				enabled && (option->state & State_Sunken),
				enabled,
				option->state & State_On,
				false,
				false
			);
			return;
		}
		break;
	case PE_PanelStatusBar:
		painter->fillRect(option->rect, c.face);
		return;
	case PE_FrameStatusBarItem:
		if (widget && widget->parentWidget()) {
			const QWidget* bar = widget->parentWidget();
			int first = INT_MAX;
			for (QObject* child : bar->children()) {
				if (!child->isWidgetType() || child->inherits("QSizeGrip")) {
					continue;
				}
				QWidget* pane = static_cast<QWidget*>(child);
				if (!pane->isHidden()) {
					first = std::min(first, pane->x());
				}
			}
			if (widget->x() == first) {
				return;
			}

			Box rc(option->rect.x(), 2, option->rect.x() + option->rect.width(), bar->height());
			draw3dRect(painter, rc, c.statusPane, c.statusPane);
			return;
		}
		break;
	case PE_PanelTipLabel: {
		Box rc(option->rect);
		fill(painter, rc, c.infoBackground);
		draw3dRect(painter, rc, c.face, QColor(0, 0, 0));
		return;
	}
	case PE_FrameTabBarBase:
		if (isWorkspaceTabBar(widget)) {
			QWidget* owner = widget->parentWidget();
			if (painter->device() == static_cast<QPaintDevice*>(owner)) {
				const QTabWidget* tabs = qobject_cast<const QTabWidget*>(owner);
				if (tabs && tabs->count() == 0) {
					drawClientEdge(painter, owner->rect());
					painter->fillRect(
						owner->rect().adjusted(clientEdgeWidth, clientEdgeWidth, -clientEdgeWidth, -clientEdgeWidth),
						c.appWorkspace
					);
				}
				else {
					Box rc(owner->rect());
					draw3dRect(painter, rc, c.tabFrameBorder, c.tabFrameBorder);
				}
			}
			else {
				drawWorkspaceHeader(painter, widget->rect());
			}
			return;
		}
		break;
	case PE_IndicatorTabTearLeft:
	case PE_IndicatorTabTearRight:
		if (isWorkspaceTabBar(widget)) {
			return;
		}
		break;
	case PE_IndicatorDockWidgetResizeHandle:
		if (widget) {
			fillFramePart(painter, option->rect, widget);
		}
		else {
			painter->fillRect(option->rect, dockBarBrush(widget));
		}
		return;
	case PE_PanelLineEdit:
		if (isToolBarComboEdit(widget)) {
			painter->fillRect(option->rect, (option->state & State_Enabled) ? c.window : c.face);
			return;
		}
		break;
	case PE_FrameLineEdit: {
		QRegion ring = QRegion(option->rect) - QRegion(option->rect.adjusted(1, 1, -1, -1));
		painter->save();
		painter->setClipRegion(QRegion(option->rect.adjusted(1, 1, -1, -1)), Qt::IntersectClip);
		QProxyStyle::drawPrimitive(element, option, painter, widget);
		painter->restore();
		painter->save();
		painter->setClipRegion(ring, Qt::IntersectClip);
		painter->fillRect(option->rect, option->palette.brush(QPalette::Window));
		QProxyStyle::drawPrimitive(element, option, painter, widget);
		painter->restore();
		return;
	}
	default:
		break;
	}
	QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void WhidbeyStyle::drawControl(
	ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget
) const
{
	const WhidbeyColors& c = colors();

	switch (element) {
	case CE_CheckBoxLabel:
	case CE_RadioButtonLabel:
		painter->save();
		painter->setClipRect(option->rect, Qt::IntersectClip);
		QProxyStyle::drawControl(element, option, painter, widget);
		painter->restore();
		return;
	case CE_PushButton:
		if (const QStyleOptionButton* button = qstyleoption_cast<const QStyleOptionButton*>(option);
			button && (button->state & State_HasFocus)) {
			QStyleOptionButton unfocused = *button;
			unfocused.state &= ~State_HasFocus;
			QProxyStyle::drawControl(element, &unfocused, painter, widget);
			QStyleOptionFocusRect focus;
			focus.QStyleOption::operator=(*button);
			focus.rect = subElementRect(SE_PushButtonFocusRect, button, widget);
			proxy()->drawPrimitive(PE_FrameFocusRect, &focus, painter, widget);
			return;
		}
		break;
	case CE_PushButtonLabel:
		if (const QStyleOptionButton* button = qstyleoption_cast<const QStyleOptionButton*>(option)) {
			QStyleOptionButton label = *button;
			if (label.icon.isNull() && !(label.features & QStyleOptionButton::HasMenu)) {
				QRect content = label.rect.adjusted(-1, -1, 1, 1);
				int textHeight = label.fontMetrics.height();
				label.rect = QRect(
					content.left(), content.top() + (content.height() - textHeight) / 2, content.width(), textHeight
				);
			}
			if (!(label.state & State_Enabled)) {
				label.palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledButtonText);
			}
			QProxyStyle::drawControl(element, &label, painter, widget);
			return;
		}
		break;
	case CE_MenuBarEmptyArea: {
		painter->fillRect(option->rect, dockBarBrush(widget));
		Box rc(option->rect);
		for (int y = 5; y < rc.height() + 1 - 7; y += 4) {
			drawGripperDot(painter, rc.left + 5 + (widget ? widget->property("dockOffset").toInt() : 0), rc.top + y);
		}
		return;
	}
	case CE_MenuBarItem:
		drawMenuBarItem(option, painter, widget);
		return;
	case CE_MenuItem:
		drawMenuItem(option, painter, widget);
		return;
	case CE_MenuEmptyArea:
		return;
	case CE_ToolBar:
		drawToolBarBackground(painter, option, widget);
		return;
	case CE_TabBarTabShape:
		if (isWorkspaceTabBar(widget)) {
			drawWorkspaceTabShape(option, painter, widget);
			return;
		}
		break;
	case CE_TabBarTabLabel:
		if (isWorkspaceTabBar(widget)) {
			drawWorkspaceTabLabel(option, painter, widget);
			return;
		}
		break;
	case CE_DockWidgetTitle:
		if (const QStyleOptionDockWidget* dock = qstyleoption_cast<const QStyleOptionDockWidget*>(option)) {
			if (!dock->verticalTitleBar) {
				int buttons = (dock->closable ? 1 : 0) + (dock->floatable ? 1 : 0);
				drawPaneCaption(painter, dock->rect, dock->title, false, dock->state & State_Enabled, buttons);
				return;
			}
		}
		break;
	case CE_ComboBoxLabel:
		if (const QStyleOptionComboBox* combo = qstyleoption_cast<const QStyleOptionComboBox*>(option);
			combo && inToolBar(widget)) {
			if (!combo->editable) {
				QRect textRect = proxy()->subControlRect(CC_ComboBox, combo, SC_ComboBoxEditField, widget);
				painter->save();
				painter->setPen((combo->state & State_Enabled) ? c.windowText : c.grayText);
				drawText(
					painter,
					textRect.adjusted(2, 0, 0, 0),
					Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
					combo->currentText
				);
				painter->restore();
			}
			return;
		}
		break;
	case CE_SizeGrip:
#ifndef _WIN32
	{
		Box rc(option->rect);
		int width = rc.right;
		int height = rc.bottom;
		for (int i = 0; i < 3; i++) {
			drawLine(painter, width - i * 5, height - 1, width, height - 1 - i * 5, c.shadow);
			drawLine(painter, width - 1 - i * 5, height - 1, width, height - 2 - i * 5, c.shadow);
			drawLine(painter, width - 2 - i * 5, height - 1, width, height - 3 - i * 5, c.shadow);
		}
		for (int i = 0; i < 3; i++) {
			drawLine(painter, width - 3 - i * 5, height - 1, width, height - 4 - i * 5, c.buttonHighlight);
		}
		return;
	}
#else
		break;
#endif
	default:
		break;
	}
	QProxyStyle::drawControl(element, option, painter, widget);
}

void WhidbeyStyle::drawComplexControl(
	ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget
) const
{
	if (control == CC_ToolButton && inToolBar(widget)) {
		drawToolBarButton(option, painter, widget);
		return;
	}
	if (control == CC_ComboBox && inToolBar(widget)) {
		drawToolBarCombo(option, painter, widget);
		return;
	}
	QProxyStyle::drawComplexControl(control, option, painter, widget);
	if (control == CC_GroupBox && (option->subControls & SC_GroupBoxLabel)) {
		QRect label = subControlRect(CC_GroupBox, option, SC_GroupBoxLabel, widget);
		painter->fillRect(label.left() - 1, label.top(), 2, label.height(), option->palette.brush(QPalette::Window));
	}
}

QPixmap WhidbeyStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* option) const
{
	if (iconMode == QIcon::Active || iconMode == QIcon::Selected) {
		return pixmap;
	}
	if (iconMode != QIcon::Disabled) {
		return QProxyStyle::generatedIconPixmap(iconMode, pixmap, option);
	}

	const WhidbeyColors& c = colors();
	QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
	for (int y = 0; y < image.height(); y++) {
		QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
		for (int x = 0; x < image.width(); x++) {
			int alpha = qAlpha(line[x]);
			if (alpha == 0) {
				continue;
			}

			int red = qRed(line[x]) * 255 / alpha;
			int green = qGreen(line[x]) * 255 / alpha;
			int blue = qBlue(line[x]) * 255 / alpha;
			double gray = (red * 0.299 + green * 0.587 + blue * 0.114) / 255.0;
			double dark = 1.0 - gray;
			red = static_cast<quint8>(dark * c.disabledIconDark.red() + gray * c.disabledIconLight.red());
			green = static_cast<quint8>(dark * c.disabledIconDark.green() + gray * c.disabledIconLight.green());
			blue = static_cast<quint8>(dark * c.disabledIconDark.blue() + gray * c.disabledIconLight.blue());
			line[x] = qRgba(red * alpha / 255, green * alpha / 255, blue * alpha / 255, alpha);
		}
	}

	QPixmap result = QPixmap::fromImage(image);
	result.setDevicePixelRatio(pixmap.devicePixelRatio());
	return result;
}
