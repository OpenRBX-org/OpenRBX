#include "PreviewView.h"

#include "ChildFrm.h"
#include "Dialog.h"
#include "MainFrm.h"
#include "Resources.h"
#include "Roblox.h"
#include "WhidbeyStyle.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QShortcut>

#define NOMINMAX
#include <windows.h>

static const unsigned previewToolBarId = 30723;
static const unsigned previewCloseId = 58112;
static const unsigned previewNumPageId = 58113;
static const unsigned previewNextId = 58114;
static const unsigned previewPrevId = 58115;
static const unsigned previewPrintId = 58116;
static const unsigned previewZoomInId = 58117;
static const unsigned previewZoomOutId = 58118;
static const unsigned onePageStringId = 61505;
static const unsigned twoPageStringId = 61506;
static const unsigned magnifyCursorId = 30978;
static const unsigned filePrintId = 57607;
static const unsigned toolBarButtons[] = {
	previewPrintId,
	previewNextId,
	previewPrevId,
	previewNumPageId,
	previewZoomInId,
	previewZoomOutId,
	previewCloseId,
};
static const int previewMargin = 8;

static long multMultDivDiv(int factor, int num1, int num2, int den1, int den2)
{
	long long numerator = static_cast<long long>(num1) * num2;
	long long denominator = static_cast<long long>(den1) * den2;
	if (denominator < 0) {
		denominator = -denominator;
		numerator = -numerator;
	}
	numerator *= factor;
	if (denominator == 0) {
		return numerator < 0 ? LONG_MIN : LONG_MAX;
	}
	return static_cast<long>((numerator + denominator / 2) / denominator);
}

namespace {

class PreviewToolBar : public CDialog
{
public:
	PreviewToolBar(CPreviewView* view) : CDialog(previewToolBarId, nullptr, CDialog::ChildPage), m_view(view)
	{
		setFixedHeight(height());
		setAutoFillBackground(true);
		setMinimumWidth(0);
		setMaximumWidth(QWIDGETSIZE_MAX);
		SendMessageW(
			reinterpret_cast<HWND>(theApp.mainFrame()->winId()), WM_CHANGEUISTATE, MAKEWPARAM(UIS_INITIALIZE, 0), 0
		);
	}

protected:
	void OnCommand(int id) override
	{
		CommandRoute route = {m_view, m_view->document(), theApp.mainFrame(), &theApp};
		routeCommand(route, id);
	}

	void paintEvent(QPaintEvent* event) override
	{
		CDialog::paintEvent(event);
		QPainter painter(this);
		COLORREF shadow = GetSysColor(COLOR_BTNSHADOW);
		COLORREF highlight = GetSysColor(COLOR_BTNHIGHLIGHT);
		painter.fillRect(QRect(0, 0, width(), 1), QColor(GetRValue(shadow), GetGValue(shadow), GetBValue(shadow)));
		painter.fillRect(
			QRect(0, 1, width(), 1), QColor(GetRValue(highlight), GetGValue(highlight), GetBValue(highlight))
		);
	}

private:
	CPreviewView* m_view;
};

} // namespace

