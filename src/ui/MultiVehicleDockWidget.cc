/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MultiVehicleDockWidget.h"
#include "ui_MultiVehicleDockWidget.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"

MultiVehicleDockWidget::MultiVehicleDockWidget(const QString& title, QAction* action, QWidget *parent)
    : QGCDockWidget(title, action, parent)
    , _ui(new Ui::MultiVehicleDockWidget)
{
    _ui->setupUi(this);
    
    setWindowTitle(title);
    
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &MultiVehicleDockWidget::_activeVehicleChanged);
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::vehicleAdded, this, &MultiVehicleDockWidget::_vehicleAdded);
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::vehicleRemoved, this, &MultiVehicleDockWidget::_vehicleRemoved);
}

void MultiVehicleDockWidget::init(void)
{
    QmlObjectListModel* vehicles = qgcApp()->toolbox()->multiVehicleManager()->vehicles();

    for (int i=0; i<vehicles->count(); i++) {
        _vehicleAdded(qobject_cast<Vehicle*>(vehicles->get(i)));
    }

    if (qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()) {
        _activeVehicleChanged(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle());
    }
}

MultiVehicleDockWidget::~MultiVehicleDockWidget()
{
    delete _ui;
}

void MultiVehicleDockWidget::_vehicleRemoved(Vehicle* vehicle)
{
    int id = vehicle->id();
    
    if (_vehicleWidgets.contains(id)) {
        _vehicleWidgets[id]->deleteLater();
        _vehicleWidgets.remove(id);
    }
}

void MultiVehicleDockWidget::_vehicleAdded(Vehicle* vehicle)
{
    int id = vehicle->id();
    
    if (!_vehicleWidgets.contains(id)) {
        QWidget* vehicleWidget = _newVehicleWidget(vehicle, _ui->stackedWidget);
        _vehicleWidgets[id] = vehicleWidget;
        _ui->stackedWidget->addWidget(vehicleWidget);
    }
}

void MultiVehicleDockWidget::_activeVehicleChanged(Vehicle* vehicle)
{
    if (vehicle) {
        int id = vehicle->id();
        
        if (!_vehicleWidgets.contains(id)) {
            _vehicleAdded(vehicle);
        }
        
        QWidget* vehicleWidget = _vehicleWidgets[id];
        _ui->stackedWidget->setCurrentWidget(vehicleWidget);
    }
}
