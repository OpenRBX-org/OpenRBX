#include "ImageEditorDialog.h"

#include "FileDialog.h"
#include "Resources.h"
#include "SystemColorDialog.h"
#include "WhidbeyStyle.h"

#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QFile>
#include <QFileInfo>
#include <QHelpEvent>
#include <QImageReader>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <QtEndian>
#include <algorithm>
#include <functional>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

static const unsigned imageEditorDialogId = 9115;
static const unsigned toolsBitmapId = 9101;
static const unsigned lineCursorId = 9110;
static const unsigned pickColorCursorId = 9111;
static const unsigned fillColorCursorId = 9112;
static const unsigned pencilCursorId = 9113;
static const int pictureId = 100;
static const int previewId = 101;
static const int colorsId = 102;
static const int toolsId = 103;
static const int moveId = 104;
static const int eraseId = 105;
static const int pickerId = 106;
static const unsigned pencilId = 9135;
static const unsigned fillId = 9136;
static const unsigned getColorId = 9137;
static const unsigned lineId = 9138;
static const unsigned rectangleId = 9139;
static const unsigned circleId = 9140;
static const unsigned copyId = 9141;
static const unsigned pasteId = 9142;
static const unsigned clearId = 9143;
static const unsigned undoId = 9144;
static const unsigned redoId = 9145;
static const unsigned openId = 9146;
static const unsigned moveLeftId = 1;
static const unsigned moveDownId = 2;
static const unsigned moveUpId = 3;
static const unsigned moveRightId = 4;
static const unsigned filterStringId = 9178;
static const size_t undoCount = 15;
static const QRgb eraseColor = 0x00fffeff;
static const int toolIconSize = 16;
static const int toolButtonWidth = toolIconSize + 7;
static const int toolButtonHeight = toolIconSize + 7;
static const int toolRowStep = toolButtonHeight;
static const int toolIconInset = 3;
static const int swatchSize = 11;
static const int swatchStep = 13;
static const int previewLeft = 10;

enum ThemeToolBarState
{
	TS_HOT = 2,
	TS_PRESSED = 3,
	TS_CHECKED = 5,
	TS_HOTCHECKED = 6,
};
static const int TP_BUTTON = 1;

static bool isErase(QRgb color)
{
	return qAlpha(color) == 0;
}

static QImage blankImage(QSize size)
{
	QImage image(size, QImage::Format_ARGB32);
	image.fill(eraseColor);
	return image;
}

static QImage loadAlphaStrip(unsigned id)
{
	QFile file(Resources::bitmapPath(id));
	if (!file.open(QIODevice::ReadOnly)) {
		return QImage();
	}
	QByteArray data = file.readAll();
	auto read32 = [&data](int offset) { return qFromLittleEndian<qint32>(data.constData() + offset); };
	int bits = read32(10);
	int width = read32(18);
	int height = read32(22);
	QImage image(width, std::abs(height), QImage::Format_ARGB32);
	for (int y = 0; y < image.height(); y++) {
		int row = height > 0 ? image.height() - 1 - y : y;
		const uchar* source = reinterpret_cast<const uchar*>(data.constData()) + bits + row * width * 4;
		QRgb* target = reinterpret_cast<QRgb*>(image.scanLine(y));
		for (int x = 0; x < width; x++) {
			target[x] = qRgba(source[x * 4 + 2], source[x * 4 + 1], source[x * 4], source[x * 4 + 3]);
		}
	}
	return image;
}

static QImage disabledImage(const QImage& image, QColor background)
{
	QImage result(image.size(), QImage::Format_ARGB32);
	for (int y = 0; y < image.height(); y++) {
		for (int x = 0; x < image.width(); x++) {
			QRgb pixel = image.pixel(x, y);
			int alpha = qAlpha(pixel);
			double r = qRed(pixel) * alpha / 255.0;
			double g = qGreen(pixel) * alpha / 255.0;
			double b = qBlue(pixel) * alpha / 255.0;
			double gray = std::clamp(0.2097 * r + 0.7234 * g + 0.0778 * b - 4.065, 0.0, 255.0);
			auto blend = [&](int back) { return qRound(gray * alpha / 255.0 + back * (255 - alpha) / 255.0); };
			result.setPixel(x, y, qRgb(blend(background.red()), blend(background.green()), blend(background.blue())));
		}
	}
	return result;
}

