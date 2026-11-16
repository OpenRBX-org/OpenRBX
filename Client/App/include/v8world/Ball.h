#ifndef V8WORLD_BALL_H
#define V8WORLD_BALL_H

#include "v8world/Geometry.h"

namespace RBX {

class Ball : public Geometry
{
private:
	Matrix3 getMomentSolid(float mass) const;
	G3D::Matrix3 getMomentHollow(float) const;

	void onSetSize() override;

public:
	bool hitTest(const Ray& ray, Vector3& hitPoint, bool& inside) override;

	GeometryType getGeometryType() const override { return GEOMETRY_BALL; }

	float getRadius() const override { return realRadius; }

	Vector3 getCenterToCorner(const Matrix3& rotation) const override
	{
		return Vector3(realRadius, realRadius, realRadius);
	}

	Matrix3 getMoment(float mass) const override { return getMomentSolid(mass); }

	float getGridVolume() const override;

	Ball() : realRadius(0) {}
	~Ball() override = default;

private:
	float realRadius;
};

} // namespace RBX

#endif // V8WORLD_BALL_H
