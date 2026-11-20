#include "Printing.h"

#include "ChildFrm.h"
#include "CommandIds.h"
#include "Dialog.h"
#include "DocView.h"
#include "MainFrm.h"
#include "Resources.h"
#include "Roblox.h"
#include "SystemMessageBox.h"

#include <QCoreApplication>
#include <QPainter>
#include <algorithm>

#ifdef _WIN32
#include "FileDialog.h"
#include "NativeWindow.h"
#include "PreviewView.h"

#include <QDir>
#include <QFileInfo>
#include <QImage>

#define NOMINMAX
#include <commdlg.h>
#include <dlgs.h>
#include <oleauto.h>
#include <windows.h>
#include <winspool.h>

#undef SetDlgItemText
#else
#include <QApplication>
#include <QDateTime>
#include <QLocale>
#include <QPageSetupDialog>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#endif

static const unsigned printingDialogId = 30722;
static const unsigned printDocNameId = 201;
static const unsigned printPrinterNameId = 202;
static const unsigned printPortNameId = 203;
static const unsigned printPageNumId = 204;
static const unsigned printPageNumStringId = 61507;
static const unsigned previewPageDescStringId = 61508;
static const unsigned failedToStartPrintStringId = 61702;
static const unsigned idleMessageStringId = 57345;

static unsigned previewPages = 0;
static bool userAbort = false;

class PrintingDialog : public CDialog
{
public:
	explicit PrintingDialog(QWidget* parent) : CDialog(printingDialogId, parent)
	{
		userAbort = false;
		setWindowTitle(Resources::string(appTitleStringId));
	}

protected:
	void OnCancel() override
	{
		userAbort = true;
		hide();
	}
};

#ifdef _WIN32
static const unsigned printDefaultExtStringId = 61509;
static const unsigned printDefaultStringId = 61510;
static const unsigned printFilterStringId = 61511;
static const unsigned printCaptionStringId = 61512;

static HGLOBAL devMode = nullptr;
static HGLOBAL devNames = nullptr;

static UINT_PTR CALLBACK commonDialogHook(HWND window, UINT message, WPARAM wParam, LPARAM)
{
	if (message == WM_INITDIALOG) {
		centerWindowOverOwner(window);
		return TRUE;
	}
	if (message == WM_COMMAND && LOWORD(wParam) == pshHelp) {
		return TRUE;
	}
	return FALSE;
}

static bool helpEnabled()
{
	return theApp.mainFrame()->hasCommand(ID_HELP) || theApp.hasCommand(ID_HELP);
}

class PrintDialog
{
public:
	PrintDialog(
		bool printSetupOnly,
		DWORD flags = PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOPAGENUMS | PD_HIDEPRINTTOFILE | PD_NOSELECTION
	)
	{
		memset(&m_pd, 0, sizeof(m_pd));
		m_pd.lStructSize = sizeof(m_pd);
		m_pd.Flags = flags | PD_ENABLEPRINTHOOK | PD_ENABLESETUPHOOK;
		if (helpEnabled()) {
			m_pd.Flags |= PD_SHOWHELP;
		}
		m_pd.lpfnPrintHook = commonDialogHook;
		m_pd.lpfnSetupHook = commonDialogHook;
		if (printSetupOnly) {
			m_pd.Flags |= PD_PRINTSETUP;
		}
		else {
			m_pd.Flags |= PD_RETURNDC;
		}
		m_pd.Flags &= ~PD_RETURNIC;
	}

	INT_PTR DoModal()
	{
		m_pd.hwndOwner = reinterpret_cast<HWND>(theApp.mainFrame()->winId());
		INT_PTR result = PrintDlgW(&m_pd);
		return result ? result : IDCANCEL;
	}

	bool GetDefaults()
	{
		m_pd.Flags |= PD_RETURNDEFAULT;
		return PrintDlgW(&m_pd) != FALSE;
	}

	QString devNamesString(WORD DEVNAMES::* offset) const
	{
		if (m_pd.hDevNames == nullptr) {
			return QString();
		}
		DEVNAMES* names = static_cast<DEVNAMES*>(GlobalLock(m_pd.hDevNames));
		QString text = QString::fromWCharArray(reinterpret_cast<const wchar_t*>(names) + names->*offset);
		GlobalUnlock(m_pd.hDevNames);
		return text;
	}

