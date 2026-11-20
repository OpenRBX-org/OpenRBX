#include "ScintillaDocView.h"

#include "CommandIds.h"
#include "Printing.h"
#include "Resources.h"
#include "WhidbeyStyle.h"

#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QFile>
#include <QFileInfo>
#include <QFontMetrics>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QResizeEvent>
#include <QScreen>

namespace Scintilla {

static const unsigned ID_EDIT_FIND = 0xe124;
static const unsigned ID_EDIT_REPEAT = 0xe128;
static const unsigned ID_EDIT_REPLACE = 0xe129;

static const unsigned IDD_SCINTILLA_FINDDLGORD = 101;
static const unsigned IDD_SCINTILLA_REPLACEDLGORD = 102;
static const unsigned IDS_SCINTILLA_DEFAULT_PRINT_HEADER = 197;
static const unsigned IDS_SCINTILLA_DEFAULT_PRINT_FOOTER = 198;

static const int IDC_REPLACE = 1024;
static const int IDC_REPLACE_ALL = 1025;
static const int IDC_HELP_BUTTON = 1038;
static const int IDC_WHOLE_WORD = 1040;
static const int IDC_MATCH_CASE = 1041;
static const int IDC_REGULAR_EXPRESSION = 1042;
static const int IDC_DIRECTION_UP = 1056;
static const int IDC_DIRECTION_DOWN = 1057;
static const int IDC_FIND_WHAT = 1152;
static const int IDC_REPLACE_WITH = 1153;

static const int findTextLimit = 127;

class ScintillaEditState
{
public:
	CScintillaFindReplaceDlg* pFindReplaceDlg = nullptr;
	bool bFindOnly = true;
	QString strFind;
	QString strReplace;
	bool bCase = false;
	bool bNext = true;
	bool bWord = false;
	bool bRegularExpression = false;
};

static ScintillaEditState scintillaEditState;

static int scintillaViewCount;

class WaitCursor
{
public:
	WaitCursor() { QGuiApplication::setOverrideCursor(Qt::WaitCursor); }
	~WaitCursor() { QGuiApplication::restoreOverrideCursor(); }
};

CScintillaFindReplaceDlg::CScintillaFindReplaceDlg(bool bFindDialogOnly, QWidget* pParentWnd)
	: CDialog(bFindDialogOnly ? IDD_SCINTILLA_FINDDLGORD : IDD_SCINTILLA_REPLACEDLGORD, pParentWnd),
	  m_bRegularExpression(false), m_findOnly(bFindDialogOnly), m_terminated(false), m_flags(0), m_owner(nullptr)
{
}

CScintillaFindReplaceDlg::~CScintillaFindReplaceDlg()
{
	if (scintillaEditState.pFindReplaceDlg == this) {
		scintillaEditState.pFindReplaceDlg = nullptr;
	}
}

bool CScintillaFindReplaceDlg::Create(
	bool bFindDialogOnly,
	const QString& findWhat,
	const QString& replaceWith,
	unsigned dwFlags,
	CScintillaView* pParentWnd
)
{
	const Resources::DialogTemplate* resource =
		Resources::dialog(bFindDialogOnly ? IDD_SCINTILLA_FINDDLGORD : IDD_SCINTILLA_REPLACEDLGORD);
	if (resource == nullptr) {
		return false;
	}

	m_owner = pParentWnd;
	m_flags = dwFlags;
	m_findWhat = findWhat.left(findTextLimit);
	m_replaceWith = replaceWith.left(findTextLimit);

	setModal(false);
	if (QWidget* owner = pParentWnd->GetCtrl().GetWidget()) {
		setFramePosition(owner->mapToGlobal(MapDialogRect(resource->x, resource->y, 0, 0).topLeft()));
	}

	return OnInitDialog();
}

bool CScintillaFindReplaceDlg::OnInitDialog()
{
	bool bReturn = CDialog::OnInitDialog();

	QLineEdit* findWhat = qobject_cast<QLineEdit*>(GetDlgItem(IDC_FIND_WHAT));
	if (findWhat) {
		findWhat->setMaxLength(findTextLimit);
		findWhat->setText(m_findWhat);
		connect(findWhat, &QLineEdit::textChanged, this, [this]() { UpdateButtons(); });
	}
	if (QLineEdit* replaceWith = qobject_cast<QLineEdit*>(GetDlgItem(IDC_REPLACE_WITH))) {
		replaceWith->setMaxLength(findTextLimit);
		replaceWith->setText(m_replaceWith);
	}

	if (QCheckBox* wholeWord = qobject_cast<QCheckBox*>(GetDlgItem(IDC_WHOLE_WORD))) {
		wholeWord->setChecked(m_flags & FindReplaceWholeWord);
	}
	if (QCheckBox* matchCase = qobject_cast<QCheckBox*>(GetDlgItem(IDC_MATCH_CASE))) {
		matchCase->setChecked(m_flags & FindReplaceMatchCase);
	}
	if (QRadioButton* up = qobject_cast<QRadioButton*>(GetDlgItem(IDC_DIRECTION_UP))) {
		up->setChecked(!(m_flags & FindReplaceDown));
	}
	if (QRadioButton* down = qobject_cast<QRadioButton*>(GetDlgItem(IDC_DIRECTION_DOWN))) {
		down->setChecked(m_flags & FindReplaceDown);
	}
	if (QWidget* help = GetDlgItem(IDC_HELP_BUTTON)) {
		help->setVisible(false);
	}

	if (QCheckBox* pCtrl = qobject_cast<QCheckBox*>(GetDlgItem(IDC_REGULAR_EXPRESSION))) {
		pCtrl->setChecked(m_bRegularExpression);
		pCtrl->setEnabled(false);
		pCtrl->setVisible(false);
		connect(pCtrl, &QCheckBox::clicked, this, [this]() { OnRegularExpression(); });
	}

	UpdateButtons();
	if (findWhat) {
		findWhat->selectAll();
		findWhat->setFocus();
	}

	return bReturn;
}

void CScintillaFindReplaceDlg::OnRegularExpression()
{
	if (QCheckBox* pCtrl = qobject_cast<QCheckBox*>(GetDlgItem(IDC_REGULAR_EXPRESSION))) {
		m_bRegularExpression = pCtrl->isChecked();
	}
}

void CScintillaFindReplaceDlg::UpdateButtons()
{
	bool hasText = !GetDlgItemText(IDC_FIND_WHAT).isEmpty();
	for (int id : {IDOK, IDC_REPLACE, IDC_REPLACE_ALL}) {
		if (QWidget* button = GetDlgItem(id)) {
			button->setEnabled(hasText);
		}
	}
}

void CScintillaFindReplaceDlg::UpdateTextAndFlags(unsigned action)
{
	m_flags &=
		~(FindReplaceFindNext | FindReplaceReplace | FindReplaceReplaceAll | FindReplaceDialogTerm |
		  FindReplaceWholeWord | FindReplaceMatchCase);
	m_flags |= action;

	if (QCheckBox* wholeWord = qobject_cast<QCheckBox*>(GetDlgItem(IDC_WHOLE_WORD));
		wholeWord && wholeWord->isChecked()) {
		m_flags |= FindReplaceWholeWord;
	}
	if (QCheckBox* matchCase = qobject_cast<QCheckBox*>(GetDlgItem(IDC_MATCH_CASE));
		matchCase && matchCase->isChecked()) {
		m_flags |= FindReplaceMatchCase;
	}
	if (m_findOnly) {
		QRadioButton* down = qobject_cast<QRadioButton*>(GetDlgItem(IDC_DIRECTION_DOWN));
		if (down && down->isChecked()) {
			m_flags |= FindReplaceDown;
		}
		else {
			m_flags &= ~FindReplaceDown;
		}
	}

	m_findWhat = GetDlgItemText(IDC_FIND_WHAT);
	if (!m_findOnly) {
		m_replaceWith = GetDlgItemText(IDC_REPLACE_WITH);
	}
}

void CScintillaFindReplaceDlg::Notify()
{
	if (m_owner) {
		m_owner->OnFindReplaceCmd(this);
	}
	else if (IsTerminating() && scintillaEditState.pFindReplaceDlg == this) {
		scintillaEditState.pFindReplaceDlg = nullptr;
	}
}

void CScintillaFindReplaceDlg::OnCommand(int id)
{
	switch (id) {
	case IDC_REPLACE:
		UpdateTextAndFlags(FindReplaceReplace);
		Notify();
		break;
	case IDC_REPLACE_ALL:
		UpdateTextAndFlags(FindReplaceReplaceAll);
		Notify();
		break;
	default:
		CDialog::OnCommand(id);
		break;
	}
}

void CScintillaFindReplaceDlg::OnOK()
{
	UpdateTextAndFlags(FindReplaceFindNext);
	Notify();
}

void CScintillaFindReplaceDlg::OnCancel()
{
	Close();
}

void CScintillaFindReplaceDlg::Close()
{
	if (m_terminated) {
		return;
	}
	m_terminated = true;

	UpdateTextAndFlags(FindReplaceDialogTerm);
	Notify();
	hide();
	deleteLater();
}

CScintillaView::CScintillaView()
{
	scintillaViewCount++;

	addCommandUpdate(ID_EDIT_CUT, [this](CommandState& state) { OnUpdateNeedSel(state); });
	addCommandUpdate(ID_EDIT_PASTE, [this](CommandState& state) { OnUpdateNeedPaste(state); });
	addCommandUpdate(ID_EDIT_FIND, [this](CommandState& state) { OnUpdateNeedText(state); });
	addCommandUpdate(ID_EDIT_REPEAT, [this](CommandState& state) { OnUpdateNeedFind(state); });
	addCommandUpdate(ID_EDIT_UNDO, [this](CommandState& state) { OnUpdateEditUndo(state); });
	addCommandUpdate(ID_EDIT_REDO, [this](CommandState& state) { OnUpdateEditRedo(state); });
	addCommand(ID_EDIT_CUT, [this]() { OnEditCut(); });
	addCommand(ID_EDIT_COPY, [this]() { OnEditCopy(); });
	addCommand(ID_EDIT_PASTE, [this]() { OnEditPaste(); });
	addCommand(ID_EDIT_CLEAR, [this]() { OnEditClear(); });
	addCommand(ID_EDIT_UNDO, [this]() { OnEditUndo(); });
	addCommand(ID_EDIT_REDO, [this]() { OnEditRedo(); });
	addCommand(ID_EDIT_SELECT_ALL, [this]() { OnEditSelectAll(); });
	addCommand(ID_EDIT_FIND, [this]() { OnEditFind(); });
	addCommand(ID_EDIT_REPLACE, [this]() { OnEditReplace(); });
	addCommand(ID_EDIT_REPEAT, [this]() { OnEditRepeat(); });
	addCommandUpdate(ID_EDIT_COPY, [this](CommandState& state) { OnUpdateNeedSel(state); });
	addCommandUpdate(ID_EDIT_CLEAR, [this](CommandState& state) { OnUpdateNeedTextAndFollowingText(state); });
	addCommandUpdate(ID_EDIT_SELECT_ALL, [this](CommandState& state) { OnUpdateNeedText(state); });
	addCommandUpdate(ID_EDIT_REPLACE, [this](CommandState& state) { OnUpdateNeedText(state); });
	addCommand(ID_FILE_PRINT, [this]() { filePrint(); });
	addCommand(ID_FILE_PRINT_DIRECT, [this]() { filePrint(true); });
	addCommand(ID_FILE_PRINT_PREVIEW, [this]() { filePrintPreview(); });
}

CScintillaView::~CScintillaView()
{
	scintillaViewCount--;

	if (scintillaEditState.pFindReplaceDlg) {
		if (scintillaEditState.pFindReplaceDlg->GetOwner() == this) {
			scintillaEditState.pFindReplaceDlg->SetOwner(nullptr);
		}

		if (scintillaViewCount == 0) {
			CScintillaFindReplaceDlg* pDialog = scintillaEditState.pFindReplaceDlg;
			pDialog->Close();
			scintillaEditState.pFindReplaceDlg = nullptr;
		}
	}
}

CScintillaCtrl& CScintillaView::GetCtrl()
{
	return m_Edit;
}

bool CScintillaView::create()
{
	if (!View::create()) {
		return false;
	}

	if (!m_Edit.Create(this)) {
		return false;
	}

	m_Edit.SetNotifyHandler([this](SCNotification* pSCNotification) { OnNotify(pSCNotification); });
	m_Edit.SetCommandHandler([this](int notification) { OnCommand(notification); });
	m_Edit.MoveWindow(controlRect());
	setFocusProxy(m_Edit.GetWidget());
	return true;
}

void CScintillaView::DeleteContents()
{
	CScintillaCtrl& rCtrl = GetCtrl();
	rCtrl.ClearAll();
	rCtrl.EmptyUndoBuffer();
}

bool CScintillaView::onPreparePrinting(PrintInfo& info)
{
	CScintillaCtrl& rCtrl = GetCtrl();

	Sci_Position nStartChar = rCtrl.GetSelectionStart();
	Sci_Position nEndChar = rCtrl.GetSelectionEnd();
	if (nStartChar != nEndChar) {
		info.checkSelection();
	}

	return doPreparePrinting(info);
}

void CScintillaView::onBeginPrinting(PrintDC& dc, PrintInfo& info)
{
	CScintillaCtrl& rCtrl = GetCtrl();

	if (info.printSelection()) {
		m_aPageStart.push_back(rCtrl.GetSelectionStart());
	}
	else {
		m_aPageStart.push_back(0);
	}
}

bool CScintillaView::PaginateTo(PrintDC& dc, PrintInfo& info)
{
	QRect rectSave = info.rectDraw;
	unsigned nPageSave = info.curPage;
	dc.painter->save();
	dc.painter->setClipRect(QRect(0, 0, 0, 0));
	info.curPage = static_cast<unsigned>(m_aPageStart.size());
	while (info.curPage < nPageSave) {
		onPrepareDC(dc, &info);
		info.rectDraw = QRect(QPoint(0, 0), dc.resolution);
		onPrint(dc, info);
		if (info.curPage == m_aPageStart.size()) {
			break;
		}
		++info.curPage;
	}
	bool bResult = info.curPage == nPageSave;
	dc.painter->restore();
	info.curPage = nPageSave;
	info.rectDraw = rectSave;
	return bResult;
}

void CScintillaView::onPrepareDC(PrintDC& dc, PrintInfo* info)
{
	if (info->curPage <= info->maxPage()) {
		if (info->curPage > m_aPageStart.size() && !PaginateTo(dc, *info)) {
			info->continuePrinting = false;
		}
	}
	else {
		info->continuePrinting = false;
	}
}

void CScintillaView::PrintHeader(PrintDC& dc, PrintInfo& info, Sci_RangeToFormat& frPrint)
{
	QString sHeader =
		Resources::string(IDS_SCINTILLA_DEFAULT_PRINT_HEADER).arg(document()->title(), Printing::currentDateTime());

	QPainter* painter = dc.painter;
	painter->save();
	painter->setFont(dc.font);
	painter->setPen(QPen(Qt::black, 0));

	QFontMetrics metrics(dc.font, dc.attrib);
	int nHeaderDepth = 2 * metrics.height();
	painter->drawText(QPoint(frPrint.rcPage.left, frPrint.rcPage.top + nHeaderDepth / 3 + metrics.ascent()), sHeader);

	painter->drawLine(
		frPrint.rcPage.left,
		frPrint.rcPage.top + nHeaderDepth - 5,
		frPrint.rcPage.right - 1,
		frPrint.rcPage.top + nHeaderDepth - 5
	);

	painter->restore();

	if (frPrint.rc.top < frPrint.rcPage.top + nHeaderDepth) {
		frPrint.rc.top = frPrint.rcPage.top + nHeaderDepth;
	}
}

void CScintillaView::PrintFooter(PrintDC& dc, PrintInfo& info, Sci_RangeToFormat& frPrint)
{
	QString sFooter = Resources::string(IDS_SCINTILLA_DEFAULT_PRINT_FOOTER).arg(QString::number(info.curPage));

	QPainter* painter = dc.painter;
	painter->save();
	painter->setFont(dc.font);
	painter->setPen(QPen(Qt::black, 0));

	QFontMetrics metrics(dc.font, dc.attrib);
	int nFooterDepth = 2 * metrics.height();
	painter->drawText(
		QPoint(frPrint.rcPage.left, frPrint.rcPage.bottom - nFooterDepth * 2 / 3 + metrics.ascent()), sFooter
	);

	painter->drawLine(
		frPrint.rcPage.left,
		frPrint.rcPage.bottom - nFooterDepth + 5,
		frPrint.rcPage.right - 1,
		frPrint.rcPage.bottom - nFooterDepth + 5
	);

	painter->restore();

	if (frPrint.rc.bottom > frPrint.rcPage.bottom - nFooterDepth) {
		frPrint.rc.bottom = frPrint.rcPage.bottom - nFooterDepth;
	}
}

Sci_Position CScintillaView::PrintPage(PrintDC& dc, PrintInfo& info, Sci_Position nIndexStart, Sci_Position nIndexStop)
{
	Sci_RangeToFormat rfPrint;
	rfPrint.hdc = dc.painter->device();
	rfPrint.hdcTarget = dc.attrib;

	rfPrint.rcPage.left = info.rectDraw.left();
	rfPrint.rcPage.top = info.rectDraw.top();
	rfPrint.rcPage.right = info.rectDraw.right();
	rfPrint.rcPage.bottom = info.rectDraw.bottom();

	rfPrint.rc = rfPrint.rcPage;

	rfPrint.chrg.cpMin = static_cast<Sci_PositionCR>(nIndexStart);
	rfPrint.chrg.cpMax = static_cast<Sci_PositionCR>(nIndexStop);

	PrintHeader(dc, info, rfPrint);
	PrintFooter(dc, info, rfPrint);

	return GetCtrl().FormatRange(true, &rfPrint);
}

void CScintillaView::onPrint(PrintDC& dc, PrintInfo& info)
{
	unsigned nPage = info.curPage;
	Sci_Position nIndex = m_aPageStart[nPage - 1];

	Sci_Position nEndPrint = 0;
	if (info.printSelection()) {
		nEndPrint = GetCtrl().GetSelectionEnd();
	}
	else {
		nEndPrint = GetCtrl().GetTextLength();
	}

	nIndex = PrintPage(dc, info, nIndex, nEndPrint);
	if (nIndex >= nEndPrint) {
		info.setMaxPage(nPage);
	}

	if (nPage == m_aPageStart.size()) {
		if (nIndex < nEndPrint) {
			m_aPageStart.push_back(nIndex);
		}
	}
}

void CScintillaView::onEndPrinting(PrintDC& dc, PrintInfo& info)
{
	m_aPageStart.clear();
}

void CScintillaView::OnUpdateNeedPaste(CommandState& state)
{
	state.enabled = GetCtrl().CanPaste();
}

void CScintillaView::OnUpdateNeedText(CommandState& state)
{
	state.enabled = GetCtrl().GetTextLength() != 0;
}

void CScintillaView::OnUpdateNeedTextAndFollowingText(CommandState& state)
{
	CScintillaCtrl& rCtrl = GetCtrl();
	int nLength = rCtrl.GetTextLength();
	Sci_Position nStartChar = rCtrl.GetSelectionStart();
	state.enabled = nLength && (nStartChar != nLength);
}

void CScintillaView::OnUpdateNeedFind(CommandState& state)
{
	state.enabled = GetCtrl().GetLength() != 0 && !scintillaEditState.strFind.isEmpty();
}

void CScintillaView::OnUpdateEditUndo(CommandState& state)
{
	state.enabled = GetCtrl().CanUndo();
}

void CScintillaView::OnUpdateEditRedo(CommandState& state)
{
	state.enabled = GetCtrl().CanRedo();
}

void CScintillaView::OnUpdateNeedSel(CommandState& state)
{
	CScintillaCtrl& rCtrl = GetCtrl();
	Sci_Position nStartChar = rCtrl.GetSelectionStart();
	Sci_Position nEndChar = rCtrl.GetSelectionEnd();
	state.enabled = nStartChar != nEndChar;
}

void CScintillaView::OnEditCut()
{
	GetCtrl().Cut();
}

void CScintillaView::OnEditCopy()
{
	GetCtrl().Copy();
}

void CScintillaView::OnEditPaste()
{
	GetCtrl().Paste();
}

void CScintillaView::OnEditClear()
{
	GetCtrl().Clear();
}

void CScintillaView::OnEditUndo()
{
	GetCtrl().Undo();
}

void CScintillaView::OnEditRedo()
{
	GetCtrl().Redo();
}

void CScintillaView::OnEditSelectAll()
{
	GetCtrl().SelectAll();
}

void CScintillaView::OnEditFind()
{
	OnEditFindReplace(true);
}

void CScintillaView::OnEditReplace()
{
	OnEditFindReplace(false);
}

void CScintillaView::OnEditRepeat()
{
	if (!FindText(
			scintillaEditState.strFind,
			scintillaEditState.bNext,
			scintillaEditState.bCase,
			scintillaEditState.bWord,
			scintillaEditState.bRegularExpression
		)) {
		TextNotFound();
	}
}

void CScintillaView::AdjustFindDialogPosition()
{
	CScintillaCtrl& rCtrl = GetCtrl();
	Sci_Position nStart = rCtrl.GetSelectionStart();
	QPoint point(rCtrl.PointXFromPosition(nStart), rCtrl.PointYFromPosition(nStart));
	point = rCtrl.ClientToScreen(point);
	QRect rectDlg = scintillaEditState.pFindReplaceDlg->frameGeometry();
	if (rectDlg.contains(point)) {
		if (point.y() > rectDlg.height()) {
			rectDlg.translate(0, point.y() - (rectDlg.y() + rectDlg.height()) - 20);
		}
		else {
			int nVertExt = QGuiApplication::primaryScreen()->geometry().height();
			if (point.y() + rectDlg.height() < nVertExt) {
				rectDlg.translate(0, 40 + point.y() - rectDlg.y());
			}
		}
		scintillaEditState.pFindReplaceDlg->move(rectDlg.topLeft());
	}
}

void CScintillaView::OnEditFindReplace(bool bFindOnly)
{
	if (scintillaEditState.pFindReplaceDlg != nullptr) {
		if (scintillaEditState.bFindOnly == bFindOnly) {
			scintillaEditState.pFindReplaceDlg->show();
			scintillaEditState.pFindReplaceDlg->activateWindow();
			return;
		}
		else {
			CScintillaFindReplaceDlg* pDialog = scintillaEditState.pFindReplaceDlg;
			pDialog->Close();
			scintillaEditState.pFindReplaceDlg = nullptr;
		}
	}
	CScintillaCtrl& rCtrl = GetCtrl();
	QString strFind = rCtrl.GetSelText();
	if (strFind.isEmpty() || strFind.contains('\n') || strFind.contains('\r')) {
		strFind = scintillaEditState.strFind;
	}

	QString strReplace = scintillaEditState.strReplace;
	scintillaEditState.pFindReplaceDlg = new CScintillaFindReplaceDlg(bFindOnly, window());
	unsigned dwFlags = 0;
	if (scintillaEditState.bNext) {
		dwFlags |= FindReplaceDown;
	}
	if (scintillaEditState.bCase) {
		dwFlags |= FindReplaceMatchCase;
	}
	if (scintillaEditState.bWord) {
		dwFlags |= FindReplaceWholeWord;
	}
	if (scintillaEditState.bRegularExpression) {
		scintillaEditState.pFindReplaceDlg->SetRegularExpression(true);
	}

	if (!scintillaEditState.pFindReplaceDlg->Create(bFindOnly, strFind, strReplace, dwFlags, this)) {
		delete scintillaEditState.pFindReplaceDlg;
		scintillaEditState.pFindReplaceDlg = nullptr;
		return;
	}
	scintillaEditState.bFindOnly = bFindOnly;
	scintillaEditState.pFindReplaceDlg->show();
	scintillaEditState.pFindReplaceDlg->activateWindow();
}

void CScintillaView::OnFindNext(const QString& find, bool bNext, bool bCase, bool bWord, bool bRegularExpression)
{
	scintillaEditState.strFind = find;
	scintillaEditState.bCase = bCase;
	scintillaEditState.bWord = bWord;
	scintillaEditState.bNext = bNext;
	scintillaEditState.bRegularExpression = bRegularExpression;

	if (!FindText(scintillaEditState.strFind, bNext, bCase, bWord, bRegularExpression)) {
		TextNotFound();
	}
	else {
		AdjustFindDialogPosition();
	}
}

void CScintillaView::OnReplaceSel(
	const QString& find, bool bNext, bool bCase, bool bWord, bool bRegularExpression, const QString& replace
)
{
	scintillaEditState.strFind = find;
	scintillaEditState.strReplace = replace;
	scintillaEditState.bCase = bCase;
	scintillaEditState.bWord = bWord;
	scintillaEditState.bNext = bNext;
	scintillaEditState.bRegularExpression = bRegularExpression;

	if (!SameAsSelected(scintillaEditState.strFind, bCase, bWord, bRegularExpression)) {
		if (!FindText(scintillaEditState.strFind, bNext, bCase, bWord, bRegularExpression)) {
			TextNotFound();
		}
		else {
			AdjustFindDialogPosition();
		}
		return;
	}

	GetCtrl().ReplaceSel(scintillaEditState.strReplace.toLocal8Bit().constData());
	if (!FindText(scintillaEditState.strFind, bNext, bCase, bWord, bRegularExpression)) {
		TextNotFound();
	}
	else {
		AdjustFindDialogPosition();
	}
}

void CScintillaView::OnReplaceAll(
	const QString& find, const QString& replace, bool bCase, bool bWord, bool bRegularExpression
)
{
	scintillaEditState.strFind = find;
	scintillaEditState.strReplace = replace;
	scintillaEditState.bCase = bCase;
	scintillaEditState.bWord = bWord;
	scintillaEditState.bNext = true;
	scintillaEditState.bRegularExpression = bRegularExpression;

	WaitCursor wait;

	CScintillaCtrl& rCtrl = GetCtrl();
	rCtrl.SetSel(0, 0);

	rCtrl.HideSelection(true);
	bool bFoundSomething = false;
	QByteArray replaceText = scintillaEditState.strReplace.toLocal8Bit();
	while (FindTextSimple(scintillaEditState.strFind, scintillaEditState.bNext, bCase, bWord, bRegularExpression)) {
		bFoundSomething = true;
		rCtrl.ReplaceSel(replaceText.constData());
	}

	rCtrl.HideSelection(false);

	if (!bFoundSomething) {
		TextNotFound();
	}
}

void CScintillaView::OnFindReplaceCmd(CScintillaFindReplaceDlg* pDialog)
{
	if (pDialog->IsTerminating()) {
		scintillaEditState.pFindReplaceDlg = nullptr;
	}
	else if (pDialog->FindNext()) {
		OnFindNext(
			pDialog->GetFindString(),
			pDialog->SearchDown(),
			pDialog->MatchCase(),
			pDialog->MatchWholeWord(),
			pDialog->GetRegularExpression()
		);
	}
	else if (pDialog->ReplaceCurrent()) {
		OnReplaceSel(
			pDialog->GetFindString(),
			pDialog->SearchDown(),
			pDialog->MatchCase(),
			pDialog->MatchWholeWord(),
			pDialog->GetRegularExpression(),
			pDialog->GetReplaceString()
		);
	}
	else if (pDialog->ReplaceAll()) {
		OnReplaceAll(
			pDialog->GetFindString(),
			pDialog->GetReplaceString(),
			pDialog->MatchCase(),
			pDialog->MatchWholeWord(),
			pDialog->GetRegularExpression()
		);
	}
}

bool CScintillaView::SameAsSelected(const QString& compare, bool bCase, bool bWord, bool bRegularExpression)
{
	CScintillaCtrl& rCtrl = GetCtrl();

	Sci_Position nStartChar = rCtrl.GetSelectionStart();
	Sci_Position nEndChar = rCtrl.GetSelectionEnd();
	QByteArray compareText = compare.toLocal8Bit();
	size_t nLen = static_cast<size_t>(compareText.size());
	if (!bRegularExpression && (nLen != static_cast<size_t>(nEndChar - nStartChar))) {
		return false;
	}

	int iFlags = bCase ? SCFIND_MATCHCASE : 0;
	iFlags |= bWord ? SCFIND_WHOLEWORD : 0;
	iFlags |= bRegularExpression ? SCFIND_REGEXP : 0;
	rCtrl.SetSearchFlags(iFlags);
	rCtrl.TargetFromSelection();
	if (rCtrl.SearchInTarget(static_cast<int>(nLen), compareText.constData()) < 0) {
		return false;
	}

	return (rCtrl.GetTargetStart() == nStartChar) && (rCtrl.GetTargetEnd() == nEndChar);
}

bool CScintillaView::FindText(const QString& find, bool bNext, bool bCase, bool bWord, bool bRegularExpression)
{
	WaitCursor wait;
	return FindTextSimple(find, bNext, bCase, bWord, bRegularExpression);
}

bool CScintillaView::FindTextSimple(const QString& find, bool bNext, bool bCase, bool bWord, bool bRegularExpression)
{
	CScintillaCtrl& rCtrl = GetCtrl();
	Sci_TextToFind ft;
	ft.chrg.cpMin = static_cast<Sci_PositionCR>(rCtrl.GetSelectionStart());
	ft.chrg.cpMax = static_cast<Sci_PositionCR>(rCtrl.GetSelectionEnd());
	QByteArray findText = find.toLocal8Bit();
	ft.lpstrText = findText.constData();
	if (ft.chrg.cpMin != ft.chrg.cpMax) {
		if (bNext) {
			ft.chrg.cpMin++;
		}
		else {
			ft.chrg.cpMax = ft.chrg.cpMin - 1;
		}
	}

	int nLength = rCtrl.GetLength();
	if (bNext) {
		ft.chrg.cpMax = nLength;
	}
	else {
		ft.chrg.cpMin = 0;
	}

	unsigned dwFlags = bCase ? SCFIND_MATCHCASE : 0;
	dwFlags |= bWord ? SCFIND_WHOLEWORD : 0;
	dwFlags |= bRegularExpression ? SCFIND_REGEXP : 0;

	if (!bNext) {
		Sci_PositionCR ncpMinTemp = ft.chrg.cpMin;
		ft.chrg.cpMin = ft.chrg.cpMax;
		ft.chrg.cpMax = ncpMinTemp;
	}

	return (FindAndSelect(dwFlags, ft) != -1);
}

Sci_Position CScintillaView::FindAndSelect(unsigned dwFlags, Sci_TextToFind& ft)
{
	CScintillaCtrl& rCtrl = GetCtrl();
	Sci_Position index = rCtrl.FindText(static_cast<int>(dwFlags), &ft);
	if (index != -1) {
		rCtrl.SetSel(ft.chrgText.cpMin, ft.chrgText.cpMax);
	}
	return index;
}

void CScintillaView::TextNotFound()
{
	QApplication::beep();
}

void CScintillaView::resizeEvent(QResizeEvent* event)
{
	View::resizeEvent(event);

	if (m_Edit.GetWidget()) {
		m_Edit.MoveWindow(controlRect());
	}
}

void CScintillaView::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	WhidbeyStyle::drawClientEdge(&painter, rect());
}

