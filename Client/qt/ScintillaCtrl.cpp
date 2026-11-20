#include "ScintillaCtrl.h"

#include <QAbstractScrollArea>
#include <QElapsedTimer>
#include <QMimeData>
#include <string>

#define signals Q_SIGNALS
#define slots Q_SLOTS
#include "ScintillaEditBase.h"
#undef signals
#undef slots

#include "ILexer.h"
#include "Lexilla.h"

namespace Scintilla {

CScintillaCtrl::CScintillaCtrl() : m_widget(nullptr)
{
}

bool CScintillaCtrl::Create(QWidget* pParentWnd)
{
	m_widget = new ScintillaEditBase(pParentWnd);

	QObject::connect(m_widget, &ScintillaEditBase::notify, m_widget, [this](NotificationData* notification) {
		if (m_notifyHandler) {
			m_notifyHandler(reinterpret_cast<SCNotification*>(notification));
		}
	});
	QObject::connect(m_widget, &ScintillaEditBase::command, m_widget, [this](uptr_t wParam, sptr_t lParam) {
		if (m_commandHandler) {
			m_commandHandler(static_cast<int>((wParam >> 16) & 0xffff));
		}
	});

	SetCodePage(0);
	return true;
}

CScintillaCtrl::~CScintillaCtrl()
{
	m_notifyHandler = nullptr;
	m_commandHandler = nullptr;
	delete m_widget;
}

void CScintillaCtrl::SetNotifyHandler(std::function<void(SCNotification*)> handler)
{
	m_notifyHandler = std::move(handler);
}

void CScintillaCtrl::SetCommandHandler(std::function<void(int)> handler)
{
	m_commandHandler = std::move(handler);
}

QWidget* CScintillaCtrl::GetWidget() const
{
	return m_widget;
}

void CScintillaCtrl::MoveWindow(const QRect& rect)
{
	m_widget->setGeometry(rect);
}

void CScintillaCtrl::SetFocus()
{
	m_widget->setFocus();
}

bool CScintillaCtrl::HasFocus() const
{
	return m_widget->hasFocus();
}

QPoint CScintillaCtrl::ClientToScreen(const QPoint& point) const
{
	return m_widget->viewport()->mapToGlobal(point);
}

sptr_t CScintillaCtrl::Call(unsigned int message, uptr_t wParam, sptr_t lParam)
{
	return m_widget->send(message, wParam, lParam);
}

QString CScintillaCtrl::GetSelText()
{
	std::string text(static_cast<size_t>(Call(SCI_GETSELTEXT, 0, 0)), '\0');
	GetSelText(text.data());
	return QString::fromLocal8Bit(text.c_str());
}

void CScintillaCtrl::SetLexer(int lexer)
{
	int count = GetLexerCount();
	for (int index = 0; index < count; index++) {
		char name[100];
		GetLexerName(static_cast<unsigned int>(index), name, sizeof(name));
		ILexer5* instance = CreateLexer(name);
		if (instance == nullptr) {
			continue;
		}
		if (instance->GetIdentifier() == lexer) {
			Call(SCI_SETILEXER, 0, reinterpret_cast<sptr_t>(instance));
			return;
		}
		instance->Release();
	}
	Call(SCI_SETILEXER, 0, 0);
}

void CScintillaCtrl::AddText(int length, const char* text)
{
	Call(SCI_ADDTEXT, static_cast<uptr_t>(length), reinterpret_cast<sptr_t>(text));
}

void CScintillaCtrl::ClearAll()
{
	Call(SCI_CLEARALL, 0, 0);
}

int CScintillaCtrl::GetLength()
{
	return static_cast<int>(Call(SCI_GETLENGTH, 0, 0));
}

Sci_Position CScintillaCtrl::GetCurrentPos()
{
	return static_cast<Sci_Position>(Call(SCI_GETCURRENTPOS, 0, 0));
}

int CScintillaCtrl::GetStyleAt(Sci_Position pos)
{
	return static_cast<int>(Call(SCI_GETSTYLEAT, static_cast<uptr_t>(pos), 0));
}

void CScintillaCtrl::Redo()
{
	Call(SCI_REDO, 0, 0);
}

void CScintillaCtrl::SetUndoCollection(bool collectUndo)
{
	Call(SCI_SETUNDOCOLLECTION, static_cast<uptr_t>(collectUndo), 0);
}

void CScintillaCtrl::SelectAll()
{
	Call(SCI_SELECTALL, 0, 0);
}

void CScintillaCtrl::SetSavePoint()
{
	Call(SCI_SETSAVEPOINT, 0, 0);
}

bool CScintillaCtrl::CanRedo()
{
	return Call(SCI_CANREDO, 0, 0) != 0;
}

void CScintillaCtrl::GotoLine(int line)
{
	Call(SCI_GOTOLINE, static_cast<uptr_t>(line), 0);
}

void CScintillaCtrl::GotoPos(Sci_Position pos)
{
	Call(SCI_GOTOPOS, static_cast<uptr_t>(pos), 0);
}

void CScintillaCtrl::SetCodePage(int codePage)
{
	Call(SCI_SETCODEPAGE, static_cast<uptr_t>(codePage), 0);
}

void CScintillaCtrl::MarkerDefine(int markerNumber, int markerSymbol)
{
	Call(SCI_MARKERDEFINE, static_cast<uptr_t>(markerNumber), static_cast<sptr_t>(markerSymbol));
}

void CScintillaCtrl::MarkerSetFore(int markerNumber, Colour fore)
{
	Call(SCI_MARKERSETFORE, static_cast<uptr_t>(markerNumber), static_cast<sptr_t>(fore));
}

void CScintillaCtrl::MarkerSetBack(int markerNumber, Colour back)
{
	Call(SCI_MARKERSETBACK, static_cast<uptr_t>(markerNumber), static_cast<sptr_t>(back));
}

int CScintillaCtrl::MarkerAdd(int line, int markerNumber)
{
	return static_cast<int>(Call(SCI_MARKERADD, static_cast<uptr_t>(line), static_cast<sptr_t>(markerNumber)));
}

void CScintillaCtrl::MarkerDelete(int line, int markerNumber)
{
	Call(SCI_MARKERDELETE, static_cast<uptr_t>(line), static_cast<sptr_t>(markerNumber));
}

int CScintillaCtrl::MarkerGet(int line)
{
	return static_cast<int>(Call(SCI_MARKERGET, static_cast<uptr_t>(line), 0));
}

int CScintillaCtrl::MarkerNext(int lineStart, int markerMask)
{
	return static_cast<int>(Call(SCI_MARKERNEXT, static_cast<uptr_t>(lineStart), static_cast<sptr_t>(markerMask)));
}

int CScintillaCtrl::MarkerPrevious(int lineStart, int markerMask)
{
	return static_cast<int>(Call(SCI_MARKERPREVIOUS, static_cast<uptr_t>(lineStart), static_cast<sptr_t>(markerMask)));
}

void CScintillaCtrl::SetMarginTypeN(int margin, int marginType)
{
	Call(SCI_SETMARGINTYPEN, static_cast<uptr_t>(margin), static_cast<sptr_t>(marginType));
}

void CScintillaCtrl::SetMarginWidthN(int margin, int pixelWidth)
{
	Call(SCI_SETMARGINWIDTHN, static_cast<uptr_t>(margin), static_cast<sptr_t>(pixelWidth));
}

int CScintillaCtrl::GetMarginWidthN(int margin)
{
	return static_cast<int>(Call(SCI_GETMARGINWIDTHN, static_cast<uptr_t>(margin), 0));
}

void CScintillaCtrl::SetMarginMaskN(int margin, int mask)
{
	Call(SCI_SETMARGINMASKN, static_cast<uptr_t>(margin), static_cast<sptr_t>(mask));
}

void CScintillaCtrl::SetMarginSensitiveN(int margin, bool sensitive)
{
	Call(SCI_SETMARGINSENSITIVEN, static_cast<uptr_t>(margin), static_cast<sptr_t>(sensitive));
}

void CScintillaCtrl::StyleClearAll()
{
	Call(SCI_STYLECLEARALL, 0, 0);
}

void CScintillaCtrl::StyleSetFore(int style, Colour fore)
{
	Call(SCI_STYLESETFORE, static_cast<uptr_t>(style), static_cast<sptr_t>(fore));
}

void CScintillaCtrl::StyleSetBack(int style, Colour back)
{
	Call(SCI_STYLESETBACK, static_cast<uptr_t>(style), static_cast<sptr_t>(back));
}

void CScintillaCtrl::StyleSetBold(int style, bool bold)
{
	Call(SCI_STYLESETBOLD, static_cast<uptr_t>(style), static_cast<sptr_t>(bold));
}

void CScintillaCtrl::StyleSetSize(int style, int sizePoints)
{
	Call(SCI_STYLESETSIZE, static_cast<uptr_t>(style), static_cast<sptr_t>(sizePoints));
}

void CScintillaCtrl::StyleSetFont(int style, const char* fontName)
{
	Call(SCI_STYLESETFONT, static_cast<uptr_t>(style), reinterpret_cast<sptr_t>(fontName));
}

void CScintillaCtrl::AutoCShow(int lenEntered, const char* itemList)
{
	Call(SCI_AUTOCSHOW, static_cast<uptr_t>(lenEntered), reinterpret_cast<sptr_t>(itemList));
}

void CScintillaCtrl::AutoCSetSeparator(int separatorCharacter)
{
	Call(SCI_AUTOCSETSEPARATOR, static_cast<uptr_t>(separatorCharacter), 0);
}

int CScintillaCtrl::GetColumn(Sci_Position pos)
{
	return static_cast<int>(Call(SCI_GETCOLUMN, static_cast<uptr_t>(pos), 0));
}

Sci_Position CScintillaCtrl::GetSelectionStart()
{
	return static_cast<Sci_Position>(Call(SCI_GETSELECTIONSTART, 0, 0));
}

Sci_Position CScintillaCtrl::GetSelectionEnd()
{
	return static_cast<Sci_Position>(Call(SCI_GETSELECTIONEND, 0, 0));
}

Sci_Position CScintillaCtrl::FindText(int flags, Sci_TextToFind* ft)
{
	return static_cast<Sci_Position>(Call(SCI_FINDTEXT, static_cast<uptr_t>(flags), reinterpret_cast<sptr_t>(ft)));
}

Sci_Position CScintillaCtrl::FormatRange(bool draw, Sci_RangeToFormat* fr)
{
	return static_cast<Sci_Position>(Call(SCI_FORMATRANGE, static_cast<uptr_t>(draw), reinterpret_cast<sptr_t>(fr)));
}

bool CScintillaCtrl::GetModify()
{
	return Call(SCI_GETMODIFY, 0, 0) != 0;
}

void CScintillaCtrl::SetSel(Sci_Position start, Sci_Position end)
{
	Call(SCI_SETSEL, static_cast<uptr_t>(start), static_cast<sptr_t>(end));
}

int CScintillaCtrl::GetSelText(char* text)
{
	return static_cast<int>(Call(SCI_GETSELTEXT, 0, reinterpret_cast<sptr_t>(text)));
}

int CScintillaCtrl::GetTextRange(Sci_TextRange* tr)
{
	return static_cast<int>(Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<sptr_t>(tr)));
}

