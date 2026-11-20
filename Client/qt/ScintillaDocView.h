#ifndef SCINTILLADOCVIEW_H
#define SCINTILLADOCVIEW_H

#include "Dialog.h"
#include "DocView.h"
#include "ScintillaCtrl.h"

#include <QString>
#include <vector>

class QFile;

namespace Scintilla {

class CScintillaView;

enum FindReplaceFlags : unsigned
{
	FindReplaceDown = 0x00000001,
	FindReplaceWholeWord = 0x00000002,
	FindReplaceMatchCase = 0x00000004,
	FindReplaceFindNext = 0x00000008,
	FindReplaceReplace = 0x00000010,
	FindReplaceReplaceAll = 0x00000020,
	FindReplaceDialogTerm = 0x00000040,
};

class CScintillaFindReplaceDlg : public CDialog
{
public:
	CScintillaFindReplaceDlg(bool bFindDialogOnly, QWidget* pParentWnd);
	~CScintillaFindReplaceDlg() override;

	bool Create(
		bool bFindDialogOnly,
		const QString& findWhat,
		const QString& replaceWith,
		unsigned dwFlags,
		CScintillaView* pParentWnd
	);
	bool GetRegularExpression() const { return m_bRegularExpression; }
	void SetRegularExpression(bool bRegularExpression) { m_bRegularExpression = bRegularExpression; }

	QString GetFindString() const { return m_findWhat; }
	QString GetReplaceString() const { return m_replaceWith; }
	bool SearchDown() const { return m_flags & FindReplaceDown; }
	bool FindNext() const { return m_flags & FindReplaceFindNext; }
	bool MatchCase() const { return m_flags & FindReplaceMatchCase; }
	bool MatchWholeWord() const { return m_flags & FindReplaceWholeWord; }
	bool ReplaceCurrent() const { return m_flags & FindReplaceReplace; }
	bool ReplaceAll() const { return m_flags & FindReplaceReplaceAll; }
	bool IsTerminating() const { return m_flags & FindReplaceDialogTerm; }

	CScintillaView* GetOwner() const { return m_owner; }
	void SetOwner(CScintillaView* owner) { m_owner = owner; }
	void Close();

protected:
	bool OnInitDialog() override;
	void OnOK() override;
	void OnCancel() override;
	void OnCommand(int id) override;
	void OnRegularExpression();

	bool m_bRegularExpression;

private:
	void UpdateTextAndFlags(unsigned action);
	void UpdateButtons();
	void Notify();

	bool m_findOnly;
	bool m_terminated;
	unsigned m_flags;
	QString m_findWhat;
	QString m_replaceWith;
	CScintillaView* m_owner;
};

class CScintillaView : public View
{
public:
	CScintillaView();
	~CScintillaView() override;

	CScintillaCtrl& GetCtrl();

	bool create() override;
	void activate(bool active, View* other) override;

	bool onPreparePrinting(PrintInfo& info) override;
	void onBeginPrinting(PrintDC& dc, PrintInfo& info) override;
	void onPrepareDC(PrintDC& dc, PrintInfo* info) override;
	void onPrint(PrintDC& dc, PrintInfo& info) override;
	void onEndPrinting(PrintDC& dc, PrintInfo& info) override;

protected:
	virtual bool PaginateTo(PrintDC& dc, PrintInfo& info);
	virtual void PrintHeader(PrintDC& dc, PrintInfo& info, Sci_RangeToFormat& frPrint);
	virtual void PrintFooter(PrintDC& dc, PrintInfo& info, Sci_RangeToFormat& frPrint);
	virtual Sci_Position PrintPage(PrintDC& dc, PrintInfo& info, Sci_Position nIndexStart, Sci_Position nIndexStop);
	virtual void OnFindNext(const QString& find, bool bNext, bool bCase, bool bWord, bool bRegularExpression);
	void TextNotFound();
	virtual bool FindText(const QString& find, bool bNext, bool bCase, bool bWord, bool bRegularExpression);
	virtual void OnEditFindReplace(bool bFindOnly);
	virtual bool FindTextSimple(const QString& find, bool bNext, bool bCase, bool bWord, bool bRegularExpression);
	virtual void OnReplaceSel(
		const QString& find, bool bNext, bool bCase, bool bWord, bool bRegularExpression, const QString& replace
	);
	virtual void OnReplaceAll(
		const QString& find, const QString& replace, bool bCase, bool bWord, bool bRegularExpression
	);
	virtual bool SameAsSelected(const QString& compare, bool bCase, bool bWord, bool bRegularExpression);
	virtual Sci_Position FindAndSelect(unsigned dwFlags, Sci_TextToFind& ft);
	virtual void AdjustFindDialogPosition();

	virtual void OnNotify(SCNotification* pSCNotification);
	virtual void DeleteContents();
	virtual void Serialize(QFile& file, bool loading);
	virtual void OnCommand(int notification);

	virtual void OnCharAdded(SCNotification* pSCNotification);
	virtual void OnMarginClick(SCNotification* pSCNotification);
	virtual void OnDwellStart(SCNotification* pSCNotification);
	virtual void OnDwellEnd(SCNotification* pSCNotification);
	virtual void OnKillFocus();

	void resizeEvent(QResizeEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
	QRect controlRect() const;

	CScintillaCtrl m_Edit;
	std::vector<Sci_Position> m_aPageStart;

	void OnUpdateNeedSel(CommandState& state);
	void OnUpdateNeedPaste(CommandState& state);
	void OnUpdateNeedText(CommandState& state);
	void OnUpdateNeedFind(CommandState& state);
	void OnUpdateEditUndo(CommandState& state);
	void OnUpdateEditRedo(CommandState& state);
	void OnUpdateNeedTextAndFollowingText(CommandState& state);
	void OnEditCut();
	void OnEditCopy();
	void OnEditPaste();
	void OnEditClear();
	void OnEditUndo();
	void OnEditRedo();
	void OnEditSelectAll();
	void OnEditFind();
	void OnEditReplace();
	void OnEditRepeat();
	void OnFindReplaceCmd(CScintillaFindReplaceDlg* pDialog);

	friend class CScintillaDoc;
	friend class CScintillaFindReplaceDlg;
};

class CScintillaDoc : public Document
{
protected:
	CScintillaDoc();

public:
	virtual CScintillaView* GetView() const;

	void deleteContents() override;
	bool isModified() const override;
	void setModified(bool modified) override;
	bool openDocument(const QString& pathName) override;
	bool saveDocument(const QString& pathName) override;
	virtual void Serialize(QFile& file, bool loading);

private:
	bool m_bModified;
};

} // namespace Scintilla

#endif // SCINTILLADOCVIEW_H
