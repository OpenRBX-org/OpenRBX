#ifndef PANECAPTION_H
#define PANECAPTION_H

#include <QWidget>
#include <functional>

class QDockWidget;

class PaneCaption : public QWidget
{
public:
	explicit PaneCaption(QDockWidget* pane);

	void onClose(std::function<void()> callback);
	void onPin(std::function<void()> callback);
	void setAutoHidden(bool autoHidden);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

protected:
	void paintEvent(QPaintEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void leaveEvent(QEvent* event) override;

private:
	int hitTest(QPoint point) const;
	void updateActive();

	QDockWidget* m_pane;
	std::function<void()> m_close;
	std::function<void()> m_pin;
	bool m_active = false;
	bool m_autoHidden = false;
	int m_hot = -1;
	int m_pressed = -1;
};

#endif // PANECAPTION_H
