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

#include "APMAirframeComponentController.h"
#include "APMAirframeComponentAirframes.h"
#include "APMRemoteParamsDownloader.h"
#include "QGCMAVLink.h"
#include "MultiVehicleManager.h"
#include "AutoPilotPluginManager.h"
#include "QGCApplication.h"
#include "QGCMessageBox.h"

#include <QVariant>
#include <QQmlProperty>

bool APMAirframeComponentController::_typesRegistered = false;

APMAirframeComponentController::APMAirframeComponentController(void) :
    _currentVehicleIndex(0),
    _autostartId(0),
    _showCustomConfigPanel(false),
    _airframeTypesModel(new APMAirframeModel(this))
{
    if (!_typesRegistered) {
        _typesRegistered = true;
        qmlRegisterUncreatableType<APMAirframeType>("QGroundControl.Controllers", 1, 0, "APMAiframeType", "Can only reference APMAirframeType");
        qmlRegisterUncreatableType<APMAirframe>("QGroundControl.Controllers", 1, 0, "APMAiframe", "Can only reference APMAirframe");
    }
    
    QStringList usedParams;
    //usedParams << "SYS_AUTOSTART" << "SYS_AUTOCONFIG";
    if (!_allParametersExists(FactSystem::defaultComponentId, usedParams)) {
        return;
    }
    
    APMRemoteParamsDownloader *paramDownloader = new APMRemoteParamsDownloader();
    connect(paramDownloader, SIGNAL(finished()), this, SLOT(_fillAirFrames()));
}

APMAirframeComponentController::~APMAirframeComponentController()
{

}

void APMAirframeComponentController::_fillAirFrames()
{
    // Load up member variables
    bool autostartFound = false;
    _autostartId = 0; //getParameterFact(FactSystem::defaultComponentId, "SYS_AUTOSTART")->value().toInt();
    QList<APMAirframeType*> airframeTypes;
    for (int tindex = 0; tindex < APMAirframeComponentAirframes::get().count(); tindex++) {
        const APMAirframeComponentAirframes::AirframeType_t* pType = APMAirframeComponentAirframes::get().values().at(tindex);
        APMAirframeType* airframeType = new APMAirframeType(pType->name, pType->imageResource, this);
        Q_CHECK_PTR(airframeType);

        for (int index = 0; index < pType->rgAirframeInfo.count(); index++) {
            const APMAirframeComponentAirframes::AirframeInfo_t* pInfo = pType->rgAirframeInfo.at(index);
            Q_CHECK_PTR(pInfo);

            if (_autostartId == pInfo->autostartId) {
                Q_ASSERT(!autostartFound);
                autostartFound = true;
                _currentAirframeType = pType->name;
                _currentVehicleName = pInfo->name;
                _currentVehicleIndex = index;
            }
            airframeType->addAirframe(pInfo->name, pInfo->autostartId);
        }
        airframeTypes.append(airframeType);
    }

    _airframeTypesModel->setAirframeTypes(airframeTypes);

    if (_autostartId != 0 && !autostartFound) {
        _showCustomConfigPanel = true;
        emit showCustomConfigPanelChanged(true);
    }
    emit loadAirframesCompleted();
}

void APMAirframeComponentController::changeAutostart(void)
{
	if (qgcApp()->toolbox()->multiVehicleManager()->vehicles().count() > 1) {
        QGCMessageBox::warning("APMAirframe Config", "You cannot change APMAirframe configuration while connected to multiple vehicles.");
		return;
	}
	
    qgcApp()->setOverrideCursor(Qt::WaitCursor);

    /*
    Fact* sysAutoStartFact  = getParameterFact(-1, "SYS_AUTOSTART");
    Fact* sysAutoConfigFact = getParameterFact(-1, "SYS_AUTOCONFIG");
    
    // We need to wait for the vehicleUpdated signals to come back before we reboot
    _waitParamWriteSignalCount = 0;
    connect(sysAutoStartFact, &Fact::vehicleUpdated, this, &APMAirframeComponentController::_waitParamWriteSignal);
    connect(sysAutoConfigFact, &Fact::vehicleUpdated, this, &APMAirframeComponentController::_waitParamWriteSignal);
    
    // We use forceSetValue to params are sent even if the previous value is that same as the new value
    sysAutoStartFact->forceSetValue(_autostartId);
    sysAutoConfigFact->forceSetValue(1);
    */

    qDebug() << "Button Clicked!";
}

void APMAirframeComponentController::_waitParamWriteSignal(QVariant value)
{
    Q_UNUSED(value);
    
    _waitParamWriteSignalCount++;
    if (_waitParamWriteSignalCount == 2) {
        // Now that both params have made it to the vehicle we can reboot it. All these signals are flying
        // around on the main thread, so we need to allow the stack to unwind back to the event loop before
        // we reboot.
        QTimer::singleShot(800, this, &APMAirframeComponentController::_rebootAfterStackUnwind);
    }
}

void APMAirframeComponentController::_rebootAfterStackUnwind(void)
{    
    _uas->executeCommand(MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN, 1, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    for (unsigned i = 0; i < 2000; i++) {
        QGC::SLEEP::usleep(500);
        qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    qgcApp()->toolbox()->linkManager()->disconnectAll();
    qgcApp()->restoreOverrideCursor();
}

APMAirframeType::APMAirframeType(const QString& name, const QString& imageResource, QObject* parent) :
    QObject(parent),
    _name(name),
    _imageResource(imageResource)
{
}

APMAirframeType::~APMAirframeType()
{
}

void APMAirframeType::addAirframe(const QString& name, int autostartId)
{
    APMAirframe* airframe = new APMAirframe(name, autostartId);
    Q_CHECK_PTR(airframe);
    
    _airframes.append(QVariant::fromValue(airframe));
}

APMAirframe::APMAirframe(const QString& name, int autostartId, QObject* parent) :
    QObject(parent),
    _name(name),
    _autostartId(autostartId)
{
}

APMAirframe::~APMAirframe()
{
}

APMAirframeModel::APMAirframeModel(QObject *parent) : QAbstractListModel(parent)
{
}

QVariant APMAirframeModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    APMAirframeType *airframeType = _airframeTypes.at(index.row());
    switch(role) {
    case NAME: return airframeType->name();
    case IMAGE: return airframeType->imageResource();
    }
    return QVariant();
}

int APMAirframeModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return _airframeTypes.count();
}

void APMAirframeModel::setAirframeTypes(const QList<APMAirframeType*>& airframeTypes)
{
    beginResetModel();
    _airframeTypes = airframeTypes;
    endResetModel();
}

QString APMAirframeType::imageResource() const
{
    return _imageResource;
}

QString APMAirframeType::name() const
{
    return _name;
}

