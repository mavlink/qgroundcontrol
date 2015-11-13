/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

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
