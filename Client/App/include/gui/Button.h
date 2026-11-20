#ifndef GUI_BUTTON_H
#define GUI_BUTTON_H

#include "gui/GuiDraw.h"
#include "gui/Widget.h"
#include "util/KeyCode.h"

#include <string>

namespace RBX {

class Verb;

class KeyButton : public Widget
{
private:
	SDLKey keyCode;

protected:
	bool keyIsDown;

	void onDown(const UIEvent& event) override;
	void onUp(const UIEvent& event) override;
	bool isEnabled() override { return isVisible(); }
	GuiResponse process(const GuiEvent& event) override;

public:
	KeyButton(const std::string& name, SDLKey keyCode);
};

class ImageKeyButton : public KeyButton
{
private:
	GuiDrawImage imageDraw;

public:
	ImageKeyButton(Adorn* adorn, const std::string& name, SDLKey keyCode);

	void render2d(Adorn* adorn) override;

	G3D::Vector2 getSize() const override { return imageDraw.getImageSize(); }
};

class ImageWidget : public Widget
{
private:
	GuiDrawImage guiImageDraw;

public:
	ImageWidget(Adorn* adorn, const std::string& name);

	void render2d(Adorn* adorn) override;

	G3D::Vector2 getSize() const override { return guiImageDraw.getImageSize(); }
};

class ImageButton : public VerbWidget
{
protected:
	GuiDrawImage guiImageDraw;

public:
	ImageButton(Adorn* adorn, Verb* verb, const std::string& name);

	void render2d(Adorn* adorn) override;

	void setSize(const G3D::Vector2& size) { guiImageDraw.setImageSize(size); }

	G3D::Vector2 getSize() const override { return guiImageDraw.getImageSize(); }
};

} // namespace RBX

#endif // GUI_BUTTON_H
