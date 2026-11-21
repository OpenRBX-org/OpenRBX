#include "ThumbnailGenerator.h"

#include "appdraw/AdornG3D.h"
#include "gui/GuiDraw.h"
#include "rbxgraphics/TextureProxyBase.h"
#include "rbxview/View.h"
#include "renderlib/Mesh.h"
#include "script/Script.h"
#include "script/ScriptContext.h"
#include "util/ContentProvider.h"
#include "util/Name.h"
#include "util/RunStateOwner.h"
#include "util/base64.hpp"
#include "util/standardout.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Lighting.h"
#include "v8datamodel/ModelInstance.h"
#include "v8datamodel/Selection.h"
#include "v8datamodel/Workspace.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <G3D/GImage.h>
#include <GLG3D/GWindow.h>
#include <GLG3D/RenderDevice.h>
#include <cstring>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>

const char sThumbnailGenerator[] = "ThumbnailGenerator";

static std::mutex osContextMutex;

static RBX::Reflection::BoundFuncDesc<ThumbnailGenerator, std::string(std::string, int, int, bool), 4> clickFunction(
	&ThumbnailGenerator::click,
	"Click",
	"fileType",
	"width",
	"height",
	"hideSky",
	RBX::Reflection::FunctionDescriptor::NeedTrustedCaller
);

static RBX::Reflection::BoundFuncDesc<ThumbnailGenerator, std::string(std::string, std::string, int, int), 4>
	clickTextureFunction(
		&ThumbnailGenerator::clickTexture,
		"ClickTexture",
		"textureId",
		"fileType",
		"width",
		"height",
		RBX::Reflection::FunctionDescriptor::NeedTrustedCaller
	);

ThumbnailGenerator::ThumbnailGenerator() = default;

static EGLDisplay softwareDisplay()
{
	static const EGLDisplay display = [] {
#ifdef _WIN32
		EGLDisplay display = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
#else
		EGLDisplay display = EGL_NO_DISPLAY;
		auto queryDevices = reinterpret_cast<PFNEGLQUERYDEVICESEXTPROC>(eglGetProcAddress("eglQueryDevicesEXT"));
		auto queryDeviceString =
			reinterpret_cast<PFNEGLQUERYDEVICESTRINGEXTPROC>(eglGetProcAddress("eglQueryDeviceStringEXT"));
		EGLDeviceEXT devices[16];
		EGLint count = 0;
		if (queryDevices && queryDeviceString && queryDevices(16, devices, &count)) {
			for (EGLint i = 0; i < count && display == EGL_NO_DISPLAY; i++) {
				const char* extensions = queryDeviceString(devices[i], EGL_EXTENSIONS);
				if (extensions && std::strstr(extensions, "EGL_MESA_device_software")) {
					display = eglGetPlatformDisplay(EGL_PLATFORM_DEVICE_EXT, devices[i], nullptr);
				}
			}
		}
#endif
		if (display == EGL_NO_DISPLAY || !eglInitialize(display, nullptr, nullptr)) {
			throw std::runtime_error("No software EGL display");
		}
		return display;
	}();
	return display;
}

static EGLConfig chooseConfig(EGLDisplay display, bool alphaChannel)
{
	const EGLint attributes[] = {
		EGL_SURFACE_TYPE,
		EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE,
		EGL_OPENGL_BIT,
		EGL_RED_SIZE,
		8,
		EGL_GREEN_SIZE,
		8,
		EGL_BLUE_SIZE,
		8,
		EGL_ALPHA_SIZE,
		alphaChannel ? 8 : 0,
		EGL_DEPTH_SIZE,
		16,
		EGL_STENCIL_SIZE,
		8,
		EGL_NONE
	};
	EGLConfig config;
	EGLint count = 0;
	if (!eglChooseConfig(display, attributes, &config, 1, &count) || count == 0) {
		throw std::runtime_error("eglChooseConfig failed");
	}
	return config;
}

class OSContext : public G3D::GWindow
{
private:
	EGLDisplay display;
	EGLConfig config;
	EGLContext ctx;
	EGLSurface surface = EGL_NO_SURFACE;

	OSContext(bool alphaChannel)
		: display(softwareDisplay()), config(chooseConfig(display, alphaChannel)), ctx(createContext(EGL_NO_CONTEXT))
	{
	}

	static EGLContext shared(bool alphaChannel)
	{
		if (alphaChannel) {
			static OSContext shared(true);
			return shared.ctx;
		}
		else {
			static OSContext shared(false);
			return shared.ctx;
		}
	}

