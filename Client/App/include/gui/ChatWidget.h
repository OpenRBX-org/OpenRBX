#ifndef GUI_CHATWIDGET_H
#define GUI_CHATWIDGET_H

#include "gui/GUI.h"
#include "gui/GuiDraw.h"
#include "gui/Widget.h"

#include <string>

namespace RBX {

class UnifiedImageWidget : public UnifiedWidget
{
protected:
	GuiDrawImage guiImageDraw;

public:
	UnifiedImageWidget(Adorn* adorn, const std::string& image) : guiImageDraw(adorn, image) {}

	Widget::WidgetState getWidgetState() const
	{
		switch (getMenuState()) {
		case HOVER:
			return Widget::HOVER;
		case SHOWN_APPEARING:
		case SHOWN:
			return Widget::DOWN_OVER;
		default:
			return Widget::NOTHING;
		}
	}

	void render2dMe(Adorn* p_unk0x04) override;

	void setSize(const G3D::Vector2& p_unk0x04) { guiImageDraw.setImageSize(p_unk0x04); }
	G3D::Vector2 getSize() const override { return guiImageDraw.getImageSize(); }
};

class ChatButton : public UnifiedImageWidget
{
private:
	bool isVisible() const override;

public:
	ChatButton(Adorn* adorn, const std::string& image) : UnifiedImageWidget(adorn, image) {}
};

class ChatWidget : public UnifiedWidget
{
private:
	std::string findMenuString(GuiItem* p_unk0x04);

	std::string code;

	void onMenuStateChanged() override;
	GuiResponse process(const GuiEvent& p_unk0x04) override;

public:
	ChatWidget(const std::string& p_unk0x04, std::string p_unk0x08);
};

} // namespace RBX

#endif // GUI_CHATWIDGET_H
