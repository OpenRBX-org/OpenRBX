#ifndef WHIDBEYSTYLE_H
#define WHIDBEYSTYLE_H

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QImage>
#include <QPalette>
#include <QProxyStyle>

class QPainter;

struct WhidbeyColors
{
	QColor window;
	QColor windowText;
	QColor menuText;
	QColor face;
	QColor shadow;
	QColor darkShadow;
	QColor light;
	QColor buttonHighlight;
	QColor buttonLight;
	QColor buttonText;
	QColor grayText;
	QColor highlight;
	QColor highlightText;
	QColor activeCaption;
	QColor inactiveCaption;
	QColor captionText;
	QColor inactiveCaptionText;
	QColor infoBackground;
	QColor infoText;
	QColor hotLight;

	QColor toolbarFace;
	QColor toolbarGripper;
	QColor toolbarText;
	QColor toolbarGrayText;
	QColor separator;
	QColor disabled;
	QColor menuBarFace;
	QColor menuBarBorder;
	QColor menuBarText;
	QColor menuBarGrayText;
	QColor highlightFace;
	QColor highlightBorder;
	QColor highlightPushed;
	QColor highlightPushedBorder;
	QColor highlightChecked;
	QColor highlightCheckedBorder;
	QColor highlightDisabledBorder;
	QColor highlightedText;
	QColor pushedText;
	QColor staticFrame;

	QColor menuExpandDark;
	QColor menuExpandedLight;
	QColor menuExpandedDark;
	QColor commandBarLight;
	QColor commandBarDark;
	QColor dockBarLight;
	QColor dockBarDark;
	QColor popupControlLight;
	QColor popupControlDark;
	QColor toolbarExpandLight;
	QColor toolbarExpandDark;
	QColor toolbarShadow;
	QColor statusPane;
	QColor disabledIconLight;
	QColor disabledIconDark;

	QColor paneCaptionActive;
	QColor paneCaptionInactive;
	QColor paneCaptionActiveText;
	QColor paneCaptionInactiveText;
	QColor paneCaptionBorder;

	QColor tabFrameBorder;
	QColor appWorkspace;
	QColor tabHeaderFace;
	QColor tabButtonNormal;
	QColor tabButtonSelected;
	QColor tabNormalText;
	QColor tabSelectedText;
	QColor tabBorderShadow;
	QColor tabBorderHighlight;
};

class QToolBar;
class QStyleOptionMenuItem;

class QMenu;

class WhidbeyStyle : public QProxyStyle
{
public:
	enum PaneCaptionButton
	{
		PaneCaptionClose,
		PaneCaptionPin
	};

	enum TabNavigateButton
	{
		TabNavigateLeft,
		TabNavigateRight,
		TabNavigateClose
	};

	WhidbeyStyle();

	static const WhidbeyColors& colors();
	static QPalette systemPalette();

	static QFont commandBarFont();
	static QFont iconTitleFont();
	static QFont paneCaptionFont();

	static const int clientEdgeWidth = 2;
	static const int emptyToolBarExtent = 45;
	static void drawClientEdge(QPainter* painter, const QRect& rect);
	static QString endEllipsis(const QFont& font, const QString& text, int width);
	static void drawText(QPainter* painter, const QRect& rect, int flags, const QString& text);
	static void drawControlRectangle(QPainter* painter, const QRect& rect, bool selected, bool pressed);
	static void drawAlphaImage(QPainter* painter, const QPoint& position, const QImage& image);
	static QColor themeColor(const wchar_t* themeClass, int part, int state, int property, const QColor& fallback);
	static bool drawThemeBackground(
		QPainter* painter, const QRect& rect, const wchar_t* themeClass, int part, int state, const QColor& background
	);
	static QSize listBoxControlSize();
	static void drawListBoxControlBackground(QPainter* painter, const QRect& rect);
	static void drawListBoxControl(
		QPainter* painter, const QRect& rect, const QIcon& icon, const QString& text, bool popup, bool selected
	);
	static QRect toolBarExpandRect(const QToolBar* toolBar);
	static QPixmap premultipliedPixmap(const QImage& source);

	static void setMenuAnimation(int animation);
	static int popupGripperWidth(const QWidget* menu);
	static const int popupTextPadding = 8;
	static void setCustomizeMode(bool customize);
	static bool customizeMode();
	static void setCustomizeSelection(QWidget* control, QRect frame = QRect());
	static QWidget* customizeSelection();
	static void animateMenuExpansion(QMenu* menu, const QRect& previous);
	static void setPopupShadowsEnabled(bool enabled);

	static void fillHorizontalGradient(QPainter* painter, const QRect& rect, const QColor& left, const QColor& right);

	static int paneCaptionHeight();
	static QRect paneCaptionButtonRect(const QRect& caption, int index);
	static void drawPaneCaption(
		QPainter* painter, const QRect& caption, const QString& title, bool active, bool enabled, int buttonCount
	);
	static void drawPaneCaptionButton(
		QPainter* painter,
		const QRect& rect,
		PaneCaptionButton button,
		bool active,
		bool hot,
		bool pressed,
		bool autoHidden
	);

	static QSize tabNavigateButtonSize();
	static int workspaceHeaderHeight(const QFontMetrics& metrics);
	static void drawWorkspaceHeader(QPainter* painter, const QRect& rect);
	static void drawTabNavigateButton(
		QPainter* painter, const QRect& rect, TabNavigateButton button, bool enabled, bool hot, bool pressed
	);

	using QProxyStyle::polish;
	using QProxyStyle::unpolish;

#ifndef _WIN32
	void polish(QApplication* application) override;
#endif
	void polish(QWidget* widget) override;
	void unpolish(QWidget* widget) override;
	bool eventFilter(QObject* object, QEvent* event) override;

	QPalette standardPalette() const override;
	int pixelMetric(
		PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr
	) const override;
	int styleHint(
		StyleHint hint,
		const QStyleOption* option = nullptr,
		const QWidget* widget = nullptr,
		QStyleHintReturn* returnData = nullptr
	) const override;
	QSize sizeFromContents(
		ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget
	) const override;
	QRect subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const override;
	QRect subControlRect(
		ComplexControl control, const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget
	) const override;
	void drawItemText(
		QPainter* painter,
		const QRect& rect,
		int flags,
		const QPalette& palette,
		bool enabled,
		const QString& text,
		QPalette::ColorRole textRole = QPalette::NoRole
	) const override;
	void drawPrimitive(
		PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr
	) const override;
	void drawControl(
		ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr
	) const override;
	void drawComplexControl(
		ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget = nullptr
	) const override;
	QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* option) const override;

private:
	void startMenuAnimation(QMenu* menu);
	bool paintMenuAnimation(QMenu* menu);

	int mnemonicFlags(const QStyleOption* option, const QWidget* widget) const;
	void drawMenuPanel(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
	void drawMenuItem(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
	void drawToolBarPopupItem(const QStyleOptionMenuItem* item, QPainter* painter, const QWidget* widget) const;
	void drawMenuBarItem(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
	void drawToolBarButton(const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const;
	void drawToolBarCombo(const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const;
	void drawWorkspaceTabShape(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
	void drawWorkspaceTabLabel(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
};

#endif // WHIDBEYSTYLE_H
