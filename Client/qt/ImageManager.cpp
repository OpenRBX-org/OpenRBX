#include "ImageManager.h"

#include "CommandIds.h"
#include "Resources.h"
#include "WhidbeyStyle.h"

#include <QImage>
#include <QPixmap>

struct CommandImage
{
	unsigned id;
	const char* name;
};

static const CommandImage browserImages[] = {
	{ID_BROWSER_STOP, "stop"},
	{ID_BROWSER_HOME, "home_brown"},
	{ID_BROWSER_REFRESH, "refresh_doc"},
	{ID_BROWSER_BACK, "arrowleft_green"},
	{ID_BROWSER_FORWARD, "arrowright_green"},
};

static const CommandImage commandImages[] = {
	{ID_FILE_NEW, "new_document"},
	{ID_FILE_OPEN, "open_document"},
	{ID_FILE_SAVE, "save"},
	{ID_EDIT_UNDO, "undo"},
	{ID_EDIT_REDO, "redo"},
	{ID_EDIT_CLEAR, "delete_x"},
	{ID_EDIT_COPY, "copy_clipboard"},
	{ID_EDIT_CUT, "cut_clipboard"},
	{ID_EDIT_PASTE, "paste_clipboard"},
	{ID_TOOL_FILL, "paintcan"},
	{ID_TOOL_LOCK, "lock"},
	{ID_EDIT_LOCK, "lock"},
	{ID_EDIT_UNLOCK_ALL, "lock_open"},
	{ID_TOOLS_GC, "recycle"},
	{ID_TOOLS_EXECUTE_SCRIPT, "script_go"},
	{ID_VIEW_REPORT, "report"},
	{ID_VIEW_PROPERTIES, "table"},
	{ID_VIEW_LINE_NUMBERS, "text_list_numbers"},
	{ID_VIEW_START_PAGE, "world"},
	{ID_TOOLS_REPORT_CRASHES, "bug_error"},
	{ID_TOOLS_REPORT_ABUSE, "shield"},
	{ID_FORMAT_UNGROUP, "ungroup"},
	{ID_FORMAT_GROUP, "group"},
};

ImageManager& ImageManager::instance()
{
	static ImageManager imageManager;
	return imageManager;
}

bool ImageManager::loadToolBar(unsigned toolBarId)
{
	const Resources::ToolBar* toolBar = Resources::toolBar(toolBarId);
	if (toolBar == nullptr) {
		return false;
	}

	QImage strip(Resources::bitmapPath(toolBarId));
	if (strip.isNull()) {
		return false;
	}

	bool alpha = strip.hasAlphaChannel();
	strip = strip.convertToFormat(QImage::Format_ARGB32);
	QRgb mask = strip.pixel(0, 0);
	for (int y = 0; y < strip.height(); y++) {
		QRgb* line = reinterpret_cast<QRgb*>(strip.scanLine(y));
		for (int x = 0; x < strip.width(); x++) {
			if (line[x] == mask) {
				line[x] = qRgba(0, 0, 0, 0);
			}
		}
	}

	int index = 0;
	for (unsigned id : toolBar->buttons) {
		if (id == 0) {
			continue;
		}

		m_icons[id] = QIcon(QPixmap::fromImage(strip.copy(index * toolBar->width, 0, toolBar->width, toolBar->height)));
		m_alpha[id] = alpha;
		index++;
	}
	return true;
}

void ImageManager::loadImage(unsigned id, const char* name)
{
	QIcon icon;
	for (int size : {16, 32}) {
		QPixmap normal = WhidbeyStyle::premultipliedPixmap(
			QImage(Resources::pngPath(QStringLiteral("%1_%2").arg(name).arg(size).toUpper()))
		);
		if (normal.isNull()) {
			continue;
		}
		icon.addPixmap(normal, QIcon::Normal);

		QPixmap disabled = WhidbeyStyle::premultipliedPixmap(
			QImage(Resources::pngPath(QStringLiteral("%1_%2_D").arg(name).arg(size).toUpper()))
		);
		if (!disabled.isNull()) {
			icon.addPixmap(disabled, QIcon::Disabled);
		}

		QPixmap hot = WhidbeyStyle::premultipliedPixmap(
			QImage(Resources::pngPath(QStringLiteral("%1_%2_H").arg(name).arg(size).toUpper()))
		);
		if (!hot.isNull()) {
			icon.addPixmap(hot, QIcon::Active);
		}
	}
	m_icons[id] = icon;
	m_alpha[id] = true;
}

void ImageManager::loadBrowserImages()
{
	for (const CommandImage& image : browserImages) {
		loadImage(image.id, image.name);
	}
}

void ImageManager::loadCommandImages()
{
	loadBrowserImages();
	for (const CommandImage& image : commandImages) {
		loadImage(image.id, image.name);
	}
}

QIcon ImageManager::icon(unsigned id) const
{
	auto it = m_icons.find(id);
	return it != m_icons.end() ? it->second : QIcon();
}

bool ImageManager::isAlpha(unsigned id) const
{
	auto it = m_alpha.find(id);
	return it != m_alpha.end() && it->second;
}
