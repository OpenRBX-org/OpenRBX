#ifndef GUI_CHATOUTPUT_H
#define GUI_CHATOUTPUT_H

#include "gui/GUI.h"
#include "gui/ProfanityFilter.h"
#include "util/Events.h"
#include "util/RunStateOwner.h"

#include <G3D/Color3.h>
#include <deque>
#include <string>

namespace RBX {

namespace Network {
class Player;
class Players;
struct ChatMessage;
} // namespace Network

class ChatLine
{
public:
	G3D::Color3 userColor;
	std::string user;
	std::string message;
	float dieTime;
	bool profane;

	ChatLine(Network::Player* player, const std::string& message, float dieTime);
	ChatLine(const std::string& userName, const std::string& message, float dieTime);
	~ChatLine();
};

class ChatOutput : public GuiItem,
				   public Listener<Network::Players, Network::ChatMessage>,
				   public Listener<RunService, Heartbeat>
{
private:
	RunService* runService;
	Network::Players* players;
	ProfanityFilter filter;
	std::deque<ChatLine*> fifo;
	float time;

	void removeOldest();
	bool frontIsOld();
	bool ParseCommand(Network::ChatMessage chatMessage);

	void onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider) override;
	void onEvent(const Network::Players* source, Network::ChatMessage event) override;
	void onEvent(const RunService* source, Heartbeat event) override;
	void render2d(Adorn* adorn) override;

public:
	ChatOutput();
	~ChatOutput() override;
};

} // namespace RBX

#endif // GUI_CHATOUTPUT_H
