#include "SplitterPane.h"

#include "WhidbeyStyle.h"

#include <QPainter>
#include <QVBoxLayout>

SplitterPane::SplitterPane(QWidget* contents, const QMargins& margins, bool ring, QWidget* parent)
	: QWidget(parent), m_margins(margins), m_ring(ring)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(margins);
	layout->addWidget(contents);
}

void SplitterPane::paintEvent(QPaintEvent* event)
{
	const WhidbeyColors& c = WhidbeyStyle::colors();
	QPainter painter(this);
	painter.fillRect(rect(), c.face);
	if (!m_ring) {
		return;
	}

	QRect ring = rect().marginsRemoved(m_margins).adjusted(-1, -1, 1, 1);
	painter.fillRect(ring.left(), ring.top(), ring.width() - 1, 1, c.shadow);
	painter.fillRect(ring.left(), ring.top(), 1, ring.height() - 1, c.shadow);
	painter.fillRect(ring.right(), ring.top(), 1, ring.height(), c.buttonHighlight);
	painter.fillRect(ring.left(), ring.bottom(), ring.width(), 1, c.buttonHighlight);
}
