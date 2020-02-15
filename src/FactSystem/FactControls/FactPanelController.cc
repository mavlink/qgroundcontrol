/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FactPanelController.h"
#include "MultiVehicleManager.h"
#include "UAS.h"
#include "QGCApplication.h"
#include "ParameterManager.h"

#include <QQmlEngine>

/// @file
///     @author Don Gagne <don@thegagnes.com>

QGC_LOGGING_CATEGORY(FactPanelControllerLog, "FactPanelControllerLog")

FactPanelController::FactPanelController()
{
    _vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (_vehicle) {
        _uas = _vehicle->uas();
        _autopilot = _vehicle->autopilotPlugin();
    } else {
        _vehicle = qgcApp()->toolbox()->multiVehicleManager()->offlineEditingVehicle();
    }

    _missingParametersTimer.setInterval(500);
    _missingParametersTimer.setSingleShot(true);
    connect(&_missingParametersTimer, &QTimer::timeout, this, &FactPanelController::_checkForMissingParameters);
}

void FactPanelController::_notifyPanelMissingParameter(const QString& missingParam)
{
    if (qgcApp()->mainRootWindow()) {
        QVariant returnedValue;
        QMetaObject::invokeMethod(
            qgcApp()->mainRootWindow(),
            "showMissingParameterOverlay",
            Q_RETURN_ARG(QVariant, returnedValue),
            Q_ARG(QVariant, missingParam));
    }
}

void FactPanelController::_notifyPanelErrorMsg(const QString& errorMsg)
{
    if(qgcApp()->mainRootWindow()) {
        QVariant returnedValue;
        QMetaObject::invokeMethod(
            qgcApp()->mainRootWindow(),
            "showFactError",
            Q_RETURN_ARG(QVariant, returnedValue),
            Q_ARG(QVariant, errorMsg));
    }
}

void FactPanelController::_reportMissingParameter(int componentId, const QString& name)
{
    if (componentId == FactSystem::defaultComponentId) {
        componentId = _vehicle->defaultComponentId();
    }

    qgcApp()->reportMissingParameter(componentId, name);

    QString missingParam = QString("%1:%2").arg(componentId).arg(name);

    qCWarning(FactPanelControllerLog) << "Missing parameter:" << missingParam;

    // If missing parameters a reported from the constructor of a derived class we
    // will not have access to _factPanel yet. Just record list of missing facts
    // in that case instead of notify. Once _factPanel is available they will be
    // send out for real.
    if (qgcApp()->mainRootWindow()) {
        _notifyPanelMissingParameter(missingParam);
    } else {
        _delayedMissingParams += missingParam;
    }
}

bool FactPanelController::_allParametersExists(int componentId, QStringList names)
{
    bool noMissingFacts = true;

    foreach (const QString &name, names) {
        if (_vehicle && !_vehicle->parameterManager()->parameterExists(componentId, name)) {
            _reportMissingParameter(componentId, name);
            noMissingFacts = false;
        }
    }

    return noMissingFacts;
}


Fact* FactPanelController::getParameterFact(int componentId, const QString& name, bool reportMissing)
{
    if (_vehicle && _vehicle->parameterManager()->parameterExists(componentId, name)) {
        Fact* fact = _vehicle->parameterManager()->getParameter(componentId, name);
        QQmlEngine::setObjectOwnership(fact, QQmlEngine::CppOwnership);
        return fact;
    } else {
        if (reportMissing) {
            _reportMissingParameter(componentId, name);
        }
        return nullptr;
    }
}

bool FactPanelController::parameterExists(int componentId, const QString& name)
{
    return _vehicle ? _vehicle->parameterManager()->parameterExists(componentId, name) : false;
}

void FactPanelController::_showInternalError(const QString& errorMsg)
{
    _notifyPanelErrorMsg(tr("Internal Error: %1").arg(errorMsg));
    qCWarning(FactPanelControllerLog) << "Internal Error" << errorMsg;
    qgcApp()->showMessage(errorMsg);
}

void FactPanelController::getMissingParameters(QStringList rgNames)
{
    for (const QString& name: rgNames) {
        _missingParameterWaitList.append(name);
        _vehicle->parameterManager()->refreshParameter(MAV_COMP_ID_AUTOPILOT1, name);
    }

    _missingParametersTimer.start();
}

void FactPanelController::_checkForMissingParameters(void)
{
    QStringList waitList = _missingParameterWaitList;
    for (const QString& name: waitList) {
        if (_vehicle->parameterManager()->parameterExists(MAV_COMP_ID_AUTOPILOT1, name)) {
            _missingParameterWaitList.removeOne(name);
        }
    }

    if (_missingParameterWaitList.isEmpty()) {
        emit missingParametersAvailable();
    } else {
        _missingParametersTimer.start();
    }
}
