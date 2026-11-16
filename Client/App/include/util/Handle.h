#ifndef UTIL_HANDLE_H
#define UTIL_HANDLE_H

#include <memory>

namespace RBX {

class Instance;

class InstanceHandle
{
private:
	std::shared_ptr<Instance> target;

public:
	InstanceHandle() {}
	InstanceHandle(Instance* target);
	InstanceHandle(std::shared_ptr<Instance>);
	InstanceHandle(const InstanceHandle& other) = default;

	InstanceHandle& operator=(const InstanceHandle& other) = default;

	InstanceHandle& operator=(std::shared_ptr<Instance>);

	bool empty() const;

	std::shared_ptr<Instance> getTarget() const { return target; }

	void linkTo(std::shared_ptr<Instance> target);

	bool operator==(const InstanceHandle&) const;
	bool operator!=(const InstanceHandle&) const;
	bool operator<(const InstanceHandle& other) const { return operatorLess(other); }
	bool operator>(const InstanceHandle&) const;

protected:
	bool operatorEqual(const InstanceHandle&) const;
	bool operatorLess(const InstanceHandle& other) const;
	bool operatorGreater(const InstanceHandle&) const;
};

} // namespace RBX

#endif // UTIL_HANDLE_H
