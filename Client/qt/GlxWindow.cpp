#include "GlxWindow.h"

#include <GL/glx.h>
#include <GLG3D/GLCaps.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <algorithm>
#include <stdexcept>

static GLXContext shareContext;

int defaultScreen(Display* display)
{
	return DefaultScreen(display);
}

void resizeNativeWindow(Display* display, unsigned long window, int width, int height)
{
	XResizeWindow(display, window, std::max(1, width), std::max(1, height));
	XSync(display, False);
}

static XVisualInfo* chooseVisual(Display* display, const G3D::GWindow::Settings& settings)
{
	int attributes[] = {
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		GLX_RED_SIZE,
		settings.rgbBits,
		GLX_GREEN_SIZE,
		settings.rgbBits,
		GLX_BLUE_SIZE,
		settings.rgbBits,
		GLX_ALPHA_SIZE,
		settings.alphaBits,
		GLX_DEPTH_SIZE,
		settings.depthBits,
		GLX_STENCIL_SIZE,
		settings.stencilBits,
		None,
	};
	return glXChooseVisual(display, DefaultScreen(display), attributes);
}

GlxWindow::GlxWindow(const G3D::GWindow::Settings& settings, Display* display, unsigned long parent)
	: m_settings(settings), m_display(display), m_window(0), m_colormap(0), m_context(nullptr)
{
	XVisualInfo* visual = chooseVisual(display, settings);
	if (!visual) {
		throw std::runtime_error("No GLX visual matches the requested pixel format");
	}

	if (!shareContext) {
		shareContext = glXCreateContext(display, visual, nullptr, True);
	}

	m_colormap = XCreateColormap(display, parent, visual->visual, AllocNone);
	XSetWindowAttributes attributes = {};
	attributes.colormap = m_colormap;
	attributes.border_pixel = 0;
	attributes.event_mask = 0;
	m_window = XCreateWindow(
		display,
		parent,
		0,
		0,
		std::max(1, settings.width),
		std::max(1, settings.height),
		0,
		visual->depth,
		InputOutput,
		visual->visual,
		CWColormap | CWBorderPixel | CWEventMask,
		&attributes
	);
	XMapWindow(display, m_window);

	m_context = glXCreateContext(display, visual, shareContext, True);
	XFree(visual);
	if (!m_context) {
		XDestroyWindow(display, m_window);
		XFreeColormap(display, m_colormap);
		throw std::runtime_error("Failed to create OpenGL context");
	}

	makeCurrent();
	G3D::GLCaps::init();
}

GlxWindow::~GlxWindow()
{
	if (glXGetCurrentContext() == m_context) {
		glXMakeCurrent(m_display, None, nullptr);
	}
	glXDestroyContext(m_display, m_context);
	XDestroyWindow(m_display, m_window);
	XFreeColormap(m_display, m_colormap);
	XFlush(m_display);
}

void GlxWindow::reallyMakeCurrent() const
{
	glXMakeCurrent(m_display, m_window, m_context);
}

void GlxWindow::getSettings(G3D::GWindow::Settings& settings) const
{
	settings = m_settings;
}

int GlxWindow::width() const
{
	return m_settings.width;
}

int GlxWindow::height() const
{
	return m_settings.height;
}

G3D::Rect2D GlxWindow::dimensions() const
{
	return G3D::Rect2D::xywh(0, 0, static_cast<float>(width()), static_cast<float>(height()));
}

void GlxWindow::setDimensions(const G3D::Rect2D& dims)
{
	XMoveResizeWindow(
		m_display,
		m_window,
		static_cast<int>(dims.x0()),
		static_cast<int>(dims.y0()),
		std::max(1, static_cast<int>(dims.width())),
		std::max(1, static_cast<int>(dims.height()))
	);
}

void GlxWindow::setPosition(int x, int y)
{
	XMoveWindow(m_display, m_window, x, y);
}

bool GlxWindow::hasFocus() const
{
	return true;
}

std::string GlxWindow::getAPIVersion() const
{
	return "1.1";
}

std::string GlxWindow::getAPIName() const
{
	return "GLX";
}

void GlxWindow::setGammaRamp(const G3D::Array<G3D::uint16>& gammaRamp)
{
}

void GlxWindow::setCaption(const std::string& caption)
{
	m_caption = caption;
}

int GlxWindow::numJoysticks() const
{
	return 0;
}

std::string GlxWindow::joystickName(unsigned int sticknum)
{
	return std::string();
}

std::string GlxWindow::caption()
{
	return m_caption;
}

void GlxWindow::swapGLBuffers()
{
	glXSwapBuffers(m_display, m_window);
}

void GlxWindow::notifyResize(int w, int h)
{
	m_settings.width = w;
	m_settings.height = h;
	XResizeWindow(m_display, m_window, std::max(1, w), std::max(1, h));
}

void GlxWindow::setRelativeMousePosition(double x, double y)
{
	XWarpPointer(m_display, None, m_window, 0, 0, 0, 0, static_cast<int>(x), static_cast<int>(y));
}

void GlxWindow::setRelativeMousePosition(const G3D::Vector2& p)
{
	setRelativeMousePosition(p.x, p.y);
}

void GlxWindow::getRelativeMouseState(G3D::Vector2& position, G3D::uint8& mouseButtons) const
{
	int x;
	int y;
	getRelativeMouseState(x, y, mouseButtons);
	position = G3D::Vector2(static_cast<float>(x), static_cast<float>(y));
}

void GlxWindow::getRelativeMouseState(int& x, int& y, G3D::uint8& mouseButtons) const
{
	Window root;
	Window child;
	int rootX;
	int rootY;
	unsigned int mask = 0;
	x = 0;
	y = 0;
	XQueryPointer(m_display, m_window, &root, &child, &rootX, &rootY, &x, &y, &mask);
	mouseButtons = static_cast<G3D::uint8>(
		((mask & Button1Mask) ? 1 : 0) | ((mask & Button3Mask) ? 2 : 0) | ((mask & Button2Mask) ? 4 : 0)
	);
}

void GlxWindow::getRelativeMouseState(double& x, double& y, G3D::uint8& mouseButtons) const
{
	int ix;
	int iy;
	getRelativeMouseState(ix, iy, mouseButtons);
	x = ix;
	y = iy;
}

void GlxWindow::getJoystickState(unsigned int stickNum, G3D::Array<float>& axis, G3D::Array<bool>& button)
{
}

void GlxWindow::setInputCapture(bool c)
{
	m_inputCapture = c;
}

bool GlxWindow::inputCapture() const
{
	return m_inputCapture;
}

void GlxWindow::setMouseVisible(bool b)
{
	m_mouseVisible = b;
}

bool GlxWindow::mouseVisible() const
{
	return m_mouseVisible;
}
