#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

#include "network/Replicator.h"
#include "reflection/signal.h"
#include "v8tree/Service.h"

#include <RakNetTypes.h>
#include <algorithm>
#include <memory>
#include <string>

namespace RBX {

class Instance;

namespace Network {

class Player;
class Players;

extern char sServer[];

class Server : public DescribedCreatable<Server, Peer, sServer>, public Service
{
	int outgoingPort;
	std::shared_ptr<Players> players;

	static int lastId;

	class ClientProxy : public Replicator
	{
		std::shared_ptr<Player> remotePlayer;
		Server* server;

	public:
		ClientProxy(SystemAddress systemAddress, Server* server);
		~ClientProxy() override;

		void sendTop();
		PluginReceiveResult OnReceive(RakPeerInterface* peer, Packet* packet) override;

	protected:
		Player* findTargetPlayer() override { return remotePlayer.get(); }

		void onSentMarker(long id) override;

	private:
		bool remoteDeleteOnDisconnect(const Instance* instance) const override
		{
			TopReplConts::const_iterator begin = topReplicationContainers.begin();
			TopReplConts::const_iterator end = topReplicationContainers.end();
			return std::find(begin, end, instance) == end;
		}

		bool canSendItems() override { return true; }
	};

public:
	Server();
	~Server() override;

	static Reflection::SignalDesc<Server, void(std::string, std::shared_ptr<Instance>)> event_IncommingConnection;

	static bool serverIsPresent(const Instance* context, bool testInDatamodel);

	void start(int port, int threadSleepTime);
	void stop(int blockDuration);
	int getClientCount();

	XmlElement* write() override { return nullptr; }

	PluginReceiveResult OnReceive(RakPeerInterface* peer, Packet* packet) override;

protected:
	void onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider) override;

	bool askAddChild(const Instance* instance) const override
	{
		return dynamic_cast<const ClientProxy*>(instance) != nullptr;
	}
};

} // namespace Network
} // namespace RBX

#endif // NETWORK_SERVER_H
