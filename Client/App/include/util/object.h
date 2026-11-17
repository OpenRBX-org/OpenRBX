#ifndef UTIL_OBJECT_H
#define UTIL_OBJECT_H

#include "util/Name.h"

#include <cstdlib>
#include <map>
#include <memory>

namespace RBX {

class Instance;

class Object
{
public:
	virtual ~Object() = default;
};

template <class T>
std::shared_ptr<T> shared_from(T* r)
{
	return r != nullptr ? std::static_pointer_cast<T>(r->shared_from_this()) : std::shared_ptr<T>();
}

template <class T, class Base>
std::shared_ptr<T> shared_from_dynamic_cast(std::enable_shared_from_this<Base>* r)
{
	return r != nullptr ? std::dynamic_pointer_cast<T>(r->shared_from_this()) : std::shared_ptr<T>();
}

template <class T, class Base>
std::shared_ptr<T> shared_from_polymorphic_downcast(std::enable_shared_from_this<Base>* r)
{
	return r != nullptr ? std::static_pointer_cast<T>(r->shared_from_this()) : std::shared_ptr<T>();
}

template <class T>
class Creatable : public Object
{
public:
	class Deleter
	{
	public:
		void operator()(T* instance)
		{
			T::predelete(instance);
			delete instance;
		}
	};

	template <class U>
	static std::shared_ptr<U> create()
	{
		return std::shared_ptr<U>(new U(), Deleter());
	}

	template <class U, class A>
	static std::shared_ptr<U> create(A p1)
	{
		return std::shared_ptr<U>(new U(p1), Deleter());
	}

	template <class U, class A, class B>
	static std::shared_ptr<U> create(A p1, B p2)
	{
		return std::shared_ptr<U>(new U(p1, p2), Deleter());
	}

	template <class U, class A, class B, class C>
	static std::shared_ptr<U> create(A p1, B p2, C p3)
	{
		return std::shared_ptr<U>(new U(p1, p2, p3), Deleter());
	}

	template <class U, class A, class B, class C, class D>
	static std::shared_ptr<U> create(A p1, B p2, C p3, D p4)
	{
		return std::shared_ptr<U>(new U(p1, p2, p3, p4), Deleter());
	}

	template <class U, class A, class B, class C, class D, class E>
	static std::shared_ptr<U> create(A p1, B p2, C p3, D p4, E p5)
	{
		return std::shared_ptr<U>(new U(p1, p2, p3, p4, p5), Deleter());
	}

	template <class U, class A, class B, class C, class D, class E, class F>
	static std::shared_ptr<U> create(A p1, B p2, C p3, D p4, E p5, F p6)
	{
		return std::shared_ptr<U>(new U(p1, p2, p3, p4, p5, p6), Deleter());
	}

protected:
	Creatable() {}
	~Creatable() override = default;

	static void* operator new(size_t size) { return malloc(size); }

	static void operator delete(void* p) { free(p); }
};

class ICreator
{
public:
	virtual std::shared_ptr<Object> create() const = 0;
};

template <class T>
class AbstractFactoryProduct : public Creatable<T>
{
protected:
	AbstractFactoryProduct() {}

	static std::map<const Name*, const ICreator*>& getCreators()
	{
		static std::map<const Name*, const ICreator*> creators;
		return creators;
	}

public:
	virtual const Name& getClassName() const = 0;

	static std::shared_ptr<T> create(const Name& name)
	{
		std::map<const Name*, const ICreator*>::iterator iter = getCreators().find(&name);
		if (iter == getCreators().end()) {
			return std::shared_ptr<T>();
		}

		return std::dynamic_pointer_cast<T>(iter->second->create());
	}
};

template <class T, class Base, const char* sName>
class FactoryProduct : public Base
{
public:
	class Creator : public ICreator
	{
	public:
		std::shared_ptr<Object> create() const override { return Creatable<Instance>::create<T>(); }

		const Name& getClassName() const { return Name::declare<sName>(); }

		Creator() { FactoryProduct::getCreators()[&getClassName()] = this; }

		~Creator() { FactoryProduct::getCreators().erase(&getClassName()); }
	};

	const Creator& getCreator() const { return creator; }

	static const Name& className() { return Name::declare<sName>(); }

	const Name& getClassName() const override { return className(); }

protected:
	FactoryProduct() {}

	template <class U>
	FactoryProduct(U arg0) : Base(arg0)
	{
	}

	~FactoryProduct() override = default;

private:
	static const Creator creator;
};

template <class T, class Base, const char* sName>
const typename FactoryProduct<T, Base, sName>::Creator FactoryProduct<T, Base, sName>::creator;

template <class Base, const char* sName>
class NonFactoryProduct : public Base
{
public:
	NonFactoryProduct() {}

	template <class U>
	NonFactoryProduct(U arg0) : Base(arg0)
	{
	}

	static const Name& className() { return Name::declare<sName>(); }

	const Name& getClassName() const { return className(); }
};

} // namespace RBX

#endif // UTIL_OBJECT_H
