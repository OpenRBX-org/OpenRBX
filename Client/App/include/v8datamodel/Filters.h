#ifndef V8DATAMODEL_FILTERS_H
#define V8DATAMODEL_FILTERS_H

#include "util/HitTestFilter.h"

#include <memory>

namespace RBX {

class Instance;
class ModelInstance;
class PartInstance;

class Unlocked : public HitTestFilter
{
public:
	static bool unlocked(const Primitive* testMe);

	virtual Result filterResult(const Primitive* testMe) const { return unlocked(testMe) ? INCLUDE_PRIM : STOP_TEST; }
};

class PartByLocalCharacter : public HitTestFilter
{
protected:
	std::shared_ptr<ModelInstance> character;
	std::shared_ptr<PartInstance> head;

public:
	PartByLocalCharacter(Instance* root);

	virtual Result filterResult(const Primitive* testMe) const;
};

class UnlockedPartByLocalCharacter : public PartByLocalCharacter
{
public:
	UnlockedPartByLocalCharacter(Instance* root) : PartByLocalCharacter(root) {}

	virtual Result filterResult(const Primitive* testMe) const;
};

class IgnoreDescendantsOf : public HitTestFilter
{
protected:
	std::shared_ptr<Instance> inst;

public:
	IgnoreDescendantsOf(Instance* i);

	virtual Result filterResult(const Primitive* testMe) const;
};

} // namespace RBX

#endif // V8DATAMODEL_FILTERS_H
