#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>
#include <vector>

class QLabel;

class StatusBar : public QStatusBar
{
public:
	explicit StatusBar(QWidget* parent);

	void addPane(QLabel* label, int textWidth);

protected:
	bool event(QEvent* event) override;
	bool eventFilter(QObject* watched, QEvent* event) override;
	void paintEvent(QPaintEvent* event) override;

private:
	struct Pane
	{
		QLabel* label;
		int textWidth;
		int left;
		int right;
	};

	void layoutPanes();
	QRect messageRect() const;

	std::vector<Pane> m_panes;
	bool m_layingOut = false;
};

#endif // STATUSBAR_H
