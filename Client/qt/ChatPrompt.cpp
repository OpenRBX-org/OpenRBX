#include "ChatPrompt.h"

#include "network/Players.h"
#include "util/object.h"
#include "v8datamodel/DataModel.h"

#include <QApplication>
#include <QKeyEvent>

static const char* const chatPromptText = "To chat click here or press the \"/\" key";

RBX::GuiResponse ChatEnter::process(const RBX::GuiEvent& event)
{
	if (event.isKeyPressedEvent(SDLK_SLASH)) {
		CChatPrompt* prompt = m_prompt;
		if (prompt) {
			QMetaObject::invokeMethod(prompt, [prompt]() { prompt->activate(); }, Qt::QueuedConnection);
		}
		return RBX::GuiResponse::used();
	}

	return RBX::GuiResponse::notUsed();
}

ChatEnter::ChatEnter() : m_prompt(nullptr)
{
}

CChatPrompt::CChatPrompt(QWidget* parent) : QPlainTextEdit(parent), m_viewport(nullptr), m_players(nullptr)
{
	setFrameShape(QFrame::NoFrame);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setLineWrapMode(QPlainTextEdit::WidgetWidth);
	document()->setDocumentMargin(1);
	setEnabled(false);

	QFont font = QApplication::font("QMenu");
	font.setBold(true);
	setFont(font);

	setPlainText(chatPromptText);
	CtlColor();
}

CChatPrompt::~CChatPrompt()
{
	setDataModel(nullptr, nullptr);
}

void CChatPrompt::CtlColor()
{
	QPalette colors = palette();
	colors.setColor(QPalette::Base, QColor(64, 64, 64));
	colors.setColor(QPalette::Window, QColor(64, 64, 64));

	QTextCharFormat format;
	if (hasFocus()) {
		colors.setColor(QPalette::Text, QColor(0, 0, 0));
		format.setBackground(QColor(230, 230, 250));
	}
	else {
		colors.setColor(QPalette::Text, QColor(255, 255, 200));
		format.setBackground(QColor(64, 64, 64));
	}
	setPalette(colors);

	QTextCursor cursor(document());
	cursor.select(QTextCursor::Document);
	cursor.mergeCharFormat(format);
	mergeCurrentCharFormat(format);
}

void CChatPrompt::clearPromptText()
{
	if (toPlainText() == chatPromptText) {
		setPlainText("");
	}
}

void CChatPrompt::focusInEvent(QFocusEvent* event)
{
	QPlainTextEdit::focusInEvent(event);
	clearPromptText();
	CtlColor();
}

void CChatPrompt::focusOutEvent(QFocusEvent* event)
{
	QPlainTextEdit::focusOutEvent(event);
	CtlColor();
}

void CChatPrompt::keyPressEvent(QKeyEvent* event)
{
	switch (event->key()) {
	case Qt::Key_Return:
	case Qt::Key_Enter: {
		QString text = toPlainText();
		if (m_players && !text.isEmpty()) {
			try {
				{
					RBX::DataModel::Lock lock(m_dataModel);
					m_players->chat(text.toStdString());
				}
				setPlainText(chatPromptText);
			}
			catch (std::exception&) {
			}
		}
		m_viewport->setFocus();
		break;
	}
	case Qt::Key_Escape:
		setPlainText(chatPromptText);
		m_viewport->setFocus();
		break;
	default:
		QPlainTextEdit::keyPressEvent(event);
		break;
	}
}

void CChatPrompt::setDataModel(RBX::DataModel* dataModel, QWidget* parent)
{
	m_dataModel = RBX::shared_from(dataModel);
	RBX::DataModel::Lock lock(m_dataModel);

	if (m_chatEnter) {
		m_chatEnter->m_prompt = nullptr;
		m_chatEnter->setParent(nullptr);
		m_chatEnter.reset();
	}

	m_viewport = parent;
	m_players = dataModel ? dataModel->create<RBX::Network::Players>() : nullptr;
	if (dataModel) {
		m_chatEnter = RBX::Creatable<RBX::Instance>::create<ChatEnter>();
		m_chatEnter->m_prompt = this;
		m_chatEnter->setParent(dataModel->getGuiHooks());
	}

	setEnabled(m_players != nullptr);
}

void CChatPrompt::activate()
{
	setFocus();
}
