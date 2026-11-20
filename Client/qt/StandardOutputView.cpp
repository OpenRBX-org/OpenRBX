#include "StandardOutputView.h"

#include "BrowserScrollBar.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextFrame>
#include <algorithm>
#include <cmath>
#include <ctime>

COutputView::COutputView(QWidget* parent) : QTextBrowser(parent), m_lineCount(0)
{
	installBrowserScrollBar(this);
	setFont(QApplication::font("QMessageBox"));
	setOpenLinks(false);

	QTextFrameFormat format = document()->rootFrame()->frameFormat();
	format.setTopMargin(15);
	format.setBottomMargin(0);
	format.setLeftMargin(10);
	format.setRightMargin(10);
	document()->rootFrame()->setFrameFormat(format);
}

void COutputView::appendMessage(RBX::MessageType type, const char* text, long long time)
{
	QString escaped = QString::fromLocal8Bit(text).toHtmlEscaped();

	QString html;
	if (type != RBX::MESSAGE_OUTPUT && time != 0) {
		std::time_t value = static_cast<std::time_t>(time);
		char buffer[32];
#ifdef _WIN32
		ctime_s(buffer, sizeof(buffer), &value);
#else
		ctime_r(&value, buffer);
#endif
		html = QString::fromLatin1(buffer);
		html += " - ";
	}

	switch (type) {
	case RBX::MESSAGE_INFO:
		html += "<FONT color=\"blue\">";
		html += escaped;
		html += "</FONT>";
		break;
	case RBX::MESSAGE_WARNING:
		html += "<STRONG><FONT color=\"orange\">";
		html += escaped;
		html += "</FONT></STRONG>";
		break;
	case RBX::MESSAGE_ERROR:
		html += "<STRONG><FONT color=\"red\">";
		html += escaped;
		html += "</FONT></STRONG>";
		break;
	case RBX::MESSAGE_OUTPUT:
		html += escaped;
		break;
	}

	QTextCursor cursor(document());
	cursor.movePosition(QTextCursor::End);
	if (m_lineCount > 0) {
		cursor.insertBlock(QTextBlockFormat(), QTextCharFormat());
	}
	cursor.insertHtml(html);
	m_lineCount++;

	if (m_lineCount > 300) {
		QTextCursor first(document()->firstBlock());
		first.select(QTextCursor::BlockUnderCursor);
		first.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
		first.removeSelectedText();
		m_lineCount--;
	}

	QRectF lastBlock = document()->documentLayout()->blockBoundingRect(document()->lastBlock());
	verticalScrollBar()->setValue(std::max(0, static_cast<int>(std::ceil(lastBlock.bottom())) - viewport()->height()));
}

CStandardOutputView::CStandardOutputView(QWidget* parent) : COutputView(parent)
{
	std::scoped_lock lock(RBX::StandardOut::singleton()->sync);
	RBX::startListening<RBX::StandardOut, RBX::StandardOutMessage>(RBX::StandardOut::singleton(), this);
}

CStandardOutputView::~CStandardOutputView()
{
	std::scoped_lock lock(RBX::StandardOut::singleton()->sync);
	RBX::stopListening<RBX::StandardOut, RBX::StandardOutMessage>(RBX::StandardOut::singleton(), this);
}

void CStandardOutputView::onEvent(const RBX::StandardOut* source, RBX::StandardOutMessage message)
{
	std::scoped_lock lock(m_pendingMutex);
	if (m_pendingMessages.empty()) {
		QMetaObject::invokeMethod(this, [this]() { flushMessages(); }, Qt::QueuedConnection);
	}
	m_pendingMessages.push_back(message);
	if (m_pendingMessages.size() > 300) {
		m_pendingMessages.pop_front();
	}
}

void CStandardOutputView::flushMessages()
{
	while (true) {
		std::string message;
		RBX::MessageType type;
		long long time;
		{
			std::scoped_lock lock(m_pendingMutex);
			if (m_pendingMessages.empty()) {
				break;
			}

			type = m_pendingMessages.front().type;
			message = m_pendingMessages.front().message;
			time = m_pendingMessages.front().time;
			m_pendingMessages.pop_front();
		}

		appendMessage(type, message.c_str(), time);
	}
}
