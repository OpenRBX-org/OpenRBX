#ifndef WEBTOOLBOX_H
#define WEBTOOLBOX_H

#include "HtmlView.h"

#include <memory>

class CRobloxWnd;
class CWorkspace;

class CWebToolbox : public CHtmlView
{
public:
	CWebToolbox(QWidget* parent, CRobloxWnd* viewport, std::shared_ptr<CWorkspace> workspace);

	void initialUpdate() override;
	void activate(bool active, View* other) override;

	void showCategory(int category);
	void setWorkspace(std::shared_ptr<CWorkspace> workspace);

protected:
	std::shared_ptr<CExternal> OnGetExternal() override;
	void OnBrowserFocus() override;

	void focusInEvent(QFocusEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;

private:
	void focusViewport();

	CRobloxWnd* m_viewport;
	std::shared_ptr<CWorkspace> m_workspace;
};

#endif // WEBTOOLBOX_H
