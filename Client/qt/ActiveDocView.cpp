#include "ActiveDocView.h"

#include "RobloxDoc.h"

CRobloxDoc* ActiveDocView::s_activeDocument;

std::set<ActiveDocView*> ActiveDocView::s_views;

std::shared_ptr<RBX::DataModel> ActiveDocView::s_activeDataModel;

void ActiveDocView::activate(bool activate, CRobloxDoc* doc)
{
	std::shared_ptr<RBX::DataModel> oldDataModel = s_activeDataModel;

	if (activate) {
		s_activeDocument = doc;
		s_activeDataModel = doc->dataModel();
	}
	else {
		if (doc == s_activeDocument) {
			s_activeDocument = nullptr;
		}

		if (doc->dataModel() == s_activeDataModel) {
			s_activeDataModel.reset();
		}
	}

	if (s_activeDataModel != oldDataModel) {
		for (std::set<ActiveDocView*>::iterator it = s_views.begin(); it != s_views.end(); ++it) {
			(*it)->activeDataModelChanged(s_activeDataModel, oldDataModel);
		}
	}
}

ActiveDocView::ActiveDocView()
{
	s_views.insert(this);
}

ActiveDocView::~ActiveDocView()
{
	s_views.erase(this);
}
