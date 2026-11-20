#ifndef SCRIPTDOC_H
#define SCRIPTDOC_H

#include "NullDoc.h"
#include "ScintillaDocView.h"
#include "util/Events.h"
#include "util/RunStateOwner.h"
#include "v8tree/Instance.h"

#include <memory>
#include <mutex>

class CChildFrame;

namespace RBX {
class Script;
} // namespace RBX

class CScriptDoc : public CNullDoc
{
public:
	void setScript(std::shared_ptr<RBX::Script> script);

	friend class CScriptEditor;

private:
	std::shared_ptr<RBX::Script> m_script;
};

class CScriptEditor : public Scintilla::CScintillaView,
					  public RBX::Listener<RBX::Instance, RBX::AncestorChanged>,
					  public RBX::Listener<RBX::Instance, RBX::PropertyChanged>,
					  public RBX::Listener<RBX::RunService, RBX::RunState>
{
public:
	CScriptEditor();
	~CScriptEditor() override;

	bool create() override;
	void initialUpdate() override;

	void updateFrameTitle();
	void commitText();
	void loadText();
	void setScript(std::shared_ptr<RBX::Script> script);
	void onUpdate(int hint);

protected:
	void onEvent(const RBX::Instance* source, RBX::AncestorChanged event) override;
	void onEvent(const RBX::Instance* source, RBX::PropertyChanged event) override;
	void onEvent(const RBX::RunService* source, RBX::RunState event) override;
	void onRunStateChanged(RBX::RunState event);
	void onPropertyChanged(const RBX::Reflection::PropertyDescriptor* descriptor);
	void onScriptChanged();

	void OnCharAdded(SCNotification* pSCNotification) override;
	void OnDwellStart(SCNotification* pSCNotification) override;
	void OnDwellEnd(SCNotification* pSCNotification) override;
	void OnKillFocus() override;

	void OnOptionsAddmarker();
	void OnUpdateOptionsAddmarker(CommandState& state);
	void OnOptionsDeletemarker();
	void OnUpdateOptionsDeletemarker(CommandState& state);
	void OnOptionsFindNextmarker();
	void OnOptionsFindPrevmarker();
	void OnOptionsFoldMargin();
	void OnUpdateOptionsFoldMargin(CommandState& state);
	void OnOptionsSelectionMargin();
	void OnUpdateOptionsSelectionMargin(CommandState& state);
	void OnOptionsViewLinenumbers();
	void OnUpdateOptionsViewLinenumbers(CommandState& state);
	void OnUpdateInsert(CommandState& state);
	void OnUpdateStyle(CommandState& state);
	void OnUpdateFold(CommandState& state);
	void OnUpdateLine(CommandState& state);

	void SetAStyle(int style, int fore, int back = 0xffffff, int size = -1, const char* face = nullptr);
	void DefineMarker(int marker, int markerType, int fore, int back);

private:
	std::shared_ptr<std::recursive_mutex> m_mutex;
	CChildFrame* m_parentFrame;
	std::shared_ptr<RBX::Script> m_script;
	std::shared_ptr<RBX::RunService> m_runService;
	bool m_loading;
	bool m_running;
	bool m_embedded;
	bool m_created;
};

#endif // SCRIPTDOC_H
