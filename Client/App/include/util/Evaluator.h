#ifndef UTIL_EVALUATOR_H
#define UTIL_EVALUATOR_H

#include <string>

namespace RBX {

class Evaluator
{
public:
	virtual std::string evaluate(const std::string& valueName) const = 0;
};

} // namespace RBX

#endif // UTIL_EVALUATOR_H