CPreviewView::CPreviewView(View* origView)
	: m_toolBar(new PreviewToolBar(this)), m_origView(origView), m_horizontal(new QScrollBar(Qt::Horizontal, this)),
	  m_vertical(new QScrollBar(Qt::Vertical, this))
{
	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);
	m_horizontal->hide();
	m_vertical->hide();

	HDC screen = GetDC(nullptr);
	m_sizeScreenPPI = QSize(GetDeviceCaps(screen, LOGPIXELSX), GetDeviceCaps(screen, LOGPIXELSY));
	ReleaseDC(nullptr, screen);

	auto scrollAction = [this](QScrollBar* bar, int action) {
		if (m_zoomState == ZOOM_OUT) {
			if (bar == m_vertical) {
				onPageScroll(action);
			}
			return;
		}
		int step = bar == m_vertical ? m_pageDev.height() : m_pageDev.width();
		if (action == QAbstractSlider::SliderPageStepAdd) {
			bar->setSliderPosition(qMin(bar->value() + step, bar->maximum()));
		}
		else if (action == QAbstractSlider::SliderPageStepSub) {
			bar->setSliderPosition(qMax(bar->value() - step, 0));
		}
	};
	connect(m_vertical, &QScrollBar::actionTriggered, this, [this, scrollAction](int action) {
		scrollAction(m_vertical, action);
	});
	connect(m_horizontal, &QScrollBar::actionTriggered, this, [this, scrollAction](int action) {
		scrollAction(m_horizontal, action);
	});
	connect(m_vertical, &QScrollBar::sliderReleased, this, [this]() {
		if (m_zoomState == ZOOM_OUT) {
			setCurrentPage(m_vertical->sliderPosition(), true);
		}
	});
	connect(m_vertical, &QScrollBar::valueChanged, this, [this]() {
		if (m_zoomState != ZOOM_OUT) {
			update();
		}
	});
	connect(m_horizontal, &QScrollBar::valueChanged, this, [this]() {
		if (m_zoomState != ZOOM_OUT) {
			update();
		}
	});

	addCommand(previewCloseId, [this]() { onPreviewClose(); });
	addCommand(previewNumPageId, [this]() { onNumPageChange(); });
	addCommand(previewNextId, [this]() { onNextPage(); });
	addCommand(previewPrevId, [this]() { onPrevPage(); });
	addCommand(previewPrintId, [this]() { onPreviewPrint(); });
	addCommand(previewZoomInId, [this]() { onZoomIn(); });
	addCommand(previewZoomOutId, [this]() { onZoomOut(); });

	addCommandUpdate(previewNumPageId, [this](CommandState& state) {
		unsigned pages = m_zoomState == ZOOM_OUT ? m_pages : m_zoomOutPages;
		state.text = Resources::string(pages == 1 ? twoPageStringId : onePageStringId);
		state.enabled = m_zoomState == ZOOM_OUT && m_maxPages != 1 && (m_previewInfo->maxPage() > 1 || m_pages > 1);
	});
	addCommandUpdate(previewNextId, [this](CommandState& state) {
		state.enabled = m_currentPage + m_pages - 1 < m_previewInfo->maxPage();
	});
	addCommandUpdate(previewPrevId, [this](CommandState& state) {
		state.enabled = m_currentPage > m_previewInfo->minPage();
	});
	addCommandUpdate(previewZoomInId, [this](CommandState& state) { state.enabled = m_zoomState != ZOOM_IN; });
	addCommandUpdate(previewZoomOutId, [this](CommandState& state) { state.enabled = m_zoomState != ZOOM_OUT; });
}

CPreviewView::~CPreviewView()
{
	delete m_toolBar;
}

QWidget* CPreviewView::toolBar() const
{
	return m_toolBar;
}

bool CPreviewView::setPrintView(View* printView)
{
	m_printView = printView;

	m_previewInfo = std::make_unique<PrintInfo>();
	m_previewInfo->setPreview();

	if (!m_printView->onPreparePrinting(*m_previewInfo)) {
		return false;
	}

	HDC printer = m_previewInfo->printerDC();
	m_attrib = QImage(1, 1, QImage::Format_RGB32);
	m_attrib.setDotsPerMeterX(qRound(GetDeviceCaps(printer, LOGPIXELSX) / 0.0254));
	m_attrib.setDotsPerMeterY(qRound(GetDeviceCaps(printer, LOGPIXELSY) / 0.0254));
	m_previewDC.attrib = &m_attrib;
	m_previewDC.resolution = QSize(GetDeviceCaps(printer, HORZRES), GetDeviceCaps(printer, VERTRES));
	m_previewDC.font = Printing::deviceFont(printer);

	QImage prepareTarget(1, 1, QImage::Format_RGB32);
	prepareTarget.setDotsPerMeterX(m_attrib.dotsPerMeterX());
	prepareTarget.setDotsPerMeterY(m_attrib.dotsPerMeterY());
	QPainter preparePainter(&prepareTarget);
	PrintDC beginDC = m_previewDC;
	beginDC.painter = &preparePainter;
	m_printView->onBeginPrinting(beginDC, *m_previewInfo);
	preparePainter.end();

	m_sizePrinterPPI = QSize(GetDeviceCaps(printer, LOGPIXELSX), GetDeviceCaps(printer, LOGPIXELSY));

	m_pages = m_previewInfo->numPreviewPages;
	if (m_pages == 0) {
		m_pages = 1;
	}
	else if (m_pages > m_maxPages) {
		m_pages = m_maxPages;
	}

	m_zoomOutPages = m_pages;

	setScrollSizes(QSize(1, 1), QSize());

	setPageScrollRange();

	setCurrentPage(m_previewInfo->curPage, true);

	for (int key : {Qt::Key_PageDown, Qt::Key_PageUp, Qt::Key_Escape}) {
		QShortcut* shortcut =
			new QShortcut(QKeySequence(key), childFrame(), nullptr, nullptr, Qt::WidgetWithChildrenShortcut);
		unsigned id = key == Qt::Key_PageDown ? previewNextId : key == Qt::Key_PageUp ? previewPrevId : previewCloseId;
		connect(shortcut, &QShortcut::activated, this, [this, id]() {
			CommandRoute route = {this, document(), theApp.mainFrame(), &theApp};
			routeCommand(route, id);
		});
		connect(this, &QObject::destroyed, shortcut, &QObject::deleteLater);
	}
	return true;
}