	QString GetDriverName() const { return devNamesString(&DEVNAMES::wDriverOffset); }
	QString GetDeviceName() const { return devNamesString(&DEVNAMES::wDeviceOffset); }
	QString GetPortName() const { return devNamesString(&DEVNAMES::wOutputOffset); }

	HDC CreatePrinterDC()
	{
		if (m_pd.hDevNames == nullptr) {
			return nullptr;
		}
		DEVNAMES* names = static_cast<DEVNAMES*>(GlobalLock(m_pd.hDevNames));
		DEVMODEW* mode = m_pd.hDevMode ? static_cast<DEVMODEW*>(GlobalLock(m_pd.hDevMode)) : nullptr;
		const wchar_t* base = reinterpret_cast<const wchar_t*>(names);
		m_pd.hDC =
			CreateDCW(base + names->wDriverOffset, base + names->wDeviceOffset, base + names->wOutputOffset, mode);
		if (mode) {
			GlobalUnlock(m_pd.hDevMode);
		}
		GlobalUnlock(m_pd.hDevNames);
		return m_pd.hDC;
	}

	PRINTDLGW m_pd;
};

PrintInfo::PrintInfo()
	: m_dialog(std::make_unique<PrintDialog>(false, PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOSELECTION))
{
	setMinPage(1);
	setMaxPage(0xffff);
}

PrintInfo::~PrintInfo()
{
	if (m_dialog->m_pd.hDC != nullptr) {
		DeleteDC(m_dialog->m_pd.hDC);
		m_dialog->m_pd.hDC = nullptr;
	}
}

unsigned PrintInfo::minPage() const
{
	return m_dialog->m_pd.nMinPage;
}

unsigned PrintInfo::maxPage() const
{
	return m_dialog->m_pd.nMaxPage;
}

void PrintInfo::setMinPage(unsigned page)
{
	m_dialog->m_pd.nMinPage = static_cast<WORD>(page);
}

void PrintInfo::setMaxPage(unsigned page)
{
	m_dialog->m_pd.nMaxPage = static_cast<WORD>(page);
}

unsigned PrintInfo::fromPage() const
{
	return (m_dialog->m_pd.Flags & PD_PAGENUMS) ? m_dialog->m_pd.nFromPage : minPage();
}

unsigned PrintInfo::toPage() const
{
	return (m_dialog->m_pd.Flags & PD_PAGENUMS) ? m_dialog->m_pd.nToPage : maxPage();
}

bool PrintInfo::printSelection() const
{
	return m_dialog->m_pd.Flags & PD_SELECTION;
}

void PrintInfo::checkSelection()
{
	m_dialog->m_pd.Flags &= ~PD_NOSELECTION;
	m_dialog->m_pd.Flags |= PD_SELECTION;
}

void PrintInfo::setPreview()
{
	m_dialog->m_pd.Flags |= PD_PRINTSETUP;
	m_dialog->m_pd.Flags &= ~PD_RETURNDC;
	preview = true;
}

HDC__* PrintInfo::printerDC() const
{
	return m_dialog->m_pd.hDC;
}

static void updatePrinterSelection(bool forceDefaults)
{
	if (!forceDefaults && devNames != nullptr) {
		DEVNAMES* names = static_cast<DEVNAMES*>(GlobalLock(devNames));
		if (names->wDefault & DN_DEFAULTPRN) {
			PrintDialog pd(true);
			if (!pd.GetDefaults()) {
				GlobalUnlock(devNames);
				return;
			}

			const wchar_t* base = reinterpret_cast<const wchar_t*>(names);
			if (pd.m_pd.hDevNames == nullptr) {
				GlobalUnlock(devNames);
				if (devMode != nullptr) {
					GlobalFree(devMode);
				}
				GlobalFree(devNames);
				devMode = nullptr;
				devNames = nullptr;
				return;
			}
			else if (
				pd.GetDriverName() != QString::fromWCharArray(base + names->wDriverOffset) ||
				pd.GetDeviceName() != QString::fromWCharArray(base + names->wDeviceOffset) ||
				pd.GetPortName() != QString::fromWCharArray(base + names->wOutputOffset)
			) {
				GlobalUnlock(devNames);
				if (devMode != nullptr) {
					GlobalFree(devMode);
				}
				GlobalFree(devNames);
				devMode = pd.m_pd.hDevMode;
				devNames = pd.m_pd.hDevNames;
				return;
			}
			else {
				if (pd.m_pd.hDevMode != nullptr) {
					GlobalFree(pd.m_pd.hDevMode);
				}
				if (pd.m_pd.hDevNames != nullptr) {
					GlobalFree(pd.m_pd.hDevNames);
				}
			}
		}
		GlobalUnlock(devNames);
	}
	else {
		PrintDialog pd(true);
		pd.GetDefaults();

		if (devMode != nullptr) {
			GlobalFree(devMode);
		}
		if (devNames != nullptr) {
			GlobalFree(devNames);
		}

		devMode = pd.m_pd.hDevMode;
		devNames = pd.m_pd.hDevNames;
	}
}

