#include "StatusBar.h"

#include "WhidbeyStyle.h"

#include <QEvent>
#include <QLabel>
#include <QPainter>
#include <QSizeGrip>
#include <QStyleOption>
#include <algorithm>

static const int paneBorder = 6;
static const int paneGap = 2;
static const int sizeGripSpace = 19;
static const int sizeGripWidth = 16;

static void fillNormalized(QPainter* painter, int x, int y, int width, int height, const QColor& color)
{
	if (width < 0) {
		x += width;
		width = -width;
	}
	if (height < 0) {
		y += height;
		height = -height;
	}
	if (width > 0 && height > 0) {
		painter->fillRect(x, y, width, height, color);
	}
}

static void draw3dRect(QPainter* painter, int x, int y, int width, int height, const QColor& color)
{
	fillNormalized(painter, x, y, width - 1, 1, color);
	fillNormalized(painter, x, y, 1, height - 1, color);
	fillNormalized(painter, x + width, y, -1, height, color);
	fillNormalized(painter, x, y + height, width, -1, color);
}

StatusBar::StatusBar(QWidget* parent) : QStatusBar(parent)
{
}

void StatusBar::addPane(QLabel* label, int textWidth)
{
	if (m_panes.empty()) {
		addWidget(label, 1);
	}
	else {
		addPermanentWidget(label);
	}
	label->setMinimumWidth(0);
	label->setAttribute(Qt::WA_TransparentForMouseEvents);
	label->installEventFilter(this);
	m_panes.push_back({label, textWidth, 0, 0});
	if (QSizeGrip* grip = findChild<QSizeGrip*>()) {
		grip->raise();
	}
	layoutPanes();
}

bool StatusBar::event(QEvent* event)
{
	bool result = QStatusBar::event(event);
	switch (event->type()) {
	case QEvent::LayoutRequest:
	case QEvent::Resize:
	case QEvent::Show:
		layoutPanes();
		break;
	default:
		break;
	}
	return result;
}

bool StatusBar::eventFilter(QObject* watched, QEvent* event)
{
	if (!m_layingOut && (event->type() == QEvent::Move || event->type() == QEvent::Resize)) {
		layoutPanes();
	}
	return QStatusBar::eventFilter(watched, event);
}

void StatusBar::layoutPanes()
{
	if (m_panes.empty() || m_layingOut) {
		return;
	}

	const bool zoomed = window()->isMaximized();
	const int barWidth = width();
	const int inside = barWidth - (zoomed ? 0 : sizeGripSpace);

	int extra = inside + paneGap;
	for (size_t i = 0; i < m_panes.size(); i++) {
		extra -= m_panes[i].textWidth + paneBorder + paneGap;
	}

	std::vector<int> rights;
	int right = 0;
	for (size_t i = 0; i < m_panes.size(); i++) {
		right += m_panes[i].textWidth + paneBorder;
		if (i == 0 && extra > 0) {
			right += extra;
		}
		rights.push_back(right);
		right += paneGap;
	}

	const int limit = zoomed ? barWidth : barWidth - sizeGripWidth;
	m_layingOut = true;
	for (size_t i = 0; i < m_panes.size(); i++) {
		Pane& pane = m_panes[i];
		pane.left = i == 0 ? 0 : std::min(m_panes[i - 1].right + paneGap, limit);
		pane.right = i + 1 == m_panes.size() ? rights[i] : std::min(rights[i], limit);
		pane.right = std::max(pane.right, pane.left);
		QRect geometry = pane.label->geometry();
		pane.label->setGeometry(
			pane.left + 2, geometry.y(), std::max(0, pane.right - pane.left - 4), geometry.height()
		);
	}
	m_layingOut = false;
	update();
}

QRect StatusBar::messageRect() const
{
	int left = 6;
	int right = width() - 12;
	if (QSizeGrip* grip = findChild<QSizeGrip*>(); grip && grip->isVisible()) {
		right = grip->x();
	}
	for (size_t i = 1; i < m_panes.size(); i++) {
		if (m_panes[i].label->isVisible()) {
			right = std::min(right, m_panes[i].label->x() - 2);
		}
	}
	return QRect(left, 0, right - left, height());
}

void StatusBar::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	QStyleOption option;
	option.initFrom(this);
	style()->drawPrimitive(QStyle::PE_PanelStatusBar, &option, &painter, this);

	const QColor color = WhidbeyStyle::colors().statusPane;
	for (size_t i = 1; i < m_panes.size(); i++) {
		const Pane& pane = m_panes[i];
		if (pane.label->isVisible()) {
			draw3dRect(&painter, pane.left, 2, pane.right - pane.left, height() - 2, color);
		}
	}

	const QString message = currentMessage();
	if (!message.isEmpty()) {
		painter.setPen(palette().windowText().color());
		painter.drawText(messageRect(), Qt::AlignLeading | Qt::AlignVCenter | Qt::TextSingleLine, message);
	}
}
