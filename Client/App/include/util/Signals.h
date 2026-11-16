#ifndef UTIL_SIGNALS_H
#define UTIL_SIGNALS_H

#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <utility>

namespace RBX {
namespace Signals {

enum ConnectPosition
{
	at_back,
	at_front
};

namespace Detail {

class ConnectionBody
{
public:
	ConnectionBody(const ConnectionBody&) = delete;
	ConnectionBody& operator=(const ConnectionBody&) = delete;

	bool connected() const { return live; }

	void disconnect()
	{
		if (live) {
			live = false;
			onDisconnect();
		}
	}

protected:
	ConnectionBody() = default;
	virtual ~ConnectionBody() = default;

	virtual void onDisconnect() = 0;

private:
	bool live = true;
};

} // namespace Detail

class Connection
{
public:
	Connection() = default;
	explicit Connection(std::shared_ptr<Detail::ConnectionBody> body) : body(std::move(body)) {}

	bool connected() const { return body && body->connected(); }

	void disconnect() const
	{
		if (const std::shared_ptr<Detail::ConnectionBody> local = body) {
			local->disconnect();
		}
	}

	bool operator==(const Connection& other) const { return body == other.body; }

private:
	std::shared_ptr<Detail::ConnectionBody> body;
};

class ScopedConnection : public Connection
{
public:
	ScopedConnection() = default;
	ScopedConnection(const Connection& other) : Connection(other) {}
	ScopedConnection(const ScopedConnection&) = delete;
	ScopedConnection& operator=(const ScopedConnection&) = delete;

	ScopedConnection& operator=(const Connection& other)
	{
		const Connection old = *this;
		Connection::operator=(other);
		old.disconnect();
		return *this;
	}

	~ScopedConnection() { disconnect(); }
};

template <class Signature>
class Signal;

template <class... Args>
class Signal<void(Args...)>
{
public:
	typedef std::function<void(Args...)> Function;

	Signal() : impl(std::make_shared<Impl>()) {}
	Signal(const Signal&) = delete;
	Signal& operator=(const Signal&) = delete;

	Connection connect(Function function, ConnectPosition position = at_back)
	{
		const GroupIterator group = position == at_front ? impl->groups.begin() : std::prev(impl->groups.end());
		return impl->insert(group, std::move(function), position);
	}

	Connection connect(int group, Function function, ConnectPosition position = at_back)
	{
		const GroupIterator found = impl->groups.try_emplace(Key(Key::group, group)).first;
		return impl->insert(found, std::move(function), position);
	}

	bool empty() const
	{
		for (const auto& group : impl->groups) {
			for (const ConnectedSlot& slot : group.second) {
				if (slot.connection.body->connected()) {
					return false;
				}
			}
		}

		return true;
	}

	void operator()(Args... args)
	{
		const std::shared_ptr<Impl> local = impl;
		const CallDepth depth(*local);

		for (auto group = local->groups.begin(); group != local->groups.end(); ++group) {
			for (auto slot = group->second.begin(); slot != group->second.end(); ++slot) {
				if (slot->connection.body->connected()) {
					slot->function(args...);
				}
			}
		}
	}

private:
	struct Key
	{
		enum Kind
		{
			front,
			group,
			back
		};

		Kind kind;
		int value;

		Key(Kind kind, int value) : kind(kind), value(value) {}

		bool operator<(const Key& other) const { return kind != other.kind ? kind < other.kind : value < other.value; }
	};

	struct Impl;
	class Body;

	struct ControllingConnection
	{
		std::shared_ptr<Body> body;

		explicit ControllingConnection(std::shared_ptr<Body> body) : body(std::move(body)) {}
		ControllingConnection(const ControllingConnection&) = delete;
		ControllingConnection& operator=(const ControllingConnection&) = delete;
		~ControllingConnection() { body->disconnect(); }
	};

	struct ConnectedSlot
	{
		ControllingConnection connection;
		Function function;

		ConnectedSlot(std::shared_ptr<Body> body, Function function)
			: connection(std::move(body)), function(std::move(function))
		{
		}
	};

	typedef std::list<ConnectedSlot> Slots;
	typedef std::map<Key, Slots> Groups;
	typedef typename Groups::iterator GroupIterator;
	typedef typename Slots::iterator SlotIterator;

	class Body : public Detail::ConnectionBody
	{
	public:
		explicit Body(Impl& impl) : impl(impl) {}

		GroupIterator group;
		SlotIterator slot;

	private:
		void onDisconnect() override { impl.slotDisconnected(*this); }

		Impl& impl;
	};

	struct Impl
	{
		Groups groups;
		int callDepth = 0;
		bool clearing = false;
		bool delayedDisconnect = false;

		Impl()
		{
			groups.try_emplace(Key(Key::front, 0));
			groups.try_emplace(Key(Key::back, 0));
		}

		Impl(const Impl&) = delete;
		Impl& operator=(const Impl&) = delete;

		~Impl()
		{
			clearing = true;
			groups.clear();
		}

		Connection insert(GroupIterator group, Function function, ConnectPosition position)
		{
			const std::shared_ptr<Body> body = std::make_shared<Body>(*this);
			Slots& slots = group->second;

			body->group = group;
			body->slot = position == at_front ? slots.emplace(slots.begin(), body, std::move(function))
											  : slots.emplace(slots.end(), body, std::move(function));

			return Connection(body);
		}

		void slotDisconnected(Body& body)
		{
			if (clearing) {
				return;
			}

			if (callDepth > 0) {
				delayedDisconnect = true;
			}
			else {
				body.group->second.erase(body.slot);
			}
		}

		void removeDisconnectedSlots()
		{
			for (GroupIterator group = groups.begin(); group != groups.end();) {
				Slots& slots = group->second;

				for (SlotIterator slot = slots.begin(); slot != slots.end();) {
					slot = slot->connection.body->connected() ? std::next(slot) : slots.erase(slot);
				}

				if (slots.empty() && group->first.kind == Key::group) {
					group = groups.erase(group);
				}
				else {
					++group;
				}
			}
		}
	};

	class CallDepth
	{
	public:
		explicit CallDepth(Impl& impl) : impl(impl) { ++impl.callDepth; }
		CallDepth(const CallDepth&) = delete;
		CallDepth& operator=(const CallDepth&) = delete;

		~CallDepth()
		{
			if (--impl.callDepth == 0 && impl.delayedDisconnect) {
				impl.removeDisconnectedSlots();
				impl.delayedDisconnect = false;
			}
		}

	private:
		Impl& impl;
	};

	std::shared_ptr<Impl> impl;
};

} // namespace Signals
} // namespace RBX

#endif // UTIL_SIGNALS_H