static bool getPrinterDeviceDefaults(PRINTDLGW* printDlg)
{
	updatePrinterSelection(devNames == nullptr);
	if (devNames == nullptr) {
		return false;
	}
	printDlg->hDevNames = devNames;
	printDlg->hDevMode = devMode;
	return true;
}

static INT_PTR doPrintDialog(PrintDialog* pd)
{
	updatePrinterSelection(false);

	pd->m_pd.hDevMode = devMode;
	pd->m_pd.hDevNames = devNames;
	INT_PTR response = pd->DoModal();

	devMode = pd->m_pd.hDevMode;
	devNames = pd->m_pd.hDevNames;
	return response;
}

namespace Printing {

void filePrintSetup()
{
	PrintDialog pd(true);
	doPrintDialog(&pd);
}

unsigned numPreviewPages()
{
	return previewPages;
}

void setNumPreviewPages(unsigned pages)
{
	previewPages = pages;
}

QFont deviceFont(HDC__* dc)
{
	wchar_t face[LF_FACESIZE];
	GetTextFaceW(dc, LF_FACESIZE, face);
	TEXTMETRICW metrics;
	GetTextMetricsW(dc, &metrics);
	QFont font(QString::fromWCharArray(face));
	font.setPixelSize(metrics.tmHeight - metrics.tmInternalLeading);
	font.setWeight(static_cast<QFont::Weight>(metrics.tmWeight));
	font.setItalic(metrics.tmItalic != 0);
	return font;
}

QString currentDateTime()
{
	SYSTEMTIME time;
	GetLocalTime(&time);
	time.wMilliseconds = 0;
	DATE date;
	SystemTimeToVariantTime(&time, &date);
	BSTR text = nullptr;
	if (FAILED(VarBstrFromDate(date, LOCALE_USER_DEFAULT, 0, &text))) {
		return QString();
	}
	QString result = QString::fromWCharArray(text, static_cast<qsizetype>(SysStringLen(text)));
	SysFreeString(text);
	return result;
}

} // namespace Printing

bool View::doPreparePrinting(PrintInfo& info)
{
	PRINTDLGW& pd = info.dialog().m_pd;
	if (pd.nMinPage > pd.nMaxPage) {
		pd.nMaxPage = pd.nMinPage;
	}

	if (info.preview || info.direct) {
		if (pd.hDC == nullptr) {
			if (!getPrinterDeviceDefaults(&pd)) {
				if (doPrintDialog(&info.dialog()) != IDOK) {
					return false;
				}
			}

			if (pd.hDC == nullptr) {
				if (info.dialog().CreatePrinterDC() == nullptr) {
					return false;
				}
			}
		}

		pd.nFromPage = static_cast<WORD>(info.minPage());
		pd.nToPage = static_cast<WORD>(info.maxPage());
	}
	else {
		pd.nFromPage = static_cast<WORD>(info.minPage());
		pd.nToPage = static_cast<WORD>(info.maxPage());

		if (doPrintDialog(&info.dialog()) != IDOK) {
			return false;
		}
	}

	if (pd.hDC == nullptr) {
		return false;
	}

	info.numPreviewPages = previewPages;
	info.pageDesc = Resources::string(previewPageDescStringId);
	return true;
}

static BOOL CALLBACK abortProc(HDC, int)
{
	if (!userAbort) {
		QCoreApplication::processEvents();
	}
	return !userAbort;
}