void CPreviewView::setPageScrollRange()
{
	bool show = m_previewInfo->maxPage() < 0x8000 && m_previewInfo->maxPage() - m_previewInfo->minPage() <= 32767U;
	showScrollBar(m_vertical, show);
	if (show) {
		m_vertical->setRange(m_previewInfo->minPage(), m_previewInfo->maxPage());
		m_vertical->setPageStep(1);
		m_vertical->setSingleStep(1);
		showScrollBar(m_vertical, m_previewInfo->maxPage() > m_previewInfo->minPage());
	}
}

void CPreviewView::activate(bool active, View* other)
{
	if (active) {
		QWidget* focus = QApplication::focusWidget();
		if (!focus || !m_toolBar->isAncestorOf(focus)) {
			m_toolBar->GetDlgItem(previewPrintId)->setFocus();
		}
	}
}

void CPreviewView::updateCommandUI()
{
	if (!m_previewInfo) {
		return;
	}
	CommandRoute route = {this, document(), theApp.mainFrame(), &theApp};
	for (unsigned id : toolBarButtons) {
		CommandState state;
		state.enabled = routeCommandUpdate(route, id, state) && state.enabled;
		QPushButton* button = static_cast<QPushButton*>(m_toolBar->GetDlgItem(id));
		button->setEnabled(state.enabled);
		if (state.text) {
			button->setText(*state.text);
		}
	}
}

void CPreviewView::onPreviewClose()
{
	m_toolBar->hide();
	m_previewInfo->curPage = m_currentPage;
	QImage prepareTarget(1, 1, QImage::Format_RGB32);
	prepareTarget.setDotsPerMeterX(m_attrib.dotsPerMeterX());
	prepareTarget.setDotsPerMeterY(m_attrib.dotsPerMeterY());
	QPainter preparePainter(&prepareTarget);
	PrintDC endDC = m_previewDC;
	endDC.painter = &preparePainter;
	m_origView->onEndPrintPreview(endDC, *m_previewInfo, this);
}

QSize CPreviewView::calcScaleRatio(QSize screenSize, QSize actualSize) const
{
	int num = screenSize.height();
	int den = actualSize.height();

	if (MulDiv(actualSize.width(), num, den) > screenSize.width()) {
		num = screenSize.width();
		den = actualSize.width();
	}
	return QSize(num, den);
}

void CPreviewView::positionPage(unsigned page)
{
	QSize windowSize = calcPageDisplaySize();

	HDC printer = m_previewInfo->printerDC();
	QSize& size = m_pageInfo[page].sizeUnscaled;
	size = QSize(GetDeviceCaps(printer, PHYSICALWIDTH), GetDeviceCaps(printer, PHYSICALHEIGHT));

	size.setWidth(MulDiv(size.width(), m_sizeScreenPPI.width(), m_sizePrinterPPI.width()));
	size.setHeight(MulDiv(size.height(), m_sizeScreenPPI.height(), m_sizePrinterPPI.height()));

	m_pageInfo[page].sizeZoomOutRatio = calcScaleRatio(windowSize, size);

	setScaledSize(page);
}

QSize CPreviewView::calcPageDisplaySize()
{
	QSize windowSize, scrollSize;
	getTrueClientSize(windowSize, scrollSize);

	if (m_zoomState == ZOOM_OUT && m_previewInfo->maxPage() != 0xffff &&
		m_previewInfo->maxPage() - m_previewInfo->minPage() != 0) {
		windowSize.rwidth() -= scrollSize.width();
	}

	m_secondPageOffset = (windowSize.width() - previewMargin) / 2;

	windowSize.setWidth(
		m_pages == 2 ? (windowSize.width() - 3 * previewMargin) / 2 : windowSize.width() - 2 * previewMargin
	);

	windowSize.rheight() -= 2 * previewMargin;
	return windowSize;
}

