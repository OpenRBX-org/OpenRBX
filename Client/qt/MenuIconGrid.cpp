#include "MenuIconGrid.h"

#include "WhidbeyStyle.h"

#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <cmath>

static const int iconSize = 16;
static const int buttonSize = iconSize + 6;
static const int barBorder = 2;
static const int barTop = 4;

MenuIconGrid::MenuIconGrid(std::vector<QIcon> icons, QMenu* menu)
	: QWidget(menu), m_icons(std::move(icons)), m_menu(menu)
{
	setMouseTracking(true);
}

static QMargins menuChrome(const QMenu* menu)
{
	int frame = menu->style()->pixelMetric(QStyle::PM_MenuPanelWidth, nullptr, menu);
	int horizontal = frame + menu->style()->pixelMetric(QStyle::PM_MenuHMargin, nullptr, menu);
	int vertical = frame + menu->style()->pixelMetric(QStyle::PM_MenuVMargin, nullptr, menu);
	return QMargins(horizontal, vertical, horizontal, vertical) + menu->contentsMargins();
}

int MenuIconGrid::columns() const
{
	int width = static_cast<int>(std::sqrt(static_cast<double>(m_icons.size()))) * (iconSize + 7) + 6;
	return std::max(1, (width - 2 * barBorder) / buttonSize);
}

QSize MenuIconGrid::sizeHint() const
{
	int width = static_cast<int>(std::sqrt(static_cast<double>(m_icons.size()))) * (iconSize + 7) + 6;
	int rows = static_cast<int>((m_icons.size() + columns() - 1) / columns());
	QMargins chrome = menuChrome(m_menu);
	return QSize(
		width - chrome.left() - chrome.right(), 2 * barTop + rows * buttonSize - chrome.top() - chrome.bottom()
	);
}

QRect MenuIconGrid::buttonRect(int index) const
{
	QMargins chrome = menuChrome(m_menu);
	return QRect(
		barBorder - chrome.left() + (index % columns()) * buttonSize,
		barTop - chrome.top() + (index / columns()) * buttonSize,
		buttonSize,
		buttonSize
	);
}

int MenuIconGrid::buttonAt(const QPoint& point) const
{
	for (int i = 0; i < static_cast<int>(m_icons.size()); i++) {
		if (buttonRect(i).contains(point)) {
			return i;
		}
	}
	return -1;
}

void MenuIconGrid::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	for (int i = 0; i < static_cast<int>(m_icons.size()); i++) {
		QRect rect = buttonRect(i);
		if (i == m_selected) {
			WhidbeyStyle::drawControlRectangle(&painter, rect, true, false);
		}
		m_icons[i].paint(&painter, QRect(rect.topLeft() + QPoint(3, 3), QSize(iconSize, iconSize)));
	}
}

void MenuIconGrid::setSelected(int index)
{
	if (m_selected != index) {
		m_selected = index;
		update();
	}
}

void MenuIconGrid::mouseMoveEvent(QMouseEvent* event)
{
	setSelected(buttonAt(event->position().toPoint()));
}

void MenuIconGrid::leaveEvent(QEvent* event)
{
	setSelected(-1);
}

void MenuIconGrid::select(int index)
{
	if (index < 0) {
		return;
	}
	QIcon icon = m_icons[index];
	QWidget* top = m_menu;
	while (QMenu* parent = qobject_cast<QMenu*>(top->parentWidget())) {
		top = parent;
	}
	top->close();
	if (onSelected) {
		onSelected(icon);
	}
}

void MenuIconGrid::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		select(buttonAt(event->position().toPoint()));
	}
}
