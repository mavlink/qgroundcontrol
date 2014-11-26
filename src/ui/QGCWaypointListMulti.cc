#include "QGCWaypointListMulti.h"
#include "ui_QGCWaypointListMulti.h"
#include "UASManager.h"

QGCWaypointListMulti::QGCWaypointListMulti(QWidget *parent) :
    QWidget(parent),
    offline_uas_id(0),
    ui(new Ui::QGCWaypointListMulti)
{
    ui->setupUi(this);
    setMinimumSize(600, 80);
    connect(qgcApp()->singletonUASManager(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(systemCreated(UASInterface*)));
    connect(qgcApp()->singletonUASManager(), SIGNAL(activeUASSet(int)), this, SLOT(systemSetActive(int)));

    WaypointList* list = new WaypointList(ui->stackedWidget, qgcApp()->singletonUASManager()->getActiveUASWaypointManager());
    lists.insert(offline_uas_id, list);
    ui->stackedWidget->addWidget(list);

    if (qgcApp()->singletonUASManager()->getActiveUAS()) {
        systemCreated(qgcApp()->singletonUASManager()->getActiveUAS());
        systemSetActive(qgcApp()->singletonUASManager()->getActiveUAS()->getUASID());
    }

}

void QGCWaypointListMulti::systemDeleted(QObject* uas)
{
    // Do not dynamic cast or de-reference QObject, since object is either in destructor or may have already
    // been destroyed.

    UASInterface* mav = static_cast<UASInterface*>(uas);
    if (mav)
    {
        int id = mav->getUASID();
        WaypointList* list = lists.value(id, NULL);
        if (list)
        {
            delete list;
            lists.remove(id);
        }
    }
}

void QGCWaypointListMulti::systemCreated(UASInterface* uas)
{
    WaypointList* list = new WaypointList(ui->stackedWidget, uas->getWaypointManager());
    lists.insert(uas->getUASID(), list);
    ui->stackedWidget->addWidget(list);
    // Ensure widget is deleted when system is deleted
    connect(uas, SIGNAL(destroyed(QObject*)), this, SLOT(systemDeleted(QObject*)));
}

void QGCWaypointListMulti::systemSetActive(int uas)
{
    WaypointList* list = lists.value(uas, NULL);
    if (list) {
        ui->stackedWidget->setCurrentWidget(list);
    }
}

QGCWaypointListMulti::~QGCWaypointListMulti()
{
    delete ui;
}

void QGCWaypointListMulti::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
