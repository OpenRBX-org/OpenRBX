#ifndef PREVIEWVIEW_H
#define PREVIEWVIEW_H

#include "DocView.h"
#include "Printing.h"

#include <QImage>
#include <QRect>
#include <QSize>
#include <memory>

class CDialog;
class QScrollBar;

class CPreviewView : public View
{
public:
	explicit CPreviewView(View* origView);
	~CPreviewView() override;

	QWidget* toolBar() const;
	bool setPrintView(View* printView);
	void onPreviewClose();

	void activate(bool active, View* other) override;
	void updateCommandUI() override;

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	enum
	{
		ZOOM_OUT,
		ZOOM_MIDDLE,
		ZOOM_IN
	};

	struct PageInfo
	{
		QRect rectScreen;
		QSize sizeUnscaled;
		QSize sizeScaleRatio;
		QSize sizeZoomOutRatio;
	};

	QSize calcScaleRatio(QSize screenSize, QSize actualSize) const;
	void positionPage(unsigned page);
	QSize calcPageDisplaySize();
	void setScaledSize(unsigned page);
	bool findPageRect(QPoint& point, unsigned& page);
	void setZoomState(unsigned newState, unsigned page, QPoint point);
	void doZoom(unsigned page, QPoint point);
	void setCurrentPage(unsigned page, bool clearRatios);
	void onDisplayPageNumber(unsigned page, unsigned pagesDisplayed);
	void onNumPageChange();
	void onNextPage();
	void onPrevPage();
	void onPreviewPrint();
	void onZoomIn();
	void onZoomOut();
	void onPageScroll(int action);

	QRect clientRect() const;
	void getTrueClientSize(QSize& size, QSize& sizeSb) const;
	void setScrollSizes(QSize total, QSize page);
	void updateBars();
	void showScrollBar(QScrollBar* bar, bool show);
	void setPageScrollRange();
	QPoint deviceScrollPosition() const;
	void centerOnPoint(QPoint center);
	bool scrollBy(QSize scroll);
	void layoutScrollBars();
	void onSize(int cx, int cy);
	void updateCursor(QPoint point);

	CDialog* m_toolBar;
	View* m_printView = nullptr;
	View* m_origView;
	std::unique_ptr<PrintInfo> m_previewInfo;
	PrintDC m_previewDC;
	QImage m_attrib;
	QScrollBar* m_horizontal;
	QScrollBar* m_vertical;
	bool m_horizontalVisible = false;
	bool m_verticalVisible = false;
	bool m_insideUpdate = false;
	QSize m_totalDev;
	QSize m_pageDev;
	QSize m_lineDev;
	QSize m_sizePrinterPPI;
	QSize m_sizeScreenPPI;
	unsigned m_pages = 1;
	unsigned m_zoomOutPages = 1;
	unsigned m_maxPages = 2;
	unsigned m_currentPage = 1;
	unsigned m_zoomState = ZOOM_OUT;
	int m_secondPageOffset = 0;
	PageInfo m_pageInfo[2];
};

#endif // PREVIEWVIEW_H
