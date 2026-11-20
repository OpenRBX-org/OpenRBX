#ifndef GUI_GUI_H
#define GUI_GUI_H

#include "gui/GuiEvent.h"
#include "gui/Layout.h"
#include "rbxgraphics/Adorn.h"
#include "util/Name.h"
#include "util/Rect.h"
#include "v8tree/Instance.h"

#include <G3D/Color4.h>
#include <G3D/CoordinateFrame.h>
#include <G3D/Matrix4.h>
#include <G3D/Rect2D.h>
#include <G3D/Vector2.h>
#include <G3D/Vector2int16.h>

namespace RBX {

class GuiItem : public Instance, public GuiTarget
{
public:
	GuiItem();

private:
	bool askAddChild(const Instance* instance) const override
	{
		return dynamic_cast<const GuiItem*>(instance) != nullptr;
	}

	const Name& getClassName() const override { return Name::getNullName(); }

public:

protected:
	virtual void onLoseFocus() {}
	virtual bool canLoseFocus() { return false; }

	virtual G3D::Vector2 getPosition() const { return getGuiParent()->getChildPosition(this); }

	virtual G3D::Vector2 getChildPosition(const GuiItem* child) const { return G3D::Vector2::zero(); }

	virtual int getFontSize() const { return 14; }

	virtual bool isVisible() const { return true; }

	virtual std::string getTitle() { return getName(); }

public:
	virtual G3D::Vector2 getSize() const { return guiSize; }

	virtual void render2d(Adorn* adorn) {}

	GuiResponse process(const GuiEvent& event) override;

	void addGuiItem(shared_ptr<GuiItem> item) { item->setParent(this); }

	void setGuiSize(const G3D::Vector2& value) { guiSize = value; }
	const G3D::Vector2& getGuiSize() const { return guiSize; }

	const GuiItem* getGuiParent() const;
	GuiItem* getGuiParent();

	const GuiItem* getGuiItem(int index) const;
	GuiItem* getGuiItem(int index);

	static const G3D::Color4& enabledFill();
	static const G3D::Color4& disabledFill();
	static const G3D::Color4& menuEnabledFill();
	static const G3D::Color4& menuDisabledFill();
	static const G3D::Color4& translucentBorder();
	static const G3D::Color4& menuSelect();
	static const G3D::Color4& menuBackground();
	static const G3D::Color4& fontBorder();
	static const G3D::Color4& translucentBackdrop();
	static const G3D::Color4& toolboxColor();

protected:
	GuiItem* getFocus() { return focus.get(); }
	void loseFocus();

	Rect getMyRect() const;

	void label2d(
		Adorn* adorn, const std::string& label, const G3D::Color4& fill, const G3D::Color4& border, Adorn::XAlign align
	) const;
	G3D::Rect2D getMyRect2D() const { return getMyRect().toRect2D(); }

private:
	GuiResponse processNonFocus(const GuiEvent& event);
	void switchFocus(GuiItem* item);

	void onDescendentRemoving(const shared_ptr<Instance>& instance) override;

	shared_ptr<GuiItem> focus;
	G3D::Vector2 guiSize;
};

class GuiRoot : public GuiItem
{
	static bool preProcess(const GuiEvent&);

public:
	GuiRoot();

	static void setCanvasSize(const G3D::Vector2& value) { canvasSize = value; }
	static G3D::Vector2 getCanvasSize() { return canvasSize; }
	static G3D::Vector2 toPixelSize(const G3D::Vector2& size);
	static int normalizedFontSize(int size);

	void render2d(Adorn* adorn) override;
	static GuiResponse processTarget(const GuiEvent&, GuiTarget*);
	static void render2dItem(Adorn*, GuiItem*);

protected:
	bool askSetParent(const Instance* instance) const override { return true; }

private:
	G3D::Vector2 getSize() const override { return canvasSize; }

	static G3D::Vector2 canvasSize;
	static G3D::Matrix4 oldProjectionMatrix;
	static G3D::CoordinateFrame oldCameraWorld;
};

class TopMenuBar : public GuiItem
{
public:
	TopMenuBar() { init(); }
	TopMenuBar(const std::string& name, Layout::Style style, bool translucent = false);
	TopMenuBar(const std::string& name, Layout::Style style, G3D::Color4 backdrop);

