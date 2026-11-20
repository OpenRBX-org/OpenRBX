#include "ScriptDoc.h"

#include "ChildFrm.h"
#include "CommandIds.h"
#include "MainFrm.h"
#include "Resources.h"
#include "Roblox.h"
#include "SciLexer.h"
#include "script/Script.h"
#include "util/ContentProvider.h"
#include "util/Utilities.h"
#include "util/object.h"
#include "v8datamodel/DataModel.h"
#include "v8tree/Service.h"

#include <QApplication>
#include <QByteArray>
#include <algorithm>
#include <istream>
#include <memory>
#include <string>

static const unsigned ID_OPTIONS_SELECTION_MARGIN = 0x80e3;
static const unsigned ID_OPTIONS_FOLD_MARGIN = 0x80e4;
static const unsigned ID_OPTIONS_ADDMARKER = 0x80e5;
static const unsigned ID_OPTIONS_DELETEMARKER = 0x80e6;
static const unsigned ID_OPTIONS_FIND_NEXTMARKER = 0x80e7;
static const unsigned ID_OPTIONS_FIND_PREVMARKER = 0x80e8;
static const unsigned ID_INDICATOR_LINE = 203;
static const unsigned ID_INDICATOR_STYLE = 204;
static const unsigned ID_INDICATOR_FOLD = 205;
static const unsigned ID_INDICATOR_OVR = 0xe704;

static const unsigned lineFormatStringId = 199;
static const unsigned styleFormatStringId = 200;
static const unsigned foldFormatStringId = 201;

static const int updateHintScript = 1;

static constexpr int colourFromRgb(int red, int green, int blue)
{
	return red | (green << 8) | (blue << 16);
}

void CScriptDoc::setScript(std::shared_ptr<RBX::Script> script)
{
	m_script = script;
	setTitle(QString::fromLocal8Bit(script->getName().c_str()));

	std::vector<View*> documentViews = views();
	for (View* view : documentViews) {
		if (CScriptEditor* editor = dynamic_cast<CScriptEditor*>(view)) {
			editor->onUpdate(updateHintScript);
		}
	}
}

CScriptEditor::CScriptEditor()
	: m_parentFrame(nullptr), m_loading(false), m_running(false), m_embedded(false), m_created(false)
{
	addCommand(ID_OPTIONS_ADDMARKER, [this]() { OnOptionsAddmarker(); });
	addCommand(ID_OPTIONS_DELETEMARKER, [this]() { OnOptionsDeletemarker(); });
	addCommandUpdate(ID_OPTIONS_DELETEMARKER, [this](CommandState& state) { OnUpdateOptionsDeletemarker(state); });
	addCommand(ID_OPTIONS_FIND_NEXTMARKER, [this]() { OnOptionsFindNextmarker(); });
	addCommand(ID_OPTIONS_FIND_PREVMARKER, [this]() { OnOptionsFindPrevmarker(); });
	addCommand(ID_OPTIONS_FOLD_MARGIN, [this]() { OnOptionsFoldMargin(); });
	addCommand(ID_OPTIONS_SELECTION_MARGIN, [this]() { OnOptionsSelectionMargin(); });
	addCommandUpdate(ID_OPTIONS_SELECTION_MARGIN, [this](CommandState& state) {
		OnUpdateOptionsSelectionMargin(state);
	});
	addCommand(ID_VIEW_LINE_NUMBERS, [this]() { OnOptionsViewLinenumbers(); });
	addCommandUpdate(ID_VIEW_LINE_NUMBERS, [this](CommandState& state) { OnUpdateOptionsViewLinenumbers(state); });
	addCommandUpdate(ID_OPTIONS_ADDMARKER, [this](CommandState& state) { OnUpdateOptionsAddmarker(state); });
	addCommandUpdate(ID_OPTIONS_FOLD_MARGIN, [this](CommandState& state) { OnUpdateOptionsFoldMargin(state); });
	addCommandUpdate(ID_INDICATOR_LINE, [this](CommandState& state) { OnUpdateLine(state); });
	addCommandUpdate(ID_INDICATOR_STYLE, [this](CommandState& state) { OnUpdateStyle(state); });
	addCommandUpdate(ID_INDICATOR_FOLD, [this](CommandState& state) { OnUpdateFold(state); });
	addCommandUpdate(ID_INDICATOR_OVR, [this](CommandState& state) { OnUpdateInsert(state); });
}

