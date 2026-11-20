#include "appdraw/RbxFont.h"

#include "appdraw/VisualEngine.h"

#include <G3D/fileutils.h>
#include <G3D/g3dmath.h>
#include <OgreDataStream.h>
#include <OgreTextureManager.h>
#include <cstdio>
#include <cstdlib>

namespace Ogre {

Vector2 RbxFont::computePackedArray(
	const std::string& s, double x, double y, double width, double height, RBX::Adorn::Spacing spacing, Vector2* quads
)
{
	unsigned int length = s.size();
	double scaleX = width / m_charWidth;
	float scaleY = height / m_charHeight;
	float advance = m_subWidth['M'] * 0.85f * scaleX;
	for (unsigned int i = 0; i < length; i++) {
		char c = s[i] & 0x7f;
		if (c != ' ') {
			int row = c >> 4;
			int col = c & 0xf;
			float offset =
				spacing == RBX::Adorn::PROPORTIONAL_SPACING ? (m_charWidth - m_subWidth[c]) * scaleX * 0.5 : 0.0f;
			float left = x - offset;
			quads->x = m_charWidth * col;
			quads->y = m_charHeight * row + 1;
			quads++;
			quads->x = left;
			quads->y = scaleY + y;
			quads++;
			quads->x = m_charWidth * col;
			quads->y = m_charHeight * (row + 1) - 2;
			quads++;
			quads->x = left;
			quads->y = y + height - scaleY;
			quads++;
			float right = left + width;
			quads->x = m_charWidth * (col + 1) - 1;
			quads->y = m_charHeight * (row + 1) - 2;
			quads++;
			quads->x = right;
			quads->y = y + height - scaleY;
			quads++;
			quads->x = m_charWidth * (col + 1) - 1;
			quads->y = m_charHeight * row + 1;
			quads++;
			quads->x = right;
			quads->y = scaleY + y;
			quads++;
		}
		x += spacing == RBX::Adorn::PROPORTIONAL_SPACING ? m_subWidth[c] * scaleX : advance;
	}
	return Vector2(x - 0.0, height);
}

Vector2 RbxFont::get2DStringBounds(const std::string& s, double size, RBX::Adorn::Spacing spacing)
{
	double height = size * 1.5;
	double scale = m_charWidth * height / m_charHeight / m_charWidth;
	double width = 0.0;
	int length = s.size();
	if (spacing == RBX::Adorn::PROPORTIONAL_SPACING) {
		for (int i = 0; i < length; i++) {
			width += m_subWidth[s[i] & 0x7f] * scale;
		}
	}
	else {
		width = m_subWidth['M'] * length * 0.85 * scale;
	}
	return Vector2(width, height);
}

Vector2 RbxFont::draw2D(
	VisualEngine* engine,
	const std::string& s,
	const Vector2& position,
	double size,
	const G3D::Color4& color,
	const G3D::Color4& outline,
	RBX::Adorn::XAlign xAlign,
	RBX::Adorn::YAlign yAlign,
	RBX::Adorn::Spacing spacing
)
{
	float x = position.x;
	float y = position.y;
	float height = size * 1.5;
	float width = m_charWidth * height / m_charHeight;
	switch (xAlign) {
	case RBX::Adorn::XALIGN_RIGHT:
		x -= get2DStringBounds(s, size, spacing).x;
		break;
	case RBX::Adorn::XALIGN_CENTER:
		x -= get2DStringBounds(s, size, spacing).x * 0.5;
		break;
	}
	switch (yAlign) {
	case RBX::Adorn::YALIGN_TOP:
		y -= height;
		break;
	case RBX::Adorn::YALIGN_BASELINE:
		y -= m_baseline * height / m_charHeight;
		break;
	case RBX::Adorn::YALIGN_CENTER:
		y -= height * 0.5;
		break;
	}
	int count = 0;
	for (unsigned int i = 0; i < s.size(); i++) {
		count += s[i] % 128 != ' ';
	}
	if (count == 0) {
		return Vector2(0.0f, height);
	}
	Vector2* quads = (Vector2*) malloc(count * 64);
	Vector2 extent;
	extent = computePackedArray(s, x, y, width, height, spacing, quads);
	TwoDManager* twoDManager = engine->m_twoDManager;
	int texWidth = m_texture->getWidth();
	int texHeight = m_texture->getHeight();
	if (0.05f < outline.a) {
		for (int dy = -1; dy <= 1; dy += 2) {
			for (int dx = -1; dx <= 1; dx += 2) {
				if (dx != 0 || dy != 0) {
					Vector2* quad = quads;
					for (int i = 0; i < count; i++) {
						twoDManager->spriteBltFull(
							m_texture->getName(),
							outline,
							quad[1].x + dx,
							quad[1].y + dy,
							quad[5].x + dx,
							quad[5].y + dy,
							quad[0].x / texWidth,
							quad[4].y / texHeight,
							quad[4].x / texWidth,
							quad[0].y / texHeight
						);
						quad += 8;
					}
				}
			}
		}
	}
	Vector2* quad = quads;
	for (int i = 0; i < count; i++) {
		twoDManager->spriteBltFull(
			m_texture->getName(),
			color,
			quad[1].x,
			quad[1].y,
			quad[5].x,
			quad[5].y,
			quad[0].x / texWidth,
			quad[4].y / texHeight,
			quad[4].x / texWidth,
			quad[0].y / texHeight
		);
		quad += 8;
	}
	free(quads);
	return extent;
}

int fontTextureCount;

RbxFont::RbxFont(const std::string& filename, G3D::BinaryInput& in)
{
	in.readUInt32();
	for (int i = 0; i < 128; i++) {
		m_subWidth[i] = in.readUInt16();
	}
	m_baseline = in.readUInt16();
	int texWidth = in.readUInt16();
	m_charWidth = texWidth / 16;
	m_charHeight = texWidth / 16;
	int width = G3D::ceilPow2(m_charWidth * 16);
	int height = G3D::ceilPow2(m_charHeight * 8);
	const uint8* data = in.getCArray() + in.getPosition();
	char name[8];
	snprintf(name, 8, "ft%d", fontTextureCount++);
	m_texture = TextureManager::getSingleton().create(name, "General");
	DataStreamPtr stream(new MemoryDataStream((void*) data, width * height));
	m_texture->loadRawData(stream, width, height, PF_A8);
}

SharedPtr<RbxFont> RbxFont::fromFile(const std::string& filename)
{
	if (!G3D::fileExists(filename)) {
		return SharedPtr<RbxFont>();
	}
	G3D::BinaryInput in(filename, G3D::G3D_LITTLE_ENDIAN, true);
	return SharedPtr<RbxFont>(new RbxFont(filename, in));
}

} // namespace Ogre
