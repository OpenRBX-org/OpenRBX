#ifndef APPDRAW_QUADRIC_H
#define APPDRAW_QUADRIC_H

#include <OgreColourValue.h>
#include <OgreManualObject.h>
#include <OgreSceneManager.h>
#include <OgreString.h>
#include <OgreVector3.h>

namespace Ogre {

class Quadric
{
public:
	Quadric();

	void setOffset(const Vector3& offset);
	ManualObject* cylinder(
		SceneManager* sceneManager,
		const String& name,
		ManualObject* manual,
		float baseRadius,
		float topRadius,
		float height,
		int slices,
		int stacks
	);
	ManualObject* partialDisk(
		SceneManager* sceneManager,
		const String& name,
		ManualObject* manual,
		float innerRadius,
		float outerRadius,
		int slices,
		int loops,
		float startAngle,
		float sweepAngle
	);
	ManualObject* sphere(
		SceneManager* sceneManager, const String& name, ManualObject* manual, float radius, int slices, int stacks
	);
	ManualObject* disk(
		SceneManager* sceneManager,
		const String& name,
		ManualObject* manual,
		float innerRadius,
		float outerRadius,
		int slices,
		int loops
	);

	int normals;
	bool textureCoords;
	int orientation;
	int drawStyle;
	ColourValue m_colour;
	Vector3 m_offset;
};

} // namespace Ogre

#endif // APPDRAW_QUADRIC_H
