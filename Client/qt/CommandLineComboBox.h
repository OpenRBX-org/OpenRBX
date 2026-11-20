#ifndef COMMANDLINECOMBOBOX_H
#define COMMANDLINECOMBOBOX_H

#include <QComboBox>

class QLabel;

class CommandLineComboBox : public QComboBox
{
public:
	CommandLineComboBox(QLabel* caption, QWidget* parent);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;
	void showPopup() override;

private:
	QLabel* m_caption;
};

#endif // COMMANDLINECOMBOBOX_H
