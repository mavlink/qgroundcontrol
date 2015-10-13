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

#ifndef MultiVehicleDockWidget_H
#define MultiVehicleDockWidget_H

#include <QMap>

#include "QGCDockWidget.h"
#include "Vehicle.h"

namespace Ui
{
    class MultiVehicleDockWidget;
}

/// Provides a base class for a dock widget which automatically handles
/// Vehicles coming and going. It does this by using a stacked widget which
/// holds individual Vehicle specific widgets.
class MultiVehicleDockWidget : public QGCDockWidget
{
    Q_OBJECT

public:
    explicit MultiVehicleDockWidget(const QString& title, QAction* action, QWidget *parent = 0);
    ~MultiVehicleDockWidget();
    
    /// Must be called in the derived class contructor to initialize the base class
    void init(void);

protected:
    /// Derived class must implement this to create the QWidget for the
    /// specified Vehicle.
    virtual QWidget* _newVehicleWidget(Vehicle* vehicle, QWidget* parent) = 0;
    
private slots:
    void _vehicleAdded(Vehicle* vehicle);
    void _vehicleRemoved(Vehicle* vehicle);
    void _activeVehicleChanged(Vehicle* vehicle);

private:
    QMap<int, QWidget*> _vehicleWidgets;
    
    Ui::MultiVehicleDockWidget* _ui;
};

#endif
