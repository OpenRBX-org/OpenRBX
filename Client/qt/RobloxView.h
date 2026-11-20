#ifndef ROBLOXVIEW_H
#define ROBLOXVIEW_H

#include "DocView.h"
#include "RobloxWnd.h"
#include "VerbBinder.h"
#include "network/Players.h"
#include "network/SuperSafeChanged.h"
#include "util/Events.h"
#include "v8datamodel/DataModel.h"
#include "v8tree/Service.h"
#include "v8tree/Verb.h"

#include <memory>

class CMultiPlayerPane;
class CRobloxDoc;
class CRobloxView;
class CWebToolbox;

class InsertModelFromRobloxVerb : public RBX::Verb
{
public:
	InsertModelFromRobloxVerb(CRobloxView* view, RBX::DataModel* dataModel);

	bool isEnabled() const override;
	bool isChecked() const override;
	void doIt(RBX::IDataState* dataState) override;

private:
	CRobloxView* m_view;
	RBX::DataModel* m_dataModel;
	double m_created;
};

class ToggleIDEModeVerb : public RBX::Verb
{
public:
	ToggleIDEModeVerb(RBX::VerbContainer* container);

	bool isEnabled() const override;
	bool isChecked() const override;
	void doIt(RBX::IDataState* dataState) override;

private:
	double m_created;
};

class LockPlayModeVerb : public RBX::Verb
{
public:
	LockPlayModeVerb(RBX::VerbContainer* container) : RBX::Verb(container, "LockPlayMode") {}

	void doIt(RBX::IDataState* dataState) override;
};

class ToggleFullscreenVerb : public RBX::Verb
{
public:
	ToggleFullscreenVerb(RBX::VerbContainer* container) : RBX::Verb(container, "ToggleFullScreen") {}

	bool isChecked() const override;
	void doIt(RBX::IDataState* dataState) override;
};

class HelpCommand : public RBX::Verb
{
public:
	HelpCommand(RBX::VerbContainer* container, CRobloxView* view) : RBX::Verb(container, "Help"), m_view(view) {}

	void doIt(RBX::IDataState* dataState) override;

private:
	CRobloxView* m_view;
};

class CRobloxView : public View,
					public RBX::VerbContainer,
					public RBX::Listener<RBX::ServiceProvider, RBX::ServiceAdded>,
					public RBX::Listener<RBX::ServiceProvider, RBX::ServiceRemoving>,
					public RBX::Listener<RBX::Network::Players, RBX::SuperSafeChanged>
{
public:
	CRobloxView();
	~CRobloxView() override;

	bool create() override;
	void initialUpdate() override;
	void activate(bool active, View* other) override;
	void activateFrame(bool active) override;
	bool onPreparePrinting(PrintInfo& info) override;

	bool hasCommand(unsigned id) const override;
	bool executeCommand(unsigned id) override;
	bool updateCommand(unsigned id, CommandState& state) override;

	std::shared_ptr<RBX::DataModel> dataModel() const { return m_dataModel; }
	CRobloxDoc* robloxDocument() const;
	CRobloxWnd* window() { return &m_window; }
	bool isToolboxVisible() const { return m_toolbox != nullptr; }
	void postCommand(unsigned id);

protected:
	void onEvent(const RBX::ServiceProvider* source, RBX::ServiceAdded event) override;
	void onEvent(const RBX::ServiceProvider* source, RBX::ServiceRemoving event) override;
	void onEvent(const RBX::Network::Players* source, RBX::SuperSafeChanged event) override;

	void resizeEvent(QResizeEvent* event) override;
	void focusInEvent(QFocusEvent* event) override;

private:
	void layoutChildren();
	void insertModel();
	void showHelp();
	void showToolbox(bool show);
	void updateMultiPlayerPane();
	void releaseDataModel();

	VerbBinder m_verbs;
	CRobloxWnd m_window;
	std::shared_ptr<RBX::DataModel> m_dataModel;
	CMultiPlayerPane* m_multiPlayerPane = nullptr;
	CWebToolbox* m_toolbox = nullptr;
	std::unique_ptr<RBX::Verb> m_helpVerb;
	std::unique_ptr<RBX::Verb> m_toolboxVerb;
	std::unique_ptr<RBX::Verb> m_fullScreenVerb;
	std::unique_ptr<RBX::Verb> m_ideModeVerb;
	std::unique_ptr<RBX::Verb> m_lockPlayModeVerb;
};

#endif // ROBLOXVIEW_H
