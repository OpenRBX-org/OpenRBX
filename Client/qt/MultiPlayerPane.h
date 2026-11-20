#ifndef MULTIPLAYERPANE_H
#define MULTIPLAYERPANE_H

#include <QWidget>

class CChatPrompt;

namespace RBX {
class DataModel;
}

class CMultiPlayerPane : public QWidget
{
public:
	explicit CMultiPlayerPane(QWidget* parent);
	~CMultiPlayerPane() override;

	void create(QWidget* parent, RBX::DataModel* dataModel);

protected:
	void resizeEvent(QResizeEvent* event) override;

private:
	CChatPrompt* m_chatPrompt;
};

#endif // MULTIPLAYERPANE_H
