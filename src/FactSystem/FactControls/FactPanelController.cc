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

#include "FactPanelController.h"
#include "MultiVehicleManager.h"
#include "UAS.h"
#include "QGCApplication.h"

#include <QQmlEngine>

/// @file
///     @author Don Gagne <don@thegagnes.com>

QGC_LOGGING_CATEGORY(FactPanelControllerLog, "FactPanelControllerLog")

FactPanelController::FactPanelController(void)
    : _vehicle(NULL)
    , _uas(NULL)
    , _autopilot(NULL)
    , _factPanel(NULL)
{
    _vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (_vehicle) {
        _uas = _vehicle->uas();
        _autopilot = _vehicle->autopilotPlugin();
    }

    // Do a delayed check for the _factPanel finally being set correctly from Qml
    QTimer::singleShot(1000, this, &FactPanelController::_checkForMissingFactPanel);
}

QQuickItem* FactPanelController::factPanel(void)
{
    return _factPanel;
}

void FactPanelController::setFactPanel(QQuickItem* panel)
{
    // Once we finally have the _factPanel member set, send any
    // missing fact notices that were waiting to go out

    _factPanel = panel;
    foreach (const QString &missingParam, _delayedMissingParams) {
        _notifyPanelMissingParameter(missingParam);
    }
    _delayedMissingParams.clear();
}

void FactPanelController::_notifyPanelMissingParameter(const QString& missingParam)
{
    if (_factPanel) {
        QVariant returnedValue;

        QMetaObject::invokeMethod(_factPanel,
                                  "showMissingParameterOverlay",
                                  Q_RETURN_ARG(QVariant, returnedValue),
                                  Q_ARG(QVariant, missingParam));
    }
}

void FactPanelController::_notifyPanelErrorMsg(const QString& errorMsg)
{
    if (_factPanel) {
        QVariant returnedValue;

        QMetaObject::invokeMethod(_factPanel,
                                  "showError",
                                  Q_RETURN_ARG(QVariant, returnedValue),
                                  Q_ARG(QVariant, errorMsg));
    }
}

void FactPanelController::_reportMissingParameter(int componentId, const QString& name)
{
    qgcApp()->reportMissingParameter(componentId, name);

    QString missingParam = QString("%1:%2").arg(componentId).arg(name);

    // If missing parameters a reported from the constructor of a derived class we
    // will not have access to _factPanel yet. Just record list of missing facts
    // in that case instead of notify. Once _factPanel is available they will be
    // send out for real.
    if (_factPanel) {
        _notifyPanelMissingParameter(missingParam);
    } else {
        _delayedMissingParams += missingParam;
    }
}

bool FactPanelController::_allParametersExists(int componentId, QStringList names)
{
    bool noMissingFacts = true;

    foreach (const QString &name, names) {
        if (_autopilot && !_autopilot->parameterExists(componentId, name)) {
            _reportMissingParameter(componentId, name);
            noMissingFacts = false;
        }
    }

    return noMissingFacts;
}

void FactPanelController::_checkForMissingFactPanel(void)
{
    if (!_factPanel) {
        _showInternalError("Incorrect FactPanel Qml implementation. FactPanelController used without passing in factPanel.");
    }
}

Fact* FactPanelController::getParameterFact(int componentId, const QString& name)
{
    if (_autopilot && _autopilot->parameterExists(componentId, name)) {
        Fact* fact = _autopilot->getParameterFact(componentId, name);
        QQmlEngine::setObjectOwnership(fact, QQmlEngine::CppOwnership);
        return fact;
    } else {
        _reportMissingParameter(componentId, name);
        return NULL;
    }
}

bool FactPanelController::parameterExists(int componentId, const QString& name)
{
    return _autopilot ? _autopilot->parameterExists(componentId, name) : false;
}

void FactPanelController::_showInternalError(const QString& errorMsg)
{
    _notifyPanelErrorMsg(QString("Internal Error: %1").arg(errorMsg));
    qCWarning(FactPanelControllerLog) << "Internal Error" << errorMsg;
    qgcApp()->showMessage(errorMsg);
}