QRect CScintillaView::controlRect() const
{
	int edge = WhidbeyStyle::clientEdgeWidth;
	return rect().adjusted(edge, edge, -edge, -edge);
}

void CScintillaView::activate(bool active, View* other)
{
	if (scintillaEditState.pFindReplaceDlg && active) {
		scintillaEditState.pFindReplaceDlg->SetOwner(this);
	}

	View::activate(active, other);
}

void CScintillaView::Serialize(QFile& file, bool loading)
{
	CScintillaCtrl& rCtrl = GetCtrl();
	if (loading) {
		rCtrl.Cancel();
		rCtrl.SetUndoCollection(false);

		char Buffer[4096];
		qint64 nBytesRead = 0;
		do {
			nBytesRead = file.read(Buffer, 4096);
			if (nBytesRead > 0) {
				rCtrl.AddText(static_cast<int>(nBytesRead), Buffer);
			}
		} while (nBytesRead > 0);

		if (!QFileInfo(file.fileName()).isWritable()) {
			rCtrl.SetReadOnly(true);
		}
		else {
			rCtrl.SetReadOnly(false);
		}

		rCtrl.SetUndoCollection(true);
		rCtrl.EmptyUndoBuffer();
		rCtrl.SetSavePoint();
		rCtrl.GotoPos(0);
	}
	else {
		int nDocLength = rCtrl.GetLength();

		for (int i = 0; i < nDocLength; i += 4095) {
			int nGrabSize = nDocLength - i;
			if (nGrabSize > 4095) {
				nGrabSize = 4095;
			}

			Sci_TextRange tr;
			tr.chrg.cpMin = i;
			tr.chrg.cpMax = i + nGrabSize;
			char Buffer[4096];
			tr.lpstrText = Buffer;
			rCtrl.GetTextRange(&tr);

			file.write(Buffer, nGrabSize);
		}
	}
}

