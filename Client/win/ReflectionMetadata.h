#ifndef REFLECTIONMETADATA_H
#define REFLECTIONMETADATA_H

#include "reflection/function.h"
#include "reflection/property.h"
#include "reflection/signal.h"
#include "v8tree/Instance.h"

#include <algorithm>
#include <string>

namespace RBX::Reflection::Metadata {

extern char sReflection[];
extern char sClasses[];
extern char sItem[];
extern char sClass[];
extern char sProperties[];
extern char sFunctions[];
extern char sEvents[];
extern char sMember[];

class Class;

class Members : public Instance
{
public:
	Members() {}

protected:
	bool askAddChild(const Instance* instance) const override;
};

class Classes : public DescribedCreatable<Classes, Instance, sClasses>
{
public:
	Classes() {}

	Class* findClass(const ClassDescriptor* descriptor, bool recurse);
	void addHiddenClass(const ClassDescriptor& descriptor);

protected:
	bool askAddChild(const Instance* instance) const override;
};

class Properties : public DescribedCreatable<Properties, Members, sProperties>
{
public:
	Properties() {}
};

class Functions : public DescribedCreatable<Functions, Members, sFunctions>
{
public:
	Functions() {}
};

class Events : public DescribedCreatable<Events, Members, sEvents>
{
public:
	Events() {}
};

class Item : public DescribedNonCreatable<Item, Instance, sItem>
{
public:
	Item() : browsable(true), deprecated(false), preliminary(false) {}

	static RBX::Reflection::BoundProp<bool, 1> prop_Browsable;
	static RBX::Reflection::BoundProp<bool, 1> prop_Deprecated;
	static RBX::Reflection::BoundProp<bool, 1> prop_Preliminary;
	static RBX::Reflection::BoundProp<std::string, 1> prop_summary;

	bool browsable;
	bool deprecated;
	bool preliminary;
	std::string summary;
};

class Class : public DescribedCreatable<Class, Item, sClass>
{
public:
	Class() : explorerOrder(-1), explorerImageIndex(0) {}

	int getExplorerOrder() const
	{
		if (explorerOrder) {
			return explorerOrder;
		}
		return dynamic_cast<Class*>(getParent()) && dynamic_cast<Class*>(getParent())->getExplorerOrder();
	}

	int getExplorerImageIndex() const
	{
		if (explorerImageIndex == -1) {
			return dynamic_cast<Class*>(getParent()) && dynamic_cast<Class*>(getParent())->getExplorerImageIndex();
		}
		return std::max<int>(explorerImageIndex, 0);
	}

	static RBX::Reflection::BoundProp<int, 1> prop_ExplorerOrder;
	static RBX::Reflection::BoundProp<int, 1> prop_ExplorerImageIndex;

protected:
	bool askAddChild(const Instance* instance) const override;

private:
	int explorerOrder;
	int explorerImageIndex;
};

class Member : public DescribedCreatable<Member, Item, sMember>
{
public:
	Member() {}
};

class Reflection : public DescribedCreatable<Reflection, Instance, sReflection>
{
public:
	Reflection();

	Class* getClass(const ClassDescriptor* descriptor, bool recurse);
	Member* getProperty(const PropertyDescriptor* descriptor);
	Member* getFunction(const FunctionDescriptor* descriptor);
	Member* getEvent(const SignalDescriptor* descriptor);

	void load(std::string fileName);

	static std::shared_ptr<Reflection> singleton();

protected:
	bool askAddChild(const Instance* instance) const override;

private:
	Classes* m_classes;
};

void registerClasses();

} // namespace RBX::Reflection::Metadata

#endif // REFLECTIONMETADATA_H
