#ifndef VERBBINDER_H
#define VERBBINDER_H

#include "Command.h"
#include "v8datamodel/DataModel.h"
#include "v8tree/Verb.h"

#include <functional>
#include <map>
#include <memory>

class VerbBinder
{
public:
	VerbBinder(
		std::function<std::shared_ptr<RBX::DataModel>()> dataModel, std::function<RBX::VerbContainer*()> container
	);

	void bind(unsigned id, const RBX::Name& name);
	void bind(unsigned id, RBX::Verb* verb);

	bool has(unsigned id) const;
	bool execute(unsigned id);
	bool update(unsigned id, CommandState& state);

private:
	RBX::Verb* find(unsigned id) const;

	std::function<std::shared_ptr<RBX::DataModel>()> m_dataModel;
	std::function<RBX::VerbContainer*()> m_container;
	std::map<unsigned, const RBX::Name*> m_verbs;
};

#endif // VERBBINDER_H
