#ifndef MENUICONGRID_H
#define MENUICONGRID_H

#include <QIcon>
#include <QWidget>
#include <functional>
#include <vector>

class QMenu;

class MenuIconGrid : public QWidget
{
public:
	MenuIconGrid(std::vector<QIcon> icons, QMenu* menu);

	QSize sizeHint() const override;

	std::function<void(const QIcon&)> onSelected;

protected:
	void paintEvent(QPaintEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void leaveEvent(QEvent* event) override;

private:
	int columns() const;
	QRect buttonRect(int index) const;
	int buttonAt(const QPoint& point) const;
	void setSelected(int index);
	void select(int index);

	std::vector<QIcon> m_icons;
	QMenu* m_menu;
	int m_selected = -1;
};

#endif // MENUICONGRID_H
