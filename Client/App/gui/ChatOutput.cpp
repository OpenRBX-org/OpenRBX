#include "gui/ChatOutput.h"

#include "network/Player.h"
#include "network/Players.h"
#include "util/Color.h"
#include "util/Hash.h"
#include "util/Utilities.h"
#include "v8datamodel/GameSettings.h"
#include "v8datamodel/SafeChat.h"
#include "v8datamodel/Team.h"
#include "v8datamodel/Teams.h"
#include "v8tree/Service.h"

#include <algorithm>
#include <vector>

namespace RBX {

ChatLine::~ChatLine() = default;

ChatOutput::ChatOutput() : runService(nullptr), players(nullptr), time(0)
{
}

void ChatOutput::removeOldest()
{
	ChatLine* line = fifo.front();
	fifo.pop_front();
	delete line;
}

bool ChatOutput::frontIsOld()
{
	return !fifo.empty() && fifo.front()->dieTime < time;
}

void ChatOutput::onEvent(const RunService* source, Heartbeat event)
{
	time += event.step;

	while (frontIsOld()) {
		removeOldest();
	}
}

ChatOutput::~ChatOutput()
{
	while (!fifo.empty()) {
		removeOldest();
	}
}

void ChatOutput::render2d(Adorn* adorn)
{
	Rect rect = getMyRect();
	G3D::Vector2 pos(rect.low.x, rect.low.y - fifo.size() * 12);

	for (size_t i = 0; i < fifo.size(); i++) {
		std::string user = fifo[i]->user + "; ";
		G3D::Vector2 userSize = adorn->get2DStringBounds(user, 12, Adorn::PROPORTIONAL_SPACING);
		G3D::Vector2 messageSize = adorn->get2DStringBounds(fifo[i]->message, 12, Adorn::PROPORTIONAL_SPACING);
		float userWidth = std::max(userSize.x + 4, 20.0f);

		adorn->rect2d(
			G3D::Rect2D::xyxy(
				G3D::Vector2(pos.x - 2, pos.y + 1),
				G3D::Vector2(pos.x + userWidth + messageSize.x + 2, pos.y + userSize.y - 1)
			),
			translucentBackdrop()
		);

		G3D::Color3 messageColor = G3D::Color3::white();
		if (fifo[i]->profane) {
			messageColor = G3D::Color3::red();
		}

		adorn->drawFont2D(
			user,
			pos,
			12,
			G3D::Color4(fifo[i]->userColor),
			G3D::Color4::clear(),
			Adorn::XALIGN_LEFT,
			Adorn::YALIGN_TOP,
			Adorn::PROPORTIONAL_SPACING
		);
		adorn->drawFont2D(
			fifo[i]->message,
			G3D::Vector2(pos.x + userWidth, pos.y),
			12,
			G3D::Color4(messageColor),
			G3D::Color4::clear(),
			Adorn::XALIGN_LEFT,
			Adorn::YALIGN_TOP,
			Adorn::PROPORTIONAL_SPACING
		);

		pos.y += 14;
	}
}

ChatLine::ChatLine(Network::Player* player, const std::string& message, float dieTime)
	: message(message), dieTime(dieTime), profane(false)
{
	user = player->getName();

	Teams* teams = ServiceProvider::find<Teams>(player);

	if (teams && teams->isTeamGame()) {
		if (!player->getNeutral()) {
			Team* team = teams->getTeamFromPlayer(player);

			if (team) {
				userColor = team->getTeamColor().color3();
			}
			else {
				userColor = G3D::Color3::black();
			}
		}
		else {
			userColor = G3D::Color3::white();
		}
	}
	else {
		userColor = Color::colorFromIndex8(Hash::hash(user) & 7);
	}
}

bool ChatOutput::ParseCommand(Network::ChatMessage chatMessage)
{
	std::vector<std::string> tokens = tokenize(chatMessage.message, " -_/,");

	if (tokens.size() == 0) {
		return false;
	}

	toLower(tokens[0]);

	if (tokens[0] == "sc") {
		std::string message = SafeChat::singleton().getMessage(tokens);
		fifo.push_back(new ChatLine(chatMessage.source.get(), message, time + 60));
		return true;
	}

	return false;
}

void ChatOutput::onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider)
{
	stopListening<RunService, Heartbeat>(runService, this);
	stopListening<Network::Players, Network::ChatMessage>(players, this);

	GuiItem::onServiceProvider(oldProvider, newProvider);

	runService = newProvider ? newProvider->create<RunService>() : nullptr;
	players = newProvider ? newProvider->create<Network::Players>() : nullptr;

	startListening<RunService, Heartbeat>(runService, this);
	startListening<Network::Players, Network::ChatMessage>(players, this);
}

void ChatOutput::onEvent(const Network::Players* source, Network::ChatMessage event)
{
	while (fifo.size() > GameSettings::singleton().chatScrollLength) {
		removeOldest();
	}

	if (event.message.size() > 0 && event.message[0] == '/') {
		if (ParseCommand(event) == true) {
			return;
		}
	}

	Network::Player* localPlayer = players->getLocalPlayer();

	if (localPlayer && localPlayer->getSuperSafeChat()) {
		return;
	}

	bool profane = filter.ContainsProfanity(event.message);

	if (localPlayer && event.source.get() != localPlayer && profane) {
		return;
	}

	ChatLine* line = new ChatLine(event.source.get(), event.message, time + 60);
	line->profane = false;
	fifo.push_back(line);
}

} // namespace RBX