CScriptEditor::~CScriptEditor()
{
	if (m_created) {
		commitText();
	}
	m_created = false;
	setScript(std::shared_ptr<RBX::Script>());
}

void CScriptEditor::onEvent(const RBX::RunService* source, RBX::RunState event)
{
	QMetaObject::invokeMethod(
		this,
		[this, event]() {
			try {
				onRunStateChanged(event);
			}
			catch (...) {
			}
		},
		Qt::QueuedConnection
	);
}

void CScriptEditor::onRunStateChanged(RBX::RunState event)
{
	m_running = event != RBX::RS_NORMAL;
	GetCtrl().SetReadOnly(m_running || !m_embedded);
}

bool CScriptEditor::create()
{
	if (!CScintillaView::create()) {
		return false;
	}

	m_created = true;
	return true;
}

void CScriptEditor::SetAStyle(int style, int fore, int back, int size, const char* face)
{
	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();

	rCtrl.StyleSetFore(style, fore);
	rCtrl.StyleSetBack(style, back);
	if (size >= 1) {
		rCtrl.StyleSetSize(style, size);
	}
	if (face) {
		rCtrl.StyleSetFont(style, face);
	}
}

void CScriptEditor::DefineMarker(int marker, int markerType, int fore, int back)
{
	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();

	rCtrl.MarkerDefine(marker, markerType);
	rCtrl.MarkerSetFore(marker, fore);
	rCtrl.MarkerSetBack(marker, back);
}

void CScriptEditor::initialUpdate()
{
	CScintillaView::initialUpdate();

	m_parentFrame = childFrame();

	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();

	rCtrl.SetLexer(SCLEX_LUA);
	rCtrl.SetKeyWords(
		0, "and break do else elseif end false for function if in local nil not or repeat return then true until while"
	);

	SetAStyle(STYLE_DEFAULT, colourFromRgb(0, 0, 0), colourFromRgb(0xff, 0xff, 0xff), 9, "Verdana");
	rCtrl.StyleClearAll();
	SetAStyle(SCE_LUA_DEFAULT, colourFromRgb(0, 0, 0));
	SetAStyle(SCE_LUA_COMMENT, colourFromRgb(0, 0x80, 0));
	SetAStyle(SCE_LUA_COMMENTLINE, colourFromRgb(0, 0x80, 0));
	SetAStyle(SCE_LUA_COMMENTDOC, colourFromRgb(0, 0x80, 0));
	SetAStyle(SCE_LUA_NUMBER, colourFromRgb(0, 0x80, 0x80));
	SetAStyle(SCE_LUA_WORD, colourFromRgb(0, 0, 0x80));
	SetAStyle(SCE_LUA_WORD2, colourFromRgb(0, 0, 0x80));
	SetAStyle(SCE_LUA_WORD3, colourFromRgb(0, 0, 0x80));
	SetAStyle(SCE_LUA_WORD4, colourFromRgb(0, 0, 0x80));
	SetAStyle(SCE_LUA_WORD5, colourFromRgb(0, 0, 0x80));
	SetAStyle(SCE_LUA_WORD6, colourFromRgb(0, 0, 0x80));
	SetAStyle(SCE_LUA_WORD7, colourFromRgb(0, 0, 0x80));
	SetAStyle(SCE_LUA_WORD8, colourFromRgb(0, 0, 0x80));
	rCtrl.StyleSetBold(SCE_LUA_WORD, true);
	SetAStyle(SCE_LUA_STRING, colourFromRgb(0x80, 0, 0x80));
	SetAStyle(SCE_LUA_IDENTIFIER, colourFromRgb(0, 0, 0));
	SetAStyle(SCE_LUA_PREPROCESSOR, colourFromRgb(0x80, 0, 0));
	SetAStyle(SCE_LUA_OPERATOR, colourFromRgb(0x80, 0x80, 0));

	rCtrl.SetMarginWidthN(2, 16);
	rCtrl.SetMarginSensitiveN(2, true);
	rCtrl.SetMarginTypeN(2, SC_MARGIN_SYMBOL);
	rCtrl.SetMarginMaskN(2, static_cast<int>(SC_MASK_FOLDERS));
	rCtrl.SetProperty("fold", "1");

	DefineMarker(SC_MARKNUM_FOLDEROPEN, SC_MARK_MINUS, colourFromRgb(0xff, 0xff, 0xff), colourFromRgb(0, 0, 0xff));
	DefineMarker(SC_MARKNUM_FOLDER, SC_MARK_PLUS, colourFromRgb(0xff, 0xff, 0xff), colourFromRgb(0, 0, 0));
	DefineMarker(SC_MARKNUM_FOLDERSUB, SC_MARK_EMPTY, colourFromRgb(0xff, 0xff, 0xff), colourFromRgb(0, 0, 0));
	DefineMarker(SC_MARKNUM_FOLDERTAIL, SC_MARK_EMPTY, colourFromRgb(0xff, 0xff, 0xff), colourFromRgb(0, 0, 0));
	DefineMarker(SC_MARKNUM_FOLDEREND, SC_MARK_EMPTY, colourFromRgb(0xff, 0xff, 0xff), colourFromRgb(0, 0, 0));
	DefineMarker(SC_MARKNUM_FOLDEROPENMID, SC_MARK_EMPTY, colourFromRgb(0xff, 0xff, 0xff), colourFromRgb(0, 0, 0));
	DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_EMPTY, colourFromRgb(0xff, 0xff, 0xff), colourFromRgb(0, 0, 0));

	rCtrl.AutoCSetSeparator(10);
	rCtrl.SetMouseDwellTime(1000);
	rCtrl.SetMarginWidthN(0, 32);
}

