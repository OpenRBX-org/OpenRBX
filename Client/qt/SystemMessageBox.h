#ifndef SYSTEMMESSAGEBOX_H
#define SYSTEMMESSAGEBOX_H

#include <QMessageBox>

namespace SystemMessageBox {

QMessageBox::StandardButton information(
	QWidget* parent, const QString& title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok
);
QMessageBox::StandardButton question(
	QWidget* parent,
	const QString& title,
	const QString& text,
	QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No
);
QMessageBox::StandardButton warning(
	QWidget* parent,
	const QString& title,
	const QString& text,
	QMessageBox::StandardButtons buttons = QMessageBox::Ok,
	QMessageBox::StandardButton defaultButton = QMessageBox::NoButton
);
QMessageBox::StandardButton critical(
	QWidget* parent, const QString& title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok
);

} // namespace SystemMessageBox

#endif // SYSTEMMESSAGEBOX_H