static void printPage(View* view, PrintDC& dc, PrintInfo& info, HDC printer)
{
	const int width = dc.resolution.width();
	const int height = dc.resolution.height();
	const int bandHeight = qMax(1, (32 << 20) / qMax(1, width * 4));
	const int dotsPerMeterX = qRound(GetDeviceCaps(printer, LOGPIXELSX) / 0.0254);
	const int dotsPerMeterY = qRound(GetDeviceCaps(printer, LOGPIXELSY) / 0.0254);

	for (int top = 0; top < height; top += bandHeight) {
		QImage band(width, qMin(bandHeight, height - top), QImage::Format_RGB32);
		band.setDotsPerMeterX(dotsPerMeterX);
		band.setDotsPerMeterY(dotsPerMeterY);
		band.fill(Qt::white);

		QPainter painter(&band);
		painter.translate(0, -top);
		PrintDC bandDC = dc;
		bandDC.painter = &painter;
		view->onPrint(bandDC, info);
		painter.end();

		BITMAPINFO bitmap = {};
		bitmap.bmiHeader.biSize = sizeof(bitmap.bmiHeader);
		bitmap.bmiHeader.biWidth = band.width();
		bitmap.bmiHeader.biHeight = -band.height();
		bitmap.bmiHeader.biPlanes = 1;
		bitmap.bmiHeader.biBitCount = 32;
		bitmap.bmiHeader.biCompression = BI_RGB;
		SetDIBitsToDevice(
			printer,
			0,
			top,
			band.width(),
			band.height(),
			0,
			0,
			0,
			band.height(),
			band.constBits(),
			&bitmap,
			DIB_RGB_COLORS
		);
	}
}

