/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirframeComponentController.h"
#include "AirframeComponentAirframes.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "LinkManager.h"
#include "Fact.h"
#include "Vehicle.h"
#include "ParameterManager.h"

#include <QtCore/QThread>
#include <QtCore/QVariant>
#include <QtQml/qqml.h>
#include <QtGui/QCursor>

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
    if (!_allParametersExists(ParameterManager::defaultComponentId, usedParams)) {
        return;
    }
    
    // Load up member variables
    
    bool autostartFound = false;
    _autostartId = getParameterFact(ParameterManager::defaultComponentId, "SYS_AUTOSTART")->rawValue().toInt();

    
    for (int tindex = 0; tindex < AirframeComponentAirframes::get().count(); tindex++) {

        const AirframeComponentAirframes::AirframeType_t* pType = AirframeComponentAirframes::get().values().at(tindex);

        AirframeType* airframeType = new AirframeType(pType->name, pType->imageResource, this);
        Q_CHECK_PTR(airframeType);

        for (int index = 0; index < pType->rgAirframeInfo.count(); index++) {
            const AirframeComponentAirframes::AirframeInfo_t* pInfo = pType->rgAirframeInfo.at(index);
            Q_CHECK_PTR(pInfo);

            if (_autostartId == pInfo->autostartId) {
                if (autostartFound) {
                    qWarning() << "AirframeComponentController::AirframeComponentController duplicate ids found:" << _autostartId;
                }
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
    if (MultiVehicleManager::instance()->vehicles()->count() > 1) {
        qgcApp()->showAppMessage(tr("You cannot change airframe configuration while connected to multiple vehicles."));
		return;
	}

    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    
    Fact* sysAutoStartFact  = getParameterFact(-1, "SYS_AUTOSTART");
    Fact* sysAutoConfigFact = getParameterFact(-1, "SYS_AUTOCONFIG");
    
    // We need to wait for the vehicleUpdated signals to come back before we reboot
    _waitParamWriteSignalCount = 0;
    connect(sysAutoStartFact, &Fact::vehicleUpdated, this, &AirframeComponentController::_waitParamWriteSignal);
    connect(sysAutoConfigFact, &Fact::vehicleUpdated, this, &AirframeComponentController::_waitParamWriteSignal);
    
    // We use forceSetValue to ensure params are sent even if the previous value is that same as the new value
    sysAutoStartFact->forceSetRawValue(_autostartId);
    sysAutoConfigFact->forceSetRawValue(1);
}

void AirframeComponentController::_waitParamWriteSignal(QVariant value)
{
    Q_UNUSED(value);
    
    _waitParamWriteSignalCount++;
    if (_waitParamWriteSignalCount == 2) {
        // Now that both params have made it to the vehicle we can reboot it. All these signals are flying
        // around on the main thread, so we need to allow the stack to unwind back to the event loop before
        // we reboot.
        QTimer::singleShot(800, this, &AirframeComponentController::_rebootAfterStackUnwind);
    }
}

void AirframeComponentController::_rebootAfterStackUnwind(void)
{    
    _vehicle->sendMavCommand(_vehicle->defaultComponentId(), MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN, true /* showError */, 1.0f);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    for (unsigned i = 0; i < 2000; i++) {
        QThread::usleep(500);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    QGuiApplication::restoreOverrideCursor();
    LinkManager::instance()->disconnectAll();
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
