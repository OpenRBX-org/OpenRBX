#ifndef REPORTCONTROL_H
#define REPORTCONTROL_H

#include <QTreeWidget>

class ReportControl : public QTreeWidget
{
public:
	explicit ReportControl(QWidget* parent = nullptr);

	void setColumnSortAllowed(bool allowed);
	void focusFirstRow();

protected:
	void mouseReleaseEvent(QMouseEvent* event) override;
	void drawRow(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	virtual int treeColumn() const;

private:
	QRect collapseRect(QTreeWidgetItem* row) const;

	bool m_columnSortAllowed = false;
	int m_sortColumn = -1;
	Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
};

#endif // REPORTCONTROL_H
