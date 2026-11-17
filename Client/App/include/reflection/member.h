#ifndef REFLECTION_MEMBER_H
#define REFLECTION_MEMBER_H

#include "reflection/descriptor.h"

#include <algorithm>
#include <vector>

namespace RBX::Reflection {

class ClassDescriptor;
class DescribedBase;

template <class T>
class MemberDescriptorContainer
{
public:
	static bool compare(const T* a, const T* b)
	{
		const Name* bn = &b->name;
		const Name* an = &a->name;

		return an != bn && an->name < bn->name;
	}

	using Collection = std::vector<T*>;
	using ConstMemberType = typename T::ConstMember;
	using MemberType = typename T::Member;

	class ConstIterator
	{
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = ConstMemberType;
		using difference_type = void;
		using pointer = ConstMemberType;
		using reference = ConstMemberType&;

		ConstIterator(const typename Collection::const_iterator& iter, const DescribedBase* instance)
			: instance(instance), iter(iter)
		{
		}

		ConstMemberType operator*() const { return ConstMemberType(**iter, instance); }

		bool operator==(const ConstIterator& other) const { return iter == other.iter; }

		bool operator!=(const ConstIterator& other) const { return iter != other.iter; }

		ConstIterator operator++(int)
		{
			ConstIterator result = *this;
			++iter;
			return result;
		}

		ConstIterator& operator++()
		{
			++iter;
			return *this;
		}

		const T& getDescriptor() const { return **iter; }

	private:
		const DescribedBase* instance;
		typename Collection::const_iterator iter;
	};

	class Iterator
	{
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = MemberType;
		using difference_type = void;
		using pointer = MemberType;
		using reference = MemberType&;

		Iterator(const typename Collection::const_iterator& iter, DescribedBase* instance)
			: instance(instance), iter(iter)
		{
		}
		MemberType operator*() const { return MemberType(**iter, instance); }

		bool operator==(const Iterator& other) const { return iter == other.iter; }

		bool operator!=(const Iterator& other) const { return iter != other.iter; }

		Iterator operator++(int)
		{
			Iterator result = *this;
			++iter;
			return result;
		}

		Iterator& operator++()
		{
			++iter;
			return *this;
		}

	private:
		DescribedBase* instance;
		typename Collection::const_iterator iter;
	};

	void declare(T* descriptor);

	typename Collection::const_iterator descriptors_begin() const { return descriptors.begin(); }
	typename Collection::const_iterator descriptors_end() const { return descriptors.end(); }

	typename Collection::const_iterator findDescriptor(const Name& name) const;

	Iterator findMember(const Name& name, DescribedBase* instance) const
	{
		return Iterator(findDescriptor(name), instance);
	}

	ConstIterator findConstMember(const Name& name, const DescribedBase* instance) const
	{
		return ConstIterator(findDescriptor(name), instance);
	}

	Iterator members_begin(DescribedBase* instance) const { return Iterator(descriptors.begin(), instance); }
	ConstIterator members_begin(const DescribedBase* instance) const
	{
		return ConstIterator(descriptors.begin(), instance);
	}

	Iterator members_end(DescribedBase* instance) const { return Iterator(descriptors.end(), instance); }
	ConstIterator members_end(const DescribedBase* instance) const
	{
		return ConstIterator(descriptors.end(), instance);
	}

protected:
	MemberDescriptorContainer(MemberDescriptorContainer* base);

	void mergeMembers(const MemberDescriptorContainer* source);

	Collection descriptors;
	std::vector<MemberDescriptorContainer*> derivedContainers;
	const MemberDescriptorContainer* base;
};
template <class T>
typename MemberDescriptorContainer<T>::Collection::const_iterator MemberDescriptorContainer<T>::findDescriptor(
	const Name& name
) const
{
	size_t min = 0, max = descriptors.size();
	while (min < max) {
		size_t mid = (min + max) / 2;
		int result = name.compare(descriptors[mid]->name);
		if (result < 0) {
			max = mid;
		}
		else if (result > 0) {
			min = mid + 1;
		}
		else {
			return descriptors.begin() + mid;
		}
	}

	return descriptors.end();
}

template <class T>
void MemberDescriptorContainer<T>::declare(T* descriptor)
{
	std::scoped_lock lock(sync());

	typename Collection::iterator where = std::lower_bound(descriptors.begin(), descriptors.end(), descriptor, compare);
	if (where == descriptors.end() || *where != descriptor) {
		descriptors.insert(where, descriptor);
	}

	for (typename std::vector<MemberDescriptorContainer*>::iterator it = derivedContainers.begin();
		 it != derivedContainers.end();
		 ++it) {
		(*it)->declare(descriptor);
	}
}
template <class T>
void MemberDescriptorContainer<T>::mergeMembers(const MemberDescriptorContainer* source)
{
	for (typename Collection::const_iterator it = source->descriptors.begin(); it != source->descriptors.end(); ++it) {
		declare(*it);
	}

	if (source->base != nullptr) {
		mergeMembers(source->base);
	}
}

template <class T>
MemberDescriptorContainer<T>::MemberDescriptorContainer(MemberDescriptorContainer* base) : base(base)
{
	if (base != nullptr) {
		std::scoped_lock lock(sync());

		mergeMembers(base);

		base->derivedContainers.push_back(this);
	}
}

class MemberDescriptor : public Descriptor
{
public:
	bool isMemberOf(const ClassDescriptor& classDescriptor) const;
	bool isMemberOf(const DescribedBase* instance) const;

	const Name& category;
	const ClassDescriptor& owner;

protected:
	MemberDescriptor(const ClassDescriptor& owner, const char* name, const char* category)
		: Descriptor(name), category(Name::declare(category, -1)), owner(owner)
	{
	}

	~MemberDescriptor() override = default;
};

} // namespace RBX::Reflection

#endif // REFLECTION_MEMBER_H
