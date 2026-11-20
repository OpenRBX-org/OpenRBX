#include "TearOffGripper.h"

#include "WhidbeyStyle.h"

#include <QMenu>
#include <QPainter>
#include <QWidgetAction>
#include <algorithm>

static const int gripperHeight = 9;
static const int dotSpacing = 4;
static const int dotSpan = 32;

TearOffGripper::TearOffGripper(QWidget* parent) : QWidget(parent)
{
}

QSize TearOffGripper::sizeHint() const
{
	return QSize(1, gripperHeight);
}

void TearOffGripper::addTo(QMenu* menu)
{
	QWidgetAction* action = new QWidgetAction(menu);
	action->setDefaultWidget(new TearOffGripper(menu));
	action->setEnabled(false);
	menu->addAction(action);
}

void TearOffGripper::paintEvent(QPaintEvent* event)
{
	const WhidbeyColors& c = WhidbeyStyle::colors();
	QPainter painter(this);
	painter.fillRect(0, 0, width(), gripperHeight - 1, c.menuBarFace);
	painter.fillRect(0, 0, width(), gripperHeight - 2, c.toolbarFace);

	int center = width() / 2;
	int half = std::min(dotSpan, width() / 2 - 10);
	for (int x = center - half; x < center + half; x += dotSpacing) {
		painter.fillRect(x + 1, 3, 2, 2, c.buttonHighlight);
		painter.fillRect(x, 2, 2, 2, c.toolbarGripper);
	}
}
