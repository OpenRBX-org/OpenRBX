#ifndef SYSTEMCOLORDIALOG_H
#define SYSTEMCOLORDIALOG_H

#include <QColor>
#include <optional>

class QWidget;

namespace SystemColorDialog {

std::optional<QColor> getColor(QColor initial, QWidget* parent);

} // namespace SystemColorDialog

#endif // SYSTEMCOLORDIALOG_H
