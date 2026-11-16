#ifndef STANDARDOUTPUTVIEW_H
#define STANDARDOUTPUTVIEW_H

#include "OutputView.h"
#include "util/StandardOut.h"

#include <deque>
#include <mutex>

class CStandardOutputView : public COutputView, public RBX::Listener<RBX::StandardOut, RBX::StandardOutMessage>
{
	DECLARE_DYNCREATE(CStandardOutputView)

protected:
	CStandardOutputView();

public:
	~CStandardOutputView() override;

protected:
	void onEvent(const RBX::StandardOut* source, RBX::StandardOutMessage message) override;

	afx_msg LRESULT FUN_0043b8a0(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

private:
	std::deque<RBX::StandardOutMessage> m_unk0x114;
	std::mutex m_unk0x128;
};

#endif // STANDARDOUTPUTVIEW_H
