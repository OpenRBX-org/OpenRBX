#ifndef COMMAND_H
#define COMMAND_H

#include <QString>
#include <functional>
#include <map>
#include <optional>
#include <vector>

struct CommandState
{
	bool enabled = true;
	std::optional<bool> checked;
	std::optional<QString> text;
};

class CommandTarget
{
public:
	virtual ~CommandTarget() = default;

	virtual bool hasCommand(unsigned id) const;
	virtual bool executeCommand(unsigned id);
	virtual bool updateCommand(unsigned id, CommandState& state);

protected:
	void addCommand(unsigned id, std::function<void()> handler);
	void addCommandUpdate(unsigned id, std::function<void(CommandState&)> handler);

private:
	std::map<unsigned, std::function<void()>> m_commands;
	std::map<unsigned, std::function<void(CommandState&)>> m_updates;
};

using CommandRoute = std::vector<CommandTarget*>;

bool routeCommandUpdate(const CommandRoute& route, unsigned id, CommandState& state);
bool routeCommand(const CommandRoute& route, unsigned id);

#endif // COMMAND_H
