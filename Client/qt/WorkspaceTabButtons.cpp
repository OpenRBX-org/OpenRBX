#include "WorkspaceTabButtons.h"

#include "WhidbeyStyle.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTabBar>
#include <QTabWidget>
#include <QTimerEvent>
#include <QToolButton>

static const int buttonCount = 3;
static const int repeatDelay = 500;
static const int repeatInterval = 50;

WorkspaceTabButtons::WorkspaceTabButtons(QTabWidget* workspace) : QWidget(workspace), m_workspace(workspace)
{
	setMouseTracking(true);
	setFocusPolicy(Qt::NoFocus);

	for (int button : {WhidbeyStyle::TabNavigateLeft, WhidbeyStyle::TabNavigateRight}) {
		if (QToolButton* internal = scrollButton(button)) {
			internal->installEventFilter(this);
		}
	}
	connect(m_workspace, &QTabWidget::currentChanged, this, [this](int) { update(); });
}

void WorkspaceTabButtons::onClose(std::function<void()> callback)
{
	m_close = std::move(callback);
}

QSize WorkspaceTabButtons::sizeHint() const
{
	QSize button = WhidbeyStyle::tabNavigateButtonSize();
	return QSize(
		button.width() * buttonCount + 1, WhidbeyStyle::workspaceHeaderHeight(m_workspace->tabBar()->fontMetrics())
	);
}

QSize WorkspaceTabButtons::minimumSizeHint() const
{
	return sizeHint();
}

QToolButton* WorkspaceTabButtons::scrollButton(int button) const
{
	const char* name = button == WhidbeyStyle::TabNavigateLeft ? "ScrollLeftButton" : "ScrollRightButton";
	return m_workspace->tabBar()->findChild<QToolButton*>(QLatin1String(name), Qt::FindDirectChildrenOnly);
}

bool WorkspaceTabButtons::buttonEnabled(int button) const
{
	if (button == WhidbeyStyle::TabNavigateClose) {
		return m_workspace->count() > 0;
	}

	QToolButton* internal = scrollButton(button);
	return internal && !internal->isHidden() && internal->isEnabled();
}

QRect WorkspaceTabButtons::buttonRect(int button) const
{
	QSize size = WhidbeyStyle::tabNavigateButtonSize();
	int right = width() - 1 - (buttonCount - 1 - button) * size.width();
	int bottom = height() / 2 + size.height() / 2;
	return QRect(right - size.width(), bottom - size.height(), size.width(), size.height());
}

int WorkspaceTabButtons::hitTest(QPoint point) const
{
	for (int button = 0; button < buttonCount; button++) {
		if (buttonRect(button).contains(point)) {
			return button;
		}
	}
	return -1;
}

void WorkspaceTabButtons::scroll(int button)
{
	QToolButton* internal = scrollButton(button);
	if (internal && buttonEnabled(button)) {
		internal->click();
	}
	update();
}

bool WorkspaceTabButtons::eventFilter(QObject* object, QEvent* event)
{
	switch (event->type()) {
	case QEvent::EnabledChange:
	case QEvent::Show:
	case QEvent::Hide:
		update();
		break;
	default:
		break;
	}
	return QWidget::eventFilter(object, event);
}

void WorkspaceTabButtons::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	WhidbeyStyle::drawWorkspaceHeader(&painter, rect());

	for (int button = 0; button < buttonCount; button++) {
		bool enabled = buttonEnabled(button);
		WhidbeyStyle::drawTabNavigateButton(
			&painter,
			buttonRect(button),
			static_cast<WhidbeyStyle::TabNavigateButton>(button),
			enabled,
			enabled && button == m_hot,
			enabled && button == m_pressed
		);
	}
}

void WorkspaceTabButtons::mouseMoveEvent(QMouseEvent* event)
{
	int hot = hitTest(event->position().toPoint());
	if (hot != m_hot) {
		m_hot = hot;
		update();
	}
}

void WorkspaceTabButtons::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton) {
		return;
	}

	int button = hitTest(event->position().toPoint());
	if (button < 0 || !buttonEnabled(button)) {
		return;
	}

	m_hot = button;
	m_pressed = button;
	if (button != WhidbeyStyle::TabNavigateClose) {
		scroll(button);
		m_repeatTimer.start(repeatDelay, this);
	}
	update();
}

void WorkspaceTabButtons::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton) {
		return;
	}

	m_repeatTimer.stop();
	int pressed = m_pressed;
	m_pressed = -1;
	m_hot = hitTest(event->position().toPoint());
	update();

	if (pressed == WhidbeyStyle::TabNavigateClose && m_hot == pressed && m_close) {
		m_close();
	}
}

void WorkspaceTabButtons::leaveEvent(QEvent* event)
{
	if (m_hot != -1) {
		m_hot = -1;
		update();
	}
}

void WorkspaceTabButtons::timerEvent(QTimerEvent* event)
{
	if (event->timerId() != m_repeatTimer.timerId()) {
		QWidget::timerEvent(event);
		return;
	}

	if (m_pressed < 0 || m_pressed == WhidbeyStyle::TabNavigateClose) {
		m_repeatTimer.stop();
		return;
	}

	m_repeatTimer.start(repeatInterval, this);
	if (m_hot == m_pressed) {
		scroll(m_pressed);
	}
}
