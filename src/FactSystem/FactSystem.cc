/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "FactSystem.h"
#include "UASManager.h"
#include "QGCApplication.h"
#include "VehicleComponent.h"

#include <QtQml>

IMPLEMENT_QGC_SINGLETON(FactSystem, FactSystem)

const char* FactSystem::_factSystemQmlUri = "QGroundControl.FactSystem";

FactSystem::FactSystem(QObject* parent) :
    QGCSingleton(parent)
{
    qmlRegisterType<Fact>(_factSystemQmlUri, 1, 0, "Fact");
    qmlRegisterType<FactValidator>(_factSystemQmlUri, 1, 0, "FactValidator");
    
    // FIXME: Where should these go?
    qmlRegisterUncreatableType<VehicleComponent>(_factSystemQmlUri, 1, 0, "VehicleComponent", "Can only reference VehicleComponent");
}

FactSystem::~FactSystem()
{

}
