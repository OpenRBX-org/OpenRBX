#ifndef V8DATAMODEL_SELECTION_H
#define V8DATAMODEL_SELECTION_H

#include "util/Events.h"
#include "util/Handle.h"
#include "util/Utilities.h"
#include "v8tree/Instance.h"
#include "v8tree/Service.h"
#include "v8world/Controller.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <vector>

namespace RBX {

class SelectionChanged
{
public:
	const std::shared_ptr<Instance> addedItem;
	const std::shared_ptr<Instance> removedItem;

private:
	friend class Selection;

	SelectionChanged(std::shared_ptr<Instance> addedItem, std::shared_ptr<Instance> removedItem)
		: addedItem(addedItem), removedItem(removedItem)
	{
	}
};

class __declspec(novtable) ISelectionBase
{
private:
	friend class Selection;

	virtual void onSelectionChanged(const SelectionChanged& event) = 0;
};

extern const char sSelection[];
class Selection : public DescribedCreatable<Selection, Instance, sSelection>,
				  public Notifier<Selection, SelectionChanged>,
				  public Listener<Instance, AncestorChanged>,
				  public Service
{
public:
	Selection();
	~Selection() override;

	unsigned int size() const { return selection->size(); }

	std::shared_ptr<Instance> front() const { return size() > 0 ? selection->front() : std::shared_ptr<Instance>(); }

	template <class _InIt>
	void setSelection(_InIt _First, _InIt _Last)
	{
		clearSelection();
		for (; _First != _Last; ++_First) {
			addToSelection(*_First);
		}
	}

	std::vector<std::shared_ptr<Instance>>::const_iterator begin() const { return selection->begin(); }

	std::vector<std::shared_ptr<Instance>>::const_iterator end() const { return selection->end(); }

	std::shared_ptr<Instance> back() const { return size() > 0 ? selection->back() : std::shared_ptr<Instance>(); }

	const CopyOnWrite<std::vector<std::shared_ptr<Instance>>>& getSelection() const;

	void clearSelection();

	std::shared_ptr<const std::vector<std::shared_ptr<Instance>>> getSelection2() { return selection.read(); }
	void setSelection(std::shared_ptr<const std::vector<std::shared_ptr<Instance>>> selection)
	{
		setSelection(selection->begin(), selection->end());
	}
	void setSelection(Instance* instance);

	void addToSelection(const std::shared_ptr<Instance>& instance) { addToSelection(instance.get()); }
	void addToSelection(Instance* instance);
	void toggleSelection(Instance* instance);
	void removeFromSelection(const Instance* instance);

	bool isSelected(const Instance* instance) const
	{
		return std::find(selection->begin(), selection->end(), shared_from(instance)) != selection->end();
	}

	class AddIterator
	{
	public:
		using iterator_category = std::output_iterator_tag;
		using value_type = void;
		using difference_type = void;
		using pointer = void;
		using reference = void;

	private:
		Selection* selection;

	public:
		AddIterator(Selection* selection) : selection(selection) {}

		AddIterator& operator*() { return *this; }
		AddIterator& operator++() { return *this; }
		AddIterator operator++(int) { return *this; }

		template <class T>
		AddIterator& operator=(T* instance)
		{
			selection->addToSelection(instance);
			return *this;
		}
	};

	class RemoveIterator
	{
	public:
		using iterator_category = std::output_iterator_tag;
		using value_type = void;
		using difference_type = void;
		using pointer = void;
		using reference = void;

	private:
		Selection* selection;

	public:
		RemoveIterator(Selection* selection) : selection(selection) {}

		RemoveIterator& operator*() { return *this; }
		RemoveIterator& operator++() { return *this; }
		RemoveIterator operator++(int) { return *this; }

		template <class T>
		RemoveIterator& operator=(T* instance)
		{
			selection->removeFromSelection(instance);
			return *this;
		}
	};

	class ToggleIterator
	{
	public:
		using iterator_category = std::output_iterator_tag;
		using value_type = void;
		using difference_type = void;
		using pointer = void;
		using reference = void;

	private:
		Selection* selection;

	public:
		ToggleIterator(Selection* selection) : selection(selection) {}

		ToggleIterator& operator*() { return *this; }
		ToggleIterator& operator++() { return *this; }
		ToggleIterator operator++(int) { return *this; }

		template <class T>
		ToggleIterator& operator=(T* instance)
		{
			selection->toggleSelection(instance);
			return *this;
		}
	};

	void onEvent(const Instance* source, AncestorChanged event) override;

	void addFilteredSelection(ISelectionBase* fs);
	void removeFilteredSelection(ISelectionBase* fs);

private:
	void raiseAdded(std::shared_ptr<Instance> instance);
	void raiseRemoved(std::shared_ptr<Instance> source);

	CopyOnWrite<std::vector<std::shared_ptr<Instance>>> selection;
	std::vector<ISelectionBase*> filteredSelections;
};

template <class T>
class FilteredSelection : public NonFactoryProduct<Instance, nullptr>, public Service, public ISelectionBase
{
public:
	FilteredSelection() { setName("FilteredSelection"); }
	~FilteredSelection() override
	{
		if (rootSelection) {
			rootSelection->removeFilteredSelection(this);
		}
	}

	const std::vector<T*>& items() const { return filteredSelection; }
	unsigned int size() const;

	void clearSelection() { rootSelection->clearSelection(); }

	template <class _InIt>
	void setSelection(_InIt _First, _InIt _Last)
	{
		rootSelection->setSelection(_First, _Last);
	}

	typename std::vector<T*>::const_iterator begin() const { return filteredSelection.begin(); }
	typename std::vector<T*>::const_iterator end() const { return filteredSelection.end(); }

	template <class _Pr>
	T* FUN_00598690(_Pr predicate) const
	{
		typename std::vector<T*>::const_iterator it =
			std::find_if(filteredSelection.begin(), filteredSelection.end(), predicate);

		return it != filteredSelection.end() ? *it : nullptr;
	}

protected:
	void onAncestorChanged(const AncestorChanged& event) override
	{
		Instance::onAncestorChanged(event);

		if (!rootSelection) {
			rootSelection = shared_from(ServiceProvider::create<Selection>(event.newParent));

			if (rootSelection) {
				for (std::vector<std::shared_ptr<Instance>>::const_iterator iter = rootSelection->begin();
					 iter != rootSelection->end();
					 ++iter) {
					if (T* c = dynamic_cast<T*>(iter->get())) {
						filteredSelection.push_back(c);
					}
				}

				rootSelection->addFilteredSelection(this);
			}
		}
	}

	void onSelectionChanged(const SelectionChanged& event) override
	{
		if (T* c = dynamic_cast<T*>(event.addedItem.get())) {
			filteredSelection.push_back(c);
		}

		if (event.removedItem) {
			typename std::vector<T*>::iterator iter =
				std::find(filteredSelection.begin(), filteredSelection.end(), event.removedItem.get());
			if (iter != filteredSelection.end()) {
				filteredSelection.erase(iter);
			}
		}
	}

private:
	std::shared_ptr<Selection> rootSelection;
	std::vector<T*> filteredSelection;
};

} // namespace RBX

#endif // V8DATAMODEL_SELECTION_H