enum EllipsePixel
{
	EllipseNone,
	EllipsePen,
	EllipseBrush,
};

static std::vector<EllipsePixel> rasterizeEllipse(QSize size, QRect bounds)
{
	std::vector<EllipsePixel> pixels(size.width() * size.height(), EllipseNone);
#ifdef _WIN32
	const COLORREF none = RGB(1, 2, 3);
	const COLORREF pen = RGB(4, 5, 6);
	const COLORREF brush = RGB(7, 8, 9);
	BITMAPINFO info = {};
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = size.width();
	info.bmiHeader.biHeight = -size.height();
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;
	void* bits = nullptr;
	HDC dc = CreateCompatibleDC(nullptr);
	HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
	if (bitmap != nullptr) {
		HGDIOBJ previousBitmap = SelectObject(dc, bitmap);
		HBRUSH background = CreateSolidBrush(none);
		RECT area = {0, 0, size.width(), size.height()};
		FillRect(dc, &area, background);
		DeleteObject(background);
		HPEN outline = CreatePen(PS_SOLID, 1, pen);
		HBRUSH fill = CreateSolidBrush(brush);
		HGDIOBJ previousPen = SelectObject(dc, outline);
		HGDIOBJ previousBrush = SelectObject(dc, fill);
		Ellipse(dc, bounds.left(), bounds.top(), bounds.left() + bounds.width(), bounds.top() + bounds.height());
		SelectObject(dc, previousBrush);
		SelectObject(dc, previousPen);
		DeleteObject(fill);
		DeleteObject(outline);
		GdiFlush();
		const quint32* data = static_cast<const quint32*>(bits);
		for (size_t i = 0; i < pixels.size(); i++) {
			quint32 value = data[i] & 0xffffff;
			pixels[i] = value == 0x040506 ? EllipsePen : value == 0x070809 ? EllipseBrush : EllipseNone;
		}
		SelectObject(dc, previousBitmap);
		DeleteObject(bitmap);
	}
	DeleteDC(dc);
#else
	QImage mask(size, QImage::Format_ARGB32);
	mask.fill(Qt::transparent);
	QPainter painter(&mask);
	painter.setPen(QColor(Qt::black));
	painter.setBrush(QColor(Qt::white));
	painter.drawEllipse(QRect(bounds.topLeft(), bounds.size() - QSize(1, 1)));
	painter.end();
	for (int y = 0; y < size.height(); y++) {
		for (int x = 0; x < size.width(); x++) {
			QRgb value = mask.pixel(x, y);
			pixels[y * size.width() + x] = qAlpha(value) == 0 ? EllipseNone
										   : qRed(value) == 0 ? EllipsePen
															  : EllipseBrush;
		}
	}
#endif
	return pixels;
}

class EditorToolBar : public QWidget
{
public:
	struct Button
	{
		unsigned id;
		int image;
	};

	EditorToolBar(std::vector<Button> buttons, int columns, QImage strip, QWidget* parent)
		: QWidget(parent), m_buttons(std::move(buttons)), m_columns(columns), m_strip(std::move(strip))
	{
		setMouseTracking(true);
		int rows = (static_cast<int>(m_buttons.size()) + m_columns - 1) / m_columns;
		setFixedSize(m_columns * toolButtonWidth, (rows - 1) * toolRowStep + toolButtonHeight);
	}

	std::function<void(unsigned)> onCommand;
	std::function<bool(unsigned)> isChecked;
	std::function<bool(unsigned)> isEnabled;

protected:
	QRect buttonRect(int index) const
	{
		return QRect(
			(index % m_columns) * toolButtonWidth, (index / m_columns) * toolRowStep, toolButtonWidth, toolButtonHeight
		);
	}

	int buttonAt(QPoint point) const
	{
		for (int i = 0; i < static_cast<int>(m_buttons.size()); i++) {
			if (buttonRect(i).contains(point)) {
				return i;
			}
		}
		return -1;
	}

	bool enabled(int index) const { return !isEnabled || isEnabled(m_buttons[index].id); }

