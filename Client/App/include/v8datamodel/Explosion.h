#ifndef V8DATAMODEL_EXPLOSION_H
#define V8DATAMODEL_EXPLOSION_H

#include "reflection/property.h"
#include "util/Handle.h"
#include "util/IRenderable.h"
#include "util/RunStateOwner.h"
#include "v8tree/Instance.h"

#include <G3D/Array.h>
#include <G3D/Vector3.h>

namespace RBX {

class Primitive;

extern char sExplosion[];

class Explosion : public DescribedCreatable<Explosion, Instance, sExplosion>,
				  public Listener<RunService, Stepped>,
				  public IRenderable
{
private:
	float age;
	G3D::Vector3 position;
	float blastRadius;
	float blastPressure;

	float renderTime() const { return 0.1f; }
	float visualRadius() const;
	float killRadius() const;
	float blastMaxObjectRadius() const { return 2.0f * blastRadius; }
	float killMaxObjectRadius() const;

	void doKill();
	void doBlast(const G3D::Array<Primitive*>& primitives);
	void signalBlast(const G3D::Array<Primitive*>& primitives);

	void onEvent(const RunService* source, Stepped event) override;

	bool askSetParent(const Instance* instance) const override;

	void onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider) override;

	bool shouldRender3dAdorn() const override { return true; }

	void render3dAdorn(Adorn* adorn) override;

public:
	Explosion();

	~Explosion() override = default;

	static Reflection::BoundProp<G3D::Vector3, 1> propPosition;
	static Reflection::BoundProp<float, 1> propBlastPressure;

	void setVisualOnly();
	void setBlastRadius(float _blastRadius);

	float getBlastRadius() const { return blastRadius; }
};

} // namespace RBX

#endif // V8DATAMODEL_EXPLOSION_H