void View::filePrint(bool direct)
{
	PrintInfo printInfo;

	if (direct) {
		const CRobloxCommandLineInfo* commandLine = theApp.commandLineInfo();
		if (commandLine != nullptr) {
			if (commandLine->shellCommand == CRobloxCommandLineInfo::FilePrintTo) {
				printInfo.dialog().m_pd.hDC = CreateDCW(
					reinterpret_cast<const wchar_t*>(commandLine->driverName.utf16()),
					reinterpret_cast<const wchar_t*>(commandLine->printerName.utf16()),
					reinterpret_cast<const wchar_t*>(commandLine->portName.utf16()),
					nullptr
				);
				if (printInfo.dialog().m_pd.hDC == nullptr) {
					SystemMessageBox::warning(
						theApp.mainFrame(),
						Resources::string(appTitleStringId),
						Resources::string(failedToStartPrintStringId)
					);
					return;
				}
			}
		}

		printInfo.direct = true;
	}

	if (!onPreparePrinting(printInfo)) {
		return;
	}

	PRINTDLGW& pd = printInfo.dialog().m_pd;
	QString output;
	if (pd.Flags & PD_PRINTTOFILE) {
		CFileDialog dialog(
			false,
			Resources::string(printDefaultExtStringId),
			Resources::string(printDefaultStringId),
			CFileDialog::HideReadOnly | CFileDialog::OverwritePrompt,
			Resources::string(printFilterStringId)
		);
		dialog.setTitle(Resources::string(printCaptionStringId));

		if (dialog.DoModal() != IDOK) {
			return;
		}

		output = dialog.GetPathName();
	}

	QString title;
	if (document()) {
		title = document()->title();
	}
	else {
		title = childFrame()->windowTitle();
	}
	std::wstring docName = title.toStdWString();
	std::wstring outputName = output.toStdWString();
	DOCINFOW docInfo;
	memset(&docInfo, 0, sizeof(docInfo));
	docInfo.cbSize = sizeof(docInfo);
	docInfo.lpszDocName = docName.c_str();
	QString portName;
	if (output.isEmpty()) {
		docInfo.lpszOutput = nullptr;
		portName = printInfo.dialog().GetPortName();
	}
	else {
		docInfo.lpszOutput = outputName.c_str();
		portName = QFileInfo(output).fileName();
	}

	HDC printer = pd.hDC;
	QImage attrib(1, 1, QImage::Format_RGB32);
	attrib.setDotsPerMeterX(qRound(GetDeviceCaps(printer, LOGPIXELSX) / 0.0254));
	attrib.setDotsPerMeterY(qRound(GetDeviceCaps(printer, LOGPIXELSY) / 0.0254));
	QImage prepareTarget(1, 1, QImage::Format_RGB32);
	prepareTarget.setDotsPerMeterX(attrib.dotsPerMeterX());
	prepareTarget.setDotsPerMeterY(attrib.dotsPerMeterY());
	QPainter preparePainter(&prepareTarget);
	PrintDC dcPrint;
	dcPrint.painter = &preparePainter;
	dcPrint.attrib = &attrib;
	dcPrint.resolution = QSize(GetDeviceCaps(printer, HORZRES), GetDeviceCaps(printer, VERTRES));
	dcPrint.font = Printing::deviceFont(printer);
	onBeginPrinting(dcPrint, printInfo);

	SetAbortProc(printer, abortProc);

	HWND mainWindow = reinterpret_cast<HWND>(theApp.mainFrame()->winId());
	EnableWindow(mainWindow, FALSE);
	PrintingDialog dlgPrintStatus(theApp.mainFrame());

	dlgPrintStatus.SetDlgItemText(printDocNameId, title);
	dlgPrintStatus.SetDlgItemText(printPrinterNameId, printInfo.dialog().GetDeviceName());
	dlgPrintStatus.SetDlgItemText(printPortNameId, portName);
	dlgPrintStatus.show();
	dlgPrintStatus.repaint();

	int jobNumber = StartDocW(printer, &docInfo);
	if (jobNumber == SP_ERROR) {
		EnableWindow(mainWindow, TRUE);
		onEndPrinting(dcPrint, printInfo);
		dlgPrintStatus.hide();
		SystemMessageBox::warning(
			theApp.mainFrame(), Resources::string(appTitleStringId), Resources::string(failedToStartPrintStringId)
		);
		return;
	}

	unsigned endPage = printInfo.toPage();
	unsigned startPage = printInfo.fromPage();

	if (endPage < printInfo.minPage()) {
		endPage = printInfo.minPage();
	}
	if (endPage > printInfo.maxPage()) {
		endPage = printInfo.maxPage();
	}

	if (startPage < printInfo.minPage()) {
		startPage = printInfo.minPage();
	}
	if (startPage > printInfo.maxPage()) {
		startPage = printInfo.maxPage();
	}

	int step = (endPage >= startPage) ? 1 : -1;
	endPage = (endPage == 0xffff) ? 0xffff : endPage + step;

	QString pageFormat = Resources::string(printPageNumStringId);

	bool error = false;
	for (printInfo.curPage = startPage; printInfo.curPage != endPage; printInfo.curPage += step) {
		onPrepareDC(dcPrint, &printInfo);

		if (!printInfo.continuePrinting) {
			break;
		}

		dlgPrintStatus.SetDlgItemText(
			printPageNumId, QString(pageFormat).replace("%u", QString::number(printInfo.curPage))
		);

		printInfo.rectDraw = QRect(QPoint(0, 0), dcPrint.resolution);

		if (StartPage(printer) < 0) {
			error = true;
			break;
		}

		onPrepareDC(dcPrint, &printInfo);

		printPage(this, dcPrint, printInfo, printer);
		if (step > 0) {
			endPage = printInfo.maxPage() + step;
		}

		if (EndPage(printer) < 0 && GetLastError() != ERROR_SUCCESS) {
			HANDLE printerHandle;
			std::wstring deviceName = printInfo.dialog().GetDeviceName().toStdWString();
			if (!OpenPrinterW(deviceName.data(), &printerHandle, nullptr)) {
				error = true;
				break;
			}

			DWORD bytesNeeded = 0;
			if (!GetJobW(printerHandle, jobNumber, 1, nullptr, 0, &bytesNeeded) &&
				GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
				ClosePrinter(printerHandle);
				error = true;
				break;
			}

			std::vector<BYTE> job(std::max<DWORD>(bytesNeeded, sizeof(JOB_INFO_1W)));
			DWORD bytesUsed;
			BOOL ok = GetJobW(printerHandle, jobNumber, 1, job.data(), static_cast<DWORD>(job.size()), &bytesUsed);
			ClosePrinter(printerHandle);

			if (!ok || !(reinterpret_cast<JOB_INFO_1W*>(job.data())->Status & JOB_STATUS_RESTART)) {
				error = true;
				break;
			}
		}

		if (!abortProc(printer, 0)) {
			error = true;
			break;
		}
	}

	if (!error) {
		EndDoc(printer);
	}
	else {
		AbortDoc(printer);
	}

	EnableWindow(mainWindow, TRUE);

	onEndPrinting(dcPrint, printInfo);
	dlgPrintStatus.hide();
}

