#ifndef V8DATAMODEL_WORKSPACE_H
#define V8DATAMODEL_WORKSPACE_H

#include "gui/GuiEvent.h"
#include "script/Script.h"
#include "util/ContentProvider.h"
#include "util/IRenderable.h"
#include "util/InsertMode.h"
#include "util/Name.h"
#include "util/Profiling.h"
#include "util/RunStateOwner.h"
#include "util/UIEvent.h"
#include "v8datamodel/RootInstance.h"
#include "v8tree/Service.h"
#include "v8tree/Verb.h"
#include "v8world/IMoving.h"

#include <memory>
#include <set>

namespace RBX {

class Camera;
class IDataState;
class MouseCommand;
class World;

extern char sWorkspace[];

class DrawChanged
{
};

class ToolChanged
{
};

class Workspace : public DescribedNonCreatable<Workspace, RootInstance, sWorkspace>,
				  public Service,
				  public GuiTarget,
				  public VerbContainer,
				  public IRenderableBucket,
				  public IMovingManager,
				  public IScriptOwner,
				  public Notifier<Workspace, DrawChanged>,
				  public Notifier<Workspace, ToolChanged>,
				  public Listener<RunService, Heartbeat>
{
public:
	static bool showWorldCoord;
	static bool showHashGrid;

	static Workspace* findWorkspace(const Instance* context);
	static Workspace* getMyWorkspaceFast(const Instance* context);
	static World* getWorldIfInWorkspace(const Instance* context);
	static Workspace* getWorkspaceIfInWorkspace(const Instance* context);
	static bool contextInWorkspace(const Instance* context);
	static World* getMyWorldFast(const Instance* context);
	static Instance* findTopInstance(Instance*);

private:
	bool askAddChild(const Instance* instance) const override;
	void onChildChanged(Instance* instance, const PropertyChanged& event) override;
	void onDescendentAdded(Instance* instance) override;
	void onDescendentRemoving(const std::shared_ptr<Instance>& instance) override;

public:
	void render2d(Adorn* adorn) override;
	void render3dAdorn(Adorn* adorn) override;

	void render3dSelect(Adorn* adorn, SelectState selectState) override {}

private:
	IScriptOwner* scriptShouldRun(Script* script) override;
	void runScript(Script* script, ScriptContext* context) override;
	void releaseScript(Script* script) override;

	Extents computeCameraOwnerExtents() override; // ICameraOwner vtable+0x10
	void cameraMoved() override;                  // ICameraOwner vtable+0x0c

	void onExtentsChanged() const override
	{
		ModelInstance::onExtentsChanged();
		raiseDrawChanged();
	}

	void onEvent(const RunService* source, Heartbeat event) override;
	void onServiceProvider(const ServiceProvider* oldProvider, const ServiceProvider* newProvider) override;

public:
	~Workspace() override;

	const G3D::GCamera& getGCamera() const override; // ICameraOwner vtable+0x08
	Camera* getCamera() const override;              // ICameraOwner vtable+0x04
	void setCamera(Camera* value);

	Workspace(IDataState* dataState);

	World* getWorld() const { return world.get(); }
	MouseCommand* getCurrentMouseCommand() { return currentCommand.get(); }
	void cancelMouseCommand();
	IDataState& getDataState() const { return *dataState; }

	bool getInMouselookMode() { return inMouselookMode; }
	void setInMouselookMode(bool);

	void setMouseCommand(MouseCommand* newMouseCommand);
	void setDefaultMouseCommand();
	void setNullMouseCommand();

	void setImageServerView(const G3D::Rect2D& viewPortRect);
	void zoomToExtents();
	void onWrapMouse(const G3D::Vector2&);

	GuiResponse process(const GuiEvent& guiEvent) override;
	void selectAll();

	void start();
	void stop();
	void reset();
	void setThrottleEnabled(bool value);
	void joinAllHack();
	void insertItems(XmlElement*, std::vector<std::shared_ptr<Instance>>&, InsertMode, PromptMode);

	float step(float timeInterval);

	void insertContent(
		ContentId contentId, std::vector<std::shared_ptr<Instance>>& items, InsertMode insertMode, PromptMode promptMode
	);
	std::shared_ptr<const std::vector<std::shared_ptr<Instance>>> insertContent(ContentId contentId);

	void makeJoints(std::shared_ptr<const std::vector<std::shared_ptr<Instance>>> instances);
	void breakJoints(std::shared_ptr<const std::vector<std::shared_ptr<Instance>>> instances);
	void setCursor(Adorn*);

private:
	void handleFallenParts(const G3D::Array<std::shared_ptr<PartInstance>>& fallenParts);

public:
	void raiseDrawChanged() const { Notifier<Workspace, DrawChanged>::raise(DrawChanged()); }

private:
	std::shared_ptr<Instance> statsItem;
	IDataState* dataState;
	bool arrowCameraControls;
	std::unique_ptr<MouseCommand> currentCommand;
	std::unique_ptr<MouseCommand> stickyCommand;
	UIEvent idleMouseEvent;
	std::shared_ptr<Camera> currentCamera;
	int flySteps;
	bool isSimulating;
	std::set<Script*> pendingScripts;
	ScriptContext* scriptContext;
	bool inMouselookMode;

public:
	std::unique_ptr<Profiling::CodeProfiler> profileWorkspaceStep;
	std::unique_ptr<Profiling::CodeProfiler> profileRunServiceStepped;
	std::unique_ptr<Profiling::CodeProfiler> profileRunServiceHeartbeat;
	std::unique_ptr<Profiling::ThreadProfiler> profileRun;
	int imageServerViewHack;

	template <class _InIt>
	std::shared_ptr<ModelInstance> FUN_005984c0(_InIt first, _InIt last)
	{
		std::shared_ptr<ModelInstance> model = Creatable<Instance>::create<ModelInstance>();
		model->setParent(this);

		for (; first != last; ++first) {
			(*first)->setParent(model.get());
		}

		return model;
	}
};

} // namespace RBX

#endif // V8DATAMODEL_WORKSPACE_H
