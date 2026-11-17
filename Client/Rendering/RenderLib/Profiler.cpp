#include "renderlib/Profiler.h"

#include <GLG3D/GLCaps.h>
#include <string>

#ifdef _WIN32
#include <G3D/RegistryUtil.h>
#include <GLG3D/DXCaps.h>
#include <intrin.h>
#include <windows.h>
#else
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sys/sysinfo.h>
#endif

namespace RBX::Render {

static float getShaderModel();
static int getVideoMemory();
static int getCPUSpeed();
static std::string getCPUMake();
static std::string getMaxRes();
static int getRam();

bool SpecData::glLoaded;

static float getShaderModel()
{
	if ((G3D::GLCaps::supports_GL_ARB_fragment_shader() && G3D::GLCaps::supports_GL_ARB_vertex_shader()) ||
		(G3D::GLCaps::supports_GL_ARB_shader_objects() && G3D::GLCaps::supports_GL_ARB_shading_language_100())) {
		return 2.0f;
	}

	if (G3D::GLCaps::supports("GL_ATI_fragment_shader") || G3D::GLCaps::supports("GL_NV_fragement_program")) {
		return 2.0f;
	}

	if (G3D::GLCaps::supports("GL_ATI_texture_env_combine3") || G3D::GLCaps::supports("GL_NV_texture_shader3")) {
		return 1.4f;
	}

	if (G3D::GLCaps::supports("GL_NV_register_combiners")) {
		return 1.2f;
	}

	return 1.0f;
}

#ifdef _WIN32

using DelayFunction = void (*)(unsigned int uiParameter);

static __int64 GetCyclesDifference(DelayFunction DelayFunction, unsigned int uiParameter)
{
	__try {
		unsigned __int64 t0 = __rdtsc();
		DelayFunction(uiParameter);
		return __rdtsc() - t0;
	}
	__except (1) {
		return 0;
	}
}

static void Delay(unsigned int time)
{
	LARGE_INTEGER Frequency, StartCounter, EndCounter;

	if (QueryPerformanceFrequency(&Frequency)) {
		__int64 ticks = (Frequency.QuadPart / 1000) * time;

		QueryPerformanceCounter(&StartCounter);

		do {
			QueryPerformanceCounter(&EndCounter);
		} while ((EndCounter.QuadPart - StartCounter.QuadPart) < ticks);
	}
}

static void DelayOverhead(unsigned int time)
{
	LARGE_INTEGER Frequency, StartCounter, EndCounter;

	if (QueryPerformanceFrequency(&Frequency)) {
		__int64 ticks = (Frequency.QuadPart / 1000) * time;

		QueryPerformanceCounter(&StartCounter);

		do {
			QueryPerformanceCounter(&EndCounter);
		} while ((EndCounter.QuadPart - StartCounter.QuadPart) == ticks);
	}
}

static int getVideoMemory()
{
	return (int) (G3D::DXCaps::videoMemorySize() / (1024 * 1024));
}

static std::string getCPUMake()
{
	std::string cpu_name;

	if (G3D::RegistryUtil::readString(
			"HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0\\ProcessorNameString", cpu_name
		)) {
		return cpu_name;
	}

	return "Could not determine CPU name";
}

static int getCPUSpeed()
{
	int speed;

	if (G3D::RegistryUtil::read32bitNumber(
			"HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0\\~MHz", speed
		)) {
		if (speed > 300) {
			return speed;
		}
	}

	__int64 total = 0;
	__int64 overhead = 0;

	for (int i = 0; i < 2; i++) {
		total += GetCyclesDifference(Delay, 50);
		overhead += GetCyclesDifference(DelayOverhead, 50);
	}

	total = (total - overhead) / 2;
	total = total / 50;
	total = total / 1000;

	if (total < 100 || total > 50000) {
		total = 1400;
	}

	return (int) total;
}

static std::string getMaxRes()
{
	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);

	std::string w = std::to_string(width);
	std::string h = std::to_string(height);

	return w + "x" + h;
}

static int getRam()
{
	MEMORYSTATUS memoryStatus;

	GlobalMemoryStatus(&memoryStatus);

	return (int) memoryStatus.dwTotalPhys / (1024 * 1024);
}

#else

static std::string readCPUInfo(const char* key)
{
	std::ifstream cpuinfo("/proc/cpuinfo");
	std::string line;

	while (std::getline(cpuinfo, line)) {
		if (line.compare(0, std::strlen(key), key) == 0) {
			std::size_t colon = line.find(':');

			if (colon != std::string::npos) {
				std::size_t value = line.find_first_not_of(' ', colon + 1);
				return value == std::string::npos ? std::string() : line.substr(value);
			}
		}
	}

	return std::string();
}

static int getVideoMemory()
{
	return 0;
}

static std::string getCPUMake()
{
	std::string cpu_name = readCPUInfo("model name");

	if (!cpu_name.empty()) {
		return cpu_name;
	}

	return "Could not determine CPU name";
}

static int getCPUSpeed()
{
	int speed = std::atoi(readCPUInfo("cpu MHz").c_str());

	if (speed > 300) {
		return speed;
	}

	return 1400;
}

static std::string getMaxRes()
{
	return "0x0";
}

static int getRam()
{
	struct sysinfo info;

	if (sysinfo(&info) != 0) {
		return 0;
	}

	return (int) ((unsigned long long) info.totalram * info.mem_unit / (1024 * 1024));
}

#endif

SpecData::SpecData()
	: shaderModel(glLoaded ? getShaderModel() : 0.0f), videoMemory(getVideoMemory()), CPUSpeed(getCPUSpeed()),
	  glVendor(glLoaded ? G3D::GLCaps::vendor() : "?"), gfxcard(glLoaded ? G3D::GLCaps::renderer() : "?"),
	  cpu(getCPUMake()), ram(getRam()), resolution(getMaxRes())
{
}

} // namespace RBX::Render
