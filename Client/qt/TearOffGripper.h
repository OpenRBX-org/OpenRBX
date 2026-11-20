#ifndef TEAROFFGRIPPER_H
#define TEAROFFGRIPPER_H

#include <QWidget>

class QMenu;

class TearOffGripper : public QWidget
{
public:
	explicit TearOffGripper(QWidget* parent = nullptr);

	QSize sizeHint() const override;

	static void addTo(QMenu* menu);

protected:
	void paintEvent(QPaintEvent* event) override;
};

#endif // TEAROFFGRIPPER_H
