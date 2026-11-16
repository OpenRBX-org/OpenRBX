#ifndef UTIL_STANDARDOUT_H
#define UTIL_STANDARDOUT_H

#include "util/Events.h"

#include <ctime>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace RBX {

enum MessageType
{
	MESSAGE_OUTPUT = 0,
	MESSAGE_INFO = 1,
	MESSAGE_WARNING = 2,
	MESSAGE_ERROR = 3
};

struct StandardOutMessage
{
	StandardOutMessage(MessageType type, const char* message) : type(type), message(message) { _time64(&time); }

	MessageType type;
	std::string message;
	__int64 time;
};

class StandardOut : public std::enable_shared_from_this<StandardOut>, public Notifier<StandardOut, StandardOutMessage>
{
public:
	static std::shared_ptr<StandardOut> singleton();
	static void print_exception(const std::function<void()>& job, MessageType type, bool rethrow);

	void print(MessageType type, const std::exception& error);
	void print(MessageType type, const char* format, ...);

	std::recursive_mutex sync;
};

} // namespace RBX

#endif // UTIL_STANDARDOUT_H
