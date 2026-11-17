#ifndef NETWORK_REPLICATOR_H
#define NETWORK_REPLICATOR_H

#include "network/Streaming.h"
#include "reflection/reflection.h"
#include "util/Events.h"
#include "util/RunStateOwner.h"
#include "util/Signals.h"
#include "v8tree/Instance.h"
#include "v8world/Mechanism.h"

#include <PluginInterface.h>
#include <RakNetTypes.h>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <vector>

class PacketLogger;
class RakPeer;
class RakPeerInterface;
struct Packet;

namespace RakNet {
class BitStream;
}

namespace RBX {

class Assembly;
class Mechanism;
class RunService;
class ServiceProvider;

namespace Profiling {
class CodeProfiler;
class ThreadProfiler;
} // namespace Profiling

namespace Reflection {
class ConstProperty;
class Property;
class PropertyDescriptor;
} // namespace Reflection

namespace Stats {
class StatsService;
}

namespace Network {

class Player;
class Players;

extern const char sPeer[];
extern const char sReplicator[];
extern const char sMarker[];

class Marker : public DescribedNonCreatable<Marker, Reflection::DescribedBase, sMarker>
{
	bool returned;

	Marker() : returned(false) {}

public:
	Marker(const Marker&) = delete;
	Marker& operator=(const Marker&) = delete;

	static std::shared_ptr<Marker> newMarker() { return std::shared_ptr<Marker>(new Marker()); }

	int id() const { return static_cast<int>(reinterpret_cast<intptr_t>(this)); }
	void fireReturned() { event_Returned.fire(this); }

	static Reflection::SignalDesc<Marker, void()> event_Returned;
	static Reflection::SignalDesc<Marker, void(float)> event_Progress;
};

class Replicator : public DescribedNonCreatable<Replicator, IdSerializer, sReplicator>, public PluginInterface
{
	enum ValueType
	{
		ValueType_nil = 0,
		ValueType_string = 1,
		ValueType_bool = 2,
		ValueType_int = 3,
		ValueType_float = 4,
		ValueType_double = 5,
		ValueType_BrickColor = 6,
		ValueType_Color3 = 7,
		ValueType_Vector3 = 8,
		ValueType_BrickVector = 9,
		ValueType_CoordinateFrame = 10,
		ValueType_Enum = 11,
		ValueType_Ref = 12,
		ValueType_ContentId = 13
	};

	class Item;
	class NewInstanceItem;
	class DeleteInstanceItem;
	class ChangePropertyItem;
	class MarkerItem;
	class JobSender;
	class ItemSender;
	class ReplicatorStatsItem;

	double lastSendTime;
	MechanismTracker jobStagePos;
	double lastCharacterSendTime;
	std::shared_ptr<ReplicatorStatsItem> statsItem;

	using Strings = std::map<const Reflection::PropertyDescriptor*, std::shared_ptr<SharedStringDictionary>>;

	Strings strings;
	SharedStringDictionary classNames;
	SharedStringDictionary propNames;

protected:
	struct ReplicationData
	{
		std::shared_ptr<Instance> instance;
		Signals::Connection childAdded;
		Signals::Connection changed;
		bool deleteOnDisconnect;
	};

	using RepConts = std::map<const Instance*, ReplicationData>;

	RepConts replicationContainers;

	using TopReplConts = std::vector<Instance*>;

	TopReplConts topReplicationContainers;
	Players* players;

	using ItemQueue = std::list<std::shared_ptr<Item>>;

	ItemQueue pendingItems;

	using PendingInstances = std::set<const Instance*>;

	PendingInstances pendingNewInstances;
	PendingInstances pendingDeleteInstances;
	bool sendPhysicsEnabled;
	const Reflection::Property* deserializingProperty;
	Instance* removingInstance;
	std::queue<std::shared_ptr<Marker>> incomingMarkers;

	using DefaultObjects = std::map<const Name*, std::shared_ptr<Instance>>;

	DefaultObjects defaultObjects;
	std::unique_ptr<Profiling::CodeProfiler> profileReplication;
	std::unique_ptr<Profiling::CodeProfiler> profileDataListening;
	std::unique_ptr<Profiling::CodeProfiler> profileDataIn;
	std::unique_ptr<Profiling::CodeProfiler> profileDataOut;
	std::unique_ptr<Profiling::CodeProfiler> profilePhysicsIn;
	std::unique_ptr<Profiling::CodeProfiler> profilePhysicsOut;

	virtual Player* findTargetPlayer();

