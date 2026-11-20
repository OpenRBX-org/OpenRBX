#include "BrowserScrollBar.h"

#include <QAbstractScrollArea>
#include <QPainter>
#include <QProxyStyle>
#include <QScrollBar>
#include <QStyleOptionSlider>
#include <algorithm>

static const int browserScrollBarExtent = 17;
static const int browserScrollButtonLength = 35;
static const int browserScrollArrowInset = 17;
static const int browserScrollArrowWidth = 11;
static const int browserScrollArrowHeight = 9;
static const int browserScrollThumbInset = 2;
static const int browserScrollThumbMinimum = 17;

class BrowserScrollBarStyle : public QProxyStyle
{
public:
	using QProxyStyle::QProxyStyle;

	int pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const override
	{
		if (metric == PM_ScrollBarExtent) {
			return browserScrollBarExtent;
		}
		return QProxyStyle::pixelMetric(metric, option, widget);
	}

	QRect subControlRect(
		ComplexControl control, const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget
	) const override
	{
		const QStyleOptionSlider* slider = qstyleoption_cast<const QStyleOptionSlider*>(option);
		if (control != CC_ScrollBar || !slider) {
			return QProxyStyle::subControlRect(control, option, subControl, widget);
		}

		QRect bar = slider->rect;
		int track = std::max(0, bar.height() - 2 * browserScrollButtonLength - 2 * browserScrollThumbInset + 1);
		int range = slider->maximum - slider->minimum;
		int thumb = track;
		if (range > 0) {
			thumb = std::max(
				browserScrollThumbMinimum,
				static_cast<int>(static_cast<qint64>(track) * slider->pageStep / (range + slider->pageStep))
			);
		}
		thumb = std::min(thumb, track);
		int trackTop = bar.top() + browserScrollButtonLength + browserScrollThumbInset;
		int thumbTop = trackTop;
		if (range > 0) {
			thumbTop += sliderPositionFromValue(
				slider->minimum, slider->maximum, slider->sliderPosition, track - thumb, slider->upsideDown
			);
		}

		switch (subControl) {
		case SC_ScrollBarSubLine:
			return QRect(bar.left(), bar.top(), bar.width(), browserScrollButtonLength);
		case SC_ScrollBarAddLine:
			return QRect(
				bar.left(), bar.bottom() + 1 - browserScrollButtonLength, bar.width(), browserScrollButtonLength
			);
		case SC_ScrollBarSlider:
			return QRect(bar.left() + 1, thumbTop, bar.width() - 2, thumb);
		case SC_ScrollBarGroove:
			return QRect(bar.left(), trackTop, bar.width(), track);
		case SC_ScrollBarSubPage:
			return QRect(bar.left(), trackTop, bar.width(), thumbTop - trackTop);
		case SC_ScrollBarAddPage:
			return QRect(bar.left(), thumbTop + thumb, bar.width(), trackTop + track - thumbTop - thumb);
		default:
			return QRect();
		}
	}

	void drawComplexControl(
		ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget
	) const override
	{
		const QStyleOptionSlider* slider = qstyleoption_cast<const QStyleOptionSlider*>(option);
		if (control != CC_ScrollBar || !slider) {
			QProxyStyle::drawComplexControl(control, option, painter, widget);
			return;
		}

		bool enabled = (slider->state & State_Enabled) && slider->maximum > slider->minimum;
		painter->fillRect(slider->rect, enabled ? QColor(240, 240, 240) : QColor(249, 249, 249));

		for (SubControl button : {SC_ScrollBarSubLine, SC_ScrollBarAddLine}) {
			QRect rc = subControlRect(control, option, button, widget);
			bool active = enabled && (slider->activeSubControls & button);
			bool pressed = active && (slider->state & State_Sunken);
			if (active) {
				painter->fillRect(rc, pressed ? QColor(96, 96, 96) : QColor(218, 218, 218));
			}

			bool up = button == SC_ScrollBarSubLine;
			QColor glyph = !enabled ? QColor(191, 191, 191) : pressed ? QColor(255, 255, 255) : QColor(96, 96, 96);
			drawArrow(
				painter,
				rc.left() + (rc.width() - browserScrollArrowWidth) / 2,
				up ? rc.top() + browserScrollArrowInset
				   : rc.bottom() + 1 - browserScrollArrowInset - browserScrollArrowHeight,
				up,
				glyph
			);
		}

		if (enabled) {
			QRect thumb = subControlRect(control, option, SC_ScrollBarSlider, widget);
			bool active = slider->activeSubControls & SC_ScrollBarSlider;
			bool pressed = active && (slider->state & State_Sunken);
			painter->fillRect(
				thumb,
				pressed  ? QColor(96, 96, 96)
				: active ? QColor(166, 166, 166)
						 : QColor(205, 205, 205)
			);
		}
	}

private:
	static void drawArrow(QPainter* painter, int left, int top, bool up, const QColor& color)
	{
		static const unsigned char coverage[browserScrollArrowHeight][browserScrollArrowWidth] = {
			{0, 0, 0, 0, 78, 230, 78, 0, 0, 0, 0},
			{0, 0, 0, 78, 243, 255, 243, 78, 0, 0, 0},
			{0, 0, 78, 243, 255, 255, 255, 243, 78, 0, 0},
			{0, 78, 243, 255, 255, 230, 255, 255, 243, 78, 0},
			{78, 243, 255, 255, 177, 0, 177, 255, 255, 243, 78},
			{243, 255, 255, 177, 0, 0, 0, 177, 255, 255, 243},
			{255, 255, 177, 0, 0, 0, 0, 0, 177, 255, 255},
			{255, 177, 0, 0, 0, 0, 0, 0, 0, 177, 255},
			{177, 0, 0, 0, 0, 0, 0, 0, 0, 0, 177},
		};

		for (int row = 0; row < browserScrollArrowHeight; row++) {
			for (int column = 0; column < browserScrollArrowWidth; column++) {
				int alpha = coverage[up ? row : browserScrollArrowHeight - 1 - row][column];
				if (alpha != 0) {
					painter->fillRect(
						left + column, top + row, 1, 1, QColor(color.red(), color.green(), color.blue(), alpha)
					);
				}
			}
		}
	}
};

void installBrowserScrollBar(QAbstractScrollArea* view)
{
	BrowserScrollBarStyle* style = new BrowserScrollBarStyle(view->style()->name());
	style->setParent(view);
	view->verticalScrollBar()->setStyle(style);
	view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}
