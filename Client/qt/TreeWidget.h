#ifndef TREEWIDGET_H
#define TREEWIDGET_H

#include <QStyledItemDelegate>
#include <QTreeWidget>

class TreeItemDelegate : public QStyledItemDelegate
{
public:
	static const int imageSize = 16;
	static const int imageOffset = 1;
	static const int labelOffset = imageOffset + imageSize + 3;
	static const int labelPadding = 2;

	explicit TreeItemDelegate(QTreeView* tree);

	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	static int labelWidth(const QFont& font, const QString& text);
};

class TreeWidget : public QTreeWidget
{
public:
	static const int indent = 19;
	static const int leftMargin = 2;
	static const int buttonDrop = 2;

	explicit TreeWidget(QWidget* parent = nullptr);

	QRect itemLabelRect(const QTreeWidgetItem* item) const;

protected:
	void updateGeometries() override;
	void drawBranches(QPainter* painter, const QRect& branches, const QModelIndex& index) const override;
};

#endif // TREEWIDGET_H
