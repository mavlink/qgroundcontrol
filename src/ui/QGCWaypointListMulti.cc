#include "QGCWaypointListMulti.h"
#include "MultiVehicleManager.h"
#include "UAS.h"

#include "ui_QGCWaypointListMulti.h"

void* QGCWaypointListMulti::_offlineUAS = NULL;

QGCWaypointListMulti::QGCWaypointListMulti(QWidget *parent) :
    QWidget(parent),
    _ui(new Ui::QGCWaypointListMulti)
{
    _ui->setupUi(this);
    setMinimumSize(600, 80);
    
    connect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleAdded, this, &QGCWaypointListMulti::_vehicleAdded);
    connect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleRemoved, this, &QGCWaypointListMulti::_vehicleRemoved);
    connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &QGCWaypointListMulti::_activeVehicleChanged);
    
    WaypointList* list = new WaypointList(_ui->stackedWidget, MultiVehicleManager::instance()->activeWaypointManager());
    _lists.insert(_offlineUAS, list);
    _ui->stackedWidget->addWidget(list);

    if (MultiVehicleManager::instance()->activeVehicle()) {
        _vehicleAdded(MultiVehicleManager::instance()->activeVehicle());
        _activeVehicleChanged(MultiVehicleManager::instance()->activeVehicle());
    }
}

QGCWaypointListMulti::~QGCWaypointListMulti()
{
    delete _ui;
}

void QGCWaypointListMulti::_vehicleRemoved(Vehicle* vehicle)
{
    // Do not dynamic cast or de-reference QObject, since object is either in destructor or may have already
    // been destroyed.

    if (vehicle) {
        UAS* uas = vehicle->uas();
        
        WaypointList* list = _lists.value(uas, NULL);
        if (list) {
            delete list;
            _lists.remove(uas);
        }
    }
}

void QGCWaypointListMulti::_vehicleAdded(Vehicle* vehicle)
{
    UAS* uas = vehicle->uas();
    
    WaypointList* list = new WaypointList(_ui->stackedWidget, uas->getWaypointManager());
    _lists.insert(uas, list);
    _ui->stackedWidget->addWidget(list);
}

void QGCWaypointListMulti::_activeVehicleChanged(Vehicle* vehicle)
{
    if (vehicle) {
        UAS* uas = vehicle->uas();
        
        WaypointList* list = _lists.value(uas, NULL);
        if (list) {
            _ui->stackedWidget->setCurrentWidget(list);
        }
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
