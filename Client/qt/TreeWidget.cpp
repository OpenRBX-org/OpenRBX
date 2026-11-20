#include "TreeWidget.h"

#include "WhidbeyStyle.h"

#include <QPainter>
#include <algorithm>

static const int focusRectParity = 1;

TreeItemDelegate::TreeItemDelegate(QTreeView* tree) : QStyledItemDelegate(tree)
{
}

QSize TreeItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	return QSize(
		labelOffset + labelWidth(option.font, index.data().toString()),
		std::max(imageSize + 2, option.fontMetrics.height() + 3)
	);
}

void TreeItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QStyleOptionViewItem item = option;
	initStyleOption(&item, index);

	QRect row = item.rect;
	QPoint imagePosition(row.left() + imageOffset, row.top() + (row.height() - imageSize) / 2);
	QVariant decoration = index.data(Qt::DecorationRole);
	if (decoration.metaType().id() == QMetaType::QImage) {
		WhidbeyStyle::drawAlphaImage(painter, imagePosition, decoration.value<QImage>());
	}
	else {
		item.icon.paint(painter, QRect(imagePosition, QSize(imageSize, imageSize)));
	}

	QRect label(row.left() + labelOffset, row.top(), labelWidth(item.font, item.text), row.height());
	QPalette::ColorGroup group = !(item.state & QStyle::State_Enabled) ? QPalette::Disabled
								 : (item.state & QStyle::State_Active) ? QPalette::Active
																	   : QPalette::Inactive;
	bool selected = item.state & QStyle::State_Selected;
	if (selected) {
		painter->fillRect(label, item.palette.brush(group, QPalette::Highlight));
	}

	painter->save();
	painter->setFont(item.font);
	painter->setPen(item.palette.color(group, selected ? QPalette::HighlightedText : QPalette::Text));
	painter->drawText(
		QRect(
			label.left() + labelPadding + 1,
			row.top() + (row.height() - item.fontMetrics.height()) / 2,
			label.width(),
			item.fontMetrics.height()
		),
		Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine,
		item.text
	);
	painter->restore();

	if (item.state & QStyle::State_HasFocus) {
		painter->save();
		painter->setPen(Qt::black);
		for (int x = label.left(); x <= label.right(); x++) {
			if ((x + label.top()) % 2 == focusRectParity) {
				painter->drawPoint(x, label.top());
			}
			if ((x + label.bottom()) % 2 == focusRectParity) {
				painter->drawPoint(x, label.bottom());
			}
		}
		for (int y = label.top() + 1; y < label.bottom(); y++) {
			if ((label.left() + y) % 2 == focusRectParity) {
				painter->drawPoint(label.left(), y);
			}
			if ((label.right() + y) % 2 == focusRectParity) {
				painter->drawPoint(label.right(), y);
			}
		}
		painter->restore();
	}
}

int TreeItemDelegate::labelWidth(const QFont& font, const QString& text)
{
	QFont measured = font;
	measured.setKerning(false);
	return QFontMetrics(measured).horizontalAdvance(text) + 2 * labelPadding;
}

TreeWidget::TreeWidget(QWidget* parent) : QTreeWidget(parent)
{
	setIndentation(indent);
	setUniformRowHeights(true);
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
	setItemDelegate(new TreeItemDelegate(this));
}

QRect TreeWidget::itemLabelRect(const QTreeWidgetItem* item) const
{
	QRect cell = visualItemRect(item);
	return QRect(
		cell.left() + TreeItemDelegate::labelOffset,
		cell.top(),
		TreeItemDelegate::labelWidth(font(), item->text(0)),
		cell.height()
	);
}

void TreeWidget::updateGeometries()
{
	QTreeWidget::updateGeometries();
	setViewportMargins(leftMargin, 0, 0, 0);
}

void TreeWidget::drawBranches(QPainter* painter, const QRect& branches, const QModelIndex& index) const
{
	QTreeWidget::drawBranches(painter, branches.translated(0, buttonDrop), index);
}
