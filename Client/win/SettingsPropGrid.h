#ifndef SETTINGSPROPGRID_H
#define SETTINGSPROPGRID_H

#include "PropGrid.h"

class CSettingsPropGrid : public CSelectionPropGrid
{
public:
	void FUN_00442e20(
		const RBX::Reflection::ClassDescriptor* classDescriptor, const RBX::Reflection::PropertyDescriptor* descriptor
	) override;
};

#endif // SETTINGSPROPGRID_H
