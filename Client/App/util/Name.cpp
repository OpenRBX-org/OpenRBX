#include "util/Name.h"

#include <mutex>

namespace RBX {

class Name::NamMap : public std::map<std::string, Name*>
{
public:
	~NamMap();
};

static std::once_flag flag;

std::recursive_mutex& moo2()
{
	static std::recursive_mutex mutex2;

	return mutex2;
}

void initMoo()
{
	moo2();
}

std::map<int, Name*>& Name::dictionary()
{
	static std::map<int, Name*> d;

	return d;
}

Name::NamMap::~NamMap()
{
	for (iterator iter = begin(); iter != end(); ++iter) {
		delete iter->second;
	}
}

Name::NamMap& Name::namMap()
{
	static NamMap n;

	return n;
}

const Name& Name::declare(const char* sName, int dictionaryIndex)
{
	if (sName == nullptr) {
		return getNullName();
	}

	std::call_once(flag, initMoo);

	std::scoped_lock lock(moo2());
	NamMap::iterator iter = namMap().find(std::string(sName));

	if (iter != namMap().end()) {
		if (dictionaryIndex != -1) {
			iter->second->dictionaryIndex = dictionaryIndex;
			dictionary()[dictionaryIndex] = iter->second;
		}

		return *iter->second;
	}

	Name* declared = new Name(sName, dictionaryIndex);
	namMap()[sName] = declared;
	dictionary()[dictionaryIndex] = declared;

	return *declared;
}

const Name& Name::getNullName()
{
	std::call_once(flag, initMoo);

	std::scoped_lock lock(moo2());
	static const Name* nullName = &declare("", 0);

	return *nullName;
}

const Name& Name::lookup(const std::string& sName)
{
	std::call_once(flag, initMoo);

	std::scoped_lock lock(moo2());
	NamMap::iterator iter = namMap().find(sName);

	if (iter != namMap().end()) {
		return *iter->second;
	}

	return getNullName();
}

const Name& Name::lookup(const char* sName)
{
	if (sName == nullptr) {
		return getNullName();
	}

	return lookup(std::string(sName));
}

bool Name::empty() const
{
	return this == &getNullName();
}

Name::Name(const char* name, int dictionaryIndex) : dictionaryIndex(dictionaryIndex), name(name)
{
}

} // namespace RBX
