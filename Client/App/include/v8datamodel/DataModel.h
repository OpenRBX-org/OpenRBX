#ifndef V8DATAMODEL_DATAMODEL_H
#define V8DATAMODEL_DATAMODEL_H

#include "util/Evaluator.h"
#include "util/Events.h"
#include "util/RunStateOwner.h"
#include "v8tree/Service.h"
#include "v8tree/Verb.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace RBX {

class Adorn;
class ContentId;
class GuiEvent;
class GuiResponse;
class GuiRoot;
class GuiTarget;
class IMetric;
class LocalBackpack;
class RunService;
class StarterPackService;
class TimeState;
class UIEvent;
class Workspace;

extern const char sDataModel[];

class DataModel : public DescribedNonCreatable<DataModel, ServiceProvider, sDataModel>,
				  public VerbContainer,
				  public IDataState,
				  public Evaluator,
				  public Listener<RunService, RunTransition>
{
	GuiResponse processAccelerators(const GuiEvent& event);
	void processEvent(const UIEvent&);

private:
	std::shared_ptr<LocalBackpack> localBackpack;
	std::shared_ptr<Workspace> workspace;
	std::shared_ptr<RunService> runService;
	std::shared_ptr<StarterPackService> starterPackService;
	std::shared_ptr<GuiRoot> guiRoot;
	std::shared_ptr<GuiRoot> guiHooks;
	GuiTarget* guiTarget;
	bool forceArrowCursor;
	std::string uiMessage;
	int drawId;
	IMetric* tempMetric;
	bool dirty;
	bool isInitialized;
	std::shared_ptr<std::recursive_mutex> mutex;

	void initializeContents();

public:
	class Lock
	{
	public:
		Lock(const DataModel* dataModel) { doLock(dataModel); }

		Lock(std::shared_ptr<const DataModel> dataModel) { doLock(dataModel.get()); }

	private:
		void doLock(const DataModel* dataModel);

		std::shared_ptr<std::recursive_mutex> mutex;
		std::unique_lock<std::recursive_mutex> lock;
	};

	std::unique_ptr<TimeState> timeState;

	static std::shared_ptr<DataModel> createDataModel();
	static void closeDataModel(std::shared_ptr<DataModel> dataModel, bool joinRunThread);

	DataModel();
	~DataModel() override;

	void loadContent(ContentId contentId);
	void save(ContentId contentId);
	std::string httpGet(std::string url, bool synchronous);
	std::string httpPost(std::string url, std::string data, bool synchronous);
	void close();
	std::shared_ptr<const std::vector<std::shared_ptr<Instance>>> get(ContentId contentId);
	void raiseClose();
	int numChatOptions();
	void buildGui(Adorn*);
	void processGui(const UIEvent&);
	GuiRoot* getGuiRoot() const { return guiRoot.get(); }
	GuiRoot* getGuiHooks() const { return guiHooks.get(); }
	void clearContents();
	std::shared_ptr<std::recursive_mutex> getMutex() const { return mutex; }

	static std::shared_ptr<std::recursive_mutex> getMutex(Instance* instance)
	{
		if (!instance) {
			return std::shared_ptr<std::recursive_mutex>();
		}

		DataModel* dataModel = dynamic_cast<DataModel*>(instance->getRootAncestor());
		if (!dataModel) {
			return std::shared_ptr<std::recursive_mutex>();
		}

		return dataModel->getMutex();
	}

	void setDirty(bool dirty) override { this->dirty = dirty; }
	bool isDirty() const override { return dirty; }

	bool isStackTooBig() const override { return getStackSize() > 10; }

	Workspace* getWorkspace() const { return workspace.get(); }
	float step(float timeInterval);
	float getSimTime() const;
	TimeState* getTimeState();

	std::string evaluate(const std::string& valueName) const override;
	void renderPass2d(Adorn*, IMetric*);
	void renderPass3dAdorn(Adorn*);

	void setUiMessage(std::string message) { uiMessage = message; }

	void clearUiMessage() { uiMessage = ""; }

	void setUiMessageBrickCount() { uiMessage = "[[[progress]]]"; }
	bool canDelaySwapBuffer() const;

protected:
	void onChildAdded(Instance* child) override;

	void onChildChanged(Instance* instance, const PropertyChanged& event) override
	{
		ServiceProvider::onChildChanged(instance, event);
		setDirty(true);
	}

	void onDescendentAdded(Instance* instance) override
	{
		ServiceProvider::onDescendentAdded(instance);
		setDirty(true);
	}

	void onDescendentRemoving(const std::shared_ptr<Instance>& instance) override
	{
		ServiceProvider::onDescendentRemoving(instance);
		setDirty(true);
	}

	void onEvent(const RunService* source, RunTransition event) override;

private:
	static std::string doHttpGet(std::string url);
	static std::string doHttpPost(std::string url, std::string data);
};

} // namespace RBX

#endif // V8DATAMODEL_DATAMODEL_H
