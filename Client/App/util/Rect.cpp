#include "util/Rect.h"

#include <algorithm>

namespace RBX {

Vector2 Rect::positionPoint(Location xLocation, Location yLocation) const
{
	Vector2 point = center();

	switch (xLocation) {
	case LEFT:
		point.x = low.x;
		break;
	case RIGHT:
		point.x = high.x;
		break;
	}

	switch (yLocation) {
	case TOP:
		point.y = low.y;
		break;
	case BOTTOM:
		point.y = high.y;
		break;
	case CENTER:
		return point;
	}

	return point;
}

Vector2 Rect::positionPoint(const Vector2& offset, Location xLocation, Location yLocation) const
{
	Vector2 point(0, 0);

	switch (xLocation) {
	case LEFT:
		point.x = low.x + offset.x;
		break;
	case RIGHT:
		point.x = high.x - offset.x;
		break;
	}

	switch (yLocation) {
	case TOP:
		point.y = low.y + offset.y;
		break;
	case BOTTOM:
		point.y = high.y - offset.y;
		break;
	}

	return point;
}

Rect Rect::positionChild(const Rect& child, Location xLocation, Location yLocation) const
{
	float x = 0.0f;
	float y = 0.0f;

	float width = child.high.x - child.low.x;
	float height = child.high.y - child.low.y;

	switch (xLocation) {
	case LEFT:
		x = low.x;
		break;
	case RIGHT:
		x = high.x - width;
		break;
	case CENTER:
		x = (high.x - width) * 0.5f;
		break;
	}

	switch (yLocation) {
	case TOP:
		y = low.y;
		break;
	case BOTTOM:
		y = high.y - height;
		break;
	case CENTER:
		y = (high.y - height) * 0.5f;
		break;
	}

	return fromLowSize(Vector2(x, y), Vector2(width, height));
}

void Rect::unionWith(const Rect& other)
{
	low.x = std::min(low.x, other.low.x);
	low.y = std::min(low.y, other.low.y);
	high.x = std::max(high.x, other.high.x);
	high.y = std::max(high.y, other.high.y);
}

} // namespace RBX
