#ifndef V8DATAMODEL_LOCALBACKPACK_H
#define V8DATAMODEL_LOCALBACKPACK_H

#include "gui/GUI.h"
#include "gui/Widget.h"
#include "network/Player.h"
#include "v8datamodel/Hopper.h"
#include "v8tree/Service.h"

#include <memory>

class XmlElement;

namespace RBX {

class Backpack;
class LocalBackpack;

namespace Network {
class Players;
} // namespace Network

extern const char sLocalBackpack[];

extern const char sLocalBackpackItem[];

class LocalBackpackItem : public DescribedCreatable<LocalBackpackItem, Widget, sLocalBackpackItem>
{
private:
	std::shared_ptr<BackpackItem> myItem;
	GuiDrawImage guiImageDraw;

	bool askSetParent(const Instance* instance) const override { return true; }
	G3D::Vector2 getSize() const override;
	void render2d(Adorn* adorn) override;
	void onClick(const GuiEvent& event) override;
	int getBinId() const { return getParent()->findChildIndex(this) + 1; }

public:
	LocalBackpackItem();

	void setItem(BackpackItem* item) { myItem = shared_from(item); }

	BackpackItem* getItem() { return myItem.get(); }
};

class LocalBackpack : public DescribedCreatable<LocalBackpack, RelativePanel, sLocalBackpack>,
					  public Service,
					  public Listener<Instance, ChildAdded>,
					  public Listener<Instance, ChildRemoved>,
					  public Listener<Instance, DescendentAdded>,
					  public Listener<Instance, DescendentRemoving>,
					  public Listener<Network::Player, Network::CharacterAdded>,
					  public Listener<Network::Player, Network::CharacterRemoving>
{
private:
	std::shared_ptr<Network::Players> players;
	std::shared_ptr<Network::Player> localPlayer;
	std::shared_ptr<Backpack> localBackpack;
	std::shared_ptr<Instance> localCharacter;
	std::shared_ptr<BackpackItem> pendingClick;
	int lastRemovedIndex;

	bool askSetParent(const Instance* instance) const override { return true; }
	void onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider) override;
	void onEvent(const Network::Player* source, Network::CharacterRemoving event) override;
	void onEvent(const Network::Player* source, Network::CharacterAdded event) override;
	void onEvent(const Instance* source, DescendentRemoving event) override;
	void onEvent(const Instance* source, DescendentAdded event) override;
	void onEvent(const Instance* source, ChildRemoved event) override;
	void onEvent(const Instance* source, ChildAdded event) override;

	void onLocalCharacterAdded(Instance* added);
	void onLocalPlayerAdded(Network::Player* added);
	void onLocalBackpackAdded(Backpack* added);
	void clearAll();
	void clearLocalCharacter();
	void clearLocalPlayer();
	void clearLocalBackpack();
	void insertBackpackItem(BackpackItem* item);
	void removeBackpackItem(BackpackItem* item);

public:
	LocalBackpack();
	~LocalBackpack() override;

	void onHeartbeat();
	void onClick(LocalBackpackItem* clickedItem);
	bool onKey(int key);
	void cycleUp();
	void cycleDown();

	XmlElement* write() override { return nullptr; }
};

} // namespace RBX

#endif // V8DATAMODEL_LOCALBACKPACK_H
