#include "DocView.h"

#include "ChildFrm.h"
#include "CommandIds.h"
#include "FileDialog.h"
#include "MainFrm.h"
#include "Printing.h"
#include "Resources.h"
#include "Roblox.h"
#include "SystemMessageBox.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <algorithm>

static const unsigned openFileStringId = 61440;
static const unsigned saveFileStringId = 61441;
static const unsigned allFilterStringId = 61442;
static const unsigned saveFileCopyStringId = 61444;

View::View(QWidget* parent) : QWidget(parent)
{
}

bool View::onPreparePrinting(PrintInfo& info)
{
	return true;
}

void View::onBeginPrinting(PrintDC& dc, PrintInfo& info)
{
}

void View::onPrepareDC(PrintDC& dc, PrintInfo* info)
{
	if (info != nullptr) {
		info->continuePrinting = info->maxPage() != 0xffff || info->curPage == 1;
	}
}

void View::onPrint(PrintDC& dc, PrintInfo& info)
{
	onDraw(dc);
}

void View::onEndPrinting(PrintDC& dc, PrintInfo& info)
{
}

void View::onDraw(PrintDC& dc)
{
}

View::~View()
{
	if (m_document) {
		m_document->removeView(this);
	}
}

CChildFrame* View::childFrame() const
{
	for (QWidget* widget = parentWidget(); widget; widget = widget->parentWidget()) {
		if (CChildFrame* frame = dynamic_cast<CChildFrame*>(widget)) {
			return frame;
		}
	}
	return nullptr;
}

bool View::create()
{
	return true;
}

void View::initialUpdate()
{
	update();
}

void View::activate(bool active, View* other)
{
}

void View::activateFrame(bool active)
{
}

void View::updateCommandUI()
{
}

Document::Document()
{
	addCommand(ID_FILE_CLOSE, [this]() {
		if (saveModified()) {
			close();
		}
	});
	addCommand(ID_FILE_SAVE, [this]() { saveExisting(); });
	addCommand(ID_FILE_SAVE_AS, [this]() { save(QString()); });
}

Document::~Document()
{
	if (m_template) {
		m_template->removeDocument(this);
	}
}

void Document::setTitle(const QString& title)
{
	m_title = title;
	updateFrameTitles();
}

void Document::setPathName(const QString& pathName)
{
	QFileInfo info(pathName);
	m_pathName = QDir::toNativeSeparators(info.absoluteFilePath());
	setTitle(info.fileName());
	theApp.addToRecentFileList(m_pathName);
}

bool Document::isModified() const
{
	return m_modified;
}

void Document::setModified(bool modified)
{
	m_modified = modified;
}

void Document::addView(View* view)
{
	m_views.push_back(view);
	view->m_document = this;
}

void Document::removeView(View* view)
{
	std::erase(m_views, view);
	view->m_document = nullptr;
}

void Document::updateFrameTitles()
{
	for (View* view : m_views) {
		if (CChildFrame* frame = view->childFrame()) {
			frame->updateTitle();
		}
	}
}

void Document::deleteContents()
{
}

bool Document::newDocument()
{
	deleteContents();
	m_pathName.clear();
	setModified(false);
	return true;
}

bool Document::openDocument(const QString& pathName)
{
	deleteContents();
	setModified(false);
	return true;
}

bool Document::saveDocument(const QString& pathName)
{
	setModified(false);
	return true;
}

void Document::close()
{
	if (m_closing) {
		return;
	}
	m_closing = true;

	while (!m_views.empty()) {
		View* view = m_views.front();
		if (CChildFrame* frame = view->childFrame()) {
			delete frame;
		}
		else {
			delete view;
		}
	}

	deleteContents();
	delete this;
}

bool Document::saveModified()
{
	if (!isModified()) {
		return true;
	}

	QString name = m_pathName.isEmpty() ? m_title : QFileInfo(m_pathName).fileName();
	switch (SystemMessageBox::question(
		theApp.mainFrame(),
		Resources::string(appTitleStringId),
		QStringLiteral("Save changes to %1?").arg(name),
		QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
	)) {
	case QMessageBox::Yes:
		return saveExisting();
	case QMessageBox::No:
		return true;
	default:
		return false;
	}
}

