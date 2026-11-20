#ifndef UTIL_HASH_H
#define UTIL_HASH_H

#include <string>

namespace RBX {

class Hash
{
public:
	static unsigned int hash(const std::string& text);
	static unsigned int hash(const void* data, unsigned int size);
	static void hashAppend(unsigned int& hash, unsigned int value);
	static void hashAppend(unsigned int& hash, const void* buffer, unsigned int size);
};

} // namespace RBX

#endif // UTIL_HASH_H