void CPreviewView::setScaledSize(unsigned page)
{
	QSize& size = m_pageInfo[page].sizeUnscaled;
	QSize& ratio = m_pageInfo[page].sizeScaleRatio;
	QSize& zoomOutRatio = m_pageInfo[page].sizeZoomOutRatio;
	QSize windowSize = calcPageDisplaySize();
	bool paperLarger = zoomOutRatio.width() < zoomOutRatio.height();

	switch (m_zoomState) {
	case ZOOM_OUT:
		ratio = zoomOutRatio;
		break;

	case ZOOM_MIDDLE:
		if (paperLarger) {
			ratio.setHeight(zoomOutRatio.height());
			ratio.setWidth((zoomOutRatio.width() + ratio.height()) / 2);
		}
		else {
			ratio.setHeight(zoomOutRatio.height());
			ratio.setWidth((3 * zoomOutRatio.width() - ratio.height()) / 2);
		}
		break;

	case ZOOM_IN:
		if (paperLarger) {
			ratio = QSize(1, 1);
		}
		else {
			ratio.setHeight(zoomOutRatio.height());
			ratio.setWidth(2 * zoomOutRatio.width() - zoomOutRatio.height());
		}
		break;
	}

	QSize scaledSize(
		MulDiv(size.width(), ratio.width(), ratio.height()), MulDiv(size.height(), ratio.width(), ratio.height())
	);

	QRect& rect = m_pageInfo[page].rectScreen;
	rect = QRect(
		QPoint(previewMargin, previewMargin),
		QPoint(scaledSize.width() + previewMargin + 3 - 1, scaledSize.height() + previewMargin + 3 - 1)
	);

	if (m_zoomState == ZOOM_OUT) {
		rect.translate((windowSize.width() - rect.width()) / 2 - 1, (windowSize.height() - rect.height()) / 2 - 1);

		if (page == 1) {
			rect.translate(m_secondPageOffset, 0);
		}
	}
	else {
		setScrollSizes(rect.size() + QSize(previewMargin * 2, previewMargin * 2), windowSize);
	}
}