	EGLContext createContext(EGLContext sharedContext) const
	{
		if (!eglBindAPI(EGL_OPENGL_API)) {
			throw std::runtime_error("eglBindAPI failed");
		}
		EGLContext context = eglCreateContext(display, config, sharedContext, nullptr);
		if (context == EGL_NO_CONTEXT) {
			throw std::runtime_error("eglCreateContext failed");
		}
		return context;
	}

public:
	G3D::GImage image;

	OSContext(int cx, int cy, bool alphaChannel)
		: display(softwareDisplay()), config(chooseConfig(display, alphaChannel)),
		  ctx(createContext(shared(alphaChannel))), image(cx, cy, alphaChannel ? 4 : 3)
	{
		const EGLint attributes[] = {EGL_WIDTH, cx, EGL_HEIGHT, cy, EGL_NONE};
		surface = eglCreatePbufferSurface(display, config, attributes);
		if (surface == EGL_NO_SURFACE) {
			eglDestroyContext(display, ctx);
			throw std::runtime_error("eglCreatePbufferSurface failed");
		}
	}

	~OSContext() override
	{
		if (eglGetCurrentContext() == ctx) {
			eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		}
		if (surface != EGL_NO_SURFACE) {
			eglDestroySurface(display, surface);
		}
		eglDestroyContext(display, ctx);
	}

	void reallyMakeCurrent() const override
	{
		if (!eglMakeCurrent(display, surface, surface, ctx)) {
			throw std::runtime_error("eglMakeCurrent failed");
		}
	}

	void readPixels()
	{
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(
			0, 0, image.width, image.height, image.channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, image.byte()
		);
	}

	void getSettings(G3D::GWindow::Settings& settings) const override { settings = G3D::GWindow::Settings(); }

	int width() const override { return image.width; }

	int height() const override { return image.height; }

	G3D::Rect2D dimensions() const override { throw std::runtime_error("OSContext unimplemented method"); }

	void setDimensions(const G3D::Rect2D& dims) override { throw std::runtime_error("OSContext unimplemented method"); }

	void setPosition(int x, int y) override { throw std::runtime_error("OSContext unimplemented method"); }

	bool hasFocus() const override { throw std::runtime_error("OSContext unimplemented method"); }

	std::string getAPIVersion() const override { throw std::runtime_error("OSContext unimplemented method"); }

	std::string getAPIName() const override { throw std::runtime_error("OSContext unimplemented method"); }

	void setGammaRamp(const G3D::Array<G3D::uint16>& gammaRamp) override {}

	void setCaption(const std::string& caption) override {}

	int numJoysticks() const override { return 0; }

	std::string joystickName(unsigned int) override { return ""; }

	std::string caption() override { throw std::runtime_error("OSContext unimplemented method"); }

	void swapGLBuffers() override { throw std::runtime_error("OSContext unimplemented method"); }

	void notifyResize(int w, int h) override { throw std::runtime_error("OSContext unimplemented method"); }

	void setRelativeMousePosition(double x, double y) override
	{
		throw std::runtime_error("OSContext unimplemented method");
	}

	void setRelativeMousePosition(const G3D::Vector2& p) override
	{
		throw std::runtime_error("OSContext unimplemented method");
	}

	void getRelativeMouseState(G3D::Vector2& position, G3D::uint8& mouseButtons) const override
	{
		throw std::runtime_error("OSContext unimplemented method");
	}

	void getRelativeMouseState(int& x, int& y, G3D::uint8& mouseButtons) const override
	{
		throw std::runtime_error("OSContext unimplemented method");
	}

	void getRelativeMouseState(double& x, double& y, G3D::uint8& mouseButtons) const override
	{
		throw std::runtime_error("OSContext unimplemented method");
	}

	void getJoystickState(unsigned int stickNum, G3D::Array<float>& axis, G3D::Array<bool>& button) override
	{
		throw std::runtime_error("OSContext unimplemented method");
	}

	void setInputCapture(bool c) override { throw std::runtime_error("OSContext unimplemented method"); }

	bool inputCapture() const override { return false; }

	void setMouseVisible(bool b) override { throw std::runtime_error("OSContext unimplemented method"); }

	bool mouseVisible() const override { return false; }
};

static void waitForContent()
{
	while (!RBX::ContentProvider::singleton().isRequestQueueEmpty()) {
		std::this_thread::yield();
	}
}

