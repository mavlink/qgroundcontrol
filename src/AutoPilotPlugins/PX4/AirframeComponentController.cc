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

AirframeComponentController::AirframeComponentController(QObject* parent) :
    QObject(parent),
    _uas(NULL),
    _autoPilotPlugin(NULL)
{
    _uas = UASManager::instance()->getActiveUAS();
    Q_ASSERT(_uas);
    
    _autoPilotPlugin = AutoPilotPluginManager::instance()->getInstanceForAutoPilotPlugin(_uas);
    Q_ASSERT(_autoPilotPlugin);
    Q_ASSERT(_autoPilotPlugin->pluginReady());

    if (!_typesRegistered) {
        _typesRegistered = true;
        qmlRegisterUncreatableType<AirframeType>("QGroundControl.Controllers", 1, 0, "AiframeType", "Can only reference AirframeType");
        qmlRegisterUncreatableType<Airframe>("QGroundControl.Controllers", 1, 0, "Aiframe", "Can only reference Airframe");
    }
    
    // Load up member variables
    
    bool autostartFound = false;
    _autostartId = _autoPilotPlugin->getParameterFact("SYS_AUTOSTART")->value().toInt();
    
    for (const AirframeComponentAirframes::AirframeType_t* pType=&AirframeComponentAirframes::rgAirframeTypes[0]; pType->name != NULL; pType++) {
        AirframeType* airframeType = new AirframeType(pType->name, pType->imageResource, this);
        Q_CHECK_PTR(airframeType);
        
        int index = 0;
        for (const AirframeComponentAirframes::AirframeInfo_t* pInfo=&pType->rgAirframeInfo[0]; pInfo->name != NULL; pInfo++) {
            if (_autostartId == pInfo->autostartId) {
                Q_ASSERT(!autostartFound);
                autostartFound = true;
                _currentAirframeType = pType->name;
                _currentVehicleName = pInfo->name;
                _currentVehicleIndex = index;
            }
            airframeType->addAirframe(pInfo->name, pInfo->autostartId);
            index++;
        }
        
        _airframeTypes.append(QVariant::fromValue(airframeType));
    }
    
    // FIXME: Should be a user error
    Q_UNUSED(autostartFound);
    Q_ASSERT(autostartFound);
}

AirframeComponentController::~AirframeComponentController()
{

}

void AirframeComponentController::changeAutostart(void)
{
    LinkManager* linkManager = LinkManager::instance();
    
    if (linkManager->getLinks().count() > 1) {
        QGCMessageBox::warning("Airframe Config", "You cannot change airframe configuration while connected to multiple vehicles.");
        return;
    }
    
    _autoPilotPlugin->getParameterFact("SYS_AUTOSTART")->setValue(_autostartId);
    _autoPilotPlugin->getParameterFact("SYS_AUTOCONFIG")->setValue(1);
    
    // Wait for the parameters to come back to us
    
    qgcApp()->setOverrideCursor(Qt::WaitCursor);
    
    int waitSeconds = 10;
    bool success = false;
    
    QGCUASParamManagerInterface* paramMgr = _uas->getParamManager();

    while (true) {
        if (paramMgr->countPendingParams() == 0) {
            success = true;
            break;
        }
        qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
        QGC::SLEEP::sleep(1);
        if (--waitSeconds == 0) {
            break;
        }
    }
    
    
    if (!success) {
        qgcApp()->restoreOverrideCursor();
        QGCMessageBox::critical("Airframe Config", "Airframe Config parameters not received back from vehicle. Config has not been set.");
        return;
    }
    
    _uas->executeCommand(MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN, 1, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    QGC::SLEEP::sleep(1);
    qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    
    qgcApp()->reconnectAfterWait(5);
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

