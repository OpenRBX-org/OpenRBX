#include "PaneCaption.h"

#include "WhidbeyStyle.h"

#include <QApplication>
#include <QDockWidget>
#include <QMouseEvent>
#include <QPainter>

static const int buttonCount = 2;

PaneCaption::PaneCaption(QDockWidget* pane) : QWidget(pane), m_pane(pane)
{
	setMouseTracking(true);
	setFocusPolicy(Qt::NoFocus);
	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

	m_close = [this]() { m_pane->close(); };

	connect(m_pane, &QWidget::windowTitleChanged, this, [this](const QString&) { update(); });
	connect(qApp, &QApplication::focusChanged, this, [this](QWidget*, QWidget*) { updateActive(); });
	updateActive();
}

void PaneCaption::onClose(std::function<void()> callback)
{
	m_close = std::move(callback);
}

void PaneCaption::onPin(std::function<void()> callback)
{
	m_pin = std::move(callback);
}

void PaneCaption::setAutoHidden(bool autoHidden)
{
	if (m_autoHidden != autoHidden) {
		m_autoHidden = autoHidden;
		update();
	}
}

QSize PaneCaption::sizeHint() const
{
	return QSize(40, WhidbeyStyle::paneCaptionHeight());
}

QSize PaneCaption::minimumSizeHint() const
{
	return QSize(0, WhidbeyStyle::paneCaptionHeight());
}

void PaneCaption::updateActive()
{
	QWidget* focus = QApplication::focusWidget();
	bool active = focus && (focus == m_pane || m_pane->isAncestorOf(focus));
	if (active != m_active) {
		m_active = active;
		update();
	}
}

int PaneCaption::hitTest(QPoint point) const
{
	for (int button = 0; button < buttonCount; button++) {
		if (WhidbeyStyle::paneCaptionButtonRect(rect(), button).contains(point)) {
			return button;
		}
	}
	return -1;
}

void PaneCaption::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	WhidbeyStyle::drawPaneCaption(&painter, rect(), m_pane->windowTitle(), m_active, isEnabled(), buttonCount);

	for (int button = 0; button < buttonCount; button++) {
		WhidbeyStyle::drawPaneCaptionButton(
			&painter,
			WhidbeyStyle::paneCaptionButtonRect(rect(), button),
			static_cast<WhidbeyStyle::PaneCaptionButton>(button),
			m_active,
			button == m_hot,
			button == m_pressed && button == m_hot,
			m_autoHidden
		);
	}
}

void PaneCaption::mouseMoveEvent(QMouseEvent* event)
{
	int hot = hitTest(event->position().toPoint());
	if (hot != m_hot) {
		m_hot = hot;
		update();
	}

	if (m_pressed < 0) {
		event->ignore();
	}
}

void PaneCaption::mousePressEvent(QMouseEvent* event)
{
	int button = event->button() == Qt::LeftButton ? hitTest(event->position().toPoint()) : -1;
	if (button < 0) {
		event->ignore();
		return;
	}

	m_hot = button;
	m_pressed = button;
	update();
}

void PaneCaption::mouseReleaseEvent(QMouseEvent* event)
{
	if (m_pressed < 0) {
		event->ignore();
		return;
	}

	int pressed = m_pressed;
	m_pressed = -1;
	m_hot = hitTest(event->position().toPoint());
	update();

	if (m_hot != pressed) {
		return;
	}

	if (pressed == WhidbeyStyle::PaneCaptionClose) {
		if (m_close) {
			m_close();
		}
	}
	else if (m_pin) {
		m_pin();
	}
}

void PaneCaption::leaveEvent(QEvent* event)
{
	if (m_hot != -1) {
		m_hot = -1;
		update();
	}
}