std::string ThumbnailGenerator::clickTexture(std::string textureId, std::string fileType, int cx, int cy)
{
	try {
		RBX::StandardOut::singleton()->print(
			RBX::MESSAGE_INFO,
			"ThumbnailGenerator::clickTexture(%s, %s, %d, %d)",
			textureId.c_str(),
			fileType.c_str(),
			cx,
			cy
		);

		RBX::ContentProvider::singleton().getContent(textureId);

		G3D::BinaryOutput binaryOutput;
		binaryOutput.setEndian(G3D::G3D_LITTLE_ENDIAN);

		G3D::GImage::Format format = G3D::GImage::stringToFormat(fileType);

		OSContext context(cx, cy, true);

		std::scoped_lock lock(osContextMutex);

		context.makeCurrent();

		RBX::Render::Mesh::makeVAR();

		{
			RBX::Render::Mesh::varDirty = true;
			G3D::RenderDevice renderDevice;
			renderDevice.init(&context);
			renderDevice.pushState();
			renderDevice.push2D();

			renderDevice.setProjectionMatrix(G3D::Matrix4::orthogonalProjection(0, cx, 0, cy, -1, 1));

			{
				RBX::AdornG3D adorn(&renderDevice);
				RBX::GuiDrawImage image;
				if (image.setImage(&adorn, RBX::TextureId(textureId))) {
					RBX::Rect rect(0, 0, cx, cy);
					image.render2d(&adorn, true, rect, RBX::Widget::NOTHING);
				}
			}

			context.readPixels();
			context.image.encode(format, binaryOutput);

			renderDevice.pop2D();
			renderDevice.popState();
			renderDevice.cleanup();
		}

		int size = binaryOutput.size();
		std::string data;
		data.resize(size);
		memcpy(const_cast<char*>(data.c_str()), binaryOutput.getCArray(), size);

		std::stringstream stream;

		std::ostreambuf_iterator<char> out(stream);
		int state = 0;
		base64<char>().put(data.begin(), data.end(), out, state, base64<char>::noline());

		RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "ThumbnailGenerator::click() success");

		return stream.str();
	}
	catch (std::exception& e) {
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());
		throw;
	}
}

std::string ThumbnailGenerator::click(std::string fileType, int cx, int cy, bool hideSky)
{
	try {
		RBX::StandardOut::singleton()->print(
			RBX::MESSAGE_INFO,
			"ThumbnailGenerator::click(%s, %d, %d, %s)",
			fileType.c_str(),
			cx,
			cy,
			hideSky ? "true" : "false"
		);

		G3D::BinaryOutput binaryOutput;
		binaryOutput.setEndian(G3D::G3D_LITTLE_ENDIAN);

		doClick(fileType, cx, cy, hideSky, nullptr);

		waitForContent();

		doClick(fileType, cx, cy, hideSky, &binaryOutput);

		int size = binaryOutput.size();
		std::string data;
		data.resize(size);
		memcpy(const_cast<char*>(data.c_str()), binaryOutput.getCArray(), size);

		std::stringstream stream;

		std::ostreambuf_iterator<char> out(stream);
		int state = 0;
		base64<char>().put(data.begin(), data.end(), out, state, base64<char>::noline());

		RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "ThumbnailGenerator::click() success");

		return stream.str();
	}
	catch (std::exception& e) {
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());
		throw;
	}
}

void ThumbnailGenerator::doClick(std::string fileType, int cx, int cy, bool hideSky, G3D::BinaryOutput* binaryOutput)
{
	G3D::GImage::Format format = G3D::GImage::stringToFormat(fileType);
	bool alphaChannel = false;

	if (hideSky) {
		RBX::Lighting* lighting = RBX::ServiceProvider::create<RBX::Lighting>(this);
		lighting->suppressSky(true);
		if (format == G3D::GImage::PNG) {
			alphaChannel = true;
			lighting->setClearColor(G3D::Color4(0, 0, 0, 0));
		}
		else {
			lighting->setClearColor(G3D::Color4(1, 1, 1, 1));
		}

		RBX::ServiceProvider::find<RBX::Workspace>(this)->setImageServerView(G3D::Rect2D::xywh(0, 0, cx, cy));
	}

	OSContext context(cx, cy, alphaChannel);

	std::scoped_lock lock(osContextMutex);

	context.makeCurrent();

	RBX::Render::Mesh::makeVAR();

	RBX::Render::Mesh::varDirty = true;
	G3D::RenderDevice renderDevice;
	renderDevice.init(&context);

	std::unique_ptr<RBX::ViewBase> rbxView(
		new RBX::View::View(RBX::shared_from(static_cast<RBX::DataModel*>(getParent())))
	);

	rbxView->updateSettings(100, 100, false, 10);

	glMatrixMode(GL_MODELVIEW);
	rbxView->render(&renderDevice);
	glMatrixMode(GL_MODELVIEW);

	context.readPixels();
	if (alphaChannel) {
		G3D::GImage::flipRGBAVertical(
			context.image.byte(), context.image.byte(), context.image.width, context.image.height
		);
	}
	else {
		G3D::GImage::flipRGBVertical(
			context.image.byte(), context.image.byte(), context.image.width, context.image.height
		);
	}

	double s = renderDevice.getLightSaturation();
	if (s != 1.0) {
		G3D::uint8 L[256];
		for (int i = 255; i >= 0; --i) {
			L[i] = G3D::iMin(G3D::iRound(i * s), 255);
		}

		int N = context.image.width * context.image.height * context.image.channels;
		int stride = alphaChannel ? 4 : 3;
		for (int i = 0; i < N; i += stride) {
			for (int c = 0; c < 3; ++c) {
				context.image.byte()[i + c] = L[context.image.byte()[i + c]];
			}
		}
	}

	if (binaryOutput) {
		context.image.encode(format, *binaryOutput);
	}
	else if (hideSky) {
		RBX::ServiceProvider::find<RBX::Workspace>(this)->setImageServerView(G3D::Rect2D::xywh(0, 0, cx, cy));
	}

	renderDevice.cleanup();
}