	void paintEvent(QPaintEvent* event) override
	{
		const WhidbeyColors& c = WhidbeyStyle::colors();
		QPainter painter(this);
		for (int i = 0; i < static_cast<int>(m_buttons.size()); i++) {
			QRect rect = buttonRect(i);
			bool checked = isChecked && isChecked(m_buttons[i].id);
			bool on = enabled(i);
			bool hot = on && i == m_hot;
			bool pressed = hot && i == m_pressed;
			int state = pressed ? TS_PRESSED : checked ? (hot ? TS_HOTCHECKED : TS_CHECKED) : hot ? TS_HOT : 0;
			if (state != 0) {
				WhidbeyStyle::drawThemeBackground(&painter, rect, L"TOOLBAR", TP_BUTTON, state, c.face);
			}
			QImage icon = m_strip.copy(m_buttons[i].image * toolIconSize, 0, toolIconSize, toolIconSize);
			QPoint position = rect.topLeft() + QPoint(toolIconInset + (checked && !pressed ? 1 : 0), toolIconInset);
			if (on) {
				painter.drawImage(position, icon);
			}
			else {
				painter.drawImage(position, disabledImage(icon, c.face));
			}
		}
	}

	void mouseMoveEvent(QMouseEvent* event) override
	{
		int hot = buttonAt(event->position().toPoint());
		if (hot != m_hot) {
			m_hot = hot;
			update();
		}
	}

	void leaveEvent(QEvent* event) override
	{
		m_hot = -1;
		update();
	}

	void mousePressEvent(QMouseEvent* event) override
	{
		if (event->button() == Qt::LeftButton) {
			m_pressed = buttonAt(event->position().toPoint());
			update();
		}
	}

	void mouseReleaseEvent(QMouseEvent* event) override
	{
		if (event->button() != Qt::LeftButton) {
			return;
		}
		int pressed = m_pressed;
		m_pressed = -1;
		update();
		if (pressed >= 0 && pressed == buttonAt(event->position().toPoint()) && enabled(pressed) && onCommand) {
			onCommand(m_buttons[pressed].id);
		}
	}

	bool event(QEvent* event) override
	{
		if (event->type() == QEvent::ToolTip) {
			QHelpEvent* help = static_cast<QHelpEvent*>(event);
			int index = buttonAt(help->pos());
			QString text = index >= 0 ? Resources::string(m_buttons[index].id).section('\n', 1, 1) : QString();
			if (text.isEmpty()) {
				QToolTip::hideText();
			}
			else {
				QToolTip::showText(help->globalPos(), text, this, buttonRect(index));
			}
			return true;
		}
		return QWidget::event(event);
	}

private:
	std::vector<Button> m_buttons;
	int m_columns;
	QImage m_strip;
	int m_hot = -1;
	int m_pressed = -1;
};

class EditorPicker : public QWidget
{
public:
	EditorPicker(QRgb color, QWidget* parent) : QWidget(parent), m_color(color) {}

	QRgb color() const { return m_color; }
	void setColor(QRgb color)
	{
		m_color = color;
		update();
	}
	void setSelected(bool selected)
	{
		m_selected = selected;
		update();
	}

	std::function<void()> onClicked;

protected:
	void paintEvent(QPaintEvent* event) override
	{
		const WhidbeyColors& c = WhidbeyStyle::colors();
		QPainter painter(this);
		QRect rc = rect();
		if (!isErase(m_color)) {
			painter.fillRect(rc, QColor(m_color));
		}
		else {
			painter.fillRect(rc, Qt::white);
			QColor checker(235, 235, 235);
			painter.fillRect(QRect(rc.left(), rc.top(), rc.width() / 2, rc.height() / 2), checker);
			painter.fillRect(
				QRect(
					rc.left() + rc.width() / 2,
					rc.top() + rc.height() / 2,
					rc.width() - rc.width() / 2,
					rc.height() - rc.height() / 2
				),
				checker
			);
		}
		auto frame = [&painter](QRect box, QColor color) {
			painter.setPen(color);
			painter.setBrush(Qt::NoBrush);
			painter.drawRect(box.adjusted(0, 0, -1, -1));
		};
		if (!m_selected) {
			frame(rc, c.darkShadow);
		}
		else {
			frame(rc, Qt::black);
			frame(rc.adjusted(1, 1, -1, -1), Qt::white);
			frame(rc.adjusted(2, 2, -2, -2), Qt::black);
		}
	}

