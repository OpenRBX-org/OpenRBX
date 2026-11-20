#ifndef PROPERTYSHEET_H
#define PROPERTYSHEET_H

#include <QDialog>
#include <vector>

class CDialog;
class QPushButton;
class PropertySheetTabs;

class CPropertySheet : public QDialog
{
public:
	CPropertySheet(const QString& caption, QWidget* parent);

	void AddPage(CDialog* page);
	int RunModeless();

protected:
	void showEvent(QShowEvent* event) override;
	void paintEvent(QPaintEvent* event) override;

	QPushButton* m_ok;
	QPushButton* m_cancel;
	QPushButton* m_apply;
	QPushButton* m_help;

private:
	void layoutPages();
	QRect tabRect() const;

	PropertySheetTabs* m_tabs;
	std::vector<CDialog*> m_pages;
};

#endif // PROPERTYSHEET_H