void View::filePrintPreview()
{
	CChildFrame* frame = childFrame();
	CPreviewView* view = new CPreviewView(this);
	if (document()) {
		document()->addView(view);
	}
	frame->setPreviewMode(true, view);

	activate(false, this);

	if (!view->setPrintView(this)) {
		view->onPreviewClose();
		return;
	}

	frame->setActiveView(view);
	view->updateCommandUI();
}

void View::onEndPrintPreview(PrintDC& dc, PrintInfo& info, CPreviewView* view)
{
	onEndPrinting(dc, info);

	CChildFrame* frame = childFrame();
	frame->setPreviewMode(false, view);
	frame->setActiveView(this);
	view->deleteLater();

	theApp.mainFrame()->setMessageText(Resources::string(idleMessageStringId));
}
#else
static QPrinter& applicationPrinter()
{
	static QPrinter printer(QPrinter::HighResolution);
	return printer;
}

PrintInfo::PrintInfo() : m_printer(&applicationPrinter())
{
}

PrintInfo::~PrintInfo() = default;

unsigned PrintInfo::minPage() const
{
	return m_minPage;
}

unsigned PrintInfo::maxPage() const
{
	return m_maxPage;
}

void PrintInfo::setMinPage(unsigned page)
{
	m_minPage = page;
}

void PrintInfo::setMaxPage(unsigned page)
{
	m_maxPage = page;
}

unsigned PrintInfo::fromPage() const
{
	return m_pageNumbers ? m_fromPage : minPage();
}

unsigned PrintInfo::toPage() const
{
	return m_pageNumbers ? m_toPage : maxPage();
}

bool PrintInfo::printSelection() const
{
	return m_selection;
}

void PrintInfo::checkSelection()
{
	m_selectionEnabled = true;
	m_selection = true;
}

void PrintInfo::setPreview()
{
	preview = true;
}

void PrintInfo::setPageRange(unsigned from, unsigned to)
{
	m_pageNumbers = true;
	m_selection = false;
	m_fromPage = from;
	m_toPage = to;
}

void PrintInfo::setPrintAll()
{
	m_pageNumbers = false;
	m_selection = false;
}

void PrintInfo::setPrintSelection()
{
	m_pageNumbers = false;
	m_selection = true;
}

namespace Printing {

void filePrintSetup()
{
	QPageSetupDialog dialog(&applicationPrinter(), theApp.mainFrame());
	dialog.exec();
}

unsigned numPreviewPages()
{
	return previewPages;
}

void setNumPreviewPages(unsigned pages)
{
	previewPages = pages;
}

QFont deviceFont(QPrinter& printer)
{
	return QFont(QApplication::font(), &printer);
}

QString currentDateTime()
{
	return QLocale::system().toString(QDateTime::currentDateTime(), QLocale::ShortFormat);
}

} // namespace Printing

bool View::doPreparePrinting(PrintInfo& info)
{
	if (info.minPage() > info.maxPage()) {
		info.setMaxPage(info.minPage());
	}

	if (!info.preview && !info.direct) {
		QPrintDialog dialog(&info.printer(), theApp.mainFrame());
		dialog.setMinMax(static_cast<int>(info.minPage()), static_cast<int>(info.maxPage()));
		dialog.setFromTo(static_cast<int>(info.minPage()), static_cast<int>(info.maxPage()));
		dialog.setOption(QAbstractPrintDialog::PrintPageRange, true);
		dialog.setOption(QAbstractPrintDialog::PrintSelection, info.selectionEnabled());
		dialog.setPrintRange(info.printSelection() ? QAbstractPrintDialog::Selection : QAbstractPrintDialog::AllPages);
		if (dialog.exec() != QDialog::Accepted) {
			return false;
		}

		switch (dialog.printRange()) {
		case QAbstractPrintDialog::PageRange:
			info.setPageRange(static_cast<unsigned>(dialog.fromPage()), static_cast<unsigned>(dialog.toPage()));
			break;
		case QAbstractPrintDialog::Selection:
			info.setPrintSelection();
			break;
		default:
			info.setPrintAll();
			break;
		}
	}

	info.numPreviewPages = previewPages;
	info.pageDesc = Resources::string(previewPageDescStringId);
	return true;
}