	void mousePressEvent(QMouseEvent* event) override
	{
		if (event->button() == Qt::LeftButton && onClicked) {
			onClicked();
		}
	}

private:
	QRgb m_color;
	bool m_selected = false;
};

class EditorPicture : public QWidget
{
public:
	EditorPicture(ImageEditorDialog* dialog, QWidget* parent) : QWidget(parent), m_dialog(dialog)
	{
		m_line = Resources::cursor(lineCursorId);
		m_pickColor = Resources::cursor(pickColorCursorId);
		m_fillColor = Resources::cursor(fillColorCursorId);
		m_pencil = Resources::cursor(pencilCursorId);
		updateCursor();
	}

	void updateCursor()
	{
		switch (m_dialog->tool()) {
		case ImageEditorDialog::ToolFill:
			setCursor(m_fillColor);
			break;
		case ImageEditorDialog::ToolPencil:
			setCursor(m_pencil);
			break;
		case ImageEditorDialog::ToolGetColor:
			setCursor(m_pickColor);
			break;
		default:
			setCursor(m_line);
			break;
		}
	}

	void setItemSize(QSize item) { m_item = item; }

protected:
	QPoint toPicture(QPoint point) const { return QPoint(point.x() / m_item.width(), point.y() / m_item.height()); }

	void paintEvent(QPaintEvent* event) override
	{
		QPainter painter(this);
		QSize size = m_dialog->pictureSize();
		int cx = width();
		int cy = height();
		QImage canvas(cx, cy, QImage::Format_RGB32);
		canvas.fill(Qt::white);
		if (m_dialog->hasImage()) {
			QImage image = m_dialog->image();
			bool alpha = m_dialog->alpha();
			for (int x = 0; x < cx; x++) {
				for (int y = 0; y < cy; y++) {
					int row = cy - y - 1;
					QPoint point(size.width() * x / cx, size.height() - 1 - size.height() * y / cy);
					QRgb pixel = image.pixel(point);
					int checker = (((x + 3) / 4) + ((y + 3) / 4)) % 2 == 0 ? 255 : 235;
					QRgb color;
					if (alpha) {
						int a = qAlpha(pixel);
						color = qRgb(
							checker - (checker - qRed(pixel)) * a / 255,
							checker - (checker - qGreen(pixel)) * a / 255,
							checker - (checker - qBlue(pixel)) * a / 255
						);
					}
					else {
						color = isErase(pixel) ? qRgb(checker, checker, checker) : pixel;
					}
					canvas.setPixel(x, row, color);
				}
			}
			painter.drawImage(0, 0, canvas);
			painter.setPen(Qt::black);
			painter.setBrush(Qt::NoBrush);
			painter.drawRect(rect().adjusted(0, 0, -1, -1));
		}
	}

	void drawShape(QPoint point)
	{
		QImage image = m_base;
		QRgb color = m_dialog->selectedColor();
		auto set = [&image, color](int x, int y) {
			if (image.rect().contains(x, y)) {
				image.setPixel(x, y, isErase(color) ? eraseColor : (color | 0xff000000));
			}
		};
		QPoint from = toPicture(m_start);
		QPoint to = toPicture(point);
		int left = std::min(from.x(), to.x());
		int top = std::min(from.y(), to.y());
		int width = std::abs(to.x() - from.x());
		int height = std::abs(to.y() - from.y());
		switch (m_dialog->tool()) {
		case ImageEditorDialog::ToolRectangle: {
			auto fill = [&set](int x, int y, int cx, int cy) {
				for (int py = std::min(y, y + cy); py < std::max(y, y + cy); py++) {
					for (int px = std::min(x, x + cx); px < std::max(x, x + cx); px++) {
						set(px, py);
					}
				}
			};
			fill(left, top, width - 1, 1);
			fill(left, top, 1, height - 1);
			fill(left + width, top, -1, height);
			fill(left, top + height, width, -1);
			break;
		}
		case ImageEditorDialog::ToolCircle: {
			std::vector<EllipsePixel> pixels = rasterizeEllipse(image.size(), QRect(left, top, width, height));
			for (int y = 0; y < image.height(); y++) {
				for (int x = 0; x < image.width(); x++) {
					EllipsePixel pixel = pixels[y * image.width() + x];
					if (pixel == EllipsePen) {
						set(x, y);
					}
					else if (pixel == EllipseBrush) {
						image.setPixel(x, y, eraseColor);
					}
				}
			}
			break;
		}
		case ImageEditorDialog::ToolLine: {
			int x0 = from.x();
			int y0 = from.y();
			int dx = std::abs(to.x() - x0);
			int dy = -std::abs(to.y() - y0);
			int sx = x0 < to.x() ? 1 : -1;
			int sy = y0 < to.y() ? 1 : -1;
			int error = dx + dy;
			while (x0 != to.x() || y0 != to.y()) {
				set(x0, y0);
				int twice = 2 * error;
				if (twice >= dy) {
					error += dy;
					x0 += sx;
				}
				if (twice <= dx) {
					error += dx;
					y0 += sy;
				}
			}
			break;
		}
		default:
			break;
		}
		m_dialog->setImage(image);
	}

