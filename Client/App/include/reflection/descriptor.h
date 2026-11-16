#ifndef REFLECTION_DESCRIPTOR_H
#define REFLECTION_DESCRIPTOR_H

#include "util/Name.h"

#include <mutex>

namespace RBX::Reflection {

static std::recursive_mutex& sync()
{
	static std::recursive_mutex s;

	return s;
}

class Descriptor
{
public:
	Descriptor(const Descriptor&) = delete;
	Descriptor& operator=(const Descriptor&) = delete;

	virtual ~Descriptor() = default;

	const Name& name;

	Descriptor(const char* name) : name(Name::declare(name, -1)) {}
};

} // namespace RBX::Reflection

#endif // REFLECTION_DESCRIPTOR_H