void CScintillaCtrl::HideSelection(bool normal)
{
	Call(SCI_HIDESELECTION, static_cast<uptr_t>(normal), 0);
}

int CScintillaCtrl::PointXFromPosition(Sci_Position pos)
{
	return static_cast<int>(Call(SCI_POINTXFROMPOSITION, 0, static_cast<sptr_t>(pos)));
}

int CScintillaCtrl::PointYFromPosition(Sci_Position pos)
{
	return static_cast<int>(Call(SCI_POINTYFROMPOSITION, 0, static_cast<sptr_t>(pos)));
}

int CScintillaCtrl::LineFromPosition(Sci_Position pos)
{
	return static_cast<int>(Call(SCI_LINEFROMPOSITION, static_cast<uptr_t>(pos), 0));
}

void CScintillaCtrl::ReplaceSel(const char* text)
{
	Call(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(text));
}

void CScintillaCtrl::SetReadOnly(bool readOnly)
{
	Call(SCI_SETREADONLY, static_cast<uptr_t>(readOnly), 0);
}

bool CScintillaCtrl::CanPaste()
{
	return Call(SCI_CANPASTE, 0, 0) != 0;
}

bool CScintillaCtrl::CanUndo()
{
	return Call(SCI_CANUNDO, 0, 0) != 0;
}

