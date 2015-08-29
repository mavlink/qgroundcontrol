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
    QGCQmlWidgetHolder(parent)
{
    setSource(QUrl::fromUserInput("qrc:/qml/SetupView.qml"));
}

SetupView::~SetupView()
{

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
    Q_UNUSED(success);
#endif
}

void SetupView::showParameters(void)
{
    QVariant returnedValue;
    bool success = QMetaObject::invokeMethod(getRootObject(),
                                             "showParametersPanel",
                                             Q_RETURN_ARG(QVariant, returnedValue));
    Q_ASSERT(success);
    Q_UNUSED(success);
}

void SetupView::showSummary(void)
{
    QVariant returnedValue;
    bool success = QMetaObject::invokeMethod(getRootObject(),
                                             "showSummaryPanel",
                                             Q_RETURN_ARG(QVariant, returnedValue));
    Q_ASSERT(success);
    Q_UNUSED(success);
}

void SetupView::showVehicleComponentSetup(VehicleComponent* vehicleComponent)
{
    QVariant returnedValue;
    bool success = QMetaObject::invokeMethod(getRootObject(),
                                             "showVehicleComponentPanel",
                                             Q_RETURN_ARG(QVariant, returnedValue),
                                             Q_ARG(QVariant, QVariant::fromValue((VehicleComponent*)vehicleComponent)));
    Q_ASSERT(success);
    Q_UNUSED(success);
}
#endif
