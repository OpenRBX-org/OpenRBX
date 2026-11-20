#ifndef WORKSPACETABBUTTONS_H
#define WORKSPACETABBUTTONS_H

#include <QBasicTimer>
#include <QWidget>
#include <functional>

class QTabWidget;
class QToolButton;

class WorkspaceTabButtons : public QWidget
{
public:
	explicit WorkspaceTabButtons(QTabWidget* workspace);

	void onClose(std::function<void()> callback);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

	bool eventFilter(QObject* object, QEvent* event) override;

protected:
	void paintEvent(QPaintEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void timerEvent(QTimerEvent* event) override;

private:
	QRect buttonRect(int button) const;
	int hitTest(QPoint point) const;
	bool buttonEnabled(int button) const;
	QToolButton* scrollButton(int button) const;
	void scroll(int button);

	QTabWidget* m_workspace;
	std::function<void()> m_close;
	QBasicTimer m_repeatTimer;
	int m_hot = -1;
	int m_pressed = -1;
};

#endif // WORKSPACETABBUTTONS_H
