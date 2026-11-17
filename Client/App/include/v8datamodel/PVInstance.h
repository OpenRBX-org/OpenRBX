#ifndef V8DATAMODEL_PVINSTANCE_H
#define V8DATAMODEL_PVINSTANCE_H

#include "util/ComputeProp.h"
#include "util/Extents.h"
#include "util/IControllable.h"
#include "util/ILocation.h"
#include "v8datamodel/IPrimaryPart.h"
#include "v8tree/Instance.h"
#include "v8world/Controller.h"

#include <G3D/CoordinateFrame.h>
#include <G3D/Ray.h>
#include <G3D/ReferenceCount.h>
#include <G3D/Vector3.h>
#include <memory>

namespace RBX {

class Adorn;
class Primitive;
class XmlState;

extern const char sPVInstance[];

class PVInstance : public Reflection::Described<PVInstance, sPVInstance, Instance>,
				   public IControllable,
				   public virtual IPrimaryPart,
				   public virtual ILocation
{
protected:
	ComputeProp<bool, PVInstance> IsControllable;
	ComputeProp<bool, PVInstance> IsTopFlag;
	ComputeProp<G3D::ReferenceCountedPointer<Controller>, PVInstance> TopPVController;

private:
	bool computeIsControllable() const;
	bool computeIsTopFlag() const;
	G3D::ReferenceCountedPointer<Controller> computeTopPVController() const;
	void dirtyAll();

	bool canSelect;
	Controller::ControllerType controllerType;
	bool showControllerFlag;

protected:
	std::unique_ptr<CoordinateFrame> legacyOffset;

	PVInstance(const char* name);

public:
	~PVInstance() override;

	Controller::ControllerType getControllerType() const { return controllerType; }
	void setControllerType(Controller::ControllerType _control);

	bool getShowControllerFlag() const { return showControllerFlag; }

	void setShowControllerFlag(bool value);

	static const Reflection::EnumPropDescriptor<PVInstance, Controller::ControllerType> prop_ControllerType;

protected:
	virtual unsigned int topHashCode() const;
	virtual unsigned int childHashCode() const;

	void onChildAdded(Instance* child) override;
	void onChildRemoving(Instance* child) override;
	void onDescendentAdded(Instance* instance) override;
	void onDescendentRemoving(const shared_ptr<Instance>& instance) override;
	void readProperty(const XmlElement* propertyElement, IReferenceBinder& binder) override;

public:
	bool isControllable() const override;

protected:
	void renderCoordinateFrame(Adorn* adorn);
	void onControllerChanged();

	virtual void onChildControllerChanged();
	virtual void onParentControllerChanged();
	virtual void onExtentsChanged() const;

public:
	void moveToPoint(G3D::Vector3 point);
	Controller* getTopPVController() const;

	bool isChaseable() const { return getTopPVController()->hasIntelligence() && IsControllable.getValue(); }

	virtual Extents getExtentsWorld() const = 0;
	virtual Extents getExtentsLocal() const = 0;
	virtual const Primitive* getBiggestPrimitive() const = 0;
	virtual bool hitTest(const G3D::Ray& ray, G3D::Vector3& hitPoint) = 0;

	void writeCoordinateFrameData(XmlState* state);
	void writeVelocityData(XmlState* state);

	const PVInstance* getTopLevelPVParent() const;
	PVInstance* getTopLevelPVParent();

	bool isTopLevelPVInstance() const
	{
		return dynamic_cast<const PVInstance*>(getParent()) == nullptr || getTypedRoot<PVInstance>() == getParent();
	}

	void setPVGridOffsetLegacy(const CoordinateFrame& _offset);
	CoordinateFrame* getLegacyOffset() { return legacyOffset.get(); }
	void clearLegacyOffset();

	virtual void legacyTraverseState(const CoordinateFrame& state) = 0;
};

} // namespace RBX

#endif // V8DATAMODEL_PVINSTANCE_H
