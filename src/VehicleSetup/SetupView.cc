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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "SetupView.h"

#include "UASManager.h"
#include "AutoPilotPluginManager.h"
#include "VehicleComponent.h"
#include "QGCQmlWidgetHolder.h"
#include "MainWindow.h"
#include "QGCMessageBox.h"
#ifndef __mobile__
#include "FirmwareUpgradeController.h"
#endif
#include "ParameterEditorController.h"

#include <QQmlError>
#include <QQmlContext>
#include <QDebug>

SetupView::SetupView(QWidget* parent) :
    QGCQmlWidgetHolder(parent),
    _uasCurrent(NULL),
    _initComplete(false),
    _readyAutopilot(NULL)
{
#ifdef __mobile__
    _showFirmware = false;
#else
    _showFirmware = true;
#endif

    connect(UASManager::instance(), &UASManager::activeUASSet, this, &SetupView::_setActiveUAS);

    getRootContext()->setContextProperty("controller", this);
    setSource(QUrl::fromUserInput("qrc:/qml/SetupView.qml"));

    _setActiveUAS(UASManager::instance()->getActiveUAS());
}

SetupView::~SetupView()
{

}

void SetupView::_setActiveUAS(UASInterface* uas)
{
    if (_uasCurrent) {
        disconnect(_autopilot.data(), &AutoPilotPlugin::pluginReadyChanged, this, &SetupView::_pluginReadyChanged);
    }

    _pluginReadyChanged(false);

    _uasCurrent = uas;

    if (_uasCurrent) {
        _autopilot = AutoPilotPluginManager::instance()->getInstanceForAutoPilotPlugin(_uasCurrent);
        if (_autopilot.data()->pluginReady()) {
            _pluginReadyChanged(_autopilot.data()->pluginReady());
        }
        connect(_autopilot.data(), &AutoPilotPlugin::pluginReadyChanged, this, &SetupView::_pluginReadyChanged);
    }
}

void SetupView::_pluginReadyChanged(bool pluginReady)
{
    if (pluginReady) {
        _readyAutopilot = _autopilot.data();
        emit autopilotChanged(_readyAutopilot);
    } else {
        _readyAutopilot = NULL;
        emit autopilotChanged(NULL);
        _autopilot.clear();
    }
}

#ifdef UNITTEST_BUILD
void SetupView::showFirmware(void)
{
#ifndef __mobile__
    QVariant returnedValue;
    bool success = QMetaObject::invokeMethod(getRootObject(),
                                             "showFirmwarePanel",
                                             Q_RETURN_ARG(QVariant, returnedValue));
    Q_ASSERT(success);
#endif
}

void SetupView::showParameters(void)
{
    QVariant returnedValue;
    bool success = QMetaObject::invokeMethod(getRootObject(),
                                             "showParametersPanel",
                                             Q_RETURN_ARG(QVariant, returnedValue));
    Q_ASSERT(success);
}

void SetupView::showSummary(void)
{
    QVariant returnedValue;
    bool success = QMetaObject::invokeMethod(getRootObject(),
                                             "showSummaryPanel",
                                             Q_RETURN_ARG(QVariant, returnedValue));
    Q_ASSERT(success);
}

void SetupView::showVehicleComponentSetup(VehicleComponent* vehicleComponent)
{
    QVariant returnedValue;
    bool success = QMetaObject::invokeMethod(getRootObject(),
                                             "showVehicleComponentPanel",
                                             Q_RETURN_ARG(QVariant, returnedValue),
                                             Q_ARG(QVariant, QVariant::fromValue((VehicleComponent*)vehicleComponent)));
    Q_ASSERT(success);
}
#endif

AutoPilotPlugin* SetupView::autopilot(void)
{
    return _readyAutopilot;
}
