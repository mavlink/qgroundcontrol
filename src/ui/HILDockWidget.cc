/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "HILDockWidget.h"
#include "QGCHilConfiguration.h"

HILDockWidget::HILDockWidget(const QString& title, QAction* action, QWidget *parent)
    : MultiVehicleDockWidget(title, action, parent)
{
    init();
    
    loadSettings();
}

HILDockWidget::~HILDockWidget()
{

}

QWidget* HILDockWidget::_newVehicleWidget(Vehicle* vehicle, QWidget* parent)
{
    return new QGCHilConfiguration(vehicle, parent);
}