void CPreviewView::paintEvent(QPaintEvent* event)
{
	QPainter widgetPainter(this);
	WhidbeyStyle::drawClientEdge(&widgetPainter, rect());
	if (m_horizontalVisible && m_verticalVisible) {
		COLORREF face = GetSysColor(COLOR_BTNFACE);
		widgetPainter.fillRect(
			QRect(m_vertical->x(), m_horizontal->y(), m_vertical->width(), m_horizontal->height()),
			QColor(GetRValue(face), GetGValue(face), GetBValue(face))
		);
	}

	QRect client = clientRect();
	if (client.isEmpty()) {
		return;
	}

	BITMAPINFO bitmap = {};
	bitmap.bmiHeader.biSize = sizeof(bitmap.bmiHeader);
	bitmap.bmiHeader.biWidth = client.width();
	bitmap.bmiHeader.biHeight = -client.height();
	bitmap.bmiHeader.biPlanes = 1;
	bitmap.bmiHeader.biBitCount = 32;
	bitmap.bmiHeader.biCompression = BI_RGB;
	void* bits = nullptr;
	HDC screen = GetDC(nullptr);
	HDC dc = CreateCompatibleDC(screen);
	HBITMAP dib = CreateDIBSection(screen, &bitmap, DIB_RGB_COLORS, &bits, nullptr, 0);
	ReleaseDC(nullptr, screen);
	if (dib == nullptr) {
		DeleteDC(dc);
		return;
	}
	HGDIOBJ oldBitmap = SelectObject(dc, dib);
	QImage image(static_cast<uchar*>(bits), client.width(), client.height(), QImage::Format_RGB32);
	image.setDotsPerMeterX(m_attrib.dotsPerMeterX());
	image.setDotsPerMeterY(m_attrib.dotsPerMeterY());

	RECT background = {0, 0, client.width(), client.height()};
	FillRect(dc, &background, GetSysColorBrush(COLOR_APPWORKSPACE));

	if (m_printView != nullptr && m_previewInfo->printerDC() != nullptr) {
		QPoint viewportOrg(0, 0);
		if (m_zoomState != ZOOM_OUT && m_pageInfo[0].sizeScaleRatio.width() != 0) {
			viewportOrg = -deviceScrollPosition();
			if (m_totalDev.width() < client.width()) {
				viewportOrg.setX((client.width() - m_totalDev.width()) / 2);
			}
			if (m_totalDev.height() < client.height()) {
				viewportOrg.setY((client.height() - m_totalDev.height()) / 2);
			}
		}

		HPEN rectPen = CreatePen(PS_SOLID, 2, GetSysColor(COLOR_WINDOWFRAME));
		HPEN shadowPen = CreatePen(PS_SOLID, 3, GetSysColor(COLOR_BTNSHADOW));

		QImage prepareTarget(1, 1, QImage::Format_RGB32);
		prepareTarget.setDotsPerMeterX(m_attrib.dotsPerMeterX());
		prepareTarget.setDotsPerMeterY(m_attrib.dotsPerMeterY());

		m_previewInfo->continuePrinting = true;

		for (unsigned page = 0; page < m_pages; page++) {
			m_previewInfo->curPage = m_currentPage + page;

			if (m_currentPage + page >= m_currentPage && m_currentPage + page >= page &&
				m_currentPage + page <= m_previewInfo->maxPage()) {
				QPainter preparePainter(&prepareTarget);
				PrintDC prepareDC = m_previewDC;
				prepareDC.painter = &preparePainter;
				m_printView->onPrepareDC(prepareDC, m_previewInfo.get());
			}

			m_previewInfo->rectDraw = QRect(QPoint(0, 0), m_previewDC.resolution);

			QSize& ratio = m_pageInfo[page].sizeScaleRatio;
			QRect& rect = m_pageInfo[page].rectScreen;

			if (ratio.width() == 0) {
				positionPage(page);
				if (m_zoomState != ZOOM_OUT) {
					viewportOrg = -deviceScrollPosition();
					if (m_totalDev.width() < client.width()) {
						viewportOrg.setX((client.width() - m_totalDev.width()) / 2);
					}
					if (m_totalDev.height() < client.height()) {
						viewportOrg.setY((client.height() - m_totalDev.height()) / 2);
					}
				}
			}

			SetViewportOrgEx(dc, viewportOrg.x(), viewportOrg.y(), nullptr);

			HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
			HGDIOBJ oldPen = SelectObject(dc, rectPen);
			Rectangle(dc, rect.left(), rect.top(), rect.right() + 1, rect.bottom() + 1);

			SelectObject(dc, shadowPen);

			MoveToEx(dc, rect.right() + 1 + 1, rect.top() + 3, nullptr);
			LineTo(dc, rect.right() + 1 + 1, rect.bottom() + 1 + 1);
			MoveToEx(dc, rect.left() + 3, rect.bottom() + 1 + 1, nullptr);
			LineTo(dc, rect.right() + 1 + 1, rect.bottom() + 1 + 1);

			RECT fill = {rect.left() + 1, rect.top() + 1, rect.right() + 1 - 2, rect.bottom() + 1 - 2};
			FillRect(dc, &fill, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));

			SelectObject(dc, oldPen);
			SelectObject(dc, oldBrush);
			SetViewportOrgEx(dc, 0, 0, nullptr);

			if (!m_previewInfo->continuePrinting || m_currentPage + page > m_previewInfo->maxPage()) {
				if (page == 0 && m_currentPage > 1) {
					setCurrentPage(m_currentPage - 1, true);
				}
				break;
			}

			onDisplayPageNumber(m_currentPage, page + 1);

			HDC printer = m_previewInfo->printerDC();
			int vpExtX = static_cast<int>(
				multMultDivDiv(0x4000, ratio.width(), m_sizeScreenPPI.width(), ratio.height(), m_sizePrinterPPI.width())
			);
			int vpExtY = static_cast<int>(multMultDivDiv(
				0x4000, ratio.width(), m_sizeScreenPPI.height(), ratio.height(), m_sizePrinterPPI.height()
			));
			auto printerToScreen = [vpExtX, vpExtY](QPoint point) {
				return QPoint(
					static_cast<int>(multMultDivDiv(point.x(), 1, vpExtX, 1, 0x4000)),
					static_cast<int>(multMultDivDiv(point.y(), 1, vpExtY, 1, 0x4000))
				);
			};

			QPoint printOffset = printerToScreen(
				QPoint(GetDeviceCaps(printer, PHYSICALOFFSETX), GetDeviceCaps(printer, PHYSICALOFFSETY))
			);
			printOffset += rect.topLeft();
			printOffset += QPoint(1, 1);
			printOffset += viewportOrg;

			QPoint extent = printerToScreen(QPoint(m_previewDC.resolution.width(), m_previewDC.resolution.height()));

			GdiFlush();
			QPainter painter(&image);
			painter.setClipRect(QRect(printOffset + QPoint(-1, -1), printOffset + extent + QPoint(1, 1)));
			painter.translate(printOffset);
			painter.scale(vpExtX / double(0x4000), vpExtY / double(0x4000));
			PrintDC pageDC = m_previewDC;
			pageDC.painter = &painter;
			m_printView->onPrint(pageDC, *m_previewInfo);
			painter.end();
		}

		DeleteObject(rectPen);
		DeleteObject(shadowPen);
	}

	GdiFlush();
	QRgb* pixels = reinterpret_cast<QRgb*>(bits);
	for (qsizetype i = 0, count = qsizetype(client.width()) * client.height(); i < count; i++) {
		pixels[i] |= 0xff000000;
	}
	widgetPainter.drawImage(client.topLeft(), image);

	SelectObject(dc, oldBitmap);
	DeleteObject(dib);
	DeleteDC(dc);
}

