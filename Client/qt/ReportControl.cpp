#include "ReportControl.h"

#include "Resources.h"
#include "WhidbeyStyle.h"

#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionFocusRect>
#include <QStyledItemDelegate>
#include <algorithm>

static const unsigned glyphsBitmapId = 9550;
static const int glyphSize = 12;
static const int treeIndent = 20;
static const int columnShadowWidth = 4;
static const int sortTriangleSpace = 27;
static const QColor gridLineColor(234, 233, 225);
static const QColor sortedColumnShade(245, 245, 245);

static QColor mixColor(const QColor& from, const QColor& to, double amount)
{
	return QColor(
		static_cast<int>(from.red() + (to.red() - from.red()) * amount),
		static_cast<int>(from.green() + (to.green() - from.green()) * amount),
		static_cast<int>(from.blue() + (to.blue() - from.blue()) * amount)
	);
}

static int textWidth(const QFont& font, const QString& text)
{
	QFont measured = font;
	measured.setKerning(false);
	return QFontMetrics(measured).horizontalAdvance(text);
}

static QFont reportFont()
{
	QFont font = WhidbeyStyle::iconTitleFont();
	font.setStyleStrategy(QFont::PreferDefault);
	return font;
}

static int floorDivide(int value, int divisor)
{
	int quotient = value / divisor;
	return value % divisor != 0 && (value < 0) != (divisor < 0) ? quotient - 1 : quotient;
}

static void drawLine(QPainter* painter, int x, int y, int cx, int cy, const QColor& color)
{
	int steps = std::max(std::abs(cx), std::abs(cy));
	for (int step = 0; step < steps; step++) {
		painter->fillRect(
			x + floorDivide(2 * step * cx + steps - 1, 2 * steps),
			y + floorDivide(2 * step * cy + steps - 1, 2 * steps),
			1,
			1,
			color
		);
	}
}

class ReportRowDelegate : public QStyledItemDelegate
{
public:
	using QStyledItemDelegate::QStyledItemDelegate;

	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		return QSize(0, std::max(18, option.fontMetrics.height() + 4) + 1);
	}
};

class ReportHeader : public QHeaderView
{
public:
	explicit ReportHeader(QWidget* parent) : QHeaderView(Qt::Horizontal, parent) { setFont(reportFont()); }

	QSize sizeHint() const override
	{
		return QSize(QHeaderView::sizeHint().width(), fontMetrics().height() * 11 / 10 + 8);
	}

protected:
	void paintEvent(QPaintEvent* event) override
	{
		QPainter painter(viewport());
		painter.fillRect(viewport()->rect(), WhidbeyStyle::colors().face);
		for (int visual = 0; visual < count(); visual++) {
			int logical = logicalIndex(visual);
			paintSection(
				&painter,
				QRect(sectionViewportPosition(logical), 0, sectionSize(logical), height())
					.intersected(viewport()->rect()),
				logical
			);
		}
	}

	void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override
	{
		const WhidbeyColors& c = WhidbeyStyle::colors();
		int lineHeight = rect.height() - 2 - columnShadowWidth;

		painter->fillRect(rect.right(), rect.top() + 2, 1, lineHeight, c.shadow);
		painter->fillRect(rect.left(), rect.bottom() - 2, rect.width(), 1, mixColor(c.face, c.shadow, 0.25));
		painter->fillRect(rect.left(), rect.bottom() - 1, rect.width(), 1, mixColor(c.face, c.shadow, 0.4));
		painter->fillRect(rect.left(), rect.bottom(), rect.width(), 1, mixColor(c.face, c.shadow, 0.6));
		painter->fillRect(rect.left(), rect.top() + 2, 1, lineHeight, c.buttonHighlight);

		QString caption = model()->headerData(logicalIndex, Qt::Horizontal).toString();
		QRect text = rect.adjusted(3, 0, -3, -1 - columnShadowWidth);
		QRect triangle = text;
		bool sorted = isSortIndicatorShown() && sortIndicatorSection() == logicalIndex;
		if (!caption.isEmpty() && text.width() > 0) {
			if (sorted) {
				int width = std::max(-5, std::min(textWidth(font(), caption), text.width() - sortTriangleSpace));
				text.setWidth(std::min(text.width(), width + 8));
				triangle.setLeft(triangle.left() + width);
			}

			painter->setPen(c.buttonText);
			WhidbeyStyle::drawText(
				painter,
				text,
				Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
				WhidbeyStyle::endEllipsis(font(), caption, text.width())
			);
		}

		triangle.setLeft(triangle.left() + 10);
		if (sorted && triangle.width() > 15) {
			int x = triangle.left() + 10;
			int y = triangle.top() + triangle.height() / 2;
			if (sortIndicatorOrder() == Qt::DescendingOrder) {
				drawLine(painter, x, y - 3 + 6, 3, -6, c.buttonHighlight);
				drawLine(painter, x, y - 2 + 6, 3, -6, c.buttonHighlight);
				drawLine(painter, x - 4, y - 2, 3, 6, c.shadow);
				drawLine(painter, x - 1, y - 2 + 6, -4, -7, c.shadow);
				drawLine(painter, x - 4, y - 2, 8, 0, c.shadow);
			}
			else {
				drawLine(painter, x, y - 3, 3, 6, c.buttonHighlight);
				drawLine(painter, x, y - 2, 3, 6, c.buttonHighlight);
				drawLine(painter, x - 3, y + 3, 6, 0, c.buttonHighlight);
				drawLine(painter, x - 1, y - 3, -3, 6, c.shadow);
				drawLine(painter, x - 2, y - 2, -3, 6, c.shadow);
			}
		}
	}
};

