#ifndef POPUPTOOLBAR_H
#define POPUPTOOLBAR_H

#include <QWidget>
#include <vector>

class PopupToolBar : public QWidget
{
public:
	explicit PopupToolBar(QWidget* parent);

	void addControl(QWidget* control);
	void layoutControls(QSize trailingControl);
	bool isEmpty() const { return m_controls.empty(); }

	QSize sizeHint() const override;

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;
	void paintEvent(QPaintEvent* event) override;

private:
	std::vector<QWidget*> m_controls;
	QSize m_size;
	int m_separator = -1;
};

#endif // POPUPTOOLBAR_H
