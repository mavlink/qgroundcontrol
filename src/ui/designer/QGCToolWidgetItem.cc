#include "QGCToolWidgetItem.h"

#include <QMenu>
#include <QContextMenuEvent>

#include "QGCToolWidget.h"
#include "UASManager.h"
#include <QDockWidget>

QGCToolWidgetItem::QGCToolWidgetItem(const QString& name, QWidget *parent) :
    QWidget(parent),
    uas(NULL),
    isInEditMode(false),
    qgcToolWidgetItemName(name),
    _component(-1)
{
    startEditAction = new QAction(tr("Edit %1").arg(qgcToolWidgetItemName), this);
    connect(startEditAction, SIGNAL(triggered()), this, SLOT(startEditMode()));
    stopEditAction = new QAction(tr("Finish Editing %1").arg(qgcToolWidgetItemName), this);
    connect(stopEditAction, SIGNAL(triggered()), this, SLOT(endEditMode()));
    deleteAction = new QAction(tr("Delete %1").arg(qgcToolWidgetItemName), this);
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteLater()));

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)),
            this, SLOT(setActiveUAS(UASInterface*)));
    // Set first UAS if it exists
    setActiveUAS(UASManager::instance()->getActiveUAS());
}

QGCToolWidgetItem::~QGCToolWidgetItem()
{
    delete startEditAction;
    delete stopEditAction;
    delete deleteAction;
}

void QGCToolWidgetItem::contextMenuEvent (QContextMenuEvent* event)
{
    QMenu menu(this);
    if (!isInEditMode) {
        menu.addAction(startEditAction);
        menu.addAction(deleteAction);
    } else {
        menu.addAction(stopEditAction);
    }
    menu.exec(event->globalPos());
}

void QGCToolWidgetItem::setActiveUAS(UASInterface *uas)
{
    this->uas = uas;
}

void QGCToolWidgetItem::setEditMode(bool editMode)
{
    isInEditMode = editMode;

    // Attempt to undock the dock widget
    QWidget* p = this;
    QDockWidget* dock;

    do {
        p = p->parentWidget();
        dock = dynamic_cast<QDockWidget*>(p);

        if (dock)
        {
            dock->setFloating(editMode);
            break;
        }
    } while (p && !dock);

    emit editingFinished();
}
