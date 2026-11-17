#include "util/Guid.h"

#include "util/Name.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <random>

namespace RBX {

static const Name* localScope;
static std::atomic<int> nextIndex;

void Guid::generateGUID(std::string& result)
{
	std::random_device device;
	std::uint32_t data1 = device();
	std::uint32_t data2 = (device() & 0xffff0fff) | 0x00004000;
	std::uint32_t data3 = (device() & 0x3fffffff) | 0x80000000;
	std::uint32_t data4 = device();

	char guid[33];
	std::snprintf(guid, sizeof(guid), "%08X%08X%08X%08X", data1, data2, data3, data4);

	result = "RBX";
	result += guid;
}

void Guid::assign(Data data)
{
	this->data = data;
}

static int compareScopes(const Name* a, const Name* b)
{
	const int order = a->name.compare(b->name);
	return order < 0 ? -1 : (order > 0 ? 1 : 0);
}

bool Guid::Data::operator<(const Data& other) const
{
	const Name* otherScope = other.scope;
	const Name* thisScope = scope;
	int order = 0;

	if (thisScope != otherScope) {
		order = compareScopes(thisScope, otherScope);

		if (order != -1 && order != 0) {
			return false;
		}
	}

	if (order == 0) {
		return index < other.index;
	}

	return true;
}

int Guid::compare(const Guid* a, const Guid* b)
{
	const Name* scopeA = a != nullptr ? a->data.scope : nullptr;
	const Name* scopeB = b != nullptr ? b->data.scope : nullptr;
	int order;

	if (scopeA == nullptr) {
		order = scopeB != nullptr ? -1 : 0;
	}
	else if (scopeB == nullptr) {
		order = 1;
	}
	else if (scopeA == scopeB) {
		order = 0;
	}
	else {
		order = compareScopes(scopeA, scopeB);
	}

	if (order != -1) {
		if (order != 0 && order != 1) {
			return 0;
		}

		if (order == 0) {
			int indexA = a != nullptr ? a->data.index : 0;
			int indexB = b != nullptr ? b->data.index : 0;

			if (indexA > indexB) {
				return 1;
			}

			return indexA >= indexB ? 0 : -1;
		}

		return 1;
	}

	return -1;
}

int Guid::compare(const Guid* a0, const Guid* a1, const Guid* b0, const Guid* b1)
{
	int a = compare(a0, a1);
	int b = compare(b0, b1);
	const Guid* worseA = a == 1 ? a0 : a1;
	int order = compare(worseA, b == 1 ? b0 : b1);

	if (order != -1) {
		if (order != 0) {
			return order == 1;
		}

		const Guid* betterA = a == 1 ? a1 : a0;

		return compare(betterA, b == 1 ? b1 : b0);
	}

	return -1;
}

static void initLocalScope()
{
	std::string guid;

	Guid::generateGUID(guid);
	localScope = &Name::declare(guid.c_str(), -1);
}

const Name& Guid::getLocalScope()
{
	static std::once_flag flag;

	std::call_once(flag, initLocalScope);

	return *localScope;
}

Guid::Guid()
{
	data.scope = &getLocalScope();
	data.index = ++nextIndex;
}

std::string Guid::Data::readableString(int scopeLength) const
{
	char result[64];

	if (scopeLength > 0) {
		std::string s = scope->name;

		s = s.substr(std::min<size_t>(3, s.size()), std::min(scopeLength, 32));

		sprintf(result, "%s_%d", s.c_str(), index);
	}
	else {
		sprintf(result, "%d", index);
	}

	return std::string(result);
}

} // namespace RBX