template void RBX::Name::callDoDeclare<sThumbnailGenerator>();
template const RBX::Name& RBX::Name::doDeclare<sThumbnailGenerator>();
template void RBX::Name::callDoDeclare<RBX::sLighting>();
template const RBX::Name& RBX::Name::doDeclare<RBX::sLighting>();
template void RBX::Name::callDoDeclare<RBX::sRunService>();
template const RBX::Name& RBX::Name::doDeclare<RBX::sRunService>();
template void RBX::Name::callDoDeclare<RBX::sModel>();
template const RBX::Name& RBX::Name::doDeclare<RBX::sModel>();
template void RBX::Name::callDoDeclare<RBX::sSelection>();
template const RBX::Name& RBX::Name::doDeclare<RBX::sSelection>();
template void RBX::Name::callDoDeclare<RBX::sScriptContext>();
template const RBX::Name& RBX::Name::doDeclare<RBX::sScriptContext>();
template void RBX::Name::callDoDeclare<RBX::sScript>();
template const RBX::Name& RBX::Name::doDeclare<RBX::sScript>();
template void RBX::Name::callDoDeclare<RBX::sLocalScript>();
template const RBX::Name& RBX::Name::doDeclare<RBX::sLocalScript>();
template void RBX::Name::callDoDeclare<RBX::sWorkspace>();
template const RBX::Name& RBX::Name::doDeclare<RBX::sWorkspace>();

template const RBX::FactoryProduct<ThumbnailGenerator, RBX::Instance, sThumbnailGenerator>::Creator
	RBX::FactoryProduct<ThumbnailGenerator, RBX::Instance, sThumbnailGenerator>::creator;
template const RBX::FactoryProduct<RBX::Lighting, RBX::Instance, RBX::sLighting>::Creator
	RBX::FactoryProduct<RBX::Lighting, RBX::Instance, RBX::sLighting>::creator;
template const RBX::FactoryProduct<RBX::RunService, RBX::Instance, RBX::sRunService>::Creator
	RBX::FactoryProduct<RBX::RunService, RBX::Instance, RBX::sRunService>::creator;
template const RBX::FactoryProduct<RBX::ModelInstance, RBX::PVInstance, RBX::sModel>::Creator
	RBX::FactoryProduct<RBX::ModelInstance, RBX::PVInstance, RBX::sModel>::creator;
template const RBX::FactoryProduct<RBX::Selection, RBX::Instance, RBX::sSelection>::Creator
	RBX::FactoryProduct<RBX::Selection, RBX::Instance, RBX::sSelection>::creator;
template const RBX::FactoryProduct<RBX::ScriptContext, RBX::Instance, RBX::sScriptContext>::Creator
	RBX::FactoryProduct<RBX::ScriptContext, RBX::Instance, RBX::sScriptContext>::creator;
template const RBX::FactoryProduct<RBX::Script, RBX::Instance, RBX::sScript>::Creator
	RBX::FactoryProduct<RBX::Script, RBX::Instance, RBX::sScript>::creator;
template const RBX::FactoryProduct<RBX::LocalScript, RBX::Script, RBX::sLocalScript>::Creator
	RBX::FactoryProduct<RBX::LocalScript, RBX::Script, RBX::sLocalScript>::creator;
