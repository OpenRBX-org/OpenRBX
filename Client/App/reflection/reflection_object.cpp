#include "reflection/reflection.h"

#include <algorithm>
#include <cstring>
#include <iterator>

namespace RBX::Reflection {

bool ClassDescriptor::lockedDown;

bool ClassDescriptor::operator==(const ClassDescriptor& other) const
{
	return this == &other;
}

bool ClassDescriptor::operator!=(const ClassDescriptor& other) const
{
	return this != &other;
}

static const char* const registrationOrder[] = {
	"Instance",
	"HtmlService",
	"HtmlWindow",
	"ReflectionMetadataItem",
	"ReflectionMetadataClass",
	"ReflectionMetadataMember",
	"RenderSettings",
	"Player",
	"Players",
	"NetworkPeer",
	"NetworkClient",
	"NetworkServer",
	"NetworkSettings",
	"NetworkReplicator",
	"PVInstance",
	"Model",
	"RootInstance",
	"Workspace",
	"Part",
	"SpawnLocation",
	"FlagStand",
	"Seat",
	"Selection",
	"RunService",
	"ScriptContext",
	"Script",
	"LocalScript",
	"DebugSettings",
	"Team",
	"ServiceProvider",
	"DataModel",
	"GlobalSettings",
	"FaceInstance",
	"Decal",
	"Texture",
	"SpecialMesh",
	"Accoutrement",
	"Hat",
	"SoundService",
	"Sound",
	"StockSound",
	"GameSettings",
	"Visit",
	"Item",
	"Camera",
	"Lighting",
	"Sky",
	"BackpackItem",
	"HopperBin",
	"Tool",
	"Flag",
	"ShirtGraphic",
	"Shirt",
	"Skin",
	"BodyColors",
	"Teams",
	"Humanoid",
	"JointInstance",
	"AutoJoint",
	"Motor",
	"Glue",
	"Rotate",
	"RotateP",
	"RotateV",
	"Snap",
	"Weld",
	"VelocityMotor",
	"Feature",
	"MotorFeature",
	"Hole",
	"Explosion",
	"RocketPropulsion",
	"BodyGyro",
	"BodyPosition",
	"BodyVelocity",
	"BodyForce",
	"BodyThrust",
	"Message",
	"Hint",
	"IntValue",
	"BoolValue",
	"NumberValue",
	"StringValue",
	"Vector3Value",
	"CFrameValue",
	"Color3Value",
	"BrickColorValue",
	"ObjectValue",
	"Debris",
	"Mouse",
	"ReflectionMetadata",
	"ReflectionMetadataClasses",
	"ReflectionMetadataFunctions",
	"ReflectionMetadataProperties",
	"ReflectionMetadataEvents",
	"Backpack",
	"ControllerService",
	"FlagStandService",
	"ForceField",
	"Geometry",
	"Hopper",
	"LocalBackpack",
	"LocalBackpackItem",
	"StarterPack",
	"Stats",
	"Timer",
	"SpawnerService",
	"NetworkMarker",
};

static size_t registrationRank(const char* name)
{
	return std::find_if(
			   std::begin(registrationOrder),
			   std::end(registrationOrder),
			   [name](const char* entry) { return strcmp(entry, name) == 0; }
		   ) -
		   std::begin(registrationOrder);
}

ClassDescriptor::ClassDescriptor()
	: Descriptor("<<<ROOT>>>"), MemberDescriptorContainer<PropertyDescriptor>(nullptr),
	  MemberDescriptorContainer<SignalDescriptor>(nullptr), MemberDescriptorContainer<FunctionDescriptor>(nullptr),
	  base(nullptr)
{
}

ClassDescriptor::ClassDescriptor(ClassDescriptor& base, const char* name)
	: Descriptor(name), MemberDescriptorContainer<PropertyDescriptor>(&base),
	  MemberDescriptorContainer<SignalDescriptor>(&base), MemberDescriptorContainer<FunctionDescriptor>(&base),
	  base(&base)
{
	std::scoped_lock lock(sync());

	size_t rank = registrationRank(name);
	base.derivedClasses.insert(
		std::find_if(
			base.derivedClasses.begin(),
			base.derivedClasses.end(),
			[rank](const ClassDescriptor* sibling) { return registrationRank(sibling->name.c_str()) > rank; }
		),
		this
	);
}

bool MemberDescriptor::isMemberOf(const ClassDescriptor& classDescriptor) const
{
	const ClassDescriptor* c = &classDescriptor;

	do {
		if (c == &owner) {
			return true;
		}
	} while ((c = c->getBase()) != &ClassDescriptor::rootDescriptor());

	return false;
}

bool MemberDescriptor::isMemberOf(const DescribedBase* instance) const
{
	return isMemberOf(instance->getDescriptor());
}

template class MemberDescriptorContainer<FunctionDescriptor>;
template class MemberDescriptorContainer<PropertyDescriptor>;

} // namespace RBX::Reflection
