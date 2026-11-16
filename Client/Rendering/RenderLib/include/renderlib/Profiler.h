#ifndef RENDERLIB_PROFILER_H
#define RENDERLIB_PROFILER_H

#include <string>

namespace RBX::Render {

struct SpecData
{
	static bool glLoaded;

	const float shaderModel;
	const int videoMemory;
	const int CPUSpeed;
	const std::string glVendor;
	const std::string gfxcard;
	const std::string cpu;
	const int ram;
	const std::string resolution;

	SpecData();
};

} // namespace RBX::Render

#endif // RENDERLIB_PROFILER_H