	void mousePressEvent(QMouseEvent* event) override
	{
		if (event->button() != Qt::LeftButton) {
			return;
		}
		QPoint point = event->position().toPoint();
		QPoint pixel = toPicture(point);
		switch (m_dialog->tool()) {
		case ImageEditorDialog::ToolFill: {
			m_dialog->beginChange();
			QImage image = m_dialog->hasImage() ? m_dialog->image() : blankImage(m_dialog->pictureSize());
			if (image.rect().contains(pixel)) {
				QRgb target = m_dialog->pixel(pixel);
				QRgb color = m_dialog->selectedColor();
				QRgb fill = isErase(color) ? eraseColor : (color | 0xff000000);
				std::vector<QPoint> stack{pixel};
				std::vector<bool> seen(image.width() * image.height());
				while (!stack.empty()) {
					QPoint p = stack.back();
					stack.pop_back();
					if (!image.rect().contains(p) || seen[p.y() * image.width() + p.x()] ||
						m_dialog->pixel(p) != target) {
						continue;
					}
					seen[p.y() * image.width() + p.x()] = true;
					image.setPixel(p, fill);
					stack.push_back(p + QPoint(1, 0));
					stack.push_back(p + QPoint(-1, 0));
					stack.push_back(p + QPoint(0, 1));
					stack.push_back(p + QPoint(0, -1));
				}
			}
			m_dialog->setImage(image);
			m_dialog->commitChange();
			break;
		}
		case ImageEditorDialog::ToolGetColor:
			m_dialog->pickColor(m_dialog->pixel(pixel));
			break;
		default:
			m_dialog->beginChange();
			m_base = m_dialog->hasImage() ? m_dialog->image() : blankImage(m_dialog->pictureSize());
			m_draw = m_dialog->selectedColor() != m_dialog->pixel(pixel) ? m_dialog->selectedColor() : eraseColor;
			m_tracked = true;
			m_start = point;
			mouseMoveEvent(event);
			break;
		}
	}

	void mouseMoveEvent(QMouseEvent* event) override
	{
		if (!m_tracked) {
			return;
		}
		QPoint point = event->position().toPoint();
		if (m_dialog->tool() == ImageEditorDialog::ToolPencil) {
			QImage image = m_dialog->hasImage() ? m_dialog->image() : blankImage(m_dialog->pictureSize());
			QPoint pixel = toPicture(point);
			if (image.rect().contains(pixel)) {
				image.setPixel(pixel, isErase(m_draw) ? eraseColor : (m_draw | 0xff000000));
			}
			m_dialog->setImage(image);
		}
		else {
			drawShape(point);
		}
	}

	void mouseReleaseEvent(QMouseEvent* event) override
	{
		if (event->button() == Qt::LeftButton && m_tracked) {
			m_tracked = false;
			m_dialog->commitChange();
		}
	}

private:
	ImageEditorDialog* m_dialog;
	QCursor m_line;
	QCursor m_pickColor;
	QCursor m_fillColor;
	QCursor m_pencil;
	QSize m_item;
	bool m_tracked = false;
	QPoint m_start;
	QRgb m_draw = 0;
	QImage m_base;
};