void CPreviewView::onPageScroll(int action)
{
	unsigned step = (m_previewInfo->maxPage() - m_previewInfo->minPage() + 9) / 10;
	switch (action) {
	case QAbstractSlider::SliderToMaximum:
		setCurrentPage(m_previewInfo->maxPage(), true);
		break;
	case QAbstractSlider::SliderToMinimum:
		setCurrentPage(m_previewInfo->minPage(), true);
		break;
	case QAbstractSlider::SliderPageStepAdd:
		setCurrentPage(m_currentPage + step, true);
		break;
	case QAbstractSlider::SliderPageStepSub:
		setCurrentPage(m_currentPage - step, true);
		break;
	case QAbstractSlider::SliderSingleStepAdd:
		setCurrentPage(m_currentPage + 1, true);
		break;
	case QAbstractSlider::SliderSingleStepSub:
		setCurrentPage(m_currentPage - 1, true);
		break;
	default:
		return;
	}
	m_vertical->setSliderPosition(m_currentPage);
}

void CPreviewView::onNumPageChange()
{
	m_pages = 3 - m_pages;
	Printing::setNumPreviewPages(m_pages);
	m_zoomOutPages = m_pages;

	setCurrentPage(m_currentPage, true);
}

void CPreviewView::onNextPage()
{
	setCurrentPage(m_currentPage + 1, true);
}

void CPreviewView::onPrevPage()
{
	setCurrentPage(m_currentPage - 1, true);
}

void CPreviewView::onPreviewPrint()
{
	CChildFrame* frame = m_origView->childFrame();
	onPreviewClose();
	CommandRoute route = {frame->activeView(), frame->activeView()->document(), theApp.mainFrame(), &theApp};
	routeCommand(route, filePrintId);
}

bool CPreviewView::findPageRect(QPoint& point, unsigned& page)
{
	if (m_zoomState != ZOOM_OUT) {
		point += deviceScrollPosition();
	}

	for (page = 0; page < m_pages; page++) {
		const QRect& rect = m_pageInfo[page].rectScreen;
		if (point.x() >= rect.left() && point.x() <= rect.right() && point.y() >= rect.top() &&
			point.y() <= rect.bottom()) {
			point -= rect.topLeft();

			point.setX(
				MulDiv(point.x(), m_pageInfo[page].sizeScaleRatio.height(), m_pageInfo[page].sizeScaleRatio.width())
			);
			point.setY(
				MulDiv(point.y(), m_pageInfo[page].sizeScaleRatio.height(), m_pageInfo[page].sizeScaleRatio.width())
			);
			return true;
		}
	}
	return false;
}

void CPreviewView::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton) {
		return;
	}
	QPoint point = event->position().toPoint() - clientRect().topLeft();
	unsigned page;
	if (!findPageRect(point, page)) {
		return;
	}

	setZoomState(m_zoomState == ZOOM_IN ? ZOOM_OUT : m_zoomState + 1, page, point);
	updateCursor(event->position().toPoint());
}

void CPreviewView::mouseMoveEvent(QMouseEvent* event)
{
	updateCursor(event->position().toPoint());
}

void CPreviewView::updateCursor(QPoint position)
{
	if (!clientRect().contains(position)) {
		unsetCursor();
		return;
	}
	QPoint point = position - clientRect().topLeft();
	unsigned page;
	if (m_zoomState != ZOOM_IN && findPageRect(point, page)) {
		setCursor(Resources::cursor(magnifyCursorId));
	}
	else {
		setCursor(Qt::ArrowCursor);
	}
}

void CPreviewView::setZoomState(unsigned newState, unsigned page, QPoint point)
{
	if (m_zoomState != newState) {
		m_zoomState = newState;
		doZoom(page, point);
	}
}

void CPreviewView::onZoomIn()
{
	if (m_zoomState != ZOOM_IN) {
		setZoomState(m_zoomState + 1, 0, QPoint(0, 0));
	}
}

void CPreviewView::onZoomOut()
{
	if (m_zoomState != ZOOM_OUT) {
		setZoomState(m_zoomState - 1, 0, QPoint(0, 0));
	}
}

void CPreviewView::doZoom(unsigned page, QPoint point)
{
	if (m_zoomState == ZOOM_OUT) {
		m_pages = m_zoomOutPages;
		showScrollBar(m_horizontal, false);
		setPageScrollRange();
		setCurrentPage(m_currentPage, true);
	}
	else {
		m_pages = 1;

		m_pageInfo[0].sizeZoomOutRatio = m_pageInfo[page].sizeZoomOutRatio;
		m_pageInfo[0].sizeUnscaled = m_pageInfo[page].sizeUnscaled;

		setCurrentPage(m_currentPage + page, false);

		setScaledSize(0);

		QSize& ratio = m_pageInfo[page].sizeScaleRatio;

		point.setX(MulDiv(point.x(), ratio.width(), ratio.height()));
		point.setY(MulDiv(point.y(), ratio.width(), ratio.height()));

		point += m_pageInfo[0].rectScreen.topLeft();

		centerOnPoint(point);
	}
}

