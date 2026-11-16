#ifndef NETWORK_PLAYER_H
#define NETWORK_PLAYER_H

#include "reflection/property.h"
#include "reflection/signal.h"
#include "util/Events.h"
#include "util/Signals.h"
#include "v8datamodel/BrickColor.h"
#include "v8tree/Instance.h"

#include <memory>
#include <string>

namespace RBX {

class Backpack;
class ModelInstance;

namespace Network {

extern char sPlayer[];

struct CharacterAdded
{
	const std::shared_ptr<Instance> character;

	CharacterAdded(Instance* character) : character(shared_from(character)) {}
};

struct CharacterRemoving
{
	const std::shared_ptr<Instance> character;

	CharacterRemoving(Instance* character) : character(shared_from(character)) {}
};

class Player : public DescribedCreatable<Player, Instance, sPlayer>,
			   public Notifier<Player, CharacterAdded>,
			   public Notifier<Player, CharacterRemoving>
{
	std::shared_ptr<ModelInstance> character;
	BrickColor teamColor;
	bool neutral;
	Signals::ScopedConnection characterDiedConnection;
	std::string characterAppearance;
	bool under13;
	bool superSafeChat;
	int userId;

	bool askAddChild(const Instance* instance) const override { return true; }

	void onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider) override;

	void onCharacterChangedFrontend();
	void registerLocalPlayerNotIdle();

public:
	static Reflection::BoundProp<int, 1> prop_userId;
	static Reflection::BoundProp<bool, 1> prop_Under13;
	static Reflection::BoundProp<bool, 1> prop_SuperSafeChat;

	Player();
	~Player() override;

	static Reflection::SignalDesc<Player, void(std::string, std::shared_ptr<Instance>)> event_Chatted;

	XmlElement* write() override { return nullptr; }

	void setName(const std::string& name) override;

	ModelInstance* getCharacter() const { return character.get(); }

	void setCharacter(ModelInstance* value);

	BrickColor getTeamColor() const { return teamColor; }

	void setTeamColor(BrickColor value);
	bool getNeutral() const { return neutral; }

	void setNeutral(bool value);

	std::string getCharacterAppearance() const;
	void setCharacterAppearance(const std::string& value);

	bool getUnder13() const { return under13; }
	bool getSuperSafeChat() const { return superSafeChat; }
	void setUnder13(bool value);
	void setSuperSafeChat(bool value);

	int getUserID() const { return userId; }

	void rebuildBackpack();
	Backpack* getPlayerBackpack() const;

	void loadCharacter();
	void removeCharacter();
	void removeCharacterAppearance();
	void loadCharacterAppearance();
	static void onLocalPlayerNotIdle(ServiceProvider*);

private:
	void doPeriodicIdleCheck();

	double lastActivityTime;
	void onCharacterDied();
};

} // namespace Network
} // namespace RBX

#endif // NETWORK_PLAYER_H
