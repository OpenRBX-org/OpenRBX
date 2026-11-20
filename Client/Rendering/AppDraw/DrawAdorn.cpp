#include "appdraw/DrawAdorn.h"

#include "appdraw/HitTest.h"
#include "appdraw/Part.h"
#include "util/Math.h"

namespace RBX {

void DrawAdorn::cylinder(
	Adorn* adorn, const G3D::CoordinateFrame& worldC, int axis, float length, float radius, const G3D::Color3& color
)
{
	const G3D::Matrix3& axisRotation = Math::getAxisRotationMatrix(axis);
	G3D::CoordinateFrame rotatedWorldC(worldC.rotation * axisRotation, worldC.translation);

	adorn->setObjectToWorldMatrix(rotatedWorldC);

	adorn->cylinderAlongX(radius, length, color, G3D::Color4::clear());
}

void DrawAdorn::surfaceBorder(
	Adorn* adorn, const G3D::Vector3& halfRealSize, float highlight, int surfaceId, const G3D::Color4& color
)
{
	int axis = surfaceId % 3;
	int side = surfaceId < 3 ? 1 : -1;

	float cZ = side * halfRealSize[axis];

	int c2 = (axis + 1) % 3;

	G3D::Vector3 p0;
	G3D::Vector3 p1;

	p0[axis] = cZ - highlight;
	p1[axis] = cZ + highlight;

	int c3 = (axis + 2) % 3;

	for (int direction = 0; direction < 2; ++direction) {
		int cY = direction != 0 ? c2 : c3;
		int cX = direction != 0 ? c3 : c2;

		for (int polarity = -1; polarity <= 1; polarity += 2) {
			p0[cY] = polarity * halfRealSize[cY] - highlight;
			p1[cY] = polarity * halfRealSize[cY] + highlight;
			p0[cX] = -halfRealSize[cX] - highlight;
			p1[cX] = halfRealSize[cX] + highlight;

			adorn->box(G3D::AABox(p0, p1), color, G3D::Color4::clear());
		}
	}
}

void DrawAdorn::partSurface(const Part& part, int surfaceId, Adorn* adorn, const G3D::Color4& color)
{
	adorn->setObjectToWorldMatrix(part.coordinateFrame);

	G3D::Vector3 halfRealSize = part.gridSize * 0.5f;

	surfaceBorder(adorn, halfRealSize, 0.2f, surfaceId, color);
}

void DrawAdorn::handles3d(
	const G3D::Vector3& size,
	const G3D::CoordinateFrame& location,
	Adorn* adorn,
	HandleType handleType,
	const G3D::Vector3& cameraPosition
)
{
	G3D::Vector3 halfSize = size * 0.5;

	for (int posNeg = 0; posNeg < 2; posNeg++) {
		float polarity = posNeg < 1.0f ? -1.0f : 1.0f;

		for (int axis = 0; axis < 3; axis++) {
			switch (handleType) {
			case HANDLE_RESIZE:
			case HANDLE_VELOCITY: {
				G3D::Vector3 handleInPart;
				handleInPart[axis] = (halfSize[axis] + HitTest::handleOffset()) * polarity;
				G3D::Vector3 handleInWorld = location.pointToWorldSpace(handleInPart);
				float radius = HitTest::distanceToHandleRadius(cameraPosition, handleInWorld);
				adorn->setObjectToWorldMatrix(G3D::CoordinateFrame(handleInWorld));
				G3D::Color4 color(resizeColor(), 1.0f);
				adorn->sphere(G3D::Sphere(G3D::Vector3::zero(), radius), color, G3D::Color4::clear());
				break;
			}
			case HANDLE_MOVE: {
				G3D::Ray ray;
				ray.origin[axis] = polarity * halfSize[axis];
				ray.direction[axis] = polarity * 3.5f;
				G3D::Vector3 originInWorld = location.pointToWorldSpace(ray.origin);
				float scale = HitTest::distanceToHandleRadius(cameraPosition, originInWorld) * 4.0f;
				adorn->setObjectToWorldMatrix(location);
				adorn->ray(ray, G3D::Color4(G3D::Color3::orange(), 1.0f), scale);
				break;
			}
			default: {
				G3D::Vector3 handleInPart;
				handleInPart[axis] = (halfSize[axis] + 4.0f) * polarity;
				G3D::Vector3 handleInWorld = location.translation + handleInPart;
				adorn->setObjectToWorldMatrix(G3D::CoordinateFrame(handleInWorld));
				cylinder(adorn, location, 0, 1.0f, 0.5f, G3D::Color3::red());
				break;
			}
			}
		}
	}
}

void DrawAdorn::handles2d(
	const G3D::Vector3& size,
	const G3D::CoordinateFrame& location,
	const G3D::GCamera& camera,
	Adorn* adorn,
	const G3D::Color3& handleColor
)
{
	G3D::Vector3 halfSize = size * 0.5;

	for (int posNeg = 0; posNeg < 2; posNeg++) {
		float polarity = posNeg < 1.0f ? -1.0f : 1.0f;

		for (int axis = 0; axis < 3; axis++) {
			G3D::Vector3 handleInPart;
			handleInPart[axis] = (halfSize[axis] + HitTest::handleOffset()) * polarity;
			G3D::Vector3 handleInWorld = location.pointToWorldSpace(handleInPart);
			G3D::Vector2 screen = camera.project(handleInWorld, adorn->getViewport()).xy();
			G3D::Color4 color(handleColor, 1.0f);
			G3D::Rect2D rect = G3D::Rect2D::xyxy(screen - G3D::Vector2(3.0f, 3.0f), screen + G3D::Vector2(3.0f, 3.0f));
			adorn->rect2d(rect, color);
		}
	}
}

} // namespace RBX
