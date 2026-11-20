#ifndef ACTIVEDOCVIEW_H
#define ACTIVEDOCVIEW_H

#include "v8datamodel/DataModel.h"

#include <memory>
#include <set>

class CRobloxDoc;

class ActiveDocView
{
public:
	static void activate(bool activate, CRobloxDoc* doc);

	static std::shared_ptr<RBX::DataModel> activeDataModel() { return s_activeDataModel; }

	void refresh() { activeDataModelChanged(s_activeDataModel, std::shared_ptr<RBX::DataModel>()); }

	static CRobloxDoc* activeDocument() { return s_activeDocument; }

protected:
	ActiveDocView();
	~ActiveDocView();

	virtual void activeDataModelChanged(
		std::shared_ptr<RBX::DataModel> newDataModel, std::shared_ptr<RBX::DataModel> oldDataModel
	) = 0;

	static std::set<ActiveDocView*> s_views;
	static CRobloxDoc* s_activeDocument;
	static std::shared_ptr<RBX::DataModel> s_activeDataModel;
};

#endif // ACTIVEDOCVIEW_H
