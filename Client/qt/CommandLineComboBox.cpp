#include "CommandLineComboBox.h"

#include <QAbstractItemView>
#include <QLabel>
#include <QScreen>
#include <algorithm>

static const int dropDownItemCount = 12;
static const int dropDownBorder = 4;
static const int controlWidth = 100;

CommandLineComboBox::CommandLineComboBox(QLabel* caption, QWidget* parent) : QComboBox(parent), m_caption(caption)
{
	setEditable(true);
	setInsertPolicy(QComboBox::NoInsert);
	setMaxVisibleItems(dropDownItemCount);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

QSize CommandLineComboBox::sizeHint() const
{
	return QSize(std::max(0, controlWidth - m_caption->sizeHint().width()), QComboBox::sizeHint().height());
}

QSize CommandLineComboBox::minimumSizeHint() const
{
	return sizeHint();
}

void CommandLineComboBox::showPopup()
{
	QComboBox::showPopup();

	QWidget* popup = view()->window();
	int left = m_caption->mapToGlobal(QPoint(0, 0)).x();
	int right = mapToGlobal(QPoint(width(), 0)).x();
	int itemHeight = std::max(view()->sizeHintForRow(0), fontMetrics().height());
	int popupHeight = std::min(dropDownItemCount, std::max(1, count())) * itemHeight + dropDownBorder;

	int top = mapToGlobal(QPoint(0, height())).y();
	if (top + popupHeight > screen()->availableGeometry().bottom()) {
		top = mapToGlobal(QPoint(0, 0)).y() - popupHeight;
	}

	popup->setGeometry(left, top, right - left, popupHeight);
}