class EditorPreview : public QWidget
{
public:
	EditorPreview(ImageEditorDialog* dialog, QWidget* parent) : QWidget(parent), m_dialog(dialog) {}

protected:
	void paintEvent(QPaintEvent* event) override
	{
		const WhidbeyColors& c = WhidbeyStyle::colors();
		QPainter painter(this);
		painter.fillRect(rect(), c.face);
		QSize size = m_dialog->pictureSize();
		QRect button(QPoint(previewLeft, (height() - size.height()) / 2), size);
		auto draw3d = [&painter](QRect box, QColor topLeft, QColor bottomRight) {
			painter.fillRect(QRect(box.left(), box.top(), box.width() - 1, 1), topLeft);
			painter.fillRect(QRect(box.left(), box.top(), 1, box.height() - 1), topLeft);
			painter.fillRect(QRect(box.right(), box.top(), 1, box.height()), bottomRight);
			painter.fillRect(QRect(box.left(), box.bottom(), box.width(), 1), bottomRight);
		};
		QRect frame = button.adjusted(-3, -3, 3, 3);
		draw3d(frame, Qt::white, c.shadow);
		frame.adjust(-1, -1, 1, 1);
		draw3d(frame, c.face, c.darkShadow);
		if (m_dialog->hasImage()) {
			QImage image = m_dialog->image();
			if (!m_dialog->alpha()) {
				for (int y = 0; y < image.height(); y++) {
					for (int x = 0; x < image.width(); x++) {
						if (isErase(image.pixel(x, y))) {
							image.setPixel(x, y, c.face.rgb());
						}
					}
				}
			}
			painter.drawImage(button.topLeft(), image);
		}
	}

private:
	ImageEditorDialog* m_dialog;
};

ImageEditorDialog::ImageEditorDialog(const QImage& image, bool alpha, QWidget* parent)
	: CDialog(imageEditorDialogId, parent), m_size(image.isNull() ? QSize(16, 16) : image.size()), m_alpha(alpha)
{
	if (!image.isNull()) {
		m_image = image.convertToFormat(QImage::Format_ARGB32);
		for (int y = 0; y < m_image.height(); y++) {
			for (int x = 0; x < m_image.width(); x++) {
				QRgb pixel = m_image.pixel(x, y);
				int a = qAlpha(pixel);
				if (a == 0) {
					m_image.setPixel(x, y, eraseColor);
				}
				else if (m_alpha) {
					auto trip = [a](int channel) { return channel * a / 255 * 255 / a; };
					m_image.setPixel(x, y, qRgba(trip(qRed(pixel)), trip(qGreen(pixel)), trip(qBlue(pixel)), a));
				}
			}
		}
	}
}