void CPreviewView::setCurrentPage(unsigned page, bool clearRatios)
{
	m_currentPage = page;
	if (m_currentPage > m_previewInfo->maxPage()) {
		m_currentPage = m_previewInfo->maxPage();
	}
	if (m_currentPage < m_previewInfo->minPage()) {
		m_currentPage = m_previewInfo->minPage();
	}

	if (m_zoomState == ZOOM_OUT) {
		m_vertical->setValue(m_currentPage);
	}

	if (clearRatios) {
		for (unsigned i = 0; i < m_maxPages; i++) {
			m_pageInfo[i].sizeScaleRatio.setWidth(0);
		}
	}

	update();
}

void CPreviewView::onDisplayPageNumber(unsigned page, unsigned pagesDisplayed)
{
	unsigned endPage = page + pagesDisplayed - 1;
	int subString = pagesDisplayed == 1 ? 0 : 1;
	QString text = m_previewInfo->pageDesc.section(QLatin1Char('\n'), subString, subString);
	text.replace(text.indexOf("%u"), 2, QString::number(page));
	if (subString != 0) {
		text.replace(text.indexOf("%u"), 2, QString::number(endPage));
	}
	theApp.mainFrame()->setMessageText(text);
}

QRect CPreviewView::clientRect() const
{
	QRect client = rect().adjusted(
		WhidbeyStyle::clientEdgeWidth,
		WhidbeyStyle::clientEdgeWidth,
		-WhidbeyStyle::clientEdgeWidth,
		-WhidbeyStyle::clientEdgeWidth
	);
	if (m_verticalVisible) {
		client.setRight(client.right() - GetSystemMetrics(SM_CXVSCROLL));
	}
	if (m_horizontalVisible) {
		client.setBottom(client.bottom() - GetSystemMetrics(SM_CYHSCROLL));
	}
	return client;
}

void CPreviewView::getTrueClientSize(QSize& size, QSize& sizeSb) const
{
	size = clientRect().size();
	sizeSb = QSize(GetSystemMetrics(SM_CXVSCROLL), GetSystemMetrics(SM_CYHSCROLL));
	if (m_verticalVisible) {
		size.rwidth() += sizeSb.width();
	}
	if (m_horizontalVisible) {
		size.rheight() += sizeSb.height();
	}
}

void CPreviewView::setScrollSizes(QSize total, QSize page)
{
	m_totalDev = total;
	m_pageDev = page;
	m_lineDev = QSize();

	if (m_pageDev.width() == 0) {
		m_pageDev.setWidth(m_totalDev.width() / 10);
	}
	if (m_pageDev.height() == 0) {
		m_pageDev.setHeight(m_totalDev.height() / 10);
	}
	if (m_lineDev.width() == 0) {
		m_lineDev.setWidth(m_pageDev.width() / 10);
	}
	if (m_lineDev.height() == 0) {
		m_lineDev.setHeight(m_pageDev.height() / 10);
	}

	updateBars();
}

void CPreviewView::updateBars()
{
	if (m_insideUpdate) {
		return;
	}
	m_insideUpdate = true;

	QSize sizeClient, sizeSb;
	getTrueClientSize(sizeClient, sizeSb);
	if (!(sizeClient.width() > sizeSb.width() && sizeClient.height() > sizeSb.height())) {
		QRect client = clientRect();
		if (client.right() + 1 > 0 && client.bottom() + 1 > 0) {
			showScrollBar(m_horizontal, false);
			showScrollBar(m_vertical, false);
		}
		m_insideUpdate = false;
		return;
	}

	QSize sizeRange = m_totalDev - sizeClient;
	QPoint move = deviceScrollPosition();

	bool needH = sizeRange.width() > 0;
	if (!needH) {
		move.setX(0);
	}
	else {
		sizeRange.rheight() += sizeSb.height();
	}

	bool needV = sizeRange.height() > 0;
	if (!needV) {
		move.setY(0);
	}
	else {
		sizeRange.rwidth() += sizeSb.width();
	}

	if (needV && !needH && sizeRange.width() > 0) {
		needH = true;
		sizeRange.rheight() += sizeSb.height();
	}

	if (sizeRange.width() > 0 && move.x() >= sizeRange.width()) {
		move.setX(sizeRange.width());
	}
	if (sizeRange.height() > 0 && move.y() >= sizeRange.height()) {
		move.setY(sizeRange.height());
	}

	if (needH) {
		sizeClient.rheight() -= sizeSb.height();
	}
	if (needV) {
		sizeClient.rwidth() -= sizeSb.width();
	}

	m_horizontal->setValue(move.x());
	m_vertical->setValue(move.y());
	update();

	showScrollBar(m_horizontal, needH);
	if (needH) {
		m_horizontal->setPageStep(sizeClient.width());
		m_horizontal->setSingleStep(m_lineDev.width());
		m_horizontal->setRange(0, qMax(0, m_totalDev.width() - sizeClient.width()));
	}
	showScrollBar(m_vertical, needV);
	if (needV) {
		m_vertical->setPageStep(sizeClient.height());
		m_vertical->setSingleStep(m_lineDev.height());
		m_vertical->setRange(0, qMax(0, m_totalDev.height() - sizeClient.height()));
	}

	m_insideUpdate = false;
}

