#ifndef WORKSPACE_H
#define WORKSPACE_H

#include "reflection/type.h"
#include "util/ContentProvider.h"
#include "util/Events.h"
#include "util/RunStateOwner.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Selection.h"
#include "v8datamodel/Workspace.h"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

class XmlElement;

class CWorkspace : public RBX::Listener<RBX::Selection, RBX::SelectionChanged>,
				   public RBX::Listener<RBX::Workspace, RBX::DrawChanged>
{
public:
	CWorkspace();
	~CWorkspace() override;

	bool SaveToFile(const std::string& pathName);
	bool SaveSelectionToFile(const std::string& pathName);
	bool Insert(const std::string& url);
	std::unique_ptr<std::vector<RBX::Reflection::Value>> ExecScript(
		const std::string& script, const std::vector<RBX::Reflection::Value>& arguments
	);
	std::unique_ptr<std::vector<RBX::Reflection::Value>> ExecUrlScript(
		const std::string& url, const std::vector<RBX::Reflection::Value>& arguments
	);
	void Write(std::ostream& stream);
	void WriteSelection(std::ostream& stream);
	void Close();

	std::shared_ptr<RBX::DataModel> dataModel() const;

	RBX::Workspace* workspace() const;

	void load(std::istream& stream);
	void closeDataModel();
	void insertContent(RBX::ContentId contentId, bool intoSelection);

protected:
	void onEvent(const RBX::Selection* source, RBX::SelectionChanged event) override;
	void onEvent(const RBX::Workspace* source, RBX::DrawChanged event) override;

private:
	std::unique_ptr<XmlElement> writeDocument();
	std::unique_ptr<XmlElement> writeSelectionElement();
	void serialize(const XmlElement* root, std::ostream* stream);
	std::unique_ptr<std::vector<RBX::Reflection::Value>> DoExecScript(
		const char* script, const std::vector<RBX::Reflection::Value>& arguments
	);

	std::shared_ptr<RBX::DataModel> m_dataModel;
	std::shared_ptr<RBX::RunService> m_runService;
};

#endif // WORKSPACE_H
