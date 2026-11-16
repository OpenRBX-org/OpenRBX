#ifndef V8DATAMODEL_MODELINSTANCE_H
#define V8DATAMODEL_MODELINSTANCE_H

#include "util/ComputeProp.h"
#include "util/Extents.h"
#include "util/ICameraSubject.h"
#include "util/IRenderable.h"
#include "util/ISelectable3d.h"
#include "v8datamodel/IPrimaryPart.h"
#include "v8datamodel/PVInstance.h"

#include <G3D/CoordinateFrame.h>
#include <memory>

namespace RBX {

class PartInstance;

extern char sModel[];

class ModelInstance : public DescribedCreatable<ModelInstance, PVInstance, sModel>,
					  public IRenderable,
					  public virtual IPrimaryPart,
					  public virtual ICameraSubject,
					  public virtual ISelectable3d
{
public:
	static bool showModelCoord;

	void FUN_00565040(const G3D::Vector3& front);

private:
	mutable CoordinateFrame modelInPrimary;
	mutable PartInstance* primaryPart;
	std::shared_ptr<PartInstance> candidatePrimaryPart;

	void updatePrimaryPart(PartInstance* part) const;

	ComputeProp<float, ModelInstance> FlagHeight;
	ComputeProp<Extents, ModelInstance> LocalGridExtents;
	ComputeProp<Extents, ModelInstance> WorldGridExtents;

	float computeFlagHeight() const;
	Extents computeLocalGridExtents() const;
	Extents computeWorldGridExtents() const;
	void dirtyAll() const
	{
		LocalGridExtents.setDirty();
		WorldGridExtents.setDirty();
		FlagHeight.setDirty();
	}

	bool shouldRender3dAdorn() const override;
	void render3dAdorn(Adorn* adorn) override;
	void render3dSelect(Adorn* adorn, SelectState selectState) override;

protected:
	void onDescendentAdded(Instance* instance) override;
	void onDescendentRemoving(const shared_ptr<Instance>& instance) override;
	bool askSetParent(const Instance* instance) const override;

private:
	void onLastChildRemoved() override;

public:
	ModelInstance();
	~ModelInstance() override;

	PartInstance* getPrimaryPartInternal() const;
	void setPrimaryPart(PartInstance* value);

	const CoordinateFrame& getModelInPrimary() const { return modelInPrimary; }
	void setModelInPrimary(const CoordinateFrame& set) { modelInPrimary = set; }

	void onExtentsChanged() const override;

	const Primitive* getBiggestPrimitive() const override;
	bool hitTest(const G3D::Ray& worldRay, G3D::Vector3& worldHitPoint) override;

	Extents getExtentsWorld() const override;
	Extents getExtentsLocal() const override;

	const CoordinateFrame getLocation() const override;

	void onCameraNear(float distance) override;
	void getCameraIgnorePrimitives(std::vector<const Primitive*>& primitives) override;

	PartInstance* getPrimaryPart() override;
	const PartInstance* getPrimaryPartConst() const override;
	void setFrontAndTop(const G3D::Vector3&);
	void destroyIfNoParts(const ModelInstance*);

	void makeJoints();
	void breakJoints();
	float computeTotalMass() const;
	float computeLargestMoment() const;

protected:
	void legacyTraverseState(const CoordinateFrame& parentState) override;

public:
};

} // namespace RBX

#endif // V8DATAMODEL_MODELINSTANCE_H
