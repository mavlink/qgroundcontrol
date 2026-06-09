#include "MissionCommandTreeEditorTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>

#include "AppSettings.h"
#include "FirmwarePlugin.h"
#include "FirmwarePluginManager.h"
#include "PlanMasterController.h"
#include "QGCCorePlugin.h"
#include "SettingsManager.h"
#include "SimpleMissionItem.h"

void MissionCommandTreeEditorTest::_testEditorsWorker(QGCMAVLinkTypes::FirmwareClass_t firmwareClass,
                                                      QGCMAVLinkTypes::VehicleClass_t vehicleClass)
{
    QString firmwareClassString = QGCMAVLink::firmwareClassToString(firmwareClass).replace(" ", "");
    QString vehicleClassString = QGCMAVLink::vehicleClassToUserVisibleString(vehicleClass).replace(" ", "");
    AppSettings* appSettings = SettingsManager::instance()->appSettings();
    appSettings->offlineEditingFirmwareClass()->setRawValue(firmwareClass);
    appSettings->offlineEditingVehicleClass()->setRawValue(vehicleClass);
    PlanMasterController masterController;
    FirmwarePlugin* firmwarePlugin = FirmwarePluginManager::instance()->firmwarePluginForAutopilot(
        QGCMAVLink::firmwareClassToAutopilot(firmwareClass), QGCMAVLink::vehicleClassToMavType(vehicleClass));
    if (firmwarePlugin->supportedMissionCommands(vehicleClass).count() == 0) {
        firmwarePlugin = FirmwarePluginManager::instance()->firmwarePluginForAutopilot(
            QGCMAVLink::firmwareClassToAutopilot(QGCMAVLink::FirmwareClassPX4),
            QGCMAVLink::vehicleClassToMavType(vehicleClass));
    }
    int cColumns = firmwarePlugin->supportedMissionCommands(vehicleClass).count();
    QVariantList varSimpleItems;
    for (MAV_CMD command : firmwarePlugin->supportedMissionCommands(vehicleClass)) {
        SimpleMissionItem* simpleItem =
            new SimpleMissionItem(&masterController, false /* flyView */, false /* forLoad */);
        simpleItem->setCommand(command);
        varSimpleItems.append(QVariant::fromValue(simpleItem));
    }
    QQmlApplicationEngine* qmlAppEngine = QGCCorePlugin::instance()->createQmlApplicationEngine(this);
    qmlAppEngine->rootContext()->setContextProperty("planMasterController", &masterController);
    qmlAppEngine->rootContext()->setContextProperty("missionItems", varSimpleItems);
    qmlAppEngine->rootContext()->setContextProperty("cColumns", cColumns);
    qmlAppEngine->rootContext()->setContextProperty(
        "imagePath", QStringLiteral("%1/%2-%3.png")
                         .arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation), firmwareClassString,
                              vehicleClassString));
    qmlAppEngine->load(QUrl(QStringLiteral("qrc:/qml/MissionCommandTreeEditorTestWindow.qml")));
    QVERIFY_TRUE_WAIT(!qmlAppEngine->rootObjects().isEmpty(), TestTimeout::mediumMs());
    delete qmlAppEngine;
}

void MissionCommandTreeEditorTest::testEditors()
{
    // Qt font alias lookup warning is a platform-level timing artifact.
    ignoreLogMessage("qt.qpa.fonts", QtWarningMsg,
                     QRegularExpression("Populating font family aliases"));
    // RTL_CONE_SLOPE has an invalid enum value of 0 in ArduPilot metadata; skip warning is expected.
    ignoreLogMessage("FirmwarePlugin.ParameterMetaData", QtWarningMsg,
                     QRegularExpression("Skipping invalid enum value"));
    for (const QGCMAVLink::FirmwareClass_t& firmwareClass : QGCMAVLink::allFirmwareClasses()) {
        for (const QGCMAVLink::VehicleClass_t& vehicleClass : QGCMAVLink::allVehicleClasses()) {
            _testEditorsWorker(firmwareClass, vehicleClass);
        }
    }
}

UT_REGISTER_TEST(MissionCommandTreeEditorTest, TestLabel::Integration, TestLabel::MissionManager)
