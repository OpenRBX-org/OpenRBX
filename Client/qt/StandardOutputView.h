#ifndef STANDARDOUTPUTVIEW_H
#define STANDARDOUTPUTVIEW_H

#include "util/standardout.h"

#include <QTextBrowser>
#include <deque>
#include <mutex>

class COutputView : public QTextBrowser
{
public:
	explicit COutputView(QWidget* parent = nullptr);
	~COutputView() override = default;

protected:
	void appendMessage(RBX::MessageType type, const char* text, long long time);

private:
	int m_lineCount;
};

class CStandardOutputView : public COutputView, public RBX::Listener<RBX::StandardOut, RBX::StandardOutMessage>
{
public:
	explicit CStandardOutputView(QWidget* parent = nullptr);
	~CStandardOutputView() override;

protected:
	void onEvent(const RBX::StandardOut* source, RBX::StandardOutMessage message) override;

	void flushMessages();

private:
	std::deque<RBX::StandardOutMessage> m_pendingMessages;
	std::mutex m_pendingMutex;
};

#endif // STANDARDOUTPUTVIEW_H