ReportControl::ReportControl(QWidget* parent) : QTreeWidget(parent)
{
	setFont(reportFont());
	setHeader(new ReportHeader(this));
	setItemDelegate(new ReportRowDelegate(this));
	setIndentation(0);
	setRootIsDecorated(false);
	setItemsExpandable(false);
	setUniformRowHeights(true);
	setSortingEnabled(false);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setExpandsOnDoubleClick(false);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	header()->setSectionsClickable(true);
	header()->setSortIndicatorShown(false);
	connect(header(), &QHeaderView::sectionClicked, this, [this](int logicalIndex) {
		if (!m_columnSortAllowed) {
			return;
		}

		Qt::SortOrder order = m_sortColumn == logicalIndex && m_sortOrder == Qt::AscendingOrder ? Qt::DescendingOrder
																								: Qt::AscendingOrder;
		m_sortColumn = logicalIndex;
		m_sortOrder = order;
		header()->setSortIndicator(logicalIndex, order);
		header()->setSortIndicatorShown(true);
		sortItems(logicalIndex, order);
	});
}

void ReportControl::setColumnSortAllowed(bool allowed)
{
	m_columnSortAllowed = allowed;
}

void ReportControl::focusFirstRow()
{
	if (!currentItem() && topLevelItemCount() > 0) {
		setCurrentItem(topLevelItem(0));
	}
}

int ReportControl::treeColumn() const
{
	return 0;
}

QRect ReportControl::collapseRect(QTreeWidgetItem* row) const
{
	int depth = 0;
	for (QTreeWidgetItem* parent = row->parent(); parent; parent = parent->parent()) {
		depth++;
	}

	QRect rect = visualItemRect(row);
	return QRect(
		columnViewportPosition(treeColumn()) + depth * treeIndent + 4, rect.top(), glyphSize + 1, rect.height()
	);
}

void ReportControl::drawRow(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	static const QPixmap glyphs = []() {
		QImage image = QImage(Resources::bitmapPath(glyphsBitmapId)).convertToFormat(QImage::Format_ARGB32);
		for (int y = 0; y < image.height(); y++) {
			QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
			for (int x = 0; x < image.width(); x++) {
				if (line[x] == qRgb(255, 0, 255)) {
					line[x] = qRgba(0, 0, 0, 0);
				}
			}
		}
		return QPixmap::fromImage(image);
	}();

	const WhidbeyColors& c = WhidbeyStyle::colors();
	QTreeWidgetItem* row = itemFromIndex(index);
	int top = option.rect.top();
	int height = option.rect.height();

	QColor foreground = c.windowText;
	QColor background;
	if (selectionModel()->isSelected(index)) {
		foreground = hasFocus() ? c.highlightText : c.buttonText;
		background = hasFocus() ? c.highlight : c.face;
	}

	for (int column = 0; column < columnCount(); column++) {
		QRect cell(columnViewportPosition(column), top, columnWidth(column), height);
		painter->save();
		painter->setClipRect(cell.adjusted(0, -1, 0, 0));
		if (background.isValid()) {
			painter->fillRect(cell, background);
		}
		else if (header()->isSortIndicatorShown() && header()->sortIndicatorSection() == column) {
			painter->fillRect(cell, sortedColumnShade);
		}

		int left = cell.left();
		if (column == treeColumn()) {
			if (QTreeWidgetItem* parent = row->parent()) {
				for (QTreeWidgetItem* ancestor = parent; ancestor; ancestor = ancestor->parent()) {
					left += treeIndent;
				}

				bool lastSibling = parent->indexOfChild(row) == parent->childCount() - 1;
				painter->fillRect(left, top - 1, 1, lastSibling ? height / 2 + 1 : height + 1, foreground);
				painter->fillRect(left, top + height / 2, (glyphSize + 3) / 2 + 1, 1, foreground);

				int x = left;
				for (QTreeWidgetItem* ancestor = parent; ancestor->parent(); ancestor = ancestor->parent()) {
					x -= treeIndent;
					QTreeWidgetItem* grandparent = ancestor->parent();
					if (grandparent->indexOfChild(ancestor) != grandparent->childCount() - 1) {
						painter->fillRect(x, top - 1, 1, height + 1, foreground);
					}
				}
			}

			if (row->childCount() > 0) {
				painter->drawPixmap(
					left + 5,
					top + (height - glyphSize) / 2,
					glyphs,
					row->isExpanded() ? 0 : glyphSize,
					0,
					glyphSize,
					glyphSize
				);
			}
			left += glyphSize + 5;
		}

		QRect text(left + 2, top + 1, cell.right() + 1 - 2 - (left + 2), height - 1);
		if (text.width() > 0) {
			painter->setPen(foreground);
			painter->setClipRect(text, Qt::IntersectClip);
			painter->drawText(
				text,
				Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine,
				WhidbeyStyle::endEllipsis(
					option.font, index.sibling(index.row(), column).data().toString(), text.width()
				)
			);
		}
		painter->restore();
	}

	QRect focus(option.rect.left(), top, option.rect.width(), height - 1);
	if (hasFocus() && currentIndex().isValid() && currentIndex().row() == index.row() &&
		currentIndex().parent() == index.parent()) {
		QStyleOptionFocusRect focusOption;
		focusOption.initFrom(this);
		focusOption.rect = focus;
		focusOption.backgroundColor = c.windowText;
		style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOption, painter, this);
	}
	painter->fillRect(focus.left(), focus.bottom() + 1, focus.width(), 1, gridLineColor);
}

void ReportControl::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		QPoint point = event->position().toPoint();
		QTreeWidgetItem* row = itemAt(point);
		if (row && row->childCount() > 0 && collapseRect(row).contains(point)) {
			row->setExpanded(!row->isExpanded());
			return;
		}
	}
	QTreeWidget::mouseReleaseEvent(event);
}
