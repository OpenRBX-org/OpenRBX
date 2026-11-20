#ifndef SPLITTERPANE_H
#define SPLITTERPANE_H

#include <QMargins>
#include <QWidget>

class SplitterPane : public QWidget
{
public:
	SplitterPane(QWidget* contents, const QMargins& margins, bool ring, QWidget* parent);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	QMargins m_margins;
	bool m_ring;
};

#endif // SPLITTERPANE_H