	GuiResponse process(const GuiEvent& event) override;
	void render2d(Adorn* adorn) override;
	G3D::Vector2 getSize() const override;
	bool isVisible() const override { return visible; }

	void setVisible(bool value) { visible = value; }

protected:
	G3D::Vector2 getChildPosition(const GuiItem* child) const override;

	G3D::Color4 backdropColor;
	Layout::Style layoutStyle;
	bool visible;

private:
	void init();
};

class RelativePanel : public TopMenuBar
{
public:
	RelativePanel() : offset(0, 0)
	{
		Layout layout;
		init(layout);
	}

	RelativePanel(const Layout& layout) : offset(0, 0) { init(layout); }

	G3D::Vector2 getPosition() const override;

protected:
	void init(const Layout& layout);

	Rect::Location xLocation;
	Rect::Location yLocation;
	G3D::Vector2int16 offset;
};

class PercentPanel : public TopMenuBar
{
public:
	PercentPanel(Rect::Location xLocation, Rect::Location yLocation, const G3D::Vector2 position)
		: xLocation(xLocation), yLocation(yLocation), position(position)
	{
	}

	G3D::Vector2 getPosition() const override;

protected:
	Rect::Location xLocation;
	Rect::Location yLocation;
	G3D::Vector2 position;
};

class UnifiedWidget : public GuiItem
{
public:
	enum MenuState
	{
		NOTHING = 0,
		HOVER = 1,
		SHOWN_APPEARING = 2,
		SHOWN = 3
	};

private:
	MenuState menuState;

	GuiResponse processShown_InTitle(const GuiEvent& event);
	GuiResponse processShown_OutOfTitle(const GuiEvent& event);
	GuiResponse processNothing(const GuiEvent& event);
	GuiResponse processHover(const GuiEvent& event);
	GuiResponse processShown(const GuiEvent& event);
	GuiResponse processKey(const GuiEvent& event);
	void render2dChildren(Adorn* adorn);
	G3D::Vector2 getChildPosition(const GuiItem* child) const override;
	bool canLoseFocus() override { return true; }
	void onLoseFocus() override { menuState = NOTHING; }
	int getFontSize() const override;
	void init();

protected:
	bool showChildren() { return menuState >= SHOWN_APPEARING; }
	virtual void onMenuStateChanged() {}
	virtual G3D::Vector2 firstChildPosition() const;
	virtual G3D::Vector2 childOffset() const;
	virtual void render2dMe(Adorn* adorn);

public:
	UnifiedWidget(const std::string& name);
	UnifiedWidget() { init(); }

	MenuState getMenuState() const { return menuState; }
	void setMenuState(MenuState state);

	GuiResponse process(const GuiEvent& event) override;
	void render2d(Adorn* adorn) override;
};

class MenuSpacer : public GuiItem
{
protected:
	void addItem(GuiItem* item);

public:
	MenuSpacer(float x, float y) { setGuiSize(G3D::Vector2(x, y)); }
};

class TextDisplay : public GuiItem
{
private:
	void init();

protected:
	std::string label;
	int fontSize;
	G3D::Color4 fontColor;
	G3D::Color4 borderColor;
	Adorn::XAlign align;

	int getFontSize() const override;
	virtual std::string getLabel() const;

public:
	TextDisplay(const std::string& name, const std::string& text);
	TextDisplay();

	void render2d(Adorn* adorn) override;

	void setLabel(const std::string& value) { label = value; }

	void setFontSize(int value)
	{
		fontSize = value;
		setGuiSize(G3D::Vector2(10.0f * fontSize, 2.0f * fontSize));
	}

	void setFontColor(const G3D::Color4& value) { fontColor = value; }
	void setBorderColor(const G3D::Color4& value) { borderColor = value; }
	void setAlign(Adorn::XAlign value) { align = value; }
};

} // namespace RBX

#endif // GUI_GUI_H
