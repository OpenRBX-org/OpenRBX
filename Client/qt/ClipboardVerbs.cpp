#include "ClipboardVerbs.h"

#include "MainFrm.h"
#include "Roblox.h"
#include "v8datamodel/Selection.h"
#include "v8datamodel/Workspace.h"
#include "v8xml/SerializerV2.h"
#include "v8xml/XmlSerializer.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <cstring>
#include <memory>
#include <sstream>

#ifdef _WIN32
static UINT clipboardFormat()
{
	static UINT format = RegisterClipboardFormatA("Roblox");

	return format;
}

static HWND clipboardOwner()
{
	return reinterpret_cast<HWND>(theApp.mainFrame()->winId());
}
#endif

static bool clipboardAvailable()
{
#ifdef _WIN32
	return IsClipboardFormatAvailable(clipboardFormat()) == TRUE;
#else
	return theApp.clipboardHasFormat("Roblox");
#endif
}

static bool readClipboard(std::string& text)
{
#ifdef _WIN32
	if (!OpenClipboard(clipboardOwner())) {
		return false;
	}

	HANDLE handle = GetClipboardData(clipboardFormat());
	const char* data = static_cast<const char*>(GlobalLock(handle));
	text = data ? data : "";
	GlobalUnlock(handle);
	CloseClipboard();
	return true;
#else
	return theApp.readClipboard("Roblox", text);
#endif
}

static void writeClipboard(const std::string& text)
{
#ifdef _WIN32
	if (OpenClipboard(clipboardOwner())) {
		EmptyClipboard();

		if (HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, text.length() + 1)) {
			if (char* buffer = static_cast<char*>(GlobalLock(handle))) {
				std::strcpy(buffer, text.c_str());
				GlobalUnlock(handle);
				SetClipboardData(clipboardFormat(), handle);
			}
			else {
				GlobalFree(handle);
			}
		}
		CloseClipboard();
	}
#else
	theApp.writeClipboard("Roblox", text);
#endif
}

void CutVerb::doIt(RBX::IDataState* dataState)
{
	getContainer()->getVerb("Copy")->doIt(dataState);
	RBX::DeleteBase::doIt(dataState);
}

PasteVerb::PasteVerb(RBX::DataModel* dataModel, bool pasteInto)
	: RBX::Verb(dataModel, pasteInto ? "PasteInto" : "Paste"), m_dataModel(dataModel), m_pasteInto(pasteInto)
{
}

CopyVerb::CopyVerb(RBX::DataModel* dataModel) : RBX::EditSelectionVerb("Copy", dataModel)
{
}

bool PasteVerb::isEnabled() const
{
	if (!clipboardAvailable()) {
		return false;
	}

	if (!m_pasteInto) {
		return true;
	}

	return m_dataModel->create<RBX::Selection>()->size() == 1;
}

void PasteVerb::doIt(RBX::IDataState* dataState)
{
	if (m_dataModel->findFirstChildByName("NetworkClient")) {
		return;
	}

	std::string text;
	if (!readClipboard(text)) {
		return;
	}

	std::vector<std::shared_ptr<RBX::Instance>> instances;
	{
		std::istringstream stream(text);
		TextXmlParser machine(stream.rdbuf());
		std::unique_ptr<XmlElement> root(machine.parse());

		RBX::Selection* selection = RBX::ServiceProvider::create<RBX::Selection>(m_dataModel);
		RBX::Instance* parent =
			m_pasteInto && selection->size() == 1 ? selection->front().get() : m_dataModel->getWorkspace();
		RBX::InsertMode insertMode = m_pasteInto ? RBX::INSERT_TO_TREE : RBX::INSERT_TO_3D_VIEW;
		SerializerV2 serializer;
		serializer.loadInstances(root.get(), instances);
		m_dataModel->getWorkspace()->insertInstances(instances, parent, insertMode, RBX::SUPPRESS_PROMPTS);

		const std::vector<std::shared_ptr<RBX::Instance>>& items = instances;
		selection->setSelection(items.begin(), items.end());
	}

	RBX::XmlState* undoState = new RBX::UndoState();
	RBX::XmlState* redoState = new RBX::RedoState();
	for (std::vector<std::shared_ptr<RBX::Instance>>::iterator iter = instances.begin(); iter != instances.end();
		 ++iter) {
		undoState->addDelete(*iter->get());
		redoState->addParentProperty(*iter->get());
	}

	dataState->pushState(getName(), undoState, redoState);
	dataState->setDirty(true);
}

void CopyVerb::doIt(RBX::IDataState* dataState)
{
	std::unique_ptr<XmlElement> root(SerializerV2::newRootElement());
	RBX::Selection* selection = RBX::ServiceProvider::create<RBX::Selection>(dataModel);
	std::for_each(
		selection->begin(), selection->end(), [container = root.get()](const std::shared_ptr<RBX::Instance>& instance) {
			RBX::appendInstanceXml(container, instance);
		}
	);

	std::ostringstream stream;
	TextXmlWriter writer(stream);
	writer.serialize(root.get());

	writeClipboard(stream.str());
}
