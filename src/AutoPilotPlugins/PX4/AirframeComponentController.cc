/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#include "AirframeComponentController.h"
#include "AirframeComponentAirframes.h"
#include "QGCMAVLink.h"
#include "UASManager.h"
#include "AutoPilotPluginManager.h"
#include "QGCApplication.h"
#include "QGCMessageBox.h"

#include <QVariant>
#include <QQmlProperty>

bool AirframeComponentController::_typesRegistered = false;

AirframeComponentController::AirframeComponentController(void) :
    _currentVehicleIndex(0),
    _autostartId(0),
    _showCustomConfigPanel(false)
{
    if (!_typesRegistered) {
        _typesRegistered = true;
        qmlRegisterUncreatableType<AirframeType>("QGroundControl.Controllers", 1, 0, "AiframeType", "Can only reference AirframeType");
        qmlRegisterUncreatableType<Airframe>("QGroundControl.Controllers", 1, 0, "Aiframe", "Can only reference Airframe");
    }
    
    QStringList usedParams;
    usedParams << "SYS_AUTOSTART" << "SYS_AUTOCONFIG";
    if (!_allParametersExists(FactSystem::defaultComponentId, usedParams)) {
        return;
    }
    
    // Load up member variables
    
    bool autostartFound = false;
    _autostartId = getParameterFact(FactSystem::defaultComponentId, "SYS_AUTOSTART")->value().toInt();


    
    for (unsigned tindex = 0; tindex < AirframeComponentAirframes::rgAirframeTypes.count(); tindex++) {

        const AirframeComponentAirframes::AirframeType_t* pType = &AirframeComponentAirframes::rgAirframeTypes.values().at(tindex);

        AirframeType* airframeType = new AirframeType(pType->name, pType->imageResource, this);
        Q_CHECK_PTR(airframeType);

        for (unsigned index = 0; index < pType->rgAirframeInfo.count(); index++) {
            const AirframeComponentAirframes::AirframeInfo_t* pInfo = &pType->rgAirframeInfo.at(index);

            if (_autostartId == pInfo->autostartId) {
                Q_ASSERT(!autostartFound);
                autostartFound = true;
                _currentAirframeType = pType->name;
                _currentVehicleName = pInfo->name;
                _currentVehicleIndex = index;
            }
            airframeType->addAirframe(pInfo->name, pInfo->autostartId);
        }
        
        _airframeTypes.append(QVariant::fromValue(airframeType));
    }
    
    if (_autostartId != 0 && !autostartFound) {
        _showCustomConfigPanel = true;
        emit showCustomConfigPanelChanged(true);
    }
}

AirframeComponentController::~AirframeComponentController()
{

}

void AirframeComponentController::changeAutostart(void)
{
	if (UASManager::instance()->getUASList().count() > 1) {
		QGCMessageBox::warning("Airframe Config", "You cannot change airframe configuration while connected to multiple vehicles.");
		return;
	}
	
    qgcApp()->setOverrideCursor(Qt::WaitCursor);
    
    getParameterFact(-1, "SYS_AUTOSTART")->setValue(_autostartId);
    getParameterFact(-1, "SYS_AUTOCONFIG")->setValue(1);
    
    // FactSystem doesn't currently have a mechanism to wait for the parameters to come backf from the board.
    // So instead we wait for enough time for the parameters to hoepfully make it to the board.
    qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    QGC::SLEEP::sleep(3);
    qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    
    // Reboot board
    
    _uas->executeCommand(MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN, 1, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    QGC::SLEEP::sleep(1);
    qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    LinkManager::instance()->disconnectAll();
    
    qgcApp()->restoreOverrideCursor();
}

AirframeType::AirframeType(const QString& name, const QString& imageResource, QObject* parent) :
    QObject(parent),
    _name(name),
    _imageResource(imageResource)
{
    
}

AirframeType::~AirframeType()
{
    
}

void AirframeType::addAirframe(const QString& name, int autostartId)
{
    Airframe* airframe = new Airframe(name, autostartId);
    Q_CHECK_PTR(airframe);
    
    _airframes.append(QVariant::fromValue(airframe));
}

Airframe::Airframe(const QString& name, int autostartId, QObject* parent) :
    QObject(parent),
    _name(name),
    _autostartId(autostartId)
{
    
}

Airframe::~Airframe()
{
    
}