static bool printPages(View* view, PrintInfo& info, QPrinter& printer, PrintingDialog* status)
{
	QPainter painter;
	if (!painter.begin(&printer)) {
		return false;
	}

	PrintDC dc;
	dc.painter = &painter;
	dc.attrib = &printer;
	dc.resolution = printer.pageLayout().paintRectPixels(printer.resolution()).size();
	dc.font = Printing::deviceFont(printer);
	view->onBeginPrinting(dc, info);

	unsigned endPage = info.toPage();
	unsigned startPage = info.fromPage();
	endPage = qBound(info.minPage(), endPage, info.maxPage());
	startPage = qBound(info.minPage(), startPage, info.maxPage());

	int step = (endPage >= startPage) ? 1 : -1;
	endPage = (endPage == 0xffff) ? 0xffff : endPage + step;

	QString pageFormat = Resources::string(printPageNumStringId);

	bool firstPage = true;
	bool error = false;
	for (info.curPage = startPage; info.curPage != endPage; info.curPage += step) {
		view->onPrepareDC(dc, &info);

		if (!info.continuePrinting) {
			break;
		}

		if (status) {
			status->SetDlgItemText(printPageNumId, QString(pageFormat).replace("%u", QString::number(info.curPage)));
		}

		info.rectDraw = QRect(QPoint(0, 0), dc.resolution);

		if (!firstPage && !printer.newPage()) {
			error = true;
			break;
		}
		firstPage = false;

		view->onPrepareDC(dc, &info);
		view->onPrint(dc, info);
		if (step > 0) {
			endPage = info.maxPage() + step;
		}

		if (status) {
			QCoreApplication::processEvents();
			if (userAbort) {
				error = true;
				break;
			}
		}
	}

	if (error) {
		printer.abort();
	}
	painter.end();

	view->onEndPrinting(dc, info);
	return !error;
}

void View::filePrint(bool direct)
{
	PrintInfo printInfo;

	if (direct) {
		const CRobloxCommandLineInfo* commandLine = theApp.commandLineInfo();
		if (commandLine != nullptr && commandLine->shellCommand == CRobloxCommandLineInfo::FilePrintTo) {
			printInfo.printer().setPrinterName(commandLine->printerName);
			if (!printInfo.printer().isValid()) {
				SystemMessageBox::warning(
					theApp.mainFrame(),
					Resources::string(appTitleStringId),
					Resources::string(failedToStartPrintStringId)
				);
				return;
			}
		}

		printInfo.direct = true;
	}

	if (!onPreparePrinting(printInfo)) {
		return;
	}

	QString title;
	if (document()) {
		title = document()->title();
	}
	else {
		title = childFrame()->windowTitle();
	}

	QPrinter& printer = printInfo.printer();
	printer.setDocName(title);

	QWidget* mainWindow = theApp.mainFrame();
	mainWindow->setEnabled(false);
	PrintingDialog dlgPrintStatus(theApp.mainFrame());

	dlgPrintStatus.SetDlgItemText(printDocNameId, title);
	dlgPrintStatus.SetDlgItemText(printPrinterNameId, printer.printerName());
	dlgPrintStatus.SetDlgItemText(printPortNameId, printer.outputFileName());
	dlgPrintStatus.show();
	dlgPrintStatus.repaint();

	bool started = printPages(this, printInfo, printer, &dlgPrintStatus);

	mainWindow->setEnabled(true);
	dlgPrintStatus.hide();

	if (!started && !userAbort) {
		SystemMessageBox::warning(
			theApp.mainFrame(), Resources::string(appTitleStringId), Resources::string(failedToStartPrintStringId)
		);
	}
}

void View::filePrintPreview()
{
	PrintInfo printInfo;
	printInfo.setPreview();
	if (!onPreparePrinting(printInfo)) {
		return;
	}

	QPrintPreviewDialog dialog(&printInfo.printer(), theApp.mainFrame());
	QObject::connect(&dialog, &QPrintPreviewDialog::paintRequested, &dialog, [this, &printInfo](QPrinter* printer) {
		printInfo.continuePrinting = true;
		printPages(this, printInfo, *printer, nullptr);
	});
	dialog.exec();

	theApp.mainFrame()->setMessageText(Resources::string(idleMessageStringId));
}
#endif
