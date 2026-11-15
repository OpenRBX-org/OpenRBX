#ifndef NETWORK_API_H
#define NETWORK_API_H

#include <string>

namespace RBX {
namespace Network {

class API
{
public:
	static std::string version;

	static void init(const char* version);
};

} // namespace Network
} // namespace RBX

#endif // NETWORK_API_H
