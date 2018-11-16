/****************************************************************************
 *
 *   (c) 2009-2018 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

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

