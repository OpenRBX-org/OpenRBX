#include "VerbBinder.h"

#include "Resources.h"
#include "SystemMessageBox.h"

VerbBinder::VerbBinder(
	std::function<std::shared_ptr<RBX::DataModel>()> dataModel, std::function<RBX::VerbContainer*()> container
)
	: m_dataModel(std::move(dataModel)), m_container(std::move(container))
{
}

void VerbBinder::bind(unsigned id, const RBX::Name& name)
{
	m_verbs[id] = &name;
}

void VerbBinder::bind(unsigned id, RBX::Verb* verb)
{
	m_verbs[id] = verb ? &verb->getName() : &RBX::Name::getNullName();
}

RBX::Verb* VerbBinder::find(unsigned id) const
{
	RBX::VerbContainer* container = m_container();
	return container ? container->getVerb(*m_verbs.at(id)) : nullptr;
}

bool VerbBinder::has(unsigned id) const
{
	return m_verbs.contains(id);
}

bool VerbBinder::execute(unsigned id)
{
	if (!has(id)) {
		return false;
	}

	std::shared_ptr<RBX::DataModel> dataModel = m_dataModel();
	RBX::DataModel::Lock lock(dataModel);
	if (RBX::Verb* verb = find(id)) {
		try {
			verb->doIt(dataModel.get());
		}
		catch (std::exception& e) {
			SystemMessageBox::warning(nullptr, Resources::string(appTitleStringId), e.what());
		}
	}
	return true;
}

bool VerbBinder::update(unsigned id, CommandState& state)
{
	if (!has(id)) {
		return false;
	}

	std::shared_ptr<RBX::DataModel> dataModel = m_dataModel();
	RBX::DataModel::Lock lock(dataModel);
	if (RBX::Verb* verb = find(id)) {
		state.enabled = verb->isEnabled();
		state.checked = verb->isChecked();
	}
	return true;
}
