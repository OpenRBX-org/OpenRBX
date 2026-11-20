#ifndef COLORSELECTOR_H
#define COLORSELECTOR_H

#include "v8datamodel/BrickColor.h"

#include <QWidget>
#include <functional>

QColor brickColorToQColor(RBX::BrickColor color);

class ColorSelector : public QWidget
{
public:
	explicit ColorSelector(QWidget* parent = nullptr);

	void setColor(RBX::BrickColor color);
	void onColorSelected(std::function<void(RBX::BrickColor)> handler) { m_colorSelected = std::move(handler); }

	QSize sizeHint() const override;

protected:
	bool event(QEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void leaveEvent(QEvent* event) override;

private:
	QRect cellRect(int index) const;
	int hitTest(QPoint point) const;

	int m_hot = -1;
	int m_pressed = -1;
	RBX::BrickColor m_color;
	std::function<void(RBX::BrickColor)> m_colorSelected;
};

#endif // COLORSELECTOR_H