bool ImageEditorDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	QImage strip = loadAlphaStrip(toolsBitmapId);

	m_tools = new EditorToolBar(
		{{pencilId, 0},
		 {fillId, 1},
		 {undoId, 2},
		 {redoId, 3},
		 {getColorId, 4},
		 {lineId, 5},
		 {copyId, 6},
		 {pasteId, 7},
		 {rectangleId, 8},
		 {circleId, 9},
		 {clearId, 10},
		 {openId, 11}},
		4,
		strip,
		this
	);
	m_tools->move(GetDlgItem(toolsId)->geometry().topLeft() - QPoint(1, 1));
	m_tools->onCommand = [this](unsigned id) { command(id); };
	m_tools->isChecked = [this](unsigned id) {
		return id >= pencilId && id <= circleId && m_tool == static_cast<int>(id - pencilId);
	};
	m_tools->isEnabled = [this](unsigned id) {
		return id == undoId ? !m_undo.empty() : id == redoId ? !m_redo.empty() : true;
	};
	m_tools->show();

	m_move = new EditorToolBar({{moveLeftId, 12}, {moveDownId, 13}, {moveUpId, 14}, {moveRightId, 15}}, 4, strip, this);
	m_move->move(GetDlgItem(moveId)->geometry().topLeft() - QPoint(1, 1));
	m_move->onCommand = [this](unsigned id) {
		move(
			id == moveLeftId    ? QPoint(-1, 0)
			: id == moveRightId ? QPoint(1, 0)
			: id == moveUpId    ? QPoint(0, -1)
								: QPoint(0, 1)
		);
	};
	m_move->show();

	static const struct
	{
		QRgb color;
		QPoint offset;
	} colors[] = {
		{qRgb(0x00, 0x00, 0x00), {0, 0}},
		{qRgb(0x80, 0x00, 0x00), {2, 0}},
		{qRgb(0x80, 0x80, 0x00), {3, 0}},
		{qRgb(0x00, 0x00, 0x80), {6, 0}},
		{qRgb(0x00, 0x80, 0x00), {4, 0}},
		{qRgb(0x00, 0x80, 0x80), {5, 0}},
		{qRgb(0x00, 0x00, 0xff), {6, 1}},
		{qRgb(0x80, 0x80, 0x80), {1, 0}},
		{qRgb(0xff, 0x00, 0x00), {2, 1}},
		{qRgb(0x80, 0x00, 0x80), {7, 0}},
		{qRgb(0xff, 0x00, 0xff), {7, 1}},
		{qRgb(0xff, 0xff, 0x00), {3, 1}},
		{qRgb(0x00, 0xff, 0x00), {4, 1}},
		{qRgb(0x00, 0xff, 0xff), {5, 1}},
		{qRgb(0xc0, 0xc0, 0xc0), {1, 1}},
		{qRgb(0xff, 0xff, 0xff), {0, 1}},
	};
	QRect colorArea = GetDlgItem(colorsId)->geometry();
	EditorPicker* first = nullptr;
	for (const auto& entry : colors) {
		EditorPicker* swatch = new EditorPicker(entry.color, this);
		swatch->setGeometry(QRect(
			QPoint(colorArea.left(), colorArea.top() - 4) + entry.offset * swatchStep, QSize(swatchSize, swatchSize)
		));
		swatch->onClicked = [this, swatch]() { selectPicker(swatch); };
		swatch->show();
		first = first ? first : swatch;
	}
	selectPicker(first);

	EditorPicker* erase = new EditorPicker(eraseColor, this);
	erase->onClicked = [this, erase]() { selectPicker(erase); };
	ReplaceDlgItem(eraseId, erase);

	m_picker = new EditorPicker(qRgb(0, 0, 0), this);
	m_picker->onClicked = [this]() {
		if (std::optional<QColor> color = SystemColorDialog::getColor(QColor(m_picker->color()), this)) {
			m_picker->setColor(color->rgb());
		}
		selectPicker(m_picker);
	};
	ReplaceDlgItem(pickerId, m_picker);
	m_picker->raise();

	m_picture = new EditorPicture(this, this);
	QRect area = GetDlgItem(pictureId)->geometry();
	QSize item(area.width() / m_size.width(), area.height() / m_size.height());
	ReplaceDlgItem(pictureId, m_picture);
	m_picture->setItemSize(item);
	m_picture->setGeometry(
		QRect(area.topLeft(), QSize(item.width() * m_size.width() + 1, item.height() * m_size.height() + 1))
	);

	m_preview = new EditorPreview(this, this);
	QRect previewArea = GetDlgItem(previewId)->geometry();
	ReplaceDlgItem(previewId, m_preview);
	m_preview->setGeometry(previewArea.adjusted(1, 1, -1, -1));
	return true;
}

QRgb ImageEditorDialog::selectedColor() const
{
	return m_selected ? m_selected->color() : 0;
}

QRgb ImageEditorDialog::pixel(QPoint point) const
{
	if (m_image.isNull() || !m_image.rect().contains(point)) {
		return eraseColor;
	}
	QRgb value = m_image.pixel(point);
	return isErase(value) ? eraseColor : (value | 0xff000000);
}

void ImageEditorDialog::selectPicker(EditorPicker* picker)
{
	if (m_selected) {
		m_selected->setSelected(false);
	}
	if (picker) {
		picker->setSelected(true);
	}
	m_selected = picker;
}

void ImageEditorDialog::pickColor(QRgb color)
{
	m_picker->setColor(color);
	selectPicker(m_picker);
	m_tool = static_cast<Tool>(m_previousTool);
	updateCommands();
}

void ImageEditorDialog::beginChange()
{
	m_before = m_image;
}

void ImageEditorDialog::setImage(const QImage& image)
{
	m_image = image;
	m_picture->update();
	m_preview->update();
}

void ImageEditorDialog::commitChange()
{
	m_redo.clear();
	if (m_undo.size() > undoCount) {
		m_undo.pop_front();
	}
	m_undo.push_back(m_before);
	m_before = QImage();
	setImage(m_image);
	updateCommands();
}

