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
