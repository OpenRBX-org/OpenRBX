#ifndef RESOURCES_H
#define RESOURCES_H

#include "CommandIds.h"

#include <QCursor>
#include <QString>
#include <span>

constexpr unsigned appTitleStringId = 57344;
constexpr unsigned mainResourceId = 128;

namespace Resources {

enum MenuEntryKind
{
	MENU_ITEM,
	MENU_SEPARATOR,
	MENU_POPUP,
	MENU_END,
};

enum MenuEntryFlags
{
	MENU_GRAYED = 0x1,
};

struct MenuEntry
{
	MenuEntryKind kind;
	const char* text;
	unsigned id;
	unsigned flags;
};

struct ToolBar
{
	unsigned id;
	int width;
	int height;
	std::span<const unsigned> buttons;
};

enum AcceleratorFlags
{
	ACCEL_SHIFT = 0x4,
	ACCEL_CONTROL = 0x8,
	ACCEL_ALT = 0x10,
};

struct Accelerator
{
	unsigned key;
	unsigned id;
	unsigned flags;
};

struct DialogControl
{
	const char* text;
	int id;
	const char* className;
	unsigned style;
	int x;
	int y;
	int cx;
	int cy;
	unsigned exStyle;
};

struct DialogTemplate
{
	unsigned id;
	int x;
	int y;
	int cx;
	int cy;
	unsigned style;
	unsigned exStyle;
	const char* caption;
	int pointSize;
	const char* typeface;
	std::span<const DialogControl> controls;
};

std::span<const MenuEntry> menu(unsigned id);
const DialogTemplate* dialog(unsigned id);
const ToolBar* toolBar(unsigned id);
std::span<const Accelerator> accelerators(unsigned id);
QString string(unsigned id);
QString bitmapPath(unsigned id);
QString htmlPath(unsigned id);
QString iconPath(unsigned id);
QCursor cursor(unsigned id);
QString pngPath(const QString& name);

} // namespace Resources

#endif // RESOURCES_H