void ImageEditorDialog::updateCommands()
{
	m_tools->update();
	if (m_picture) {
		m_picture->updateCursor();
	}
}

void ImageEditorDialog::command(unsigned id)
{
	switch (id) {
	case undoId:
		undo();
		break;
	case redoId:
		redo();
		break;
	case clearId:
		beginChange();
		m_image = QImage();
		commitChange();
		break;
	case copyId:
		copy();
		break;
	case pasteId:
		paste();
		break;
	case openId:
		load();
		break;
	default:
		if (id >= pencilId && id <= circleId) {
			m_previousTool = m_tool == ToolGetColor ? m_previousTool : m_tool;
			m_tool = static_cast<Tool>(id - pencilId);
		}
		break;
	}
	updateCommands();
}

void ImageEditorDialog::undo()
{
	if (m_undo.empty()) {
		return;
	}
	m_redo.push_back(m_image);
	QImage image = m_undo.back();
	m_undo.pop_back();
	setImage(image);
}

void ImageEditorDialog::redo()
{
	if (m_redo.empty()) {
		return;
	}
	m_undo.push_back(m_image);
	QImage image = m_redo.back();
	m_redo.pop_back();
	setImage(image);
}

void ImageEditorDialog::move(QPoint offset)
{
	beginChange();
	QImage moved = blankImage(m_size);
	if (!m_image.isNull()) {
		QPainter painter(&moved);
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.drawImage(offset, m_image);
	}
	m_image = moved;
	commitChange();
}

void ImageEditorDialog::copy()
{
	QImage image = m_image.isNull() ? blankImage(m_size) : m_image;
	if (!m_alpha) {
		QRgb face = WhidbeyStyle::colors().face.rgb();
		for (int y = 0; y < image.height(); y++) {
			for (int x = 0; x < image.width(); x++) {
				if (isErase(image.pixel(x, y))) {
					image.setPixel(x, y, face);
				}
			}
		}
	}
	QApplication::clipboard()->setImage(image);
}

void ImageEditorDialog::paste()
{
	const QMimeData* data = QApplication::clipboard()->mimeData();
	if (!data || !data->hasImage()) {
		return;
	}
	QImage source = QApplication::clipboard()->image().convertToFormat(QImage::Format_ARGB32);
	beginChange();
	QImage image = blankImage(m_size);
	QRgb face = WhidbeyStyle::colors().face.rgb() & 0xffffff;
	for (int y = 0; y < std::min(m_size.height(), source.height()); y++) {
		for (int x = 0; x < std::min(m_size.width(), source.width()); x++) {
			QRgb pixel = source.pixel(x, y);
			image.setPixel(x, y, (pixel & 0xffffff) == face ? eraseColor : pixel);
		}
	}
	m_image = image;
	commitChange();
}

void ImageEditorDialog::load()
{
	CFileDialog dialog(true, QString(), QString(), CFileDialog::HideReadOnly, Resources::string(filterStringId));
	if (dialog.DoModal() != IDOK) {
		return;
	}
	QString path = dialog.GetPathName();
	QString extension = dialog.GetFileExt().toLower();
	QImageReader reader(path);
	if (extension == QStringLiteral("ico")) {
		reader.setScaledSize(m_size);
	}
	QImage loaded = reader.read();
	if (loaded.isNull()) {
		return;
	}
	if (extension != QStringLiteral("ico") && extension != QStringLiteral("bmp") &&
		extension != QStringLiteral("png")) {
		loaded = loaded.scaled(m_size, Qt::IgnoreAspectRatio, Qt::FastTransformation);
	}
	bool alpha = loaded.hasAlphaChannel();
	loaded = loaded.convertToFormat(QImage::Format_ARGB32);
	beginChange();
	QImage image = blankImage(m_size);
	for (int y = 0; y < std::min(m_size.height(), loaded.height()); y++) {
		for (int x = 0; x < std::min(m_size.width(), loaded.width()); x++) {
			QRgb pixel = loaded.pixel(x, y);
			image.setPixel(x, y, qAlpha(pixel) == 0 ? eraseColor : pixel);
		}
	}
	m_image = image;
	m_alpha = alpha;
	commitChange();
}
