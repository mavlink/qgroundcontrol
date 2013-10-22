#include <QDockWidget>
#include <QEvent>
#include <QLabel>

#include "dockwidgeteventfilter.h"

DockWidgetEventFilter::DockWidgetEventFilter(QObject *parent) : QObject(parent) {}

bool DockWidgetEventFilter::eventFilter(QObject *object,QEvent *event)
{
    if (event->type() == QEvent::WindowTitleChange)
    {
        QDockWidget *dock = dynamic_cast<QDockWidget *>(object);
        if(dock) {
            QLabel *label = dynamic_cast<QLabel *>(dock->titleBarWidget());
            if(label)
                label->setText(dock->windowTitle());
        }
    }
    return QObject::eventFilter(object,event);
}
