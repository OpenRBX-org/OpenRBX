#ifndef PRINTING_H
#define PRINTING_H

#include <QFont>
#include <QRect>
#include <QSize>
#include <QString>
#include <memory>

class QPaintDevice;
class QPainter;
#ifdef _WIN32
class PrintDialog;
struct HDC__;
#else
class QPrinter;
#endif

struct PrintDC
{
	QPainter* painter = nullptr;
	QPaintDevice* attrib = nullptr;
	QSize resolution;
	QFont font;
};

class PrintInfo
{
public:
	PrintInfo();
	~PrintInfo();

#ifdef _WIN32
	PrintDialog& dialog() const { return *m_dialog; }
#else
	QPrinter& printer() const { return *m_printer; }
#endif
	unsigned minPage() const;
	unsigned maxPage() const;
	void setMinPage(unsigned page);
	void setMaxPage(unsigned page);
	unsigned fromPage() const;
	unsigned toPage() const;
	bool printSelection() const;
	void checkSelection();
	void setPreview();
#ifdef _WIN32
	HDC__* printerDC() const;
#else
	bool selectionEnabled() const { return m_selectionEnabled; }
	void setPageRange(unsigned from, unsigned to);
	void setPrintAll();
	void setPrintSelection();
#endif

	bool preview = false;
	bool direct = false;
	bool continuePrinting = true;
	unsigned curPage = 1;
	unsigned numPreviewPages = 0;
	QString pageDesc;
	QRect rectDraw;

private:
#ifdef _WIN32
	std::unique_ptr<PrintDialog> m_dialog;
#else
	QPrinter* m_printer;
	unsigned m_minPage = 1;
	unsigned m_maxPage = 0xffff;
	unsigned m_fromPage = 1;
	unsigned m_toPage = 0xffff;
	bool m_pageNumbers = false;
	bool m_selection = false;
	bool m_selectionEnabled = false;
#endif
};

namespace Printing {

void filePrintSetup();
unsigned numPreviewPages();
void setNumPreviewPages(unsigned pages);
#ifdef _WIN32
QFont deviceFont(HDC__* dc);
#else
QFont deviceFont(QPrinter& printer);
#endif
QString currentDateTime();

} // namespace Printing

#endif // PRINTING_H
