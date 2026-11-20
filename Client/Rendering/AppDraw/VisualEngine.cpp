#include "appdraw/VisualEngine.h"

#include "appdraw/RbxSceneManager.h"

#include <G3D/Color4.h>
#include <G3D/Vector3.h>
#include <OgreCamera.h>
#include <OgreException.h>
#include <OgreHardwareBufferManager.h>
#include <OgreLogManager.h>
#include <OgreMeshManager.h>
#include <OgreRenderSystem.h>
#include <OgreRenderTarget.h>
#include <OgreRenderWindow.h>
#include <OgreResourceGroupManager.h>
#include <OgreSceneManager.h>
#include <OgreStringConverter.h>
#include <OgreTextureManager.h>
#include <OgreVertexIndexData.h>
#include <OgreViewport.h>
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <d3d9.h>
#include <ddraw.h>
#else
#include <GLG3D/DXCaps.h>
#endif

namespace Ogre {

bool frameRateManagerEnabled;

Root* ogreRoot;

unsigned int videoMemorySize;

bool videoMemorySizeKnown;

int sceneManagerCount;

int renderWindowCount;

RbxSubMesh::RbxSubMesh() : m_edgeListBuilt(false), m_edgeList(nullptr)
{
}

Vector3 toOgreVector3(const G3D::Vector3& v)
{
	return Vector3(v.x, v.y, v.z);
}

ColourValue toOgreColour(const G3D::Color4& color)
{
	return ColourValue(color.r, color.g, color.b, color.a);
}

unsigned int getVideoMemorySize()
{
	if (!videoMemorySizeKnown) {
#ifdef _WIN32
		CoInitialize(nullptr);
		IDirectDraw2* directDraw = nullptr;
		DWORD freeMemory = 0;
		if (SUCCEEDED(CoCreateInstance(
				CLSID_DirectDraw, nullptr, CLSCTX_INPROC_SERVER, IID_IDirectDraw2, (void**) &directDraw
			))) {
			directDraw->Initialize(nullptr);
			DDSCAPS caps;
			caps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
			DWORD totalMemory = 0;
			if (FAILED(directDraw->GetAvailableVidMem(&caps, &totalMemory, &freeMemory))) {
				totalMemory = 0;
				freeMemory = 0;
			}
			videoMemorySize = totalMemory;
			directDraw->Release();
		}
		CoUninitialize();
#else
		videoMemorySize = (unsigned int) G3D::DXCaps::videoMemorySize();
#endif
		videoMemorySizeKnown = true;
	}
	return videoMemorySize;
}

SubMesh* VisualEngine::createSubMesh(
	MeshPtr& mesh, const unsigned short* indices16, const unsigned int* indices32, size_t triangleCount
)
{
	SubMesh* subMesh = mesh->createSubMesh();
	size_t indexCount = triangleCount * 3;
	HardwareIndexBufferSharedPtr indexBuffer = HardwareBufferManager::getSingleton().createIndexBuffer(
		indices16 ? HardwareIndexBuffer::IT_16BIT : HardwareIndexBuffer::IT_32BIT,
		indexCount,
		HardwareBuffer::HBU_STATIC_WRITE_ONLY
	);
	indexBuffer->writeData(
		0, indexBuffer->getSizeInBytes(), indices16 ? (const void*) indices16 : (const void*) indices32, true
	);
	subMesh->useSharedVertices = true;
	subMesh->indexData->indexBuffer = indexBuffer;
	subMesh->indexData->indexCount = indexCount;
	subMesh->indexData->indexStart = 0;
	return subMesh;
}

VisualEngine::FrameRateManager::FrameRateManager() : m_frameCount(0)
{
}

int VisualEngine::FrameRateManager::getFrameCount() const
{
	return m_frameCount;
}

void VisualEngine::FrameRateManager::addFrame(const FrameStats& entry)
{
	m_frames[m_frameCount % 100] = entry;
	m_frameCount++;
}

VisualEngine::FrameRateManager::FrameStats& VisualEngine::FrameRateManager::getFrame(int back)
{
	int index = m_frameCount - back - 1;
	return m_frames[index % 100];
}

VisualEngine::VisualEngine()
	: m_sceneManager(nullptr), m_camera(nullptr), m_renderWindow(nullptr), m_viewport(nullptr), m_twoDManager(nullptr),
	  m_largeVertexBufferOverride(-1)
{
}

VisualEngine::~VisualEngine()
{
	if (ogreRoot) {
		if (m_sceneManager) {
			ogreRoot->destroySceneManager(m_sceneManager);
		}
		if (m_renderWindow) {
			ogreRoot->detachRenderTarget(m_renderWindow);
		}
		delete m_twoDManager;
	}
}

void destroyRoot()
{
	if (ogreRoot) {
		delete ogreRoot;
		ogreRoot = nullptr;
	}
}

float VisualEngine::FrameRateManager::getMedianFrameTime(int count)
{
	if (count <= 99 && m_frameCount >= count) {
		std::vector<float> values(count, 0.0f);
		values.clear();
		for (int i = 1; i <= count; i++) {
			values.push_back(m_frames[(m_frameCount - i) % 100].m_frameTime);
		}
		std::sort(values.begin(), values.end());
		return values[count / 2];
	}
	return -1.0f;
}

VertexData* VisualEngine::createVertexData(
	const float* positions,
	size_t vertexCount,
	const float* normals,
	const ColourValue* colours,
	const float* textureCoords
)
{
	RenderSystem* renderSystem = Root::getSingleton().getRenderSystem();
	VertexData* vertexData = new VertexData();
	vertexData->vertexCount = vertexCount;
	VertexDeclaration* declaration = vertexData->vertexDeclaration;

	declaration->addElement(0, 0, VET_FLOAT3, VES_POSITION);
	size_t vertexSize = VertexElement::getTypeSize(VET_FLOAT3);
	HardwareVertexBufferSharedPtr buffer = HardwareBufferManager::getSingleton().createVertexBuffer(
		vertexSize, vertexData->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY
	);
	buffer->writeData(0, buffer->getSizeInBytes(), positions, true);
	VertexBufferBinding* binding = vertexData->vertexBufferBinding;
	binding->setBinding(0, buffer);

	unsigned short source = 1;
	if (colours) {
		std::vector<RGBA> converted;
		converted.resize(vertexCount);
		for (size_t i = 0; i < vertexCount; i++, colours++) {
			ColourValue colour(colours->r, colours->r, colours->r, colours->r);
			renderSystem->convertColourValue(colour, &converted[i]);
		}

		declaration->addElement(1, 0, VET_COLOUR, VES_DIFFUSE);
		vertexSize = VertexElement::getTypeSize(VET_COLOUR);
		buffer = HardwareBufferManager::getSingleton().createVertexBuffer(
			vertexSize, vertexData->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY
		);
		buffer->writeData(0, buffer->getSizeInBytes(), &converted[0], true);
		binding->setBinding(1, buffer);
		source = 2;
	}

	if (normals) {
		declaration->addElement(source, 0, VET_FLOAT3, VES_NORMAL);
		vertexSize = VertexElement::getTypeSize(VET_FLOAT3);
		buffer = HardwareBufferManager::getSingleton().createVertexBuffer(
			vertexSize, vertexData->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY
		);
		buffer->writeData(0, buffer->getSizeInBytes(), normals, true);
		binding->setBinding(source, buffer);
		source++;
	}

	if (textureCoords) {
		declaration->addElement(source, 0, VET_FLOAT2, VES_TEXTURE_COORDINATES);
		vertexSize = VertexElement::getTypeSize(VET_FLOAT2);
		buffer = HardwareBufferManager::getSingleton().createVertexBuffer(
			vertexSize, vertexData->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY
		);
		buffer->writeData(0, buffer->getSizeInBytes(), textureCoords, true);
		binding->setBinding(source, buffer);
	}

	return vertexData;
}

void VisualEngine::setMeshVertexData(
	MeshPtr& mesh,
	const float* positions,
	size_t vertexCount,
	const float* normals,
	const ColourValue* colours,
	const float* textureCoords,
	const Vector3* minimum,
	const Vector3* maximum
)
{
	if (mesh->sharedVertexData) {
		delete mesh->sharedVertexData;
		mesh->sharedVertexData = nullptr;
	}
	mesh->sharedVertexData = createVertexData(positions, vertexCount, normals, colours, textureCoords);

	if (minimum && maximum) {
		mesh->_setBounds(AxisAlignedBox(minimum->x, minimum->y, minimum->z, maximum->x, maximum->y, maximum->z));
		mesh->_setBoundingSphereRadius((*maximum - *minimum).length() * 0.5);
	}
	else {
		Vector3 lo(1e9f, 1e9f, 1e9f);
		Vector3 hi(-1e9f, -1e9f, -1e9f);
		for (size_t i = 0; i < vertexCount * 3; i += 3) {
			if (lo.x > positions[i]) {
				lo.x = positions[i];
			}
			if (lo.y > positions[i + 1]) {
				lo.y = positions[i + 1];
			}
			if (lo.z > positions[i + 2]) {
				lo.z = positions[i + 2];
			}
			if (hi.x < positions[i]) {
				hi.x = positions[i];
			}
			if (hi.y < positions[i + 1]) {
				hi.y = positions[i + 1];
			}
			if (hi.z < positions[i + 2]) {
				hi.z = positions[i + 2];
			}
		}

		mesh->_setBounds(AxisAlignedBox(lo, hi));
		mesh->_setBoundingSphereRadius((hi - lo).length() * 0.5);
	}
}

MeshPtr VisualEngine::createMesh(
	const char* name,
	const char* group,
	const float* positions,
	size_t vertexCount,
	const float* normals,
	const ColourValue* colours,
	const float* textureCoords,
	const Vector3* minimum,
	const Vector3* maximum
)
{
	MeshPtr mesh = MeshManager::getSingleton().createManual(name, group);
	setMeshVertexData(mesh, positions, vertexCount, normals, colours, textureCoords, minimum, maximum);

	if (minimum && maximum) {
		mesh->_setBounds(AxisAlignedBox(minimum->x, minimum->y, minimum->z, maximum->x, maximum->y, maximum->z));
		mesh->_setBoundingSphereRadius((*maximum - *minimum).length() * 0.5);
	}
	else {
		Vector3 lo(1e9f, 1e9f, 1e9f);
		Vector3 hi(-1e9f, -1e9f, -1e9f);
		for (size_t i = 0; i < vertexCount * 3; i += 3) {
			if (lo.x > positions[i]) {
				lo.x = positions[i];
			}
			if (lo.y > positions[i + 1]) {
				lo.y = positions[i + 1];
			}
			if (lo.z > positions[i + 2]) {
				lo.z = positions[i + 2];
			}
			if (hi.x < positions[i]) {
				hi.x = positions[i];
			}
			if (hi.y < positions[i + 1]) {
				hi.y = positions[i + 1];
			}
			if (hi.z < positions[i + 2]) {
				hi.z = positions[i + 2];
			}
		}

		mesh->_setBounds(AxisAlignedBox(lo, hi));
		mesh->_setBoundingSphereRadius((hi - lo).length() * 0.5);
	}

	mesh->load();
	return mesh;
}

RbxSubMesh* VisualEngine::createRbxSubMesh(
	const float* positions,
	size_t vertexCount,
	const float* normals,
	const ColourValue* colours,
	const float* textureCoords,
	const unsigned short* indices16,
	const unsigned int* indices32,
	size_t triangleCount,
	Vector3* minimumOut,
	Vector3* maximumOut
)
{
	RbxSubMesh* subMesh = new RbxSubMesh();
	subMesh->parent = nullptr;
	subMesh->vertexData = createVertexData(positions, vertexCount, normals, colours, textureCoords);
	subMesh->useSharedVertices = false;

	size_t indexCount = triangleCount * 3;
	HardwareIndexBufferSharedPtr indexBuffer = HardwareBufferManager::getSingleton().createIndexBuffer(
		indices16 ? HardwareIndexBuffer::IT_16BIT : HardwareIndexBuffer::IT_32BIT,
		indexCount,
		HardwareBuffer::HBU_STATIC_WRITE_ONLY
	);
	indexBuffer->writeData(
		0, indexBuffer->getSizeInBytes(), indices16 ? (const void*) indices16 : (const void*) indices32, true
	);
	subMesh->indexData->indexBuffer = indexBuffer;
	subMesh->indexData->indexCount = indexCount;
	subMesh->indexData->indexStart = 0;

	Vector3 minimum(1e9f, 1e9f, 1e9f);
	Vector3 maximum(-1e9f, -1e9f, -1e9f);
	for (size_t i = 0; i < vertexCount * 3; i += 3) {
		if (minimum.x > positions[i]) {
			minimum.x = positions[i];
		}
		if (minimum.y > positions[i + 1]) {
			minimum.y = positions[i + 1];
		}
		if (minimum.z > positions[i + 2]) {
			minimum.z = positions[i + 2];
		}
		if (maximum.x < positions[i]) {
			maximum.x = positions[i];
		}
		if (maximum.y < positions[i + 1]) {
			maximum.y = positions[i + 1];
		}
		if (maximum.z < positions[i + 2]) {
			maximum.z = positions[i + 2];
		}
	}

	*minimumOut = minimum;
	*maximumOut = maximum;

	AxisAlignedBox box(minimum, maximum);
	subMesh->m_bounds = box;
	subMesh->m_boundingRadius = box.getHalfSize().length();
	return subMesh;
}

void VisualEngine::initialise(
	int renderSystem, const String& window, int width, int height, int antialiasing, std::string logFileName
)
{
	try {
		if (ogreRoot == nullptr) {
			ogreRoot = new Root("", "", logFileName);

			static RbxSceneManagerFactory factory;
			ogreRoot->addSceneManagerFactory(&factory);

			ResourceGroupManager::getSingleton().initialiseResourceGroup("General");
			ResourceGroupManager::getSingleton().addResourceLocation(".", "FileSystem", "General", false);

			if (renderSystem == 0) {
				try {
					ogreRoot->loadPlugin("rgdx");
				}
				catch (Exception& e) {
					LogManager::getSingleton().logMessage("Error loading DX9 render system.\n");
					destroyRoot();
					throw std::runtime_error(
						std::string("Cannot initialize graphics; installation error likely. Error: ") + e.what()
					);
				}
			}
			else if (renderSystem == 1) {
				try {
					ogreRoot->loadPlugin("rggl");
				}
				catch (Exception&) {
					LogManager::getSingleton().logMessage("Error loading OGL render system.\n");
					destroyRoot();
					throw std::runtime_error("Cannot initialize graphics; installation error likely.");
				}
			}

			try {
				ogreRoot->loadPlugin("rgpar");
			}
			catch (Exception&) {
				LogManager::getSingleton().logMessage("Error loading particle system.\n");
				destroyRoot();
				throw std::runtime_error("Cannot initialize gfx component; installation error likely.");
			}

			RenderSystemList* renderers = ogreRoot->getAvailableRenderers();
			ogreRoot->setRenderSystem(renderers->front());

			ConfigOptionMap& options = Root::getSingleton().getRenderSystem()->getConfigOptions();
			ConfigOptionMap::iterator floatingPoint = options.find("Floating-point mode");
			if (floatingPoint != options.end() && floatingPoint->second.currentValue != "Consistent") {
				floatingPoint->second.currentValue = "Consistent";
				LogManager::getSingleton().logMessage("NOTE forcing Floating-point mode to be Consistent");
			}

			ogreRoot->initialise(false, "OGRE Render Window");
		}

		m_videoMemorySize = getVideoMemorySize();

		ConfigOptionMap& options = Root::getSingleton().getRenderSystem()->getConfigOptions();

		char name[8];
		snprintf(name, sizeof(name), "win%d", renderWindowCount++);

		NameValuePairList params;
#ifdef _WIN32
		params["externalWindowHandle"] = window;
#else
		params["parentWindowHandle"] = window;
#endif

		bool has32Bit = false;
		ConfigOptionMap::iterator videoMode = options.find("Video Mode");
		if (videoMode != options.end()) {
			StringVector& values = videoMode->second.possibleValues;
			for (int i = 0; i < (int) values.size(); i++) {
				has32Bit = StringUtil::match(values[i], "*32-bit*", false);
				if (has32Bit) {
					break;
				}
			}
		}
		if (!has32Bit) {
			params["colourDepth"] = 16;
			LogManager::getSingleton().logMessage("RBX warning 16-bit color");
		}

		if (antialiasing == 0 || antialiasing == 1) {
			ConfigOptionMap::iterator antialias = options.find("Anti aliasing");
			int best = -1;
			if (antialias != options.end()) {
				StringVector& values = antialias->second.possibleValues;
				for (int i = 0; i < (int) values.size(); i++) {
					if (values[i] == "NonMaskable 1") {
						if (best < 1) {
							best = 1;
						}
					}
					else if (values[i] == "NonMaskable 2") {
						if (best < 2) {
							best = 2;
						}
					}
					else if (values[i] == "NonMaskable 3") {
						if (best < 3) {
							best = 3;
						}
					}
					else if (values[i] == "NonMaskable 4") {
						if (best < 4) {
							best = 4;
						}
					}
				}
			}
			if (best > 0) {
				params["FSAA"] = "1";
				params["FSAAQuality"] = "0";
				LogManager::getSingleton().logMessage("RBX antiliasing is on\n");
				m_antialiasing = 1;
			}
			else {
				LogManager::getSingleton().logMessage("RBX antiliasing is off\n");
				m_antialiasing = 2;
			}
		}
		else if (antialiasing == 2) {
			m_antialiasing = antialiasing;
		}

		try {
			m_renderWindow = ogreRoot->createRenderWindow(name, width, height, false, &params);
		}
		catch (Exception&) {
			LogManager::getSingleton().logMessage("Error createRenderWindow.\n");
			throw std::runtime_error("Cannot open window.");
		}

		if (renderSystem == 0) {
#ifdef _WIN32
			try {
				IDirect3DSurface9* depthBuffer;
				m_renderWindow->getCustomAttribute("D3DZBUFFER", &depthBuffer);
				D3DSURFACE_DESC desc;
				if (SUCCEEDED(depthBuffer->GetDesc(&desc))) {
					switch (desc.Format) {
					case D3DFMT_D16_LOCKABLE:
					case D3DFMT_D16:
						m_depthBits = 16;
						break;
					case D3DFMT_D32:
					case D3DFMT_D32F_LOCKABLE:
						m_depthBits = 32;
						break;
					case D3DFMT_D15S1:
						m_depthBits = 15;
						break;
					case D3DFMT_D24S8:
					case D3DFMT_D24X8:
					case D3DFMT_D24X4S4:
					case D3DFMT_D24FS8:
						m_depthBits = 24;
						break;
					default:
						m_depthBits = 32;
						LogManager::getSingleton().logMessage("RBX unknown zbuffer format");
						break;
					}
				}
				else {
					m_depthBits = 24;
				}
			}
			catch (...) {
				m_depthBits = 24;
			}
#endif
		}
		else {
			m_depthBits = -1;
		}
		LogManager::getSingleton().logMessage("RBX zbuffer bits " + StringConverter::toString(m_depthBits));

		unsigned int maxVertexIndex = 0xffffffff;
		m_softwareVertexProcessing = false;
		m_textureSquareOnly = false;
		m_supportDepthBias = true;
		if (renderSystem == 0) {
#ifdef _WIN32
			try {
				IDirect3DDevice9* device;
				m_renderWindow->getCustomAttribute("D3DDEVICE", &device);
				D3DCAPS9 caps;
				device->GetDeviceCaps(&caps);
				maxVertexIndex = caps.MaxVertexIndex;
				LogManager::getSingleton().logMessage(
					"RBX maxVertexIndex=" + StringConverter::toString(maxVertexIndex)
				);
				m_softwareVertexProcessing = device->GetSoftwareVertexProcessing() != FALSE;
				LogManager::getSingleton().logMessage(
					"RBX softwareVertexProcessing=" + StringConverter::toString(m_softwareVertexProcessing)
				);
				m_textureSquareOnly = (caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY) != 0;
				LogManager::getSingleton().logMessage(
					"RBX textureSquareOnly=" + StringConverter::toString(m_textureSquareOnly)
				);
				m_supportDepthBias = (caps.RasterCaps & D3DPRASTERCAPS_DEPTHBIAS) != 0;
				LogManager::getSingleton().logMessage(
					"RBX supportDepthBias=" + StringConverter::toString(m_supportDepthBias)
				);
			}
			catch (...) {
			}
#endif
		}

		ConfigOptionMap::iterator renderingDevice = options.find("Rendering Device");
		std::string deviceName;
		if (renderingDevice == options.end()) {
			deviceName = "Unknown";
		}
		else {
			deviceName = renderingDevice->second.currentValue;
		}
		LogManager::getSingleton().logMessage("RBX device=" + deviceName);
		LogManager::getSingleton().logMessage("RBX vidMemSize=" + StringConverter::toString(m_videoMemorySize));

		bool smallVertexBuffer = maxVertexIndex < 0x10000 || StringUtil::match(deviceName, "*Intel*", false) ||
								 StringUtil::match(deviceName, "*GeForce*MX*", false) ||
								 (m_videoMemorySize & 0xfff00000) < 0x2000000;
		if (smallVertexBuffer) {
			LogManager::getSingleton().logMessage("RBX useLargeVertexBuffer=false");
			m_useLargeVertexBuffer = false;
			m_use32BitIndices = false;
		}
		else {
			LogManager::getSingleton().logMessage("RBX useLargeVertexBuffer=true");
			m_useLargeVertexBuffer = true;
			m_use32BitIndices = true;
		}
		if (m_largeVertexBufferOverride == 0) {
			m_useLargeVertexBuffer = false;
			m_use32BitIndices = false;
		}
		else if (m_largeVertexBufferOverride == 1) {
			m_useLargeVertexBuffer = true;
			m_use32BitIndices = true;
		}

		bool savage = StringUtil::match(deviceName, "*Savage*", true) && !m_textureSquareOnly;
		if (savage) {
			LogManager::getSingleton().logMessage("RBX Savage lies");
			m_textureSquareOnly = true;
		}

		String sceneManagerName = String("sm") + StringConverter::toString(sceneManagerCount++);
		m_sceneManager =
			dynamic_cast<RbxSceneManager*>(ogreRoot->createSceneManager("RbxSceneManager", sceneManagerName));
		m_camera = m_sceneManager->createCamera("Camera");
		m_camera->setAutoAspectRatio(true);
		m_camera->setNearClipDistance(5.0f);
		m_viewport = m_renderWindow->addViewport(m_camera, 0, 0.0f, 0.0f, 1.0f, 1.0f);
		m_viewport->setBackgroundColour(ColourValue(0.0f, 0.0f, 0.0f));
		m_camera->setAspectRatio(Real(width) / height);
		ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
		if (m_videoMemorySize > 0) {
			TextureManager::getSingleton().setMemoryBudget(m_videoMemorySize);
		}
		m_twoDManager = new TwoDManager();
		m_twoDManager->init(m_sceneManager, m_renderWindow, RENDER_QUEUE_OVERLAY, true, false);
	}
	catch (Exception& e) {
		throw std::runtime_error(e.getFullDescription().c_str());
	}
}

} // namespace Ogre
