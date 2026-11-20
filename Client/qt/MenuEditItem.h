#ifndef MENUEDITITEM_H
#define MENUEDITITEM_H

#include <QWidget>

class QLineEdit;
class QMenu;

class MenuEditItem : public QWidget
{
public:
	MenuEditItem(const QString& caption, const QString& text, QMenu* menu);

	QLineEdit* edit() const { return m_edit; }
	QSize sizeHint() const override;

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	int labelWidth() const;
	bool selected() const;

	QString m_caption;
	QLineEdit* m_edit;
	QMenu* m_menu;
	bool m_hovered = false;
};

#endif // MENUEDITITEM_H
