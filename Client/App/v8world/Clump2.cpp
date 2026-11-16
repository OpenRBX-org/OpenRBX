#include "v8world/Clump2.h"

#include "v8kernel/Body.h"
#include "v8world/Assembly2.h"
#include "v8world/IPipelined.h"
#include "v8world/Joint.h"
#include "v8world/MotorJoint.h"
#include "v8world/Primitive.h"
#include "v8world/RigidJoint.h"

#include <cstddef>

namespace RBX {

Edge* EdgeIterator::getNextExternalUtil(Primitive* p, Edge* e)
{
	while (e != nullptr) {
		Primitive* other = e->getPrimitive(0);

		if (p == other) {
			other = e->getPrimitive(1);
		}

		if (other != nullptr && p->getAssembly() != other->getAssembly()) {
			return e;
		}

		e = p->getNextEdge(e);
	}

	return nullptr;
}

bool PrimIterator::isParent(Primitive* parentCandidate, Primitive* child, Joint* joint, SearchType searchType)
{
	if (parentCandidate == nullptr || child == nullptr) {
		return false;
	}

	bool jointed;

	if (searchType == IN_CLUMP) {
		jointed = RigidJoint::isRigidJoint(joint);
	}
	else {
		jointed = RigidJoint::isRigidJoint(joint) || MotorJoint::isMotorJoint(joint);
	}

	if (jointed) {
		if (child->getBody()->getParent() == parentCandidate->getBody()) {
			bool sameClump = parentCandidate->getClump() == child->getClump();

			return searchType == IN_CLUMP ? sameClump
										  : sameClump || parentCandidate->getAssembly() == child->getAssembly();
		}
	}

	return false;
}

Primitive* PrimIterator::findParent(Primitive* p, SearchType searchType)
{
	Joint* joint = p->getFirstJoint();

	while (joint != nullptr) {
		Primitive* other = p == joint->getPrimitive(0) ? joint->getPrimitive(1) : joint->getPrimitive(0);

		if (isParent(other, p, joint, searchType)) {
			return other;
		}

		joint = p->getNextJoint(joint);
	}

	return nullptr;
}

Primitive* PrimIterator::findFirstChild(Primitive* p, SearchType searchType)
{
	Joint* joint = p->getFirstJoint();

	while (joint != nullptr) {
		Primitive* other = p == joint->getPrimitive(0) ? joint->getPrimitive(1) : joint->getPrimitive(0);

		if (isParent(p, other, joint, searchType)) {
			return other;
		}

		joint = p->getNextJoint(joint);
	}

	return nullptr;
}

Primitive* PrimIterator::findNextSibling(Primitive* parent, Primitive* sibling, SearchType searchType)
{
	bool found = false;

	for (Joint* joint = parent->getFirstJoint(); joint != nullptr; joint = parent->getNextJoint(joint)) {
		Primitive* other = parent == joint->getPrimitive(0) ? joint->getPrimitive(1) : joint->getPrimitive(0);

		if (isParent(parent, other, joint, searchType)) {
			if (found) {
				return other;
			}

			if (other == sibling) {
				found = true;
			}
		}
	}

	return nullptr;
}

Primitive* PrimIterator::findNextRelative(Primitive* parent, Primitive* p, SearchType searchType)
{
	if (parent == nullptr) {
		return nullptr;
	}

	Primitive* sibling = findNextSibling(parent, p, searchType);

	if (sibling != nullptr) {
		return sibling;
	}

	Primitive* grandparent = findParent(parent, searchType);

	return findNextRelative(grandparent, parent, searchType);
}

PrimIterator& PrimIterator::operator++()
{
	SearchType type = searchType;
	Primitive* current = primitive;

	Primitive* next = findFirstChild(current, type);

	if (next == nullptr) {
		Primitive* parent = findParent(current, type);

		primitive = findNextRelative(parent, current, type);
	}
	else {
		primitive = next;
	}

	return *this;
}

void EdgeIterator::findEdgeOnNextPrimitive()
{
	while (edge == nullptr) {
		Primitive* next = *++PrimIterator::begin(primitive, PrimIterator::IN_ASSEMBLY);

		if ((primitive = next) == nullptr) {
			return;
		}

		edge = getNextExternalUtil(next, next->getFirstEdge());
	}
}

EdgeIterator& EdgeIterator::operator++()
{
	Primitive* current = primitive;

	edge = getNextExternalUtil(current, current->getNextEdge(edge));

	if (edge == nullptr) {
		findEdgeOnNextPrimitive();
	}

	return *this;
}

EdgeIterator EdgeIterator::begin(Primitive* p)
{
	EdgeIterator answer(p, getNextExternalUtil(p, p->getFirstEdge()));

	if (answer.edge == nullptr) {
		answer.findEdgeOnNextPrimitive();
	}

	return answer;
}

} // namespace RBX