void CScriptEditor::OnOptionsAddmarker()
{
	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
	Sci_Position nPos = rCtrl.GetCurrentPos();
	int nLine = rCtrl.LineFromPosition(nPos);
	rCtrl.MarkerAdd(nLine, 0);
}

void CScriptEditor::OnUpdateOptionsAddmarker(CommandState& state)
{
	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
	Sci_Position nPos = rCtrl.GetCurrentPos();
	int nLine = rCtrl.LineFromPosition(nPos);
	int nBits = rCtrl.MarkerGet(nLine);
	state.enabled = (nBits & 0x1) == 0;
}

void CScriptEditor::OnOptionsDeletemarker()
{
	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
	Sci_Position nPos = rCtrl.GetCurrentPos();
	int nLine = rCtrl.LineFromPosition(nPos);
	rCtrl.MarkerDelete(nLine, 0);
}

void CScriptEditor::OnUpdateOptionsDeletemarker(CommandState& state)
{
	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
	Sci_Position nPos = rCtrl.GetCurrentPos();
	int nLine = rCtrl.LineFromPosition(nPos);
	int nBits = rCtrl.MarkerGet(nLine);
	state.enabled = nBits & 0x1;
}

void CScriptEditor::OnOptionsFindNextmarker()
{
	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
	Sci_Position nPos = rCtrl.GetCurrentPos();
	int nLine = rCtrl.LineFromPosition(nPos);
	int nFoundLine = rCtrl.MarkerNext(nLine + 1, 0x1);
	if (nFoundLine >= 0) {
		rCtrl.GotoLine(nFoundLine);
	}
	else {
		QApplication::beep();
	}
}

void CScriptEditor::OnOptionsFindPrevmarker()
{
	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
	Sci_Position nPos = rCtrl.GetCurrentPos();
	int nLine = rCtrl.LineFromPosition(nPos);
	int nFoundLine = rCtrl.MarkerPrevious(nLine - 1, 0x1);
	if (nFoundLine >= 0) {
		rCtrl.GotoLine(nFoundLine);
	}
	else {
		QApplication::beep();
	}
}

void CScriptEditor::OnOptionsFoldMargin()
{
	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
	int nMarginWidth = rCtrl.GetMarginWidthN(2);
	if (nMarginWidth) {
		rCtrl.SetMarginWidthN(2, 0);
	}
	else {
		rCtrl.SetMarginWidthN(2, 16);
	}
}

void CScriptEditor::OnUpdateOptionsFoldMargin(CommandState& state)
{
	state.checked = GetCtrl().GetMarginWidthN(2) != 0;
}

void CScriptEditor::OnOptionsSelectionMargin()
{
	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
	int nMarginWidth = rCtrl.GetMarginWidthN(1);
	if (nMarginWidth) {
		rCtrl.SetMarginWidthN(1, 0);
	}
	else {
		rCtrl.SetMarginWidthN(1, 16);
	}
}

