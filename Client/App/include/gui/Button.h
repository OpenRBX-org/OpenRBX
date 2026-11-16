#ifndef GUI_BUTTON_H
#define GUI_BUTTON_H

#include "gui/GuiDraw.h"
#include "gui/Widget.h"

#include <SDL_keysym.h>
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
	GuiResponse process(const GuiEvent& p_unk0x04) override;

public:
	KeyButton(const std::string& p_unk0x04, SDLKey p_unk0x08);
};

class ImageKeyButton : public KeyButton
{
private:
	GuiDrawImage imageDraw;

public:
	ImageKeyButton(Adorn* p_unk0x04, const std::string& p_unk0x08, SDLKey p_unk0x0c);

	void render2d(Adorn* adorn) override;

	G3D::Vector2 getSize() const override { return imageDraw.getImageSize(); }
};

class ImageWidget : public Widget
{
private:
	GuiDrawImage guiImageDraw;

public:
	ImageWidget(Adorn* p_unk0x04, const std::string& p_unk0x08);

	void render2d(Adorn* adorn) override;

	G3D::Vector2 getSize() const override { return guiImageDraw.getImageSize(); }
};

class ImageButton : public VerbWidget
{
protected:
	GuiDrawImage guiImageDraw;

public:
	ImageButton(Adorn* p_unk0x04, Verb* p_unk0x08, const std::string& p_unk0x0c);

	void render2d(Adorn* adorn) override;

	void setSize(const G3D::Vector2& p_unk0x04) { guiImageDraw.setImageSize(p_unk0x04); }

	G3D::Vector2 getSize() const override { return guiImageDraw.getImageSize(); }
};

} // namespace RBX

#endif // GUI_BUTTON_H
