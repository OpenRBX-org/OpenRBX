#ifndef MENUEXPANDBUTTON_H
#define MENUEXPANDBUTTON_H

#include <QTimer>
#include <QWidget>
#include <functional>

class QMenu;
class QWidgetAction;

class MenuExpandButton : public QWidget
{
public:
	explicit MenuExpandButton(QWidget* parent = nullptr);

	QSize sizeHint() const override;

	static QWidgetAction* addTo(QMenu* menu);

	std::function<void()> onExpand;
	std::function<bool()> expandOnHover;

protected:
	void paintEvent(QPaintEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	void expand();

	QTimer m_hoverTimer;
	bool m_selected = false;
};

#endif // MENUEXPANDBUTTON_H
