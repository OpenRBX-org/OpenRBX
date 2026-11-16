#include "reflection/signal.h"

#include "reflection/object.h"

#include <cstddef>

namespace RBX::Reflection {

GenericSlotWrapper::~GenericSlotWrapper() = default;

SignalInstance::~SignalInstance() = default;

SignalInstance* SignalDescriptor::findSignalInstance(const SignalSource* source) const
{
	if (source == nullptr) {
		return nullptr;
	}

	if (source->signals.get() == nullptr) {
		return nullptr;
	}

	SignalSource::SignalMap::const_iterator it = source->signals->find(this);

	if (it == source->signals->end()) {
		return nullptr;
	}

	return it->second.get();
}

SignalDescriptor::SignalDescriptor(ClassDescriptor& classDescriptor, const char* name)
	: MemberDescriptor(classDescriptor, name, "Signals"), signalCreatedHook(nullptr)
{
	classDescriptor.MemberDescriptorContainer<SignalDescriptor>::declare(this);
}

std::shared_ptr<SignalInstance> SignalDescriptor::getSignalInstance(SignalSource& source) const
{
	if (source.signals.get() == nullptr) {
		source.signals.reset(new SignalSource::SignalMap());
	}
	else {
		SignalSource::SignalMap::iterator iter = source.signals->find(this);

		if (iter != source.signals->end()) {
			return iter->second;
		}
	}

	std::shared_ptr<SignalInstance> si(newSignalInstance(source));

	(*source.signals)[this] = si;

	if (signalCreatedHook != nullptr) {
		signalCreatedHook(&source);
	}

	return si;
}

std::shared_ptr<SignalInstance> Signal::getSignalInstance() const
{
	return descriptor->getSignalInstance(*instance);
}

SignalSource::~SignalSource() = default;

void SignalSource::disconnect_all_slots()
{
	signals.reset();
}

} // namespace RBX::Reflection
