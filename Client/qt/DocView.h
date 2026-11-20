#ifndef DOCVIEW_H
#define DOCVIEW_H

#include "Command.h"

#include <QString>
#include <QWidget>
#include <functional>
#include <string>
#include <vector>

class CChildFrame;
class CPreviewView;
class Document;
class DocumentTemplate;
class PrintInfo;
struct PrintDC;

class View : public QWidget, public CommandTarget
{
public:
	explicit View(QWidget* parent = nullptr);
	~View() override;

	Document* document() const { return m_document; }
	CChildFrame* childFrame() const;

	virtual bool create();
	virtual void initialUpdate();
	virtual void activate(bool active, View* other);
	virtual void activateFrame(bool active);
	virtual void updateCommandUI();

	virtual bool onPreparePrinting(PrintInfo& info);
	virtual void onBeginPrinting(PrintDC& dc, PrintInfo& info);
	virtual void onPrepareDC(PrintDC& dc, PrintInfo* info);
	virtual void onPrint(PrintDC& dc, PrintInfo& info);
	virtual void onEndPrinting(PrintDC& dc, PrintInfo& info);
	virtual void onDraw(PrintDC& dc);
#ifdef _WIN32
	virtual void onEndPrintPreview(PrintDC& dc, PrintInfo& info, CPreviewView* view);
#endif
	bool doPreparePrinting(PrintInfo& info);
	void filePrint(bool direct = false);
	void filePrintPreview();

private:
	friend class Document;

	Document* m_document = nullptr;
};

class Document : public CommandTarget
{
public:
	Document();
	~Document() override;

	const QString& title() const { return m_title; }
	virtual void setTitle(const QString& title);
	const QString& pathName() const { return m_pathName; }
	void setPathName(const QString& pathName);
	const std::vector<View*>& views() const { return m_views; }

	virtual bool isModified() const;
	virtual void setModified(bool modified);

	virtual bool newDocument();
	virtual bool openDocument(const QString& pathName);
	virtual bool saveDocument(const QString& pathName);
	virtual void deleteContents();
	virtual bool saveModified();
	virtual bool save(const QString& pathName, bool replace = true);
	bool saveExisting();
	void close();

	void addView(View* view);
	void removeView(View* view);

private:
	friend class DocumentTemplate;

	void updateFrameTitles();

	QString m_title;
	QString m_pathName;
	DocumentTemplate* m_template = nullptr;
	std::vector<View*> m_views;
	bool m_modified = false;
	bool m_closing = false;
};

enum class DocString
{
	WindowTitle,
	DocName,
	FileNewName,
	FilterName,
	FilterExt,
};

class DocumentTemplate
{
public:
	DocumentTemplate(unsigned resourceId, std::function<Document*()> createDocument, std::function<View*()> createView);
	virtual ~DocumentTemplate();

	virtual void removeDocument(Document* document);
	const std::string& failure() const { return m_failure; }

	QString docString(DocString index) const;
	const std::vector<Document*>& documents() const { return m_documents; }

	Document* openDocument(const QString& pathName, bool visible = true);
	CChildFrame* createFrame(Document* document);
	void initialUpdateFrame(CChildFrame* frame, bool visible = true);
	void setDefaultTitle(Document* document);

	bool saveAllModified();
	void closeAllDocuments();

private:
	friend class Document;

	unsigned m_resourceId;
	std::function<Document*()> m_createDocument;
	std::function<View*()> m_createView;
	int m_untitledCount = 0;
	std::vector<Document*> m_documents;
	std::string m_failure;
};

class DocumentManager
{
public:
	virtual ~DocumentManager();

	void addTemplate(DocumentTemplate* documentTemplate);
	virtual bool promptFileName(
		QString& fileName, unsigned titleId, unsigned flags, bool open, DocumentTemplate* documentTemplate
	);
	virtual void newFile();
	void openFile();
	virtual Document* openDocument(const QString& fileName);
	bool saveAllModified();
	void closeAllDocuments();

protected:
	std::vector<DocumentTemplate*> m_templates;
};

#endif // DOCVIEW_H
