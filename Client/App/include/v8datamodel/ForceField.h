#ifndef V8DATAMODEL_FORCEFIELD_H
#define V8DATAMODEL_FORCEFIELD_H

#include "util/IRenderable.h"
#include "v8tree/Instance.h"

namespace RBX {

extern char sForceField[];

class PartInstance;

class ForceField : public DescribedCreatable<ForceField, Instance, sForceField>, public IRenderable
{
private:
	int cycle;

	bool askSetParent(const Instance* instance) const override;

	bool shouldRender3dAdorn() const override { return true; }

	void render3dAdorn(Adorn* adorn) override;

public:
	ForceField();

	~ForceField() override = default;

	static bool partInForceField(PartInstance* part);
	static int cycles();
};

} // namespace RBX

#endif // V8DATAMODEL_FORCEFIELD_H
