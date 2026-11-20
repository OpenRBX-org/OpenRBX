#ifndef GLXWINDOW_H
#define GLXWINDOW_H

#include <G3D/Rect2D.h>
#include <GLG3D/GWindow.h>

typedef struct _XDisplay Display;
typedef struct __GLXcontextRec* GLXContext;

int defaultScreen(Display* display);
void resizeNativeWindow(Display* display, unsigned long window, int width, int height);

class GlxWindow : public G3D::GWindow
{
public:
	GlxWindow(const G3D::GWindow::Settings& settings, Display* display, unsigned long parent);
	~GlxWindow() override;

	void getSettings(G3D::GWindow::Settings& settings) const override;
	int width() const override;
	int height() const override;
	G3D::Rect2D dimensions() const override;
	void setDimensions(const G3D::Rect2D& dims) override;
	void setPosition(int x, int y) override;
	bool hasFocus() const override;
	std::string getAPIVersion() const override;
	std::string getAPIName() const override;
	void setGammaRamp(const G3D::Array<G3D::uint16>& gammaRamp) override;
	void setCaption(const std::string& caption) override;
	int numJoysticks() const override;
	std::string joystickName(unsigned int sticknum) override;
	std::string caption() override;
	void swapGLBuffers() override;
	void notifyResize(int w, int h) override;
	void setRelativeMousePosition(double x, double y) override;
	void setRelativeMousePosition(const G3D::Vector2& p) override;
	void getRelativeMouseState(G3D::Vector2& position, G3D::uint8& mouseButtons) const override;
	void getRelativeMouseState(int& x, int& y, G3D::uint8& mouseButtons) const override;
	void getRelativeMouseState(double& x, double& y, G3D::uint8& mouseButtons) const override;
	void getJoystickState(unsigned int stickNum, G3D::Array<float>& axis, G3D::Array<bool>& button) override;
	void setInputCapture(bool c) override;
	bool inputCapture() const override;
	void setMouseVisible(bool b) override;
	bool mouseVisible() const override;

protected:
	void reallyMakeCurrent() const override;

private:
	G3D::GWindow::Settings m_settings;
	Display* m_display;
	unsigned long m_window;
	unsigned long m_colormap;
	GLXContext m_context;
	std::string m_caption;
	bool m_inputCapture = false;
	bool m_mouseVisible = true;
};

#endif // GLXWINDOW_H
