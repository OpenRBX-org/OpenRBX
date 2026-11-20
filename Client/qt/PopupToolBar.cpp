#include "PopupToolBar.h"

#include "WhidbeyStyle.h"

#include <QMouseEvent>
#include <QPainter>
#include <algorithm>

static const int separatorSize = 6;

namespace {

struct ButtonInfo
{
	QSize size;
	bool beginGroup;
	bool wrap;
	QRect rect;
};

QSize calcSize(std::vector<ButtonInfo>& data)
{
	QPoint cur(0, 0);
	QSize result(0, 0);
	int row = 0;
	bool first = true;
	for (ButtonInfo& control : data) {
		if (control.beginGroup && !control.wrap && !first) {
			cur.rx() += separatorSize;
		}
		if (control.wrap) {
			cur.ry() += row + (control.beginGroup ? separatorSize : 0);
			row = 0;
			cur.setX(0);
		}
		row = std::max(row, control.size.height());

		control.rect = QRect(cur, control.size);

		result.setWidth(std::max(cur.x() + control.size.width(), result.width()));
		result.setHeight(std::max(cur.y() + control.size.height(), result.height()));

		cur.rx() += control.size.width();
		first = false;
	}
	return result;
}

void wrapToolBar(std::vector<ButtonInfo>& data, int width)
{
	int x = 0;
	bool first = true;
	int count = static_cast<int>(data.size());

	for (int i = 0; i < count; i++) {
		ButtonInfo& control = data[i];
		control.wrap = false;

		int dx = control.size.width();
		if (control.beginGroup && !first) {
			dx += separatorSize;
		}

		if (x + dx > width || (control.beginGroup && !first)) {
			for (int j = i; j >= 0 && !data[j].wrap; j--) {
				if (data[j].beginGroup) {
					i = j;
					break;
				}
			}
			x = data[i].size.width();
			data[i].wrap = true;
		}
		else {
			x += dx;
		}

		first = false;
	}
}

void sizePopupToolBar(std::vector<ButtonInfo>& data)
{
	wrapToolBar(data, 0);
	QSize sizeMin = calcSize(data);

	wrapToolBar(data, 32767);
	QSize sizeMax = calcSize(data);

	while (sizeMin.width() < sizeMax.width()) {
		QSize sizeMid((sizeMin.width() + sizeMax.width()) / 2, 0);

		wrapToolBar(data, sizeMid.width());
		sizeMid = calcSize(data);

		if (sizeMid.height() > sizeMid.width()) {
			if (sizeMin == sizeMid) {
				return;
			}
			sizeMin = sizeMid;
		}
		else if (sizeMid.height() < sizeMid.width()) {
			sizeMax = sizeMid;
		}
		else {
			return;
		}
	}
}

} // namespace

PopupToolBar::PopupToolBar(QWidget* parent) : QWidget(parent)
{
	setProperty("popupToolBar", true);
}

void PopupToolBar::addControl(QWidget* control)
{
	m_controls.push_back(control);
	control->installEventFilter(this);
}

void PopupToolBar::layoutControls(QSize trailingControl)
{
	std::vector<ButtonInfo> data;
	for (QWidget* control : m_controls) {
		data.push_back({control->sizeHint(), false, false, QRect()});
	}
	data.push_back({trailingControl, true, false, QRect()});

	wrapToolBar(data, 0);
	QSize size = calcSize(data);
	if (size.height() > size.width() * 3) {
		sizePopupToolBar(data);
	}
	else {
		wrapToolBar(data, size.width());
	}
	size = calcSize(data);

	for (size_t i = 0; i < m_controls.size(); i++) {
		m_controls[i]->setGeometry(data[i].rect);
	}

	const QRect& trailing = data.back().rect;
	m_separator = trailing.top() - separatorSize + 2;
	m_size = QSize(size.width(), trailing.top());
	resize(m_size);
}

QSize PopupToolBar::sizeHint() const
{
	return m_size;
}

bool PopupToolBar::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::MouseButtonRelease) {
		QWidget* control = static_cast<QWidget*>(watched);
		QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
		if (mouse->button() == Qt::LeftButton && control->rect().contains(mouse->position().toPoint())) {
			window()->close();
		}
	}
	return QWidget::eventFilter(watched, event);
}

void PopupToolBar::paintEvent(QPaintEvent*)
{
	if (m_separator < 0) {
		return;
	}
	QPainter painter(this);
	painter.fillRect(0, m_separator, width(), 1, WhidbeyStyle::colors().separator);
}
