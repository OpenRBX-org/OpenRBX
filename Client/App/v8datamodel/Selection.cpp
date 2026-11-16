#include "v8datamodel/Selection.h"

#include "reflection/reflection.h"
#include "util/object.h"
#include "util/standardout.h"

#include <algorithm>

static RBX::Reflection::
	BoundFuncDesc<RBX::Selection, std::shared_ptr<const std::vector<std::shared_ptr<RBX::Instance>>>(), 0>
		func_getSelection(
			&RBX::Selection::getSelection2, "Get", RBX::Reflection::FunctionDescriptor::NeedTrustedCaller
		);
static RBX::Reflection::
	BoundFuncDesc<RBX::Selection, void(std::shared_ptr<const std::vector<std::shared_ptr<RBX::Instance>>>), 1>
		func_setSelection(
			&RBX::Selection::setSelection, "Set", "selection", RBX::Reflection::FunctionDescriptor::NeedTrustedCaller
		);

namespace RBX {

const char sSelection[] = "Selection";

void Selection::removeFilteredSelection(ISelectionBase* fs)
{
	filteredSelections.erase(std::find(filteredSelections.begin(), filteredSelections.end(), fs));
}

void Selection::raiseAdded(std::shared_ptr<Instance> instance)
{
	SelectionChanged event(instance, std::shared_ptr<Instance>());

	for (std::vector<ISelectionBase*>::iterator iter = filteredSelections.begin(); iter != filteredSelections.end();
		 ++iter) {
		(*iter)->onSelectionChanged(event);
	}

	Notifier<Selection, SelectionChanged>::raise(event);
}

void Selection::raiseRemoved(std::shared_ptr<Instance> source)
{
	SelectionChanged event(std::shared_ptr<Instance>(), source);

	for (std::vector<ISelectionBase*>::iterator iter = filteredSelections.begin(); iter != filteredSelections.end();
		 ++iter) {
		(*iter)->onSelectionChanged(event);
	}

	Notifier<Selection, SelectionChanged>::raise(event);
}

Selection::~Selection() = default;

void Selection::addFilteredSelection(ISelectionBase* fs)
{
	filteredSelections.push_back(fs);
}

void Selection::toggleSelection(Instance* instance)
{
	std::shared_ptr<Instance> shared = shared_from(instance);

	std::shared_ptr<std::vector<std::shared_ptr<Instance>>> sel = selection.write();
	std::vector<std::shared_ptr<Instance>>::iterator iter = std::find(sel->begin(), sel->end(), shared);
	if (iter == sel->end()) {
		sel->push_back(shared);
		instance->Notifier<Instance, AncestorChanged>::addListener(this);
		raiseAdded(shared);
	}
	else {
		sel->erase(iter);
		instance->Notifier<Instance, AncestorChanged>::removeListener(this);
		raiseRemoved(shared);
	}
}

void Selection::addToSelection(Instance* instance)
{
	std::shared_ptr<Instance> shared = shared_from(instance);

	std::shared_ptr<std::vector<std::shared_ptr<Instance>>> sel = selection.write();
	std::vector<std::shared_ptr<Instance>>::iterator iter = std::find(sel->begin(), sel->end(), shared);
	if (iter == sel->end()) {
		sel->push_back(shared);
		instance->Notifier<Instance, AncestorChanged>::addListener(this);
		raiseAdded(shared);
	}
}

void Selection::clearSelection()
{
	while (selection->size() > 0) {
		std::shared_ptr<Instance> instance = selection->back();
		selection.write()->pop_back();
		instance->Notifier<Instance, AncestorChanged>::removeListener(this);
		raiseRemoved(instance);
	}
}

void Selection::setSelection(Instance* instance)
{
	std::shared_ptr<Instance> inst = shared_from(instance);

	while (selection->size() > 0) {
		std::shared_ptr<Instance> item = selection->back();

		if (item == inst) {
			if (selection->size() == 1) {
				return;
			}

			std::shared_ptr<std::vector<std::shared_ptr<Instance>>> sel = selection.write();
			sel->back() = sel->front();
			sel->front() = item;
		}
		else {
			selection.write()->pop_back();
			item->Notifier<Instance, AncestorChanged>::removeListener(this);
			raiseRemoved(item);
		}
	}

	if (inst) {
		selection.write()->push_back(inst);
		instance->Notifier<Instance, AncestorChanged>::addListener(this);
		raiseAdded(inst);
	}
}

void Selection::removeFromSelection(const Instance* instance)
{
	std::shared_ptr<std::vector<std::shared_ptr<Instance>>> sel = selection.write();

	std::vector<std::shared_ptr<Instance>>::iterator iter = std::find(sel->begin(), sel->end(), shared_from(instance));

	if (iter != sel->end()) {
		std::shared_ptr<Instance> nonConst = *iter;

		sel->erase(iter);

		instance->Notifier<Instance, AncestorChanged>::removeListener(this);

		raiseRemoved(nonConst);
	}
}

Selection::Selection() : selection(std::vector<std::shared_ptr<Instance>>())
{
	setName("Selection");
}

void Selection::onEvent(const Instance* source, AncestorChanged event)
{
	const Instance* sourceRoot = source->getParent() != nullptr ? source->getParent()->getRootAncestor() : source;

	if (sourceRoot != getRootAncestor()) {
		removeFromSelection(source);
	}
}

} // namespace RBX

template class RBX::FactoryProduct<RBX::Selection, RBX::Instance, RBX::sSelection>;
template class RBX::Reflection::
	Described<RBX::Selection, RBX::sSelection, RBX::FactoryProduct<RBX::Selection, RBX::Instance, RBX::sSelection>>;
template class RBX::DescribedCreatable<RBX::Selection, RBX::Instance, RBX::sSelection>;
