#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <map>
#include <optional>

#ifndef IDOK
#define IDOK 1
#endif
#ifndef IDCANCEL
#define IDCANCEL 2
#endif

class CDialog : public QDialog
{
public:
	CDialog(unsigned nIDTemplate, QWidget* parent = nullptr);
	enum Child
	{
		ChildPage
	};
	CDialog(unsigned nIDTemplate, QWidget* parent, Child child);
	~CDialog() override;

	int DoModal();
	void EndDialog(int result);
	QWidget* GetDlgItem(int id) const;
	QString GetDlgItemText(int id) const;
	void SetDlgItemText(int id, const QString& text);
	void setFramePosition(const QPoint& position);

	static QRect MapDialogRect(int x, int y, int cx, int cy);
	QString caption() const;

protected:
	void showEvent(QShowEvent* event) override;
	virtual bool OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual void OnCommand(int id);

	void reject() override;
	void ReplaceDlgItem(int id, QWidget* widget);

private:
	void CreateControls();

	unsigned m_nIDTemplate;
	std::map<int, QWidget*> m_controls;
	int m_result;
	std::optional<QPoint> m_framePosition;
};

#endif // DIALOG_H
