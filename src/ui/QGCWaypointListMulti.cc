#include "QGCWaypointListMulti.h"
#include "ui_QGCWaypointListMulti.h"
#include "UASManager.h"

void* QGCWaypointListMulti::_offlineUAS = NULL;

QGCWaypointListMulti::QGCWaypointListMulti(QWidget *parent) :
    QWidget(parent),
    _ui(new Ui::QGCWaypointListMulti)
{
    _ui->setupUi(this);
    setMinimumSize(600, 80);
    
    connect(UASManager::instance(), &UASManager::UASCreated, this, &QGCWaypointListMulti::_systemCreated);
    connect(UASManager::instance(), &UASManager::activeUASSet, this, &QGCWaypointListMulti::_systemSetActive);

    WaypointList* list = new WaypointList(_ui->stackedWidget, UASManager::instance()->getActiveUASWaypointManager());
    _lists.insert(_offlineUAS, list);
    _ui->stackedWidget->addWidget(list);

    if (UASManager::instance()->getActiveUAS()) {
        _systemCreated(UASManager::instance()->getActiveUAS());
        _systemSetActive(UASManager::instance()->getActiveUAS());
    }

}

QGCWaypointListMulti::~QGCWaypointListMulti()
{
    delete _ui;
}

void QGCWaypointListMulti::_systemDeleted(QObject* uas)
{
    // Do not dynamic cast or de-reference QObject, since object is either in destructor or may have already
    // been destroyed.

    if (uas) {
        WaypointList* list = _lists.value(uas, NULL);
        if (list) {
            delete list;
            _lists.remove(uas);
        }
    }
}

void QGCWaypointListMulti::_systemCreated(UASInterface* uas)
{
    WaypointList* list = new WaypointList(_ui->stackedWidget, uas->getWaypointManager());
    _lists.insert(uas, list);
    _ui->stackedWidget->addWidget(list);
    // Ensure widget is deleted when system is deleted
    connect(uas, &QObject::destroyed, this, &QGCWaypointListMulti::_systemDeleted);
}

void QGCWaypointListMulti::_systemSetActive(UASInterface* uas)
{
    WaypointList* list = _lists.value(uas, NULL);
    if (list) {
        _ui->stackedWidget->setCurrentWidget(list);
    }
}

void QGCWaypointListMulti::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        _ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
