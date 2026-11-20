#include "Command.h"

bool CommandTarget::hasCommand(unsigned id) const
{
	return m_commands.contains(id);
}

bool CommandTarget::executeCommand(unsigned id)
{
	auto it = m_commands.find(id);
	if (it == m_commands.end()) {
		return false;
	}
	std::function<void()> handler = it->second;
	handler();
	return true;
}

bool CommandTarget::updateCommand(unsigned id, CommandState& state)
{
	auto it = m_updates.find(id);
	if (it == m_updates.end()) {
		return false;
	}
	it->second(state);
	return true;
}

void CommandTarget::addCommand(unsigned id, std::function<void()> handler)
{
	m_commands[id] = std::move(handler);
}

void CommandTarget::addCommandUpdate(unsigned id, std::function<void(CommandState&)> handler)
{
	m_updates[id] = std::move(handler);
}

bool routeCommandUpdate(const CommandRoute& route, unsigned id, CommandState& state)
{
	for (CommandTarget* target : route) {
		if (target && target->updateCommand(id, state)) {
			return true;
		}
	}

	for (CommandTarget* target : route) {
		if (target && target->hasCommand(id)) {
			state.enabled = true;
			return true;
		}
	}

	state.enabled = false;
	return false;
}

bool routeCommand(const CommandRoute& route, unsigned id)
{
	CommandState state;
	for (CommandTarget* target : route) {
		if (target && target->updateCommand(id, state)) {
			break;
		}
	}
	if (!state.enabled) {
		return false;
	}

	for (CommandTarget* target : route) {
		if (target && target->executeCommand(id)) {
			return true;
		}
	}
	return false;
}