void CScintillaView::OnCharAdded(SCNotification* pSCNotification)
{
}

void CScintillaView::OnMarginClick(SCNotification* pSCNotification)
{
	CScintillaCtrl& rCtrl = GetCtrl();
	int nLine = rCtrl.LineFromPosition(pSCNotification->position);
	rCtrl.ToggleFold(nLine);
}

void CScintillaView::OnDwellStart(SCNotification* pSCNotification)
{
}

void CScintillaView::OnDwellEnd(SCNotification* pSCNotification)
{
}

void CScintillaView::OnKillFocus()
{
}

void CScintillaView::OnCommand(int notification)
{
	if (notification == SCEN_KILLFOCUS) {
		OnKillFocus();
	}
}

void CScintillaView::OnNotify(SCNotification* pSCNotification)
{
	switch (pSCNotification->nmhdr.code) {
	case SCN_CHARADDED:
		OnCharAdded(pSCNotification);
		break;
	case SCN_MARGINCLICK:
		OnMarginClick(pSCNotification);
		break;
	case SCN_DWELLSTART:
		OnDwellStart(pSCNotification);
		break;
	case SCN_DWELLEND:
		OnDwellEnd(pSCNotification);
		break;
	default:
		break;
	}
}

CScintillaDoc::CScintillaDoc() : m_bModified(false)
{
}

