#include "Workspace.h"

#include "script/ScriptContext.h"
#include "security/SecurityContext.h"
#include "util/Utilities.h"
#include "util/standardout.h"
#include "v8datamodel/Commands.h"
#include "v8xml/SerializerV2.h"
#include "v8xml/XmlSerializer.h"

#include <G3D/format.h>
#include <algorithm>
#include <fstream>

CWorkspace::CWorkspace()
{
	m_dataModel = RBX::DataModel::createDataModel();

	RBX::DataModel::Lock lock(m_dataModel);
	m_runService = RBX::shared_from(m_dataModel->create<RBX::RunService>());
	RBX::startListening<RBX::Workspace, RBX::DrawChanged>(m_dataModel->getWorkspace(), this);
	RBX::startListening<RBX::Selection, RBX::SelectionChanged>(m_dataModel->create<RBX::Selection>(), this);
}

CWorkspace::~CWorkspace()
{
	closeDataModel();
}

std::shared_ptr<RBX::DataModel> CWorkspace::dataModel() const
{
	return m_dataModel;
}

RBX::Workspace* CWorkspace::workspace() const
{
	return m_dataModel ? m_dataModel->getWorkspace() : nullptr;
}

void CWorkspace::onEvent(const RBX::Workspace* source, RBX::DrawChanged event)
{
	m_runService->invalidateRunViews();
}

void CWorkspace::onEvent(const RBX::Selection* source, RBX::SelectionChanged event)
{
	m_runService->invalidateRunViews();
}

void CWorkspace::load(std::istream& stream)
{
	RBX::DataModel::Lock lock(m_dataModel);
	SerializerV2 serializer;
	serializer.load(stream, m_dataModel.get());
	workspace()->joinAllHack();
}

std::unique_ptr<XmlElement> CWorkspace::writeDocument()
{
	std::unique_ptr<XmlElement> root(SerializerV2::newRootElement());
	RBX::DataModel::Lock lock(m_dataModel);
	dataModel()->writeChildren(root.get());
	return root;
}

void CWorkspace::closeDataModel()
{
	if (m_dataModel) {
		{
			RBX::DataModel::Lock lock(m_dataModel);
			RBX::stopListening<RBX::Selection, RBX::SelectionChanged>(m_dataModel->find<RBX::Selection>(), this);
			RBX::stopListening<RBX::Workspace, RBX::DrawChanged>(m_dataModel->getWorkspace(), this);
		}

		m_runService.reset();
		RBX::DataModel::closeDataModel(m_dataModel, true);
		m_dataModel.reset();
	}
}

std::unique_ptr<std::vector<RBX::Reflection::Value>> CWorkspace::DoExecScript(
	const char* script, const std::vector<RBX::Reflection::Value>& arguments
)
{
	try {
		RBX::DataModel::Lock lock(m_dataModel);
		return dataModel()->create<RBX::ScriptContext>()->executeInNewThread(
			RBX::Security::TrustedCOM, script, nullptr, arguments
		);
	}
	catch (std::exception& e) {
		std::string text(script);
		std::string message = G3D::format("DoExecScript failed because %s.  %s", e.what(), text.substr(0, 64).c_str());
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "%s", message.c_str());
		throw std::runtime_error(message);
	}
}

std::unique_ptr<std::vector<RBX::Reflection::Value>> CWorkspace::ExecUrlScript(
	const std::string& url, const std::vector<RBX::Reflection::Value>& arguments
)
{
	std::string script;

	try {
		std::unique_ptr<std::istream> content = RBX::ContentProvider::singleton().getContent(RBX::ContentId(url));
		script = RBX::readStream(*content);
	}
	catch (std::exception& e) {
		std::string message = G3D::format("Failed to get URL, %s", e.what());
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "%s", message.c_str());
		throw std::runtime_error(message);
	}

	return DoExecScript(script.c_str(), arguments);
}

void CWorkspace::Close()
{
	closeDataModel();
}

