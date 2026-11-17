#ifndef UTIL_IMETRIC_H
#define UTIL_IMETRIC_H

#include <string>

namespace RBX {

class IMetric
{
public:
	IMetric() {}
	virtual ~IMetric() = default;

	virtual std::string getMetric(const std::string&) const = 0;
};

} // namespace RBX

#endif // UTIL_IMETRIC_H
