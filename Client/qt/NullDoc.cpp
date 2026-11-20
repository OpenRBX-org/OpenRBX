#include "NullDoc.h"

#include "CommandIds.h"

CNullDoc::CNullDoc()
{
	for (unsigned id : {ID_FILE_SAVE, ID_FILE_SAVE_AS, ID_FILE_PUBLISH}) {
		addCommandUpdate(id, [](CommandState& state) { state.enabled = false; });
	}
}
