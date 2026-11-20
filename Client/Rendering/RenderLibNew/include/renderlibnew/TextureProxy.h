#ifndef RENDERLIBNEW_TEXTUREPROXY_H
#define RENDERLIBNEW_TEXTUREPROXY_H

#include "rbxgraphics/TextureProxyBase.h"

#include <G3D/Color3.h>
#include <G3D/Color3uint8.h>
#include <GLG3D/Texture.h>
#include <GLG3D/TextureManager.h>
#include <OgreTexture.h>

namespace G3D {
class RenderDevice;
}

namespace RBX::RenderNew {

class RenderScene;
class TextureProxy;

using TextureProxyRef = G3D::ReferenceCountedPointer<TextureProxy>;

class TextureProxy : public TextureProxyBase
{
protected:
	G3D::ReferenceCountedPointer<G3D::Texture> texture;
	bool loaded;
	G3D::TextureManager* textureManager;
	bool tiled;
	TextureProxyRef root;
	TextureProxyRef shadeTexture;
	G3D::Color3 shadeColor;
	Ogre::TexturePtr m_texture;

	friend class RenderScene;

	TextureProxy(TextureProxyRef root) : loaded(false), textureManager(nullptr), root(root) { filename = ""; }

public:
	TextureProxy(const std::string& filename) : loaded(false) { this->filename = filename; }

	TextureProxy(G3D::TextureManager& textureManager, const std::string& filename, bool tiled)
		: loaded(false), textureManager(&textureManager), tiled(tiled)
	{
		this->filename = filename;
	}

	~TextureProxy() override;

	TextureProxyRef shade(const G3D::Color3& baseColor, const TextureProxyRef& baseTex);

	TextureProxyRef alphaOnlyVersion();

	virtual G3D::ReferenceCountedPointer<G3D::Texture> getIfResolved() const;
	virtual G3D::ReferenceCountedPointer<G3D::Texture> resolve(G3D::RenderDevice* rd);

	static void shadePixel(const G3D::Color3uint8& base, G3D::Color3uint8& lighting);

	G3D::Vector2 getSize() override;
};

} // namespace RBX::RenderNew

#endif // RENDERLIBNEW_TEXTUREPROXY_H
