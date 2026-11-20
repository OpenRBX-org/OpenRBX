#ifndef SCINTILLACTRL_H
#define SCINTILLACTRL_H

#include "Scintilla.h"

#include <QPoint>
#include <QRect>
#include <QString>
#include <functional>

class QWidget;
class ScintillaEditBase;

namespace Scintilla {

using Colour = int;

class CScintillaCtrl
{
public:
	CScintillaCtrl();
	virtual ~CScintillaCtrl();

	bool Create(QWidget* pParentWnd);

	QWidget* GetWidget() const;
	void SetNotifyHandler(std::function<void(SCNotification*)> handler);
	void SetCommandHandler(std::function<void(int)> handler);
	void MoveWindow(const QRect& rect);
	void SetFocus();
	bool HasFocus() const;
	QPoint ClientToScreen(const QPoint& point) const;

	sptr_t Call(unsigned int message, uptr_t wParam, sptr_t lParam);
	QString GetSelText();
	void SetLexer(int lexer);

	void AddText(int length, const char* text);
	void ClearAll();
	int GetLength();
	Sci_Position GetCurrentPos();
	int GetStyleAt(Sci_Position pos);
	void Redo();
	void SetUndoCollection(bool collectUndo);
	void SelectAll();
	void SetSavePoint();
	bool CanRedo();
	void GotoLine(int line);
	void GotoPos(Sci_Position pos);
	void SetCodePage(int codePage);
	void MarkerDefine(int markerNumber, int markerSymbol);
	void MarkerSetFore(int markerNumber, Colour fore);
	void MarkerSetBack(int markerNumber, Colour back);
	int MarkerAdd(int line, int markerNumber);
	void MarkerDelete(int line, int markerNumber);
	int MarkerGet(int line);
	int MarkerNext(int lineStart, int markerMask);
	int MarkerPrevious(int lineStart, int markerMask);
	void SetMarginTypeN(int margin, int marginType);
	void SetMarginWidthN(int margin, int pixelWidth);
	int GetMarginWidthN(int margin);
	void SetMarginMaskN(int margin, int mask);
	void SetMarginSensitiveN(int margin, bool sensitive);
	void StyleClearAll();
	void StyleSetFore(int style, Colour fore);
	void StyleSetBack(int style, Colour back);
	void StyleSetBold(int style, bool bold);
	void StyleSetSize(int style, int sizePoints);
	void StyleSetFont(int style, const char* fontName);
	void AutoCShow(int lenEntered, const char* itemList);
	void AutoCSetSeparator(int separatorCharacter);
	int GetColumn(Sci_Position pos);
	Sci_Position GetSelectionStart();
	Sci_Position GetSelectionEnd();
	Sci_Position FindText(int flags, Sci_TextToFind* ft);
	Sci_Position FormatRange(bool draw, Sci_RangeToFormat* fr);
	bool GetModify();
	void SetSel(Sci_Position start, Sci_Position end);
	int GetSelText(char* text);
	int GetTextRange(Sci_TextRange* tr);
	void HideSelection(bool normal);
	int PointXFromPosition(Sci_Position pos);
	int PointYFromPosition(Sci_Position pos);
	int LineFromPosition(Sci_Position pos);
	void ReplaceSel(const char* text);
	void SetReadOnly(bool readOnly);
	bool CanPaste();
	bool CanUndo();
	void EmptyUndoBuffer();
	void Undo();
	void Cut();
	void Copy();
	void Paste();
	void Clear();
	void SetText(const char* text);
	int GetText(int length, char* text);
	int GetTextLength();
	bool GetOvertype();
	Sci_Position GetTargetStart();
	Sci_Position GetTargetEnd();
	int SearchInTarget(int length, const char* text);
	void SetSearchFlags(int flags);
	void CallTipShow(Sci_Position pos, const char* definition);
	void CallTipCancel();
	bool CallTipActive();
	int GetFoldLevel(int line);
	void ToggleFold(int line);
	void SetMouseDwellTime(int periodMilliseconds);
	void TargetFromSelection();
	void Cancel();
	void SetCursor(int cursorType);
	void SetProperty(const char* key, const char* value);
	void SetKeyWords(int keywordSet, const char* keyWords);

protected:
	ScintillaEditBase* m_widget;
	std::function<void(SCNotification*)> m_notifyHandler;
	std::function<void(int)> m_commandHandler;
};

} // namespace Scintilla

#endif // SCINTILLACTRL_H
