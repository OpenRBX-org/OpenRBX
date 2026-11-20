#include "SelectionCaption.h"

#include "Resources.h"
#include "WhidbeyStyle.h"

#include <QPainter>
#include <algorithm>

static const unsigned singleItemStringId = 180;
static const unsigned namedItemsStringId = 181;
static const unsigned classItemsStringId = 182;
static const unsigned mixedItemsStringId = 183;
static const int textRectPadding = 12;

CSelectionCaption::CSelectionCaption(QWidget* parent) : QWidget(parent)
{
	refresh();
}

CSelectionCaption::~CSelectionCaption()
{
	RBX::DataModel::Lock lock(m_dataModel);
	RBX::stopListening<RBX::Selection, RBX::SelectionChanged>(m_selection, this);
}

void CSelectionCaption::updateCaption()
{
	QString caption;
	std::string name;
	if (m_selection) {
		RBX::DataModel::Lock lock(m_dataModel);
		std::vector<std::shared_ptr<RBX::Instance>>::const_iterator it = m_selection->begin();
		if (it != m_selection->end()) {
			name = (*it)->getName();
			const RBX::Name* className = &(*it)->getClassName();
			while (++it != m_selection->end()) {
				if ((*it)->getName() != name) {
					name = "";
					if (!className) {
						break;
					}
				}
				if (&(*it)->getClassName() != className) {
					className = nullptr;
					if (name == "") {
						break;
					}
				}
			}

			int count = static_cast<int>(m_selection->size());
			if (!className) {
				caption = QString::asprintf(Resources::string(mixedItemsStringId).toLocal8Bit().constData(), count);
			}
			else if (name != "") {
				if (count > 1) {
					caption = QString::asprintf(
						Resources::string(namedItemsStringId).toLocal8Bit().constData(),
						className->name.c_str(),
						name.c_str(),
						count
					);
				}
				else {
					caption = QString::asprintf(
						Resources::string(singleItemStringId).toLocal8Bit().constData(),
						className->name.c_str(),
						name.c_str()
					);
				}
			}
			else {
				caption = QString::asprintf(
					Resources::string(classItemsStringId).toLocal8Bit().constData(), className->name.c_str(), count
				);
			}
		}
	}

	m_caption = caption;
	m_captionDrawn = true;
	update();
}

void CSelectionCaption::onEvent(const RBX::Selection* source, RBX::SelectionChanged event)
{
	if (!m_updatePosted.exchange(true)) {
		QMetaObject::invokeMethod(
			this,
			[this]() {
				m_updatePosted = false;
				updateCaption();
			},
			Qt::QueuedConnection
		);
	}
}

void CSelectionCaption::activeDataModelChanged(
	std::shared_ptr<RBX::DataModel> newDataModel, std::shared_ptr<RBX::DataModel> oldDataModel
)
{
	{
		RBX::DataModel::Lock lock(m_dataModel);
		RBX::stopListening<RBX::Selection, RBX::SelectionChanged>(m_selection, this);
		m_selection.reset();
	}

	m_dataModel.reset();

	if (newDataModel) {
		m_dataModel = newDataModel;

		RBX::DataModel::Lock lock(m_dataModel);
		m_selection = RBX::shared_from(m_dataModel->create<RBX::Selection>());
		RBX::startListening<RBX::Selection, RBX::SelectionChanged>(m_selection, this);
	}

	if (!m_selection) {
		updateCaption();
	}
}

void CSelectionCaption::paintEvent(QPaintEvent* event)
{
	const WhidbeyColors& c = WhidbeyStyle::colors();
	QPainter painter(this);
	painter.fillRect(rect(), c.face);
	if (!m_captionDrawn) {
		return;
	}

	painter.fillRect(0, 0, width() - 1, 1, c.buttonHighlight);
	painter.fillRect(0, 0, 1, height() - 1, c.buttonHighlight);
	painter.fillRect(width() - 1, 0, 1, height(), c.shadow);
	painter.fillRect(0, height() - 1, width(), 1, c.shadow);

	if (!m_caption.isEmpty()) {
		QFont measured = font();
		measured.setKerning(false);
		QFontMetrics metrics(measured);
		int textWidth =
			std::min(metrics.horizontalAdvance(m_caption) + metrics.averageCharWidth() + textRectPadding, width());
		QString text = WhidbeyStyle::endEllipsis(font(), m_caption, textWidth);

		painter.setPen(c.buttonText);
		painter.drawText(
			QRect(
				(textWidth - metrics.horizontalAdvance(text)) / 2,
				(height() - metrics.height()) / 2,
				textWidth,
				metrics.height()
			),
			Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine,
			text
		);
	}
}

void CSelectionCaption::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	m_captionDrawn = false;
}

void CSelectionCaption::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	m_captionDrawn = false;
}
