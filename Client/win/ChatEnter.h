#ifndef CHATENTER_H
#define CHATENTER_H

#include "gui/GUI.h"

class CChatPrompt;

class ChatEnter : public RBX::GuiItem
{
public:
	ChatEnter();

	RBX::GuiResponse process(const RBX::GuiEvent& event) override;

private:
	friend class CChatPrompt;

	CChatPrompt* m_unk0x10c;
};

#endif // CHATENTER_H