std::unique_ptr<XmlElement> CWorkspace::writeSelectionElement()
{
	std::unique_ptr<XmlElement> root(SerializerV2::newRootElement());
	RBX::DataModel::Lock lock(m_dataModel);
	RBX::Selection* selection = RBX::ServiceProvider::create<RBX::Selection>(workspace());
	std::for_each(
		selection->begin(), selection->end(), [container = root.get()](const std::shared_ptr<RBX::Instance>& instance) {
			RBX::appendInstanceXml(container, instance);
		}
	);
	return root;
}

void CWorkspace::serialize(const XmlElement* root, std::ostream* stream)
{
	TextXmlWriterWithEmbeddedContent writer(*stream);
	writer.serialize(root);
}

bool CWorkspace::SaveSelectionToFile(const std::string& pathName)
{
	std::ofstream stream;
	stream.open(pathName.c_str(), std::ios_base::out);

	RBX::DataModel::Lock lock(m_dataModel);
	serialize(writeSelectionElement().get(), &stream);
	return true;
}

void CWorkspace::insertContent(RBX::ContentId contentId, bool intoSelection)
{
	if (m_dataModel->findFirstChildByName("NetworkClient")) {
		return;
	}

	RBX::DataModel::Lock lock(m_dataModel);
	RBX::Selection* selection = RBX::ServiceProvider::create<RBX::Selection>(workspace());
	RBX::Instance* parent = intoSelection && selection->size() == 1 ? selection->front().get() : workspace();
	RBX::InsertMode insertMode = intoSelection ? RBX::INSERT_TO_TREE : RBX::INSERT_TO_3D_VIEW;
	RBX::PromptMode promptMode = intoSelection ? RBX::SUPPRESS_PROMPTS : RBX::ALLOW_PROMPTS;
	std::vector<std::shared_ptr<RBX::Instance>> instances;

	try {
		{
			std::unique_ptr<std::istream> stream = RBX::ContentProvider::singleton().getContent(contentId);
			TextXmlParser machine(stream->rdbuf());
			std::unique_ptr<XmlElement> root = machine.parse();
			SerializerV2().loadInstances(root.get(), instances);
		}

		workspace()->insertInstances(instances, parent, insertMode, promptMode);

		if (m_runService->getRunState() == RBX::RS_NORMAL) {
			const std::vector<std::shared_ptr<RBX::Instance>>& items = instances;
			selection->setSelection(items.begin(), items.end());
		}
	}
	catch (...) {
		std::for_each(instances.begin(), instances.end(), [](const std::shared_ptr<RBX::Instance>& instance) {
			instance->setParent(nullptr);
		});
		throw;
	}
}

std::unique_ptr<std::vector<RBX::Reflection::Value>> CWorkspace::ExecScript(
	const std::string& script, const std::vector<RBX::Reflection::Value>& arguments
)
{
	if (RBX::ContentProvider::isHttpUrl(script)) {
		return ExecUrlScript(script, arguments);
	}

	return DoExecScript(script.c_str(), arguments);
}

void CWorkspace::Write(std::ostream& stream)
{
	serialize(writeDocument().get(), &stream);
}

void CWorkspace::WriteSelection(std::ostream& stream)
{
	serialize(writeSelectionElement().get(), &stream);
}

bool CWorkspace::SaveToFile(const std::string& pathName)
{
	try {
		std::ofstream stream;
		stream.open(pathName.c_str(), std::ios_base::out | std::ios_base::binary);

		RBX::DataModel::Lock lock(m_dataModel);
		serialize(writeDocument().get(), &stream);
		return true;
	}
	catch (const std::exception& e) {
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "%s", e.what());
		return false;
	}
}

bool CWorkspace::Insert(const std::string& url)
{
	try {
		insertContent(RBX::ContentId(url), false);
		return true;
	}
	catch (const std::exception& e) {
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "%s", e.what());
		return false;
	}
}