bool Document::save(const QString& pathName, bool replace)
{
	QString newName = pathName;
	if (newName.isEmpty()) {
		newName = m_pathName;
		if (replace && newName.isEmpty()) {
			newName = m_title;
			qsizetype invalid = newName.indexOf(QRegularExpression(QStringLiteral("[#%;/\\\\]")));
			if (invalid >= 0) {
				newName.truncate(invalid);
			}
			if (m_template) {
				newName += m_template->docString(DocString::FilterExt);
			}
		}

		if (!theApp.promptFileName(
				newName,
				replace ? saveFileStringId : saveFileCopyStringId,
				CFileDialog::HideReadOnly | CFileDialog::PathMustExist,
				false,
				m_template
			)) {
			return false;
		}
	}

	if (!saveDocument(newName)) {
		return false;
	}

	if (replace) {
		setPathName(newName);
	}
	return true;
}

bool Document::saveExisting()
{
	QFileInfo info(m_pathName);
	if (m_pathName.isEmpty() || (info.exists() && !info.isWritable())) {
		return save(QString());
	}
	return save(m_pathName);
}

DocumentTemplate::DocumentTemplate(
	unsigned resourceId, std::function<Document*()> createDocument, std::function<View*()> createView
)
	: m_resourceId(resourceId), m_createDocument(std::move(createDocument)), m_createView(std::move(createView))
{
}

DocumentTemplate::~DocumentTemplate()
{
	closeAllDocuments();
}

void DocumentTemplate::removeDocument(Document* document)
{
	std::erase(m_documents, document);
}

QString DocumentTemplate::docString(DocString index) const
{
	return Resources::string(m_resourceId).split('\n').value(static_cast<int>(index));
}

void DocumentTemplate::setDefaultTitle(Document* document)
{
	QString title = docString(DocString::DocName);
	if (title.isEmpty()) {
		title = QStringLiteral("Untitled");
	}
	else {
		title += QString::number(m_untitledCount + 1);
	}
	document->setTitle(title);
}

CChildFrame* DocumentTemplate::createFrame(Document* document)
{
	theApp.showMainFrame(true);

	View* view = m_createView();
	document->addView(view);
	CChildFrame* frame = new CChildFrame(view);
	theApp.mainFrame()->addChildFrame(frame);
	if (!view->create()) {
		document->removeView(view);
		delete frame;
		return nullptr;
	}
	return frame;
}

void DocumentTemplate::initialUpdateFrame(CChildFrame* frame, bool visible)
{
	frame->view()->initialUpdate();
	if (visible) {
		frame->activateFrame();
	}
	frame->updateTitle();
}

Document* DocumentTemplate::openDocument(const QString& pathName, bool visible)
{
	m_failure.clear();

	Document* document = m_createDocument();
	m_documents.push_back(document);
	document->m_template = this;

	CChildFrame* frame = createFrame(document);
	if (frame == nullptr) {
		m_failure = "CreateNewFrame returned NULL";
		SystemMessageBox::warning(
			theApp.mainFrame(), Resources::string(appTitleStringId), "Failed to create empty document."
		);
		delete document;
		return nullptr;
	}

	if (pathName.isEmpty()) {
		setDefaultTitle(document);
		if (!document->newDocument()) {
			m_failure = "CDocument::OnNewDocument returned FALSE";
			document->close();
			return nullptr;
		}
		m_untitledCount++;
	}
	else {
		if (!document->openDocument(pathName)) {
			m_failure = "CDocument::OnNewDocument returned FALSE (v2)";
			document->close();
			return nullptr;
		}
		document->setPathName(pathName);
	}

	initialUpdateFrame(frame, visible);
	return document;
}

bool DocumentTemplate::saveAllModified()
{
	std::vector<Document*> documents = m_documents;
	return std::all_of(documents.begin(), documents.end(), [](Document* document) { return document->saveModified(); });
}

void DocumentTemplate::closeAllDocuments()
{
	while (!m_documents.empty()) {
		m_documents.back()->close();
	}
}

