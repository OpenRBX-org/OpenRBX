#include "MultiPlayerPane.h"

#include "ChatPrompt.h"

#include <QResizeEvent>

CMultiPlayerPane::CMultiPlayerPane(QWidget* parent) : QWidget(parent), m_chatPrompt(nullptr)
{
}

CMultiPlayerPane::~CMultiPlayerPane() = default;

void CMultiPlayerPane::create(QWidget* parent, RBX::DataModel* dataModel)
{
	m_chatPrompt = new CChatPrompt(this);
	m_chatPrompt->setDataModel(dataModel, parent);
	m_chatPrompt->show();
}

void CMultiPlayerPane::resizeEvent(QResizeEvent* event)
{
	if (m_chatPrompt) {
		m_chatPrompt->setGeometry(0, 0, event->size().width(), event->size().height());
	}

	QWidget::resizeEvent(event);
}
