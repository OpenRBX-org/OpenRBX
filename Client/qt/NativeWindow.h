#ifndef NATIVEWINDOW_H
#define NATIVEWINDOW_H

#include <QPoint>
#include <QRect>

class QWidget;

void removeWindowIcon(QWidget* window);
void applyDialogTemplateStyle(QWidget* window, unsigned style, unsigned exStyle);
QRect windowFrameRect(const QWidget* window);
void centerWindowOverOwner(QWidget* window);
void moveWindowFrame(QWidget* window, const QPoint& position);

#ifdef _WIN32
struct HWND__;
void centerWindowOverOwner(HWND__* window);
#endif

#endif // NATIVEWINDOW_H