DocumentManager::~DocumentManager()
{
	for (DocumentTemplate* documentTemplate : m_templates) {
		delete documentTemplate;
	}
}

void DocumentManager::addTemplate(DocumentTemplate* documentTemplate)
{
	m_templates.push_back(documentTemplate);
}

static void appendFilterSuffix(
	QString& filter, int& filterCount, CFileDialog& dialog, DocumentTemplate* documentTemplate, QString* defaultExt
)
{
	QString filterExt = documentTemplate->docString(DocString::FilterExt);
	QString filterName = documentTemplate->docString(DocString::FilterName);
	if (filterExt.isEmpty() || filterName.isEmpty()) {
		return;
	}

	if (defaultExt) {
		defaultExt->clear();
	}

	filter += filterName + QLatin1Char('|');
	QStringList extensions = filterExt.split(QLatin1Char(';'), Qt::SkipEmptyParts);
	for (qsizetype i = 0; i < extensions.size(); i++) {
		if (defaultExt && defaultExt->isEmpty()) {
			*defaultExt = extensions[i].mid(1);
			dialog.setDefExt(*defaultExt);
			dialog.setFilterIndex(filterCount + 1);
		}
		if (i > 0) {
			filter += QLatin1Char(';');
		}
		filter += QLatin1Char('*') + extensions[i];
	}
	filter += QLatin1Char('|');
	filterCount++;
}

bool DocumentManager::promptFileName(
	QString& fileName, unsigned titleId, unsigned flags, bool open, DocumentTemplate* documentTemplate
)
{
	CFileDialog dialog(open, QString(), QString(), CFileDialog::HideReadOnly | CFileDialog::OverwritePrompt);
	dialog.addFlags(flags);

	QString filter;
	QString defaultExt;
	int filterCount = 0;
	if (documentTemplate) {
		appendFilterSuffix(filter, filterCount, dialog, documentTemplate, &defaultExt);
	}
	else {
		bool first = true;
		for (DocumentTemplate* candidate : m_templates) {
			appendFilterSuffix(filter, filterCount, dialog, candidate, first ? &defaultExt : nullptr);
			first = false;
		}
	}

	filter += Resources::string(allFilterStringId) + QStringLiteral("|*.*|");
	dialog.setFilter(filter);
	dialog.setTitle(Resources::string(titleId));
	dialog.setFileName(fileName);

	if (dialog.DoModal() != IDOK) {
		return false;
	}
	fileName = dialog.GetPathName();
	return true;
}

void DocumentManager::newFile()
{
	if (!m_templates.empty()) {
		m_templates.front()->openDocument(QString());
	}
}

void DocumentManager::openFile()
{
	QString fileName;
	if (promptFileName(
			fileName, openFileStringId, CFileDialog::HideReadOnly | CFileDialog::FileMustExist, true, nullptr
		)) {
		openDocument(fileName);
	}
}

Document* DocumentManager::openDocument(const QString& fileName)
{
	QFileInfo info(fileName);
	DocumentTemplate* best = nullptr;
	for (DocumentTemplate* documentTemplate : m_templates) {
		for (Document* document : documentTemplate->documents()) {
			if (QFileInfo(document->pathName()) == info) {
				if (!document->views().empty()) {
					if (CChildFrame* frame = document->views().front()->childFrame()) {
						frame->activateFrame();
					}
				}
				return document;
			}
		}

		QString ext = documentTemplate->docString(DocString::FilterExt);
		if (!best && !ext.isEmpty() && fileName.endsWith(ext, Qt::CaseInsensitive)) {
			best = documentTemplate;
		}
	}

	if (!best && !m_templates.empty()) {
		best = m_templates.front();
	}
	return best ? best->openDocument(info.absoluteFilePath()) : nullptr;
}

bool DocumentManager::saveAllModified()
{
	return std::all_of(m_templates.begin(), m_templates.end(), [](DocumentTemplate* documentTemplate) {
		return documentTemplate->saveAllModified();
	});
}

void DocumentManager::closeAllDocuments()
{
	for (DocumentTemplate* documentTemplate : m_templates) {
		documentTemplate->closeAllDocuments();
	}
}
