#ifndef REFLECTION_SIGNAL_H
#define REFLECTION_SIGNAL_H

#include "reflection/member.h"
#include "reflection/property.h"
#include "reflection/type.h"
#include "util/Signals.h"
#include "util/standardout.h"

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace RBX::Reflection {

class Signal;
class SignalDescriptor;
class SignalSource;

class SignalInstance
{
public:
	SignalInstance(const SignalInstance&) = delete;
	SignalInstance& operator=(const SignalInstance&) = delete;

	SignalSource* getSource() { return source; }

	template <class Slot>
	Signals::Connection connectGeneric(Slot slot, Signals::ConnectPosition position);

	virtual ~SignalInstance();

protected:
	SignalInstance(SignalSource* source, const SignalDescriptor& descriptor) : source(source), descriptor(descriptor) {}

private:
	SignalSource* source;

public:
	const SignalDescriptor& descriptor;
};

class SignalSource
{
public:
	using SignalMap = std::map<const SignalDescriptor*, std::shared_ptr<SignalInstance>>;

	virtual ~SignalSource();

	void disconnect_all_slots();

private:
	friend class SignalDescriptor;

	std::unique_ptr<SignalMap> signals;
};

class GenericSlotWrapper
{
public:
	virtual ~GenericSlotWrapper();

	virtual void execute(const std::vector<std::any>& arguments) = 0;

	template <class Slot>
	static GenericSlotWrapper* create(Slot slot);
};

template <class Slot>
class TGenericSlotWrapper : public GenericSlotWrapper
{
	friend class GenericSlotWrapper;

public:
	void execute(const std::vector<std::any>& arguments) override;

private:
	TGenericSlotWrapper(const Slot& slot) : slot(slot) {}

	Slot slot;
};

template <class Slot>
void TGenericSlotWrapper<Slot>::execute(const std::vector<std::any>& arguments)
{
	try {
		slot(arguments);
	}
	catch (const std::exception& e) {
		StandardOut::singleton()->print(MESSAGE_ERROR, "Exception caught in TGenericSlotWrapper. %s", e.what());
	}
}

template <class Slot>
GenericSlotWrapper* GenericSlotWrapper::create(Slot slot)
{
	return new TGenericSlotWrapper<Slot>(slot);
}

class SignalDescriptor : public MemberDescriptor
{
public:
	using ConstMember = Signal;
	using Member = Signal;

	void (*signalCreatedHook)(SignalSource*);

protected:
	friend class SignalInstance;

	SignalInstance* findSignalInstance(const SignalSource* source) const;

private:
	virtual Signals::Connection connectGeneric(
		SignalInstance* instance, GenericSlotWrapper* wrapper, Signals::ConnectPosition position
	) const = 0;

	virtual SignalInstance* newSignalInstance(SignalSource& source) const = 0;

public:
	std::shared_ptr<SignalInstance> getSignalInstance(SignalSource& source) const;
	const SignatureDescriptor& getSignature() const;

protected:
	SignalDescriptor(ClassDescriptor& classDescriptor, const char* name);

	SignatureDescriptor signature;
};

class Signal
{
public:
	Signal(const Signal& other) = default;
	Signal(const SignalDescriptor& descriptor, DescribedBase* instance) : descriptor(&descriptor), instance(instance) {}

	const Name& getName() const { return descriptor->name; }

	const SignalDescriptor* getDescriptor() const { return descriptor; }

	std::shared_ptr<SignalInstance> getSignalInstance() const;

protected:
	const SignalDescriptor* descriptor;
	DescribedBase* instance;
};

template <class Slot>
Signals::Connection SignalInstance::connectGeneric(Slot slot, Signals::ConnectPosition pos)
{
	return descriptor.connectGeneric(this, GenericSlotWrapper::create(slot), pos);
}

template <class Signature>
class TSignalDesc : public SignalDescriptor
{
public:
	class TSignalInstance : public SignalInstance, public Signals::Signal<Signature>
	{
	public:
		TSignalInstance(SignalSource* source, const SignalDescriptor& descriptor) : SignalInstance(source, descriptor)
		{
		}
	};

protected:
	TSignalDesc(ClassDescriptor& classDescriptor, const char* name) : SignalDescriptor(classDescriptor, name) {}

private:
	SignalInstance* newSignalInstance(SignalSource& source) const override
	{
		return new TSignalInstance(&source, *this);
	}

protected:
	TSignalInstance* findSig(const SignalSource* source) const
	{
		return static_cast<TSignalInstance*>(findSignalInstance(source));
	}

	TSignalInstance& sig(SignalSource& source) const
	{
		return *static_cast<TSignalInstance*>(getSignalInstance(source).get());
	}

public:
	Signals::Connection connect(SignalSource* source, typename TSignalInstance::Function slot, int group)
	{
		if (source != nullptr) {
			return sig(*source).connect(group, std::move(slot), Signals::at_back);
		}

		return Signals::Connection();
	}

	bool empty(const SignalSource* source)
	{
		const TSignalInstance* instance = findSig(source);

		return instance == nullptr || instance->empty();
	}
};

template <int arity, class Signature>
class SignalDescImpl : public TSignalDesc<Signature>
{
protected:
	class GenericSlotAdapter
	{
	public:
		GenericSlotAdapter(GenericSlotWrapper* wrapper) : wrapper(wrapper) {}

		void operator()() const { wrapper->execute(std::vector<std::any>()); }

	private:
		std::shared_ptr<GenericSlotWrapper> wrapper;
	};

	Signals::Connection connectGeneric(
		SignalInstance* si, GenericSlotWrapper* wrapper, Signals::ConnectPosition pos
	) const override
	{
		return static_cast<typename TSignalDesc<Signature>::TSignalInstance*>(si)->connect(
			GenericSlotAdapter(wrapper), pos
		);
	}

	SignalDescImpl(ClassDescriptor& classDescriptor, const char* name) : TSignalDesc<Signature>(classDescriptor, name)
	{
	}

public:
	void fire(SignalSource* source)
	{
		typename TSignalDesc<Signature>::TSignalInstance* instance = this->findSig(source);

		if (instance != nullptr) {
			(*instance)();
		}
	}
};

template <class Signature>
class SignalDescImpl<1, Signature> : public TSignalDesc<Signature>
{
public:
	void fire(SignalSource* source, ArgumentType<Signature, 0> arg1)
	{
		typename TSignalDesc<Signature>::TSignalInstance* instance = this->findSig(source);

		if (instance != nullptr) {
			(*instance)(arg1);
		}
	}

protected:
	class GenericSlotAdapter
	{
	public:
		GenericSlotAdapter(GenericSlotWrapper* wrapper) : wrapper(wrapper) {}

		void operator()(ArgumentType<Signature, 0> arg1) const
		{
			std::vector<std::any> args(1);

			args[0] = arg1;

			wrapper->execute(args);
		}

	private:
		std::shared_ptr<GenericSlotWrapper> wrapper;
	};

	Signals::Connection connectGeneric(
		SignalInstance* si, GenericSlotWrapper* wrapper, Signals::ConnectPosition pos
	) const override
	{
		return static_cast<typename TSignalDesc<Signature>::TSignalInstance*>(si)->connect(
			GenericSlotAdapter(wrapper), pos
		);
	}

	SignalDescImpl(ClassDescriptor& classDescriptor, const char* name) : TSignalDesc<Signature>(classDescriptor, name)
	{
	}
};

template <class Signature>
class SignalDescImpl<2, Signature> : public TSignalDesc<Signature>
{
public:
	void fire(SignalSource* source, ArgumentType<Signature, 0> arg1, ArgumentType<Signature, 1> arg2)
	{
		typename TSignalDesc<Signature>::TSignalInstance* instance = this->findSig(source);

		if (instance != nullptr) {
			(*instance)(arg1, arg2);
		}
	}

protected:
	class GenericSlotAdapter
	{
	public:
		GenericSlotAdapter(GenericSlotWrapper* wrapper) : wrapper(wrapper) {}

		void operator()(ArgumentType<Signature, 0> arg1, ArgumentType<Signature, 1> arg2) const
		{
			std::vector<std::any> args(2);

			args[0] = arg1;
			args[1] = arg2;

			wrapper->execute(args);
		}

	private:
		std::shared_ptr<GenericSlotWrapper> wrapper;
	};

	Signals::Connection connectGeneric(
		SignalInstance* si, GenericSlotWrapper* wrapper, Signals::ConnectPosition pos
	) const override
	{
		return static_cast<typename TSignalDesc<Signature>::TSignalInstance*>(si)->connect(
			GenericSlotAdapter(wrapper), pos
		);
	}

	SignalDescImpl(ClassDescriptor& classDescriptor, const char* name) : TSignalDesc<Signature>(classDescriptor, name)
	{
	}
};

template <class Class, class Signature>
class SignalDesc : public SignalDescImpl<FunctionTraits<Signature>::arity, Signature>
{
public:
	SignalDesc(const char* name)
		: SignalDescImpl<FunctionTraits<Signature>::arity, Signature>(Class::classDescriptor(), name)
	{
	}

	SignalDesc(const char* name, const char* arg1name)
		: SignalDescImpl<FunctionTraits<Signature>::arity, Signature>(Class::classDescriptor(), name)
	{
		SignatureDescriptor::Item arg1 = {
			&Name::declare(arg1name, -1), &Type::singleton<ArgumentType<Signature, 0>>(), Value()
		};

		this->signature.arguments.push_back(arg1);
	}

	SignalDesc(const char* name, const char* arg1name, const char* arg2name)
		: SignalDescImpl<FunctionTraits<Signature>::arity, Signature>(Class::classDescriptor(), name)
	{
		SignatureDescriptor::Item arg1 = {
			&Name::declare(arg1name, -1), &Type::singleton<ArgumentType<Signature, 0>>(), Value()
		};

		this->signature.arguments.push_back(arg1);

		SignatureDescriptor::Item arg2 = {
			&Name::declare(arg2name, -1), &Type::singleton<ArgumentType<Signature, 1>>(), Value()
		};

		this->signature.arguments.push_back(arg2);
	}
};

} // namespace RBX::Reflection

#endif // REFLECTION_SIGNAL_H
