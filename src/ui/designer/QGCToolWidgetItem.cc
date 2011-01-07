#include "QGCToolWidgetItem.h"

#include <QMenu>
#include <QContextMenuEvent>

#include "UASManager.h"

QGCToolWidgetItem::QGCToolWidgetItem(const QString& name, QWidget *parent) :
    QWidget(parent),
    isInEditMode(false),
    qgcToolWidgetItemName(name),
    uas(NULL),
    _component(-1)
{
    startEditAction = new QAction("Edit "+qgcToolWidgetItemName, this);
    connect(startEditAction, SIGNAL(triggered()), this, SLOT(startEditMode()));
    stopEditAction = new QAction("Finish Editing Slider", this);
    connect(stopEditAction, SIGNAL(triggered()), this, SLOT(endEditMode()));

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)),
            this, SLOT(setActiveUAS(UASInterface*)));
    // Set first UAS if it exists
    setActiveUAS(UASManager::instance()->getActiveUAS());

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

void QGCToolWidgetItem::setActiveUAS(UASInterface *uas)
{
    this->uas = uas;
}