void CScriptEditor::OnUpdateOptionsSelectionMargin(CommandState& state)
{
	state.checked = GetCtrl().GetMarginWidthN(1) != 0;
}

void CScriptEditor::OnOptionsViewLinenumbers()
{
	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
	int nMarginWidth = rCtrl.GetMarginWidthN(0);
	if (nMarginWidth) {
		rCtrl.SetMarginWidthN(0, 0);
	}
	else {
		rCtrl.SetMarginWidthN(0, 32);
	}
}

void CScriptEditor::OnUpdateOptionsViewLinenumbers(CommandState& state)
{
	state.checked = GetCtrl().GetMarginWidthN(0) != 0;
}

void CScriptEditor::OnUpdateInsert(CommandState& state)
{
	QString sText;
	if (GetCtrl().GetOvertype()) {
		sText = Resources::string(ID_INDICATOR_OVR);
	}
	state.text = sText;
}

void CScriptEditor::OnUpdateStyle(CommandState& state)
{
	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
	Sci_Position nPos = rCtrl.GetCurrentPos();
	int nStyle = rCtrl.GetStyleAt(nPos);
	state.text = QString::asprintf(Resources::string(styleFormatStringId).toLatin1().constData(), nStyle);
}

void CScriptEditor::OnUpdateFold(CommandState& state)
{
	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
	Sci_Position nPos = rCtrl.GetCurrentPos();
	int nLine = rCtrl.LineFromPosition(nPos);
	int nLevel = rCtrl.GetFoldLevel(nLine) & SC_FOLDLEVELNUMBERMASK;
	nLevel -= 1024;

	state.text = QString::asprintf(Resources::string(foldFormatStringId).toLatin1().constData(), nLevel);
}

void CScriptEditor::OnUpdateLine(CommandState& state)
{
	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
	Sci_Position nPos = rCtrl.GetCurrentPos();
	int nLine = rCtrl.LineFromPosition(nPos);
	int nColumn = rCtrl.GetColumn(nPos);

	state.text = QString::asprintf(
		Resources::string(lineFormatStringId).toLatin1().constData(), nLine, nColumn, static_cast<int>(nPos)
	);
}

void CScriptEditor::OnCharAdded(SCNotification* pSCNotification)
{
	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();

	Sci_Position nStartSel = rCtrl.GetSelectionStart();
	Sci_Position nEndSel = rCtrl.GetSelectionEnd();
	if ((nStartSel == nEndSel) && (nStartSel > 12)) {
		Sci_TextRange tr;
		tr.chrg.cpMin = static_cast<Sci_PositionCR>(nStartSel - 13);
		tr.chrg.cpMax = static_cast<Sci_PositionCR>(nEndSel - 1);
		char sText[14];
		tr.lpstrText = sText;
		rCtrl.GetTextRange(&tr);

		if (qstricmp(sText, "scintilla is") == 0) {
			rCtrl.AutoCShow(0, "very cool\neasy to use\nway cool!!");
		}
	}
}

void CScriptEditor::OnDwellEnd(SCNotification* pSCNotification)
{
	Scintilla::CScintillaCtrl& ctrl = GetCtrl();
	if (ctrl.CallTipActive()) {
		ctrl.CallTipCancel();
	}
}

void CScriptEditor::updateFrameTitle()
{
	if (CChildFrame* frame = childFrame()) {
		if (!m_script) {
			frame->setWindowTitle(QString());
			theApp.mainFrame()->childFrameTitleChanged(frame);
			return;
		}
		frame->setWindowTitle(QString::fromLocal8Bit(m_script->getName().c_str()));
		theApp.mainFrame()->childFrameTitleChanged(frame);
	}
}

void CScriptEditor::OnDwellStart(SCNotification* pSCNotification)
{
	Scintilla::CScintillaCtrl& rCtrl = GetCtrl();

	if (rCtrl.HasFocus()) {
		Sci_TextRange tr;
		tr.chrg.cpMin = static_cast<Sci_PositionCR>(std::max<Sci_Position>(0, pSCNotification->position - 7));
		tr.chrg.cpMax =
			static_cast<Sci_PositionCR>(std::min<Sci_Position>(pSCNotification->position + 7, rCtrl.GetLength()));
		char sText[15];
		tr.lpstrText = sText;
		rCtrl.GetTextRange(&tr);

		if (QByteArray(sText).toUpper().contains("AUTHOR ")) {
			rCtrl.CallTipShow(pSCNotification->position, "PJ Naughter");
		}
	}
}