void CPreviewView::showScrollBar(QScrollBar* bar, bool show)
{
	bool& visible = bar == m_vertical ? m_verticalVisible : m_horizontalVisible;
	if (visible == show) {
		return;
	}
	visible = show;
	bar->setVisible(show);
	layoutScrollBars();
	QSize client = clientRect().size();
	onSize(client.width(), client.height());
}

QPoint CPreviewView::deviceScrollPosition() const
{
	return QPoint(m_horizontal->value(), m_vertical->value());
}

void CPreviewView::centerOnPoint(QPoint center)
{
	QRect client = clientRect();

	int xDesired = center.x() - client.width() / 2;
	int yDesired = center.y() - client.height() / 2;

	if (!m_horizontalVisible || xDesired < 0) {
		xDesired = 0;
	}
	else if (xDesired > m_horizontal->maximum()) {
		xDesired = m_horizontal->maximum();
	}

	if (!m_verticalVisible || yDesired < 0) {
		yDesired = 0;
	}
	else if (yDesired > m_vertical->maximum()) {
		yDesired = m_vertical->maximum();
	}

	m_horizontal->setValue(xDesired);
	m_vertical->setValue(yDesired);
}

bool CPreviewView::scrollBy(QSize scroll)
{
	if (!m_verticalVisible) {
		scroll.setHeight(0);
	}
	if (!m_horizontalVisible) {
		scroll.setWidth(0);
	}

	int xOrig = m_horizontal->value();
	int x = qBound(0, xOrig + scroll.width(), m_horizontal->maximum());
	int yOrig = m_vertical->value();
	int y = qBound(0, yOrig + scroll.height(), m_vertical->maximum());

	if (x == xOrig && y == yOrig) {
		return false;
	}

	m_horizontal->setValue(x);
	m_vertical->setValue(y);
	update();
	return true;
}

void CPreviewView::wheelEvent(QWheelEvent* event)
{
	if (event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier)) {
		return;
	}
	if (!m_verticalVisible && !m_horizontalVisible) {
		return;
	}

	UINT lines = 3;
	SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
	int delta = event->angleDelta().y();
	int toScroll = MulDiv(-delta, lines, WHEEL_DELTA);

	if (m_verticalVisible) {
		int displacement;
		if (lines == WHEEL_PAGESCROLL) {
			displacement = delta > 0 ? -m_pageDev.height() : m_pageDev.height();
		}
		else {
			displacement = qMin(toScroll * m_lineDev.height(), m_pageDev.height());
		}
		scrollBy(QSize(0, displacement));
	}
	else {
		int displacement;
		if (lines == WHEEL_PAGESCROLL) {
			displacement = delta > 0 ? -m_pageDev.width() : m_pageDev.width();
		}
		else {
			displacement = qMin(toScroll * m_lineDev.width(), m_pageDev.width());
		}
		scrollBy(QSize(displacement, 0));
	}
}

void CPreviewView::layoutScrollBars()
{
	QRect inner = rect().adjusted(
		WhidbeyStyle::clientEdgeWidth,
		WhidbeyStyle::clientEdgeWidth,
		-WhidbeyStyle::clientEdgeWidth,
		-WhidbeyStyle::clientEdgeWidth
	);
	int cx = GetSystemMetrics(SM_CXVSCROLL);
	int cy = GetSystemMetrics(SM_CYHSCROLL);
	m_vertical->setGeometry(inner.right() + 1 - cx, inner.top(), cx, inner.height() - (m_horizontalVisible ? cy : 0));
	m_horizontal->setGeometry(inner.left(), inner.bottom() + 1 - cy, inner.width() - (m_verticalVisible ? cx : 0), cy);
}

void CPreviewView::resizeEvent(QResizeEvent* event)
{
	View::resizeEvent(event);
	layoutScrollBars();
	QSize client = clientRect().size();
	onSize(client.width(), client.height());
}

void CPreviewView::onSize(int cx, int cy)
{
	if (m_zoomState == ZOOM_OUT) {
		for (unsigned i = 0; i < m_maxPages; i++) {
			m_pageInfo[i].sizeScaleRatio.setWidth(0);
		}
	}
	else {
		m_pageDev = QSize(cx, cy);
		m_lineDev = QSize(cx / 10, cy / 10);
		updateBars();
	}
	update();
}