void CScintillaCtrl::EmptyUndoBuffer()
{
	Call(SCI_EMPTYUNDOBUFFER, 0, 0);
}

void CScintillaCtrl::Undo()
{
	Call(SCI_UNDO, 0, 0);
}

void CScintillaCtrl::Cut()
{
	Call(SCI_CUT, 0, 0);
}

void CScintillaCtrl::Copy()
{
	Call(SCI_COPY, 0, 0);
}

void CScintillaCtrl::Paste()
{
	Call(SCI_PASTE, 0, 0);
}

void CScintillaCtrl::Clear()
{
	Call(SCI_CLEAR, 0, 0);
}

void CScintillaCtrl::SetText(const char* text)
{
	Call(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>(text));
}

int CScintillaCtrl::GetText(int length, char* text)
{
	return static_cast<int>(Call(SCI_GETTEXT, static_cast<uptr_t>(length), reinterpret_cast<sptr_t>(text)));
}

int CScintillaCtrl::GetTextLength()
{
	return static_cast<int>(Call(SCI_GETTEXTLENGTH, 0, 0));
}

bool CScintillaCtrl::GetOvertype()
{
	return Call(SCI_GETOVERTYPE, 0, 0) != 0;
}

Sci_Position CScintillaCtrl::GetTargetStart()
{
	return static_cast<Sci_Position>(Call(SCI_GETTARGETSTART, 0, 0));
}

