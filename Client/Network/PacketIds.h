#ifndef NETWORK_PACKETIDS_H
#define NETWORK_PACKETIDS_H

#include <MessageIdentifiers.h>

namespace RBX::Network {

enum
{
	ID_SET_GLOBALS = ID_USER_PACKET_ENUM,
	ID_REQUEST_CHARACTER,
	ID_DATA,
	ID_REQUEST_MARKER,
	ID_PHYSICS,
	ID_CHAT_ALL,
	ID_REPORT_ABUSE
};

} // namespace RBX::Network

#endif // NETWORK_PACKETIDS_H
