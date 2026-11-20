#ifndef DISPLAYMODE_H
#define DISPLAYMODE_H

#include <QSize>

class QWidget;

bool changeDisplayMode(QWidget* window, QSize size);
void restoreDisplayMode(QWidget* window);

#endif // DISPLAYMODE_H
