#ifndef APPDRAW_RBXFONT_H
#define APPDRAW_RBXFONT_H

#include "rbxgraphics/Adorn.h"

#include <G3D/BinaryInput.h>
#include <G3D/Color4.h>
#include <OgreTexture.h>
#include <OgreVector2.h>
#include <string>

namespace Ogre {

class VisualEngine;

class RbxFont
{
public:
	RbxFont(const std::string& filename, G3D::BinaryInput& in);

	Vector2 computePackedArray(
		const std::string& s,
		double x,
		double y,
		double width,
		double height,
		RBX::Adorn::Spacing spacing,
		Vector2* quads
	);
	Vector2 get2DStringBounds(const std::string& s, double size, RBX::Adorn::Spacing spacing);
	Vector2 draw2D(
		VisualEngine* engine,
		const std::string& s,
		const Vector2& position,
		double size,
		const G3D::Color4& color,
		const G3D::Color4& outline,
		RBX::Adorn::XAlign xAlign,
		RBX::Adorn::YAlign yAlign,
		RBX::Adorn::Spacing spacing
	);

	static SharedPtr<RbxFont> fromFile(const std::string& filename);

private:
	int m_subWidth[128];
	int m_charWidth;
	int m_charHeight;
	int m_baseline;
	TexturePtr m_texture;
};

extern int fontTextureCount;

} // namespace Ogre

#endif // APPDRAW_RBXFONT_H
