#ifndef SELECTIONCAPTION_H
#define SELECTIONCAPTION_H

#include "ActiveDocView.h"
#include "util/Events.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Selection.h"

#include <QWidget>
#include <atomic>
#include <memory>

class CSelectionCaption : public QWidget,
						  public ActiveDocView,
						  public RBX::Listener<RBX::Selection, RBX::SelectionChanged>
{
public:
	explicit CSelectionCaption(QWidget* parent = nullptr);
	~CSelectionCaption() override;

protected:
	void activeDataModelChanged(
		std::shared_ptr<RBX::DataModel> newDataModel, std::shared_ptr<RBX::DataModel> oldDataModel
	) override;
	void onEvent(const RBX::Selection* source, RBX::SelectionChanged event) override;

	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void showEvent(QShowEvent* event) override;

private:
	void updateCaption();

	QString m_caption;
	bool m_captionDrawn = false;
	std::atomic<bool> m_updatePosted = false;
	std::shared_ptr<RBX::DataModel> m_dataModel;
	std::shared_ptr<RBX::Selection> m_selection;
};

#endif // SELECTIONCAPTION_H
