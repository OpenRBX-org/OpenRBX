#include "util/FrameInterpolator.h"

#include "util/Timer.h"

#include <G3D/g3dmath.h>

namespace RBX {

static const float teleportDistance = 50.0f;

int FrameInterpolator::currentStep = 0;
int FrameInterpolator::currentFrame = 0;
float FrameInterpolator::alpha = 1.0f;
double FrameInterpolator::nextStepTime = 0.0;
std::atomic<std::thread::id> FrameInterpolator::renderThread;
bool FrameInterpolator::extraFrame = false;

FrameInterpolator::FrameInterpolator() : step(0), frame(0), sampled(false)
{
}

bool FrameInterpolator::isRenderThread()
{
	return renderThread == std::this_thread::get_id();
}

void FrameInterpolator::beginRender(int step, double stepTime, double period)
{
	currentStep = step;
	currentFrame++;
	nextStepTime = stepTime + period;
	alpha = period > 0.0 ? (float) G3D::clamp((getRealTime() - stepTime) / period, 0.0, 1.0) : 1.0f;
	renderThread = std::this_thread::get_id();
}

void FrameInterpolator::endRender()
{
	renderThread = std::thread::id();
}

const G3D::CoordinateFrame& FrameInterpolator::sample(const G3D::CoordinateFrame& current)
{
	if (!sampled) {
		previous = latest = current;
		step = currentStep;
		sampled = true;
	}
	else if (step != currentStep) {
		previous = step + 1 == currentStep ? latest : current;
		latest = current;
		step = currentStep;
	}
	else if (latest != current) {
		previous = latest = current;
	}
	else if (frame == currentFrame) {
		return blended;
	}

	if ((latest.translation - previous.translation).squaredLength() > teleportDistance * teleportDistance) {
		previous = latest;
	}

	blended = previous == latest ? latest : previous.lerp(latest, alpha);
	frame = currentFrame;
	return blended;
}

} // namespace RBX