	Mechanism* findTargetPlayerCharacterMechanism();

private:
	bool receivedGlobals;

public:
	bool disconnected;
	RakPeerInterface* peer;
	const SystemAddress remotePlayerId;

	Replicator(SystemAddress remotePlayerId, RakPeerInterface* peer);
	~Replicator() override;

	std::shared_ptr<Instance> getPlayer();

	static Reflection::BoundProp<int, 1> prop_maxDataModelSendBuffer;
	static Reflection::SignalDesc<Replicator, void(std::string, bool)> event_Disconnection;

	void Update(RakPeerInterface* peer) override;
	PluginReceiveResult OnReceive(RakPeerInterface* peer, Packet* packet) override;

	const Name& getClassName() const override { return Name::getNullName(); }

	XmlElement* write() override { return nullptr; }

	std::shared_ptr<Reflection::DescribedBase> sendMarker();
	bool isSerializePending(const Instance* instance) const;
	bool isPropertyChangedPending(const Reflection::Property& property) const;
	void requestCharacter();
	void closeConnection();

protected:
	static void closeReplicationItem(const std::pair<const Instance* const, ReplicationData>& pair);
	bool disconnectReplicationData(std::shared_ptr<Instance> instance);
	void onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider) override;
	void onChildAdded(std::shared_ptr<Instance> child);
	void onParentChanged(std::shared_ptr<Instance> instance);
	void onPropertyChanged(std::shared_ptr<Instance> instance, const Reflection::PropertyDescriptor* descriptor);
	const Instance* getDefault(const Name& className);

	virtual bool wantReplicate(const Instance* source) const;
	virtual void onSentMarker(long id);

	void readProperties(RakNet::BitStream& inBitstream, Instance* instance);
	void writeProperties(Instance* instance, RakNet::BitStream& outBitStream);

private:
	virtual bool remoteDeleteOnDisconnect(const Instance* instance) const;

	ReplicationData& addReplicationData(std::shared_ptr<Instance> instance, bool listenToChanges);
	void addTopReplicationContainer(Instance* instance, bool deleteOnDisconnect);
	void connectToReplicationContainers();
	void removeFromPendingProperties(const Instance* instance);
	bool removeFromPendingNewInstances(const Instance* instance);
	bool removeFromPendingDeleteInstances(const Instance* instance);
	bool isReplicationContainer(const Instance* instance) const;
	bool sendItems();
	bool sendPhysicsPacket();
	void sendPhysicsData(RakNet::BitStream& bitStream, const Assembly& assembly);
	void sendPhysicsData(RakNet::BitStream& bitStream, const Mechanism& mechanism);
	void createStatsItems(Stats::StatsService* stats);
	void serializeValue(const Reflection::ConstProperty& property, bool includeLength, RakNet::BitStream& outBitStream);
	void deserializeValue(RakNet::BitStream& inBitstream, Reflection::Property& property);
	void skipValue(RakNet::BitStream& inBitStream, ValueType type);
	void receiveData(Packet* packet);
	void readInstanceNew(RakNet::BitStream& inBitstream);
	void readInstanceDelete(RakNet::BitStream& inBitstream);
	void readChangedProperty(RakNet::BitStream& inBitstream);
	void readMarker(RakNet::BitStream& inBitstream);
	SharedStringDictionary& getSharedDictionary(const Reflection::PropertyDescriptor& descriptor);
	PluginReceiveResult OnReceive_ID_TIMESTAMP(Packet* packet);

	virtual bool canSendItems();

	static void writeValueType(bool includeLength, ValueType vt, RakNet::BitStream& outBitStream);

public:
};

class Peer : public Reflection::Described<Peer, sPeer, Instance>,
			 public PluginInterface,
			 public Listener<RunService, Heartbeat>
{
	std::unique_ptr<PacketLogger> logger;
	std::unique_ptr<Profiling::ThreadProfiler> profilePacketsThread;

protected:
	const std::unique_ptr<RakPeer> rakPeer;

	Peer();
	~Peer() override;

	bool askAddChild(const Instance* instance) const override
	{
		return dynamic_cast<const Replicator*>(instance) != nullptr;
	}

	RakPeerInterface* peerInterface();

	void updateLogger();
	void updateNetworkSimulator();

	void onEvent(const RunService* source, Heartbeat event) override;
	void Update(RakPeerInterface* peer) override;
	void onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider) override;

public:
};

} // namespace Network
} // namespace RBX

#endif // NETWORK_REPLICATOR_H
