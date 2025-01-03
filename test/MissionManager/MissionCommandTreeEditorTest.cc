/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MissionCommandTreeEditorTest.h"
#include "QGCCorePlugin.h"
#include "FirmwarePlugin.h"
#include "FirmwarePluginManager.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "SimpleMissionItem.h"
#include "PlanMasterController.h"

#include <QtTest/QTest>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>

MissionCommandTreeEditorTest::MissionCommandTreeEditorTest(void)
{
    
}

void MissionCommandTreeEditorTest::_testEditorsWorker(QGCMAVLink::FirmwareClass_t firmwareClass, QGCMAVLink::VehicleClass_t vehicleClass)
{
    QString firmwareClassString = QGCMAVLink::firmwareClassToString(firmwareClass).replace(" ", "");
    QString vehicleClassString  = QGCMAVLink::vehicleClassToUserVisibleString(vehicleClass).replace(" ", "");

    AppSettings* appSettings = SettingsManager::instance()->appSettings();
    appSettings->offlineEditingFirmwareClass()->setRawValue(firmwareClass);
    appSettings->offlineEditingVehicleClass()->setRawValue(vehicleClass);
    PlanMasterController masterController;

    FirmwarePlugin* firmwarePlugin = FirmwarePluginManager::instance()->firmwarePluginForAutopilot(QGCMAVLink::firmwareClassToAutopilot(firmwareClass), QGCMAVLink::vehicleClassToMavType(vehicleClass));
    if (firmwarePlugin->supportedMissionCommands(vehicleClass).count() == 0) {
        firmwarePlugin = FirmwarePluginManager::instance()->firmwarePluginForAutopilot(QGCMAVLink::firmwareClassToAutopilot(QGCMAVLink::FirmwareClassPX4), QGCMAVLink::vehicleClassToMavType(vehicleClass));
    }
    int cColumns = firmwarePlugin->supportedMissionCommands(vehicleClass).count();

    QVariantList varSimpleItems;
    for (MAV_CMD command: firmwarePlugin->supportedMissionCommands(vehicleClass)) {
        SimpleMissionItem* simpleItem = new SimpleMissionItem(&masterController, false /* flyView */, false /* forLoad */);
        simpleItem->setCommand(command);
        varSimpleItems.append(QVariant::fromValue(simpleItem));
    }

    QQmlApplicationEngine* qmlAppEngine = QGCCorePlugin::instance()->createQmlApplicationEngine(this);
    qmlAppEngine->rootContext()->setContextProperty("planMasterController", &masterController);
    qmlAppEngine->rootContext()->setContextProperty("missionItems", varSimpleItems);
    qmlAppEngine->rootContext()->setContextProperty("cColumns", cColumns);
    qmlAppEngine->rootContext()->setContextProperty("imagePath", QStringLiteral("/home/parallels/Downloads/%1-%2.png").arg(firmwareClassString).arg(vehicleClassString));
    qmlAppEngine->load(QUrl(QStringLiteral("qrc:/qml/MissionCommandTreeEditorTestWindow.qml")));

    QTest::qWait(1000);

    delete qmlAppEngine;
}

void MissionCommandTreeEditorTest::testEditors(void)
{
    for (const QGCMAVLink::FirmwareClass_t& firmwareClass: QGCMAVLink::allFirmwareClasses()) {
        for (const QGCMAVLink::VehicleClass_t& vehicleClass: QGCMAVLink::allVehicleClasses()) {
            _testEditorsWorker(firmwareClass, vehicleClass);
        }
    }
}

