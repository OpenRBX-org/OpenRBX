#include "util/Timer.h"

#include <chrono>

namespace RBX {

double getRealTime()
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

} // namespace RBX