void CScriptEditor::commitText()
{
	if (GetCtrl().GetModify() && !m_loading) {
		if (m_script) {
			std::scoped_lock lock(*m_mutex);
			int length = GetCtrl().GetTextLength();
			std::string text(static_cast<size_t>(length), '\0');
			GetCtrl().GetText(length, text.data());
			m_script->setEmbeddedCode(text.c_str());
		}

		GetCtrl().SetSavePoint();
	}
}

void CScriptEditor::OnKillFocus()
{
	commitText();
}

void CScriptEditor::loadText()
{
	if (m_created) {
		m_loading = true;

		if (m_script) {
			try {
				std::scoped_lock lock(*m_mutex);
				m_embedded = m_script->getScriptId().isNull();

				if (m_embedded) {
					GetCtrl().SetText(m_script->getEmbeddedCode().c_str());
				}
				else {
					std::unique_ptr<std::istream> stream(
						RBX::ContentProvider::singleton().getContent(m_script->getScriptId())
					);
					GetCtrl().SetText(RBX::readStream(*stream).c_str());
				}

				GetCtrl().SetReadOnly(m_running || !m_embedded);
			}
			catch (std::exception& e) {
				std::string message = "An error occurred while getting the script\n\n";
				message += e.what();
				GetCtrl().SetText(message.c_str());
				GetCtrl().SetReadOnly(false);
			}
		}
		else {
			GetCtrl().SetText("");
		}

		GetCtrl().SetSavePoint();
		m_loading = false;
	}
}

void CScriptEditor::onScriptChanged()
{
	updateFrameTitle();
	loadText();
}

void CScriptEditor::onEvent(const RBX::Instance* source, RBX::PropertyChanged event)
{
	const RBX::Reflection::PropertyDescriptor* descriptor = &event.getDescriptor();
	if (descriptor == &RBX::Script::prop_EmbeddedSourceCode || descriptor == &RBX::Instance::desc_Name) {
		QMetaObject::invokeMethod(
			this,
			[this, descriptor]() {
				try {
					onPropertyChanged(descriptor);
				}
				catch (...) {
				}
			},
			Qt::QueuedConnection
		);
	}
}

void CScriptEditor::onPropertyChanged(const RBX::Reflection::PropertyDescriptor* descriptor)
{
	if (descriptor == &RBX::Script::prop_EmbeddedSourceCode) {
		loadText();
	}
	else if (descriptor == &RBX::Instance::desc_Name) {
		updateFrameTitle();
	}
}

void CScriptEditor::setScript(std::shared_ptr<RBX::Script> script)
{
	if (m_script) {
		std::scoped_lock lock(*m_mutex);
		RBX::stopListening<RBX::Instance, RBX::AncestorChanged>(m_script, this);
		RBX::stopListening<RBX::Instance, RBX::PropertyChanged>(m_script, this);
		RBX::stopListening<RBX::RunService, RBX::RunState>(m_runService, this);
		m_script.reset();
		m_runService.reset();
		m_mutex.reset();
	}

	if (script) {
		m_mutex = RBX::DataModel::getMutex(script.get());
		m_script = script;

		std::scoped_lock lock(*m_mutex);
		m_runService = RBX::shared_from(RBX::ServiceProvider::find<RBX::RunService>(script.get()));
		RBX::startListening<RBX::RunService, RBX::RunState>(m_runService, this);
		RBX::startListening<RBX::Instance, RBX::PropertyChanged>(m_script, this);
		RBX::startListening<RBX::Instance, RBX::AncestorChanged>(m_script, this);
	}

	if (m_created) {
		QMetaObject::invokeMethod(this, [this]() { onScriptChanged(); }, Qt::QueuedConnection);
	}
}

void CScriptEditor::onEvent(const RBX::Instance* source, RBX::AncestorChanged event)
{
	if (!event.newParent) {
		QMetaObject::invokeMethod(
			this,
			[this]() {
				setScript(std::shared_ptr<RBX::Script>());
				if (m_parentFrame) {
					m_parentFrame->requestClose();
				}
			},
			Qt::QueuedConnection
		);
	}
}

void CScriptEditor::onUpdate(int hint)
{
	if (hint == updateHintScript) {
		setScript(static_cast<CScriptDoc*>(document())->m_script);
	}
}
