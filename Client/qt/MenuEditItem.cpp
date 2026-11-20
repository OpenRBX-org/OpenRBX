#include "MenuEditItem.h"

#include "WhidbeyStyle.h"

#include <QEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QStyleOptionMenuItem>

static const int defaultEditWidth = 100;
static const int captionGap = 5;
static const int editInset = 3;
static const int lineEditHorizontalMargin = 2;
static const int lineEditVerticalMargin = 1;

MenuEditItem::MenuEditItem(const QString& caption, const QString& text, QMenu* menu)
	: QWidget(menu), m_caption(caption), m_edit(new QLineEdit(text, this)), m_menu(menu)
{
	setMouseTracking(true);
	QFont font = menu->font();
	font.setKerning(false);
	setFont(font);
	m_edit->setFrame(false);
	m_edit->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	m_edit->setTextMargins(-lineEditHorizontalMargin, -lineEditVerticalMargin, 0, 0);
	m_edit->setFont(font);
	m_edit->installEventFilter(this);
}

int MenuEditItem::labelWidth() const
{
	QString caption = m_caption;
	caption.remove(QLatin1Char('&'));
	return WhidbeyStyle::popupGripperWidth(m_menu) + 1 + WhidbeyStyle::popupTextPadding +
		   m_menu->fontMetrics().size(Qt::TextSingleLine, caption).width() + captionGap;
}

bool MenuEditItem::selected() const
{
	return m_hovered || m_edit->hasFocus();
}

QSize MenuEditItem::sizeHint() const
{
	return QSize(defaultEditWidth + WhidbeyStyle::popupGripperWidth(m_menu), fontMetrics().height() + 7);
}

void MenuEditItem::paintEvent(QPaintEvent* event)
{
	const WhidbeyColors& c = WhidbeyStyle::colors();
	QPainter painter(this);

	QStyleOptionMenuItem option;
	option.initFrom(m_menu);
	option.rect = geometry();
	option.text = m_caption;
	option.font = font();
	option.menuItemType = QStyleOptionMenuItem::Normal;
	option.state &= ~QStyle::State_Selected;
	painter.save();
	painter.translate(-pos());
	painter.setClipRect(QRect(pos(), QSize(labelWidth(), height())));
	style()->drawControl(QStyle::CE_MenuItem, &option, &painter, m_menu);
	painter.restore();

	QRect frame(labelWidth(), 0, width() - labelWidth(), height());
	painter.fillRect(frame.adjusted(1, 1, -1, -1), c.window);
	painter.setPen(selected() ? c.highlightBorder : c.toolbarFace);
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(frame.adjusted(0, 0, -1, -1));
}

void MenuEditItem::resizeEvent(QResizeEvent* event)
{
	m_edit->setGeometry(QRect(
		QPoint(labelWidth() + 1 + editInset, editInset), QPoint(width() - 1 - editInset, height() - 1 - editInset)
	));
}

void MenuEditItem::enterEvent(QEnterEvent* event)
{
	m_hovered = true;
	update();
}

void MenuEditItem::leaveEvent(QEvent* event)
{
	m_hovered = false;
	update();
}

bool MenuEditItem::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut) {
		update();
	}
	return false;
}
