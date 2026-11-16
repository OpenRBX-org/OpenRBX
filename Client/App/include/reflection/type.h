#ifndef REFLECTION_TYPE_H
#define REFLECTION_TYPE_H

#include "reflection/descriptor.h"
#include "util/Utilities.h"

#include <G3D/format.h>
#include <any>
#include <list>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <typeinfo>
#include <vector>

namespace G3D {
class Vector3;
}

namespace RBX {

class ContentId;
class Instance;
namespace Reflection {

template <class Signature>
struct FunctionTraits;

template <class R, class... Args>
struct FunctionTraits<R(Args...)>
{
	using Result = R;
	using Arguments = std::tuple<Args...>;

	static constexpr int arity = sizeof...(Args);
};

template <class Signature>
using ResultType = typename FunctionTraits<Signature>::Result;

template <class Signature, int index>
using ArgumentType = std::tuple_element_t<index, typename FunctionTraits<Signature>::Arguments>;

template <class Target, class Source>
Target polymorphicCast(Source* source)
{
	Target target = dynamic_cast<Target>(source);

	if (target == nullptr) {
		throw std::bad_cast();
	}

	return target;
}

class Type : public Descriptor
{
public:
	template <class T>
	static const Type& singleton();

	bool operator==(const Type& other) const { return this == &other; }
	bool operator!=(const Type& other) const { return this != &other; }

	const std::type_info& type;
	const Name& tag;

protected:
	Type(const char* name, const std::type_info& type) : Descriptor(name), type(type), tag(Name::lookup(name)) {}

	Type(const char* name, const std::type_info& type, const char* tag)
		: Descriptor(name), type(type), tag(Name::declare(tag, -1))
	{
	}
};

class RefType : public Type
{
public:
	template <class T>
	static const Type& singleton();

private:
	RefType(const char* name, const std::type_info& type) : Type(name, type, "Ref") {}
};

template <class T>
const Type& RefType::singleton()
{
	static RefType type("Object", typeid(RefType));

	return type;
}

class Value
{
public:
	Value();

	template <class T>
	Value(const T& value) : _type(&Type::singleton<T>()), value(value)
	{
	}

	template <class T>
	T get() const
	{
		if (isType<T>()) {
			return std::any_cast<const T&>(value);
		}

		Value v(*this);
		return v.convert<T>();
	}

	template <class T>
	Value& operator=(const T& rhs)
	{
		_type = &Type::singleton<T>();
		value = rhs;
		return *this;
	}

	bool isVoid() const;
	const Type& type() const { return *_type; }

	template <class T>
	bool isType() const
	{
		return typeid(T) == type().type;
	}

	template <class T>
	T cast() const
	{
		return std::any_cast<T>(value);
	}

	template <class T>
	T cast()
	{
		return std::any_cast<T>(value);
	}

	template <class T>
	T& convert();

private:
	template <class T>
	T& genericConvert();

	const Type* _type;
	std::any value;
};

template <class T>
T& Value::genericConvert()
{
	if (T* t = std::any_cast<T>(&value)) {
		return *t;
	}

	if (isType<std::string>()) {
		T v;
		if (StringConverter<T>::convertToValue(*std::any_cast<std::string>(&value), v)) {
			value = v;
			_type = &Type::singleton<T>();
			return *std::any_cast<T>(&value);
		}
	}

	throw std::runtime_error(
		G3D::format("Unable to cast %s to %s", _type->tag.c_str(), Type::singleton<T>().tag.c_str())
	);
}

template <class T>
T& Value::convert()
{
	return genericConvert<T>();
}

template <>
int& Value::convert<int>();
template <>
bool& Value::convert<bool>();
template <>
float& Value::convert<float>();
template <>
double& Value::convert<double>();
template <>
G3D::Vector3& Value::convert<G3D::Vector3>();
template <>
ContentId& Value::convert<ContentId>();
template <>
std::string& Value::convert<std::string>();
template <>
std::shared_ptr<const std::vector<std::shared_ptr<Instance>>>& Value::convert<
	std::shared_ptr<const std::vector<std::shared_ptr<Instance>>>>();
template <>
std::vector<Value>& Value::convert<std::vector<Value>>();
template <>
std::shared_ptr<Instance>& Value::convert<std::shared_ptr<Instance>>();

class SignatureDescriptor
{
public:
	struct Item
	{
		const Name* name;
		const Type* type;
		const Value defaultValue;
	};

	using Arguments = std::list<Item>;

	SignatureDescriptor();

	void addArgument(const Name& name, const Type& type, const Value& defaultValue);

	const Type* resultType;
	Arguments arguments;
};

} // namespace Reflection
} // namespace RBX

#endif // REFLECTION_TYPE_H
