/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FactPanelController.h"
#include "AutoPilotPlugin.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtQml/QQmlEngine>

QGC_LOGGING_CATEGORY(FactPanelControllerLog, "qgc.factsystem.factcontrols.factpanelcontroller")

FactPanelController::FactPanelController(QObject *parent)
    : QObject(parent)
    , _vehicle(MultiVehicleManager::instance()->activeVehicle())
{
    // qCDebug(FactPanelControllerLog) << Q_FUNC_INFO << this;

    if (_vehicle) {
        _autopilot = _vehicle->autopilotPlugin();
    } else {
        _vehicle = MultiVehicleManager::instance()->offlineEditingVehicle();
    }

    _missingParametersTimer.setInterval(500);
    _missingParametersTimer.setSingleShot(true);
    (void) connect(&_missingParametersTimer, &QTimer::timeout, this, &FactPanelController::_checkForMissingParameters);
}

FactPanelController::~FactPanelController()
{
    // qCDebug(FactPanelControllerLog) << Q_FUNC_INFO << this;
}

void FactPanelController::_reportMissingParameter(int componentId, const QString &name) const
{
    if (componentId == ParameterManager::defaultComponentId) {
        componentId = _vehicle->defaultComponentId();
    }

    qgcApp()->reportMissingParameter(componentId, name);
    qCWarning(FactPanelControllerLog) << "Missing parameter:" << QStringLiteral("%1:%2").arg(componentId).arg(name);
}

bool FactPanelController::_allParametersExists(int componentId, const QStringList &names) const
{
    bool noMissingFacts = true;

    for (const QString &name : names) {
        if (_vehicle && !_vehicle->parameterManager()->parameterExists(componentId, name)) {
            _reportMissingParameter(componentId, name);
            noMissingFacts = false;
        }
    }

    return noMissingFacts;
}

Fact *FactPanelController::getParameterFact(int componentId, const QString &name, bool reportMissing) const
{
    if (_vehicle && _vehicle->parameterManager()->parameterExists(componentId, name)) {
        Fact *const fact = _vehicle->parameterManager()->getParameter(componentId, name);
        QQmlEngine::setObjectOwnership(fact, QQmlEngine::CppOwnership);
        return fact;
    }

    if (reportMissing) {
        _reportMissingParameter(componentId, name);
    }

    return nullptr;
}

bool FactPanelController::parameterExists(int componentId, const QString &name) const
{
    return (_vehicle ? _vehicle->parameterManager()->parameterExists(componentId, name) : false);
}

void FactPanelController::getMissingParameters(const QStringList &rgNames)
{
    for (const QString &name: rgNames) {
        _missingParameterWaitList.append(name);
        _vehicle->parameterManager()->refreshParameter(MAV_COMP_ID_AUTOPILOT1, name);
    }

    _missingParametersTimer.start();
}

void FactPanelController::_checkForMissingParameters()
{
    const QStringList waitList = _missingParameterWaitList;
    for (const QString &name: waitList) {
        if (_vehicle->parameterManager()->parameterExists(MAV_COMP_ID_AUTOPILOT1, name)) {
            (void) _missingParameterWaitList.removeOne(name);
        }
    }

    if (_missingParameterWaitList.isEmpty()) {
        emit missingParametersAvailable();
    } else {
        _missingParametersTimer.start();
    }
}