CScintillaView* CScintillaDoc::GetView() const
{
	for (View* pView : views()) {
		if (CScintillaView* pScintillaView = dynamic_cast<CScintillaView*>(pView)) {
			return pScintillaView;
		}
	}

	return nullptr;
}

void CScintillaDoc::setModified(bool modified)
{
	CScintillaView* pView = GetView();
	if (pView && !modified) {
		pView->GetCtrl().SetSavePoint();
	}
	m_bModified = modified;
}

bool CScintillaDoc::isModified() const
{
	CScintillaView* pView = GetView();
	return m_bModified || (pView && pView->GetCtrl().GetModify());
}

void CScintillaDoc::deleteContents()
{
	Document::deleteContents();

	WaitCursor wait;
	CScintillaView* pView = GetView();
	if (pView) {
		pView->DeleteContents();
	}
}

void CScintillaDoc::Serialize(QFile& file, bool loading)
{
	CScintillaView* pView = GetView();
	if (pView) {
		pView->Serialize(file, loading);
	}
}

bool CScintillaDoc::openDocument(const QString& pathName)
{
	QFile file(pathName);
	if (!file.open(QIODevice::ReadOnly)) {
		return false;
	}

	deleteContents();
	setModified(true);
	Serialize(file, true);
	file.close();
	setModified(false);
	return true;
}

bool CScintillaDoc::saveDocument(const QString& pathName)
{
	QFile file(pathName);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return false;
	}

	Serialize(file, false);
	bool bSuccess = file.flush() && file.error() == QFileDevice::NoError;
	file.close();
	if (bSuccess) {
		setModified(false);

		CScintillaView* pView = GetView();
		if (pView) {
			CScintillaCtrl& rCtrl = pView->GetCtrl();
			rCtrl.SetSavePoint();
			rCtrl.SetReadOnly(false);
		}
	}
	return bSuccess;
}

} // namespace Scintilla
