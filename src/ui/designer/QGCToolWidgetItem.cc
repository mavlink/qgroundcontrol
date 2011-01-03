#include "QGCToolWidgetItem.h"

#include <QMenu>
#include <QContextMenuEvent>

QGCToolWidgetItem::QGCToolWidgetItem(QWidget *parent) :
    QWidget(parent),
    isInEditMode(false),
    _component(-1)
{
    startEditAction = new QAction("Edit Slider", this);
    connect(startEditAction, SIGNAL(triggered()), this, SLOT(startEditMode()));
    stopEditAction = new QAction("Finish Editing Slider", this);
    connect(stopEditAction, SIGNAL(triggered()), this, SLOT(endEditMode()));

    endEditMode();
}

QGCToolWidgetItem::~QGCToolWidgetItem()
{
    delete startEditAction;
    delete stopEditAction;
}

void QGCToolWidgetItem::contextMenuEvent (QContextMenuEvent* event)
{
    QMenu menu(this);
    if (!isInEditMode)
    {
        menu.addAction(startEditAction);
    }
    else
    {
        menu.addAction(stopEditAction);
    }
    menu.exec(event->globalPos());
}
