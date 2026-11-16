#ifndef V8KERNEL_KERNELINDEX_H
#define V8KERNEL_KERNELINDEX_H

namespace RBX {

class KernelIndex
{
public:
	KernelIndex() : kernelIndex(-1) {}
	~KernelIndex() = default;

	bool inKernel() const { return kernelIndex >= 0; }

protected:
	mutable int kernelIndex;
};

} // namespace RBX

#endif // V8KERNEL_KERNELINDEX_H
