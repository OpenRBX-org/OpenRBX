#ifndef CHILDFRM_H
#define CHILDFRM_H

#include <QWidget>

class CPreviewView;
class View;

class CChildFrame : public QWidget
{
public:
	explicit CChildFrame(View* view);
	~CChildFrame() override;

	View* view() const { return m_view; }
	View* activeView() const { return m_activeView; }
	void setActiveView(View* view);
#ifdef _WIN32
	void setPreviewMode(bool preview, CPreviewView* view);
#endif

	void activateFrame();
	void updateTitle();
	void requestClose();

protected:
	void closeEvent(QCloseEvent* event) override;

private:
	View* m_view;
	View* m_activeView;
#ifdef _WIN32
	CPreviewView* m_preview = nullptr;
#endif
};

#endif // CHILDFRM_H
