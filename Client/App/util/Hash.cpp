#include "util/Hash.h"

namespace RBX {

void Hash::hashAppend(unsigned int& hash, const void* buffer, unsigned int size)
{
	const unsigned char* data = static_cast<const unsigned char*>(buffer);
	unsigned int a = 63689;

	for (unsigned int i = 0; i < size; i++) {
		hash = hash * a + data[i];
		a = a * 378551;
	}

	hash &= 0x7fffffff;
}

unsigned int Hash::hash(const void* data, unsigned int size)
{
	unsigned int result = 0;
	hashAppend(result, data, size);
	return result;
}

unsigned int Hash::hash(const std::string& text)
{
	return hash(text.c_str(), text.size());
}

} // namespace RBX
