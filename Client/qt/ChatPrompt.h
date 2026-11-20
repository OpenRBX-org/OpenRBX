#ifndef CHATPROMPT_H
#define CHATPROMPT_H

#include "gui/GUI.h"

#include <QPlainTextEdit>
#include <memory>

class CChatPrompt;

namespace RBX {
class DataModel;

namespace Network {
class Players;
}
} // namespace RBX

class ChatEnter : public RBX::GuiItem
{
public:
	ChatEnter();

	RBX::GuiResponse process(const RBX::GuiEvent& event) override;

private:
	friend class CChatPrompt;

	CChatPrompt* m_prompt;
};

class CChatPrompt : public QPlainTextEdit
{
public:
	explicit CChatPrompt(QWidget* parent);
	~CChatPrompt() override;

	void setDataModel(RBX::DataModel* dataModel, QWidget* parent);
	void activate();

protected:
	void keyPressEvent(QKeyEvent* event) override;
	void focusInEvent(QFocusEvent* event) override;
	void focusOutEvent(QFocusEvent* event) override;

private:
	void clearPromptText();
	void CtlColor();

	QWidget* m_viewport;
	std::shared_ptr<ChatEnter> m_chatEnter;
	std::shared_ptr<RBX::DataModel> m_dataModel;
	RBX::Network::Players* m_players;
};

#endif // CHATPROMPT_H