Sci_Position CScintillaCtrl::GetTargetEnd()
{
	return static_cast<Sci_Position>(Call(SCI_GETTARGETEND, 0, 0));
}

int CScintillaCtrl::SearchInTarget(int length, const char* text)
{
	return static_cast<int>(Call(SCI_SEARCHINTARGET, static_cast<uptr_t>(length), reinterpret_cast<sptr_t>(text)));
}

void CScintillaCtrl::SetSearchFlags(int flags)
{
	Call(SCI_SETSEARCHFLAGS, static_cast<uptr_t>(flags), 0);
}

void CScintillaCtrl::CallTipShow(Sci_Position pos, const char* definition)
{
	Call(SCI_CALLTIPSHOW, static_cast<uptr_t>(pos), reinterpret_cast<sptr_t>(definition));
}

void CScintillaCtrl::CallTipCancel()
{
	Call(SCI_CALLTIPCANCEL, 0, 0);
}

bool CScintillaCtrl::CallTipActive()
{
	return Call(SCI_CALLTIPACTIVE, 0, 0) != 0;
}

int CScintillaCtrl::GetFoldLevel(int line)
{
	return static_cast<int>(Call(SCI_GETFOLDLEVEL, static_cast<uptr_t>(line), 0));
}

void CScintillaCtrl::ToggleFold(int line)
{
	Call(SCI_TOGGLEFOLD, static_cast<uptr_t>(line), 0);
}

void CScintillaCtrl::SetMouseDwellTime(int periodMilliseconds)
{
	Call(SCI_SETMOUSEDWELLTIME, static_cast<uptr_t>(periodMilliseconds), 0);
}

void CScintillaCtrl::TargetFromSelection()
{
	Call(SCI_TARGETFROMSELECTION, 0, 0);
}

void CScintillaCtrl::Cancel()
{
	Call(SCI_CANCEL, 0, 0);
}

void CScintillaCtrl::SetCursor(int cursorType)
{
	Call(SCI_SETCURSOR, static_cast<uptr_t>(cursorType), 0);
}

void CScintillaCtrl::SetProperty(const char* key, const char* value)
{
	Call(SCI_SETPROPERTY, reinterpret_cast<uptr_t>(key), reinterpret_cast<sptr_t>(value));
}

void CScintillaCtrl::SetKeyWords(int keywordSet, const char* keyWords)
{
	Call(SCI_SETKEYWORDS, static_cast<uptr_t>(keywordSet), reinterpret_cast<sptr_t>(keyWords));
}

} // namespace Scintilla
