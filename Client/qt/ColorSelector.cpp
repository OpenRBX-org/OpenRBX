#include "ColorSelector.h"

#include <G3D/Color3uint8.h>
#include <QHelpEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>
#include <QToolTip>

QColor brickColorToQColor(RBX::BrickColor color)
{
	G3D::Color3uint8 rgb = color.color3uint8();
	return QColor(rgb.r, rgb.g, rgb.b);
}

ColorSelector::ColorSelector(QWidget* parent) : QWidget(parent), m_color(RBX::BrickColor::colorPalette().front())
{
	setMouseTracking(true);
}

QSize ColorSelector::sizeHint() const
{
	return QSize(260, 136);
}

void ColorSelector::setColor(RBX::BrickColor color)
{
	m_color = color;
	update();
}

QRect ColorSelector::cellRect(int index) const
{
	int x = index % 8;
	int y = index / 8;
	return QRect(2 + x * 32, 4 + y * 32, 32, 32);
}

int ColorSelector::hitTest(QPoint point) const
{
	int count = static_cast<int>(RBX::BrickColor::colorPalette().size());
	for (int i = 0; i < count; i++) {
		if (cellRect(i).contains(point)) {
			return i;
		}
	}
	return -1;
}

void ColorSelector::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	const RBX::BrickColor::Colors& colors = RBX::BrickColor::colorPalette();

	for (int i = 0; i < static_cast<int>(colors.size()); i++) {
		QRect cell = cellRect(i);
		bool selected = colors[i] == m_color;

		if (i == m_hot || selected) {
			QStyleOptionToolButton option;
			option.initFrom(this);
			option.styleObject = nullptr;
			option.rect = cell;
			option.state = QStyle::State_Enabled | QStyle::State_AutoRaise;
			if (i == m_hot) {
				option.state |= QStyle::State_MouseOver | QStyle::State_Raised;
			}
			if (i == m_pressed) {
				option.state |= QStyle::State_Sunken;
			}
			if (selected) {
				option.state |= QStyle::State_On;
			}
			style()->drawPrimitive(QStyle::PE_PanelButtonTool, &option, &painter, this);
		}

		cell.adjust(3, 3, -3, -3);
		painter.fillRect(cell, brickColorToQColor(colors[i]));
		painter.setPen(palette().color(QPalette::Dark));
		painter.drawRect(cell.adjusted(0, 0, -1, -1));
	}
}

bool ColorSelector::event(QEvent* event)
{
	if (event->type() == QEvent::ToolTip) {
		QHelpEvent* help = static_cast<QHelpEvent*>(event);
		int hit = hitTest(help->pos());
		if (hit >= 0) {
			QToolTip::showText(help->globalPos(), RBX::BrickColor::colorPalette()[hit].name().c_str(), this);
		}
		else {
			QToolTip::hideText();
		}
		return true;
	}
	return QWidget::event(event);
}

void ColorSelector::mouseMoveEvent(QMouseEvent* event)
{
	int hit = hitTest(event->position().toPoint());
	if (hit != -1 && hit != m_hot) {
		m_hot = hit;
		update();
	}
}

void ColorSelector::mousePressEvent(QMouseEvent* event)
{
	m_pressed = hitTest(event->position().toPoint());
	update();
}

void ColorSelector::mouseReleaseEvent(QMouseEvent* event)
{
	int hit = hitTest(event->position().toPoint());
	m_pressed = -1;
	if (hit == -1) {
		update();
		return;
	}

	m_color = RBX::BrickColor::colorPalette()[hit];
	update();
	if (m_colorSelected) {
		m_colorSelected(m_color);
	}
}

void ColorSelector::leaveEvent(QEvent* event)
{
	m_hot = -1;
	update();
}
