#include "GPSReceiverSettingsTest.h"

#include <memory>
#include <optional>

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>
#include <QtPositioning/QGeoCoordinate>
#include <QtQuick/QQuickItem>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSManager.h"
#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "GPSSettingsBindings.h"
#include "Qml/Support/GPSPanelFixtures.h"
#include "RTKSettings.h"
#include "Receiver/Support/GPSReceiverTestSupport.h"
#include "Receiver/Support/ScriptedReceiverWorker.h"
#include "SettingsManager.h"
#include "Support/GPSQmlTestHelpers.h"
#include "UnitsSettings.h"

using namespace GPSTest;

namespace {
/// A receiver session that follows the settings; scripted workers stand in for the receiver hardware.
struct SettingsReceiver
{
    explicit SettingsReceiver(RTKSettings* settings) { GPSSettingsBindings::bindReceiver(settings, &gps); }

    ScriptedReceiverWorker* worker() const { return workers.current(); }

    // Scripted workers open no link, so a recorded session names a file here and never creates it.
    static QString recordingDirectory() { return QStringLiteral("/test/recordings"); }

#ifndef QGC_NO_SERIAL_LINK
    // Every device the tests select; the panel's own port list decides which ones it shows as available.
    TestSerialPorts serial{{serialPort(QStringLiteral("/test/receiver")), serialPort(QStringLiteral("/test/other")),
                            serialPort(QStringLiteral("/test/missing"))}};
    GPSTest::ScriptedGPSReceiver scripted{nullptr,
                                          {.serialPorts = &serial.manager, .recordingDirectory = recordingDirectory}};
#else
    GPSTest::ScriptedGPSReceiver scripted{nullptr, {.recordingDirectory = recordingDirectory}};
#endif
    GPSReceiver& gps = scripted.receiver;
    ScriptedReceiverWorkerFactory& workers = scripted.workers;
};

/// A serial device picker entry, labelled with its path unless @a label is given.
QVariant portEntries(const QString& path, const QString& label = {})
{
    return QVariant::fromValue(QList<GPSSerialPortEntry>{{.value = path, .label = label.isEmpty() ? path : label}});
}

/// Serial device picker entries labelled with their paths.
QVariant portEntries(const QStringList& paths)
{
    QList<GPSSerialPortEntry> entries;
    for (const QString& path : paths) {
        entries.append({.value = path, .label = path});
    }
    return QVariant::fromValue(entries);
}

/// The receiver settings panel over saved settings for a manufacturer, and a scripted receiver that follows them.
struct Panel
{
    explicit Panel(int manufacturer)
        : settings(manufacturer)
        , receiver(settings.settings)
    {}

    /// Creates the panel; false on failure, with the reason in the engine's lastError().
    [[nodiscard]] bool create()
    {
        item = engine.create(GPSTest::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverSettings.qml")),
                             {{QStringLiteral("receiver"), QVariant::fromValue(&receiver.gps)},
                              {QStringLiteral("settings"), QVariant::fromValue(settings.settings)},
                              {QStringLiteral("autoConnectFact"), QVariant::fromValue(settings.autoConnect)},
                              {QStringLiteral("serialPorts"), portEntries(QStringLiteral("/test/receiver"))},
                              {QStringLiteral("serialBaudRates"), QVariant::fromValue(QList<int>{115200, 230400})}});
        return item != nullptr;
    }

    template <typename T = QQuickItem>
    T* find(const char* name) const
    {
        return item->findChild<T*>(QString::fromLatin1(name));
    }

    RTKSettingsFixture settings;
    SettingsReceiver receiver;
    GPSTest::QmlEngine engine;
    std::unique_ptr<QObject> item;
};

QStringList siteNames(const RTKSettings* settings)
{
    QStringList names;
    for (const RTKBaseSite& site : settings->baseSites()) {
        names.append(site.name);
    }
    return names;
}

/// Saves sites until the list is full; false when one is refused.
[[nodiscard]] bool fillBaseSites(RTKSettings* settings)
{
    for (qsizetype i = settings->baseSites().size(); i < RTKSettings::MaxBaseSites; ++i) {
        if (!settings->saveBaseSite({.name = QStringLiteral("Site %1").arg(i), .latitude = 1., .longitude = 2.})) {
            return false;
        }
    }
    return true;
}
}  // namespace

void GPSReceiverSettingsTest::_surveySaveWorkflow()
{
    // How each family maps the saved base settings is GPSReceiverSettingsBindingTest's; this is the panel's workflow.
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* connect = panel.find("rtkConnectButton");
    auto* fixed = panel.find("rtkFixedMode");
    auto* manufacturerControl = panel.find("rtkManufacturer");
    QVERIFY(connect && fixed && manufacturerControl);
    ScriptedReceiverWorkerFactory& workers = panel.receiver.workers;
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QVERIFY(panel.receiver.gps.hasReceiver());
    QCOMPARE(workers.count(), 1);
    QVERIFY(!fixed->isEnabled());
    QVERIFY(!manufacturerControl->isEnabled());

    // The application's receiver completes a survey; GPSManager keeps its position for a fixed-position start.
    RTKSettings* const settings = panel.settings.settings;
    AppReceiver base;
    QVERIFY(base.connect());
    base.survey(0.75);
    QVERIFY(GPSManager::instance()->saveCurrentBasePosition());
    QCOMPARE(settings->fixedBasePositionLatitude()->rawValue().toDouble(), kSurveyedPosition.latitudeDegrees);
    QCOMPARE(settings->fixedBasePositionLongitude()->rawValue().toDouble(), kSurveyedPosition.longitudeDegrees);
    QCOMPARE(settings->fixedBasePositionAltitude()->rawValue().toFloat(), kSurveyedPosition.altitudeMeters);
    QCOMPARE(settings->fixedBasePositionAccuracy()->rawValue().toDouble(), 0.75);
    QCOMPARE(settings->baseMode()->rawValue().toInt(), 0);

    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QVERIFY(!panel.receiver.gps.hasReceiver());
    workers.finishRetired();
    QVERIFY(fixed->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(fixed, "click"));
    QCOMPARE(settings->baseMode()->rawValue().toInt(), 1);
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QCOMPARE(workers.count(), 2);
    // The saved position starts the fixed base.
    const GPSBaseStationConfig::Mode savedPosition =
        GPSBaseStationConfig::Fixed{.position = kSurveyedPosition, .accuracyMeters = 0.75f};
    QVERIFY(workers.current()->capturedConfig().base.mode == savedPosition);
}

void GPSReceiverSettingsTest::_surveyCompletePrompt()
{
    RTKSettingsFixture settings(gpsReceiverManufacturerForType(GPSType::UBlox));
    AppReceiver base;
    QVERIFY(base.connect());
    QCOMPARE(GPSManager::instance()->receiver()->activeBaseMode(), int(BaseModeDefinition::Mode::BaseSurveyIn));
    GPSTest::QmlEngine engine;
    // The status shows the application's receiver, whose position GPSManager saves.
    auto status = engine.create(GPSTest::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverStatus.qml")));
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* prompt = status->findChild<QObject*>(QStringLiteral("rtkSurveyCompletePrompt"));
    auto* save = status->findChild<QObject*>(QStringLiteral("rtkSurveySaveButton"));
    QVERIFY(prompt && save);
    // Still surveying: nothing to keep yet.
    base.survey(0.75, true);
    QVERIFY(!prompt->property("visible").toBool());

    // A position without a known accuracy cannot start a fixed base.
    base.survey(std::nullopt);
    QVERIFY(!prompt->property("visible").toBool());
    QVERIFY(!GPSManager::instance()->saveCurrentBasePosition());
    QCOMPARE(settings.settings->fixedBasePositionLatitude()->rawValue().toDouble(), 0.0);
    QCOMPARE(settings.settings->fixedBasePositionAccuracy()->rawValue().toDouble(), 0.0);

    base.survey(0.75);
    QVERIFY(prompt->property("visible").toBool());
    QVERIFY(save->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(save, "clicked"));
    QCOMPARE(settings.settings->fixedBasePositionLatitude()->rawValue().toDouble(), kSurveyedPosition.latitudeDegrees);
    QCOMPARE(settings.settings->fixedBasePositionAccuracy()->rawValue().toDouble(), 0.75);
    QVERIFY(!save->property("visible").toBool());
    QVERIFY(prompt->property("text").toString().contains(QStringLiteral("saved")));
    // Without a site name only the fixed base position is kept.
    QVERIFY(settings.settings->baseSites().isEmpty());

    // Another view of the receiver knows the position is saved; a new survey offers saving its result again.
    auto other = engine.create(GPSTest::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverStatus.qml")));
    QVERIFY2(other, qPrintable(engine.lastError()));
    auto* otherSave = other->findChild<QObject*>(QStringLiteral("rtkSurveySaveButton"));
    QVERIFY(otherSave);
    QVERIFY(!otherSave->property("visible").toBool());
    base.survey(0.75, true);
    base.survey(0.75);
    QVERIFY(save->property("visible").toBool());
    QVERIFY(otherSave->property("visible").toBool());
}

void GPSReceiverSettingsTest::_surveySaveNeedsApplicationReceiver()
{
    RTKSettingsFixture settings(gpsReceiverManufacturerForType(GPSType::UBlox));
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    receiver.facts()->setLiveUpdates(true);
    QVERIFY(connectOverTcp(receiver));
    harness.workers.current()->ready();
    GPSTest::QmlEngine engine;
    auto status = engine.create(GPSTest::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverStatus.qml")),
                                {{QStringLiteral("receiver"), QVariant::fromValue(&receiver)},
                                 {QStringLiteral("facts"), QVariant::fromValue(receiver.facts())}});
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* prompt = status->findChild<QObject*>(QStringLiteral("rtkSurveyCompletePrompt"));
    auto* save = status->findChild<QObject*>(QStringLiteral("rtkSurveySaveButton"));
    auto* accuracy = status->findChild<QObject*>(QStringLiteral("rtkSurveyAccuracy"));
    QVERIFY(prompt && save && accuracy);

    // GPSManager saves the position of the application's receiver, so a panel that shows another receiver's finished
    // survey offers no save.
    harness.workers.current()->survey(
        {.position = kSurveyedPosition, .meanAccuracyMeters = 0.75, .valid = true, .active = false});
    QVERIFY(receiver.facts()->canSaveCurrentBasePosition());
    QVERIFY(accuracy->property("visible").toBool());
    QVERIFY(!prompt->property("visible").toBool());
    QVERIFY(!save->property("visible").toBool());
}

void GPSReceiverSettingsTest::_surveySavedAsSite()
{
    RTKSettingsFixture settings(gpsReceiverManufacturerForType(GPSType::UBlox));
    RTKSettings* const rtk = settings.settings;
    settings.saved.setFactValue(rtk->savedBaseSites(), QString());
    AppReceiver base;
    QVERIFY(base.connect());
    GPSTest::QmlEngine engine;
    auto status = engine.create(GPSTest::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverStatus.qml")));
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* name = status->findChild<QObject*>(QStringLiteral("rtkSurveySiteName"));
    auto* save = status->findChild<QObject*>(QStringLiteral("rtkSurveySaveButton"));
    auto* failed = status->findChild<QObject*>(QStringLiteral("rtkSurveySaveFailed"));
    QVERIFY(name && save && failed);
    QCOMPARE(name->property("maximumLength").toInt(), static_cast<int>(RTKSettings::MaxBaseSiteNameLength));
    QVERIFY(!name->property("visible").toBool());
    base.survey(0.75);
    QVERIFY(name->property("visible").toBool());
    QVERIFY(!failed->property("visible").toBool());

    // A full list cannot keep another site: nothing is saved, and the panel says why.
    QVERIFY(fillBaseSites(rtk));
    const QString site = QStringLiteral("North field");
    QVERIFY(name->setProperty("text", QStringLiteral(" North field ")));
    QVERIFY(QMetaObject::invokeMethod(save, "clicked"));
    QVERIFY(failed->property("visible").toBool());
    QVERIFY(save->property("visible").toBool());
    QVERIFY(!base.facts()->currentBasePositionSaved());
    QCOMPARE(rtk->fixedBasePositionLatitude()->rawValue().toDouble(), 0.0);
    QVERIFY(!siteNames(rtk).contains(site));

    // With room for it, the survey becomes the fixed base position and a site of that name.
    QVERIFY(rtk->removeBaseSite(QStringLiteral("Site 0")));
    QVERIFY(!failed->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(save, "clicked"));
    QVERIFY(!save->property("visible").toBool());
    QCOMPARE(rtk->fixedBasePositionLatitude()->rawValue().toDouble(), kSurveyedPosition.latitudeDegrees);
    const RTKBaseSite surveyed{.name = site,
                               .latitude = kSurveyedPosition.latitudeDegrees,
                               .longitude = kSurveyedPosition.longitudeDegrees,
                               .altitude = kSurveyedPosition.altitudeMeters,
                               .accuracy = 0.75};
    QCOMPARE(rtk->baseSites().last(), surveyed);
}

void GPSReceiverSettingsTest::_baseSitePicker()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    RTKSettings* const settings = panel.settings.settings;
    TestFixtures::SettingsFixture& saved = panel.settings.saved;
    saved.setFactValue(settings->savedBaseSites(), QString());
    saved.setFactValue(settings->baseMode(), static_cast<int>(BaseModeDefinition::Mode::BaseFixed));
    saved.setFactValue(settings->fixedBasePositionLatitude(), 47.5);
    saved.setFactValue(settings->fixedBasePositionLongitude(), 8.25);
    saved.setFactValue(settings->fixedBasePositionAltitude(), 512.5);
    saved.setFactValue(settings->fixedBasePositionAccuracy(), 0.75);
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* picker = panel.find("rtkBaseSitePicker");
    auto* use = panel.find("rtkBaseSiteUse");
    auto* remove = panel.find("rtkBaseSiteDelete");
    auto* name = panel.find("rtkBaseSiteName");
    auto* save = panel.find("rtkBaseSiteSave");
    auto* failed = panel.find("rtkBaseSiteSaveFailed");
    QVERIFY(picker && use && remove && name && save && failed);
    // Without a saved site there is nothing to pick, and a site needs a name, which the settings limit.
    QVERIFY(!picker->isVisible());
    QVERIFY(name->isVisible());
    QVERIFY(!save->isEnabled());
    QCOMPARE(name->property("maximumLength").toInt(), static_cast<int>(RTKSettings::MaxBaseSiteNameLength));

    const QString north = QStringLiteral("North field");
    const QString south = QStringLiteral("South field");
    QVERIFY(name->setProperty("text", north));
    QVERIFY(QMetaObject::invokeMethod(save, "clicked"));
    settings->fixedBasePositionLatitude()->setRawValue(-33.5);
    QVERIFY(name->setProperty("text", south));
    QVERIFY(QMetaObject::invokeMethod(save, "clicked"));
    QCOMPARE(siteNames(settings), (QStringList{north, south}));
    // The picker shows the site just saved, not the first one a new list selects.
    QVERIFY(picker->isVisible());
    QCOMPARE(picker->property("currentText").toString(), south);

    // Picking a site changes nothing until it is used, which makes it the fixed base position the receiver starts from.
    QVERIFY(picker->setProperty("currentIndex", 0));
    QCOMPARE(settings->fixedBasePositionLatitude()->rawValue().toDouble(), -33.5);
    QVERIFY(QMetaObject::invokeMethod(use, "clicked"));
    QCOMPARE(settings->fixedBasePositionLatitude()->rawValue().toDouble(), 47.5);
    QVERIFY(panel.receiver.gps.connectReceiver());
    const GPSBaseStationConfig::Mode northPosition = GPSBaseStationConfig::Fixed{
        .position = {.latitudeDegrees = 47.5, .longitudeDegrees = 8.25, .altitudeMeters = 512.5},
        .accuracyMeters = 0.75f};
    QVERIFY(panel.receiver.worker()->capturedConfig().base.mode == northPosition);
    // A connected receiver keeps its position, which can still be saved as a site.
    QVERIFY(!use->isEnabled());
    QVERIFY(save->isEnabled() && remove->isEnabled());
    panel.receiver.gps.disconnectReceiver();

    // A full list refuses another name and says so until there is room.
    QVERIFY(fillBaseSites(settings));
    QVERIFY(name->setProperty("text", QStringLiteral("One too many")));
    QVERIFY(QMetaObject::invokeMethod(save, "clicked"));
    QVERIFY(failed->isVisible());
    QCOMPARE(settings->baseSites().size(), RTKSettings::MaxBaseSites);
    QVERIFY(picker->setProperty("currentIndex", 1));
    QVERIFY(QMetaObject::invokeMethod(remove, "clicked"));
    QVERIFY(!failed->isVisible());
    QVERIFY(!siteNames(settings).contains(south));
    QCOMPARE(siteNames(settings).first(), north);
}

void GPSReceiverSettingsTest::_statusListsConstellations()
{
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    // The panel appears with the receiver, which an item without a parent cannot do.
    GPSTest::QmlWindowFixture window;
    GPSTest::QmlEngine engine;
    auto status = engine.create(GPSTest::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverStatus.qml")),
                                {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                                 {QStringLiteral("receiver"), QVariant::fromValue(&receiver)},
                                 {QStringLiteral("facts"), QVariant::fromValue(receiver.facts())}});
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* item = qobject_cast<QQuickItem*>(status.get());
    auto* row = status->findChild<QObject*>(QStringLiteral("rtkConstellations"));
    QVERIFY(item && row);
    QVERIFY(connectOverTcp(receiver));
    ScriptedReceiverWorker* const worker = harness.workers.current();
    worker->ready();
    // A receiver that reports no satellites per constellation has no row.
    QVERIFY(!row->property("visible").toBool());

    GPSSatelliteReport report;
    report.timestampUs = 1;
    report.inView = 17;
    report.addConstellation({GPSConstellation::GPS, 10, 8});
    report.addConstellation({GPSConstellation::Galileo, 7, std::nullopt});
    worker->satellites(report);
    QVERIFY(row->property("visible").toBool());
    // Used/in view, with a dash for a used count the receiver does not report.
    QCOMPARE(row->property("labelText").toString(), QStringLiteral("GPS\u00A08/10, Galileo\u00A0–/7"));
    const qreal width = item->implicitWidth();

    // More constellations wrap inside the row instead of widening the panel.
    for (const GPSConstellation constellation :
         {GPSConstellation::BeiDou, GPSConstellation::QZSS, GPSConstellation::SBAS, GPSConstellation::NavIC}) {
        report.addConstellation({constellation, 12, 11});
    }
    worker->satellites(report);
    QVERIFY(row->property("labelText").toString().endsWith(QStringLiteral("NavIC\u00A011/12")));
    QCOMPARE(item->implicitWidth(), width);

    receiver.disconnectReceiver();
    QVERIFY(!row->property("visible").toBool());
}

void GPSReceiverSettingsTest::_reconnectingOffersDisconnect()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* connect = panel.find("rtkConnectButton");
    auto* device = panel.find("rtkSerialDevice");
    QVERIFY(connect && device);
    GPSReceiver& receiver = panel.receiver.gps;
    // Only Connect automatically reconnects a lost receiver.
    panel.settings.autoConnect->setRawValue(true);
    QVERIFY(receiver.connectReceiver());
    panel.receiver.worker()->ready();
    expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
    panel.receiver.worker()->fail(GPSConnectionError::DeviceError);
    verifyExpectedLogMessage();
    QVERIFY(receiver.reconnecting());
    QVERIFY(!receiver.hasReceiver());
    QCOMPARE(connect->property("text").toString(), QCoreApplication::translate("GPSReceiverSettings", "Disconnect"));
    QVERIFY(connect->isEnabled());
    QVERIFY(!device->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(connect, "clicked"));
    QVERIFY(!receiver.reconnecting());
    QVERIFY(device->isEnabled());
    QCOMPARE(connect->property("text").toString(), QCoreApplication::translate("GPSReceiverSettings", "Connect"));
    QCOMPARE(panel.receiver.workers.count(), 1);
}

void GPSReceiverSettingsTest::_tcpConnectionFields()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* host = panel.find("rtkTcpHost");
    auto* port = panel.find("rtkTcpPort");
    auto* device = panel.find("rtkSerialDevice");
    auto* connectionType = panel.find("rtkConnectionType");
    auto* consent = panel.find<QObject>("rtkPersistentChangesCheckBox");
    QVERIFY(host && port && device && connectionType && consent);
    RTKSettings* const settings = panel.settings.settings;
    GPSReceiver& receiver = panel.receiver.gps;
    QVERIFY(connectionType->isVisible());
    const bool serial = receiver.serialSupported();
    QCOMPARE(device->isVisible(), serial);
    QCOMPARE(host->isVisible() && port->isVisible(), !serial);
    settings->connectionType()->setRawValue(RTKSettings::Tcp);
    QVERIFY(host->isVisible() && port->isVisible());
    QVERIFY(!device->isVisible());
    settings->baseReceiverManufacturer()->setRawValue(gpsReceiverManufacturerForType(GPSType::Quectel));
    QVERIFY(consent->property("visible").toBool());
    // Which settings withdraw the permission is GPSReceiverSettingsBindingTest's; the checkbox follows the receiver.
    QVERIFY(QMetaObject::invokeMethod(consent, "click"));
    QVERIFY(consent->property("checked").toBool());
    settings->tcpHost()->setRawValue(QStringLiteral("bridge"));
    QVERIFY(!consent->property("checked").toBool());
    settings->tcpPort()->setRawValue(2101);
    QVERIFY(receiver.connectReceiver());
    QCOMPARE(receiver.activeEndpoint(), QStringLiteral("bridge:2101"));
    QVERIFY(!host->isEnabled() && !port->isEnabled());
}

void GPSReceiverSettingsTest::_roleSelectsFields()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* role = panel.find("rtkReceiverRole");
    auto* manufacturer = panel.find("rtkManufacturer");
    auto* survey = panel.find("rtkSurveyMode");
    auto* udpPort = panel.find("rtkUdpPort");
    auto* udpExplanation = panel.find("rtkUdpExplanation");
    auto* device = panel.find("rtkSerialDevice");
    auto* autoConnect = panel.find("rtkAutoConnect");
    auto* connect = panel.find("rtkConnectButton");
    QVERIFY(role && manufacturer && survey && udpPort && udpExplanation && device && autoConnect && connect);
    RTKSettings* const settings = panel.settings.settings;
    GPSReceiver& receiver = panel.receiver.gps;
    QVERIFY(role->isVisible() && manufacturer->isVisible() && survey->isVisible());
    QVERIFY(autoConnect->isVisible());
    QVERIFY(connect->isEnabled());

    // A receiver QGroundControl does not configure hides every base setting.
    settings->receiverRole()->setRawValue(RTKSettings::Passive);
    settings->forwardReceiverRtcm()->setRawValue(false);
    QVERIFY(!manufacturer->isVisible());
    QVERIFY(!survey->isVisible());
    QCOMPARE(device->isVisible(), receiver.serialSupported());
    QVERIFY(autoConnect->isVisible());
    QVERIFY(connect->isEnabled());

    settings->connectionType()->setRawValue(RTKSettings::Udp);
    QVERIFY(udpPort->isVisible() && udpExplanation->isVisible());
    QVERIFY(!device->isVisible());
    QVERIFY(autoConnect->isVisible());
    QVERIFY(connect->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(connect, "clicked"));
    QVERIFY(receiver.hasReceiver());
    QCOMPARE(panel.receiver.worker()->type(), GPSType::Passive);
    QCOMPARE(receiver.activeRole(), RTKSettings::Passive);
    QVERIFY(!receiver.forwardingCorrections());
    receiver.disconnectReceiver();

    // UDP cannot carry base configuration.
    settings->receiverRole()->setRawValue(RTKSettings::ConfiguredBase);
    QVERIFY(udpPort->isVisible());
    QVERIFY(!connect->isEnabled());
    QVERIFY(udpExplanation->property("text").toString().contains(QStringLiteral("serial or TCP")));
    settings->connectionType()->setRawValue(RTKSettings::Serial);
    QVERIFY(!udpPort->isVisible());
    QVERIFY(connect->isEnabled());
}

void GPSReceiverSettingsTest::_automaticManufacturer()
{
    Panel panel(GPS_AUTOMATIC_MANUFACTURER);
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* manufacturer = panel.find<QObject>("rtkManufacturer");
    auto* explanation = panel.find("rtkAutomaticExplanation");
    auto* connect = panel.find<QObject>("rtkConnectButton");
    QVERIFY(manufacturer && explanation && connect);
    RTKSettings* const settings = panel.settings.settings;
    GPSReceiver& receiver = panel.receiver.gps;
    QCOMPARE(manufacturer->property("currentText").toString(), QStringLiteral("Detect automatically"));
    QVERIFY(explanation->isVisible());
    QVERIFY(explanation->property("text").toString().contains(QStringLiteral("identification queries")));
    QVERIFY(connect->property("enabled").toBool());
    // Until a receiver is identified, the base modes of every family are offered.
    settings->baseMode()->setRawValue(2);
    QVERIFY(connect->property("enabled").toBool());
    settings->baseReceiverManufacturer()->setRawValue(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY(!explanation->isVisible());
    QVERIFY(!connect->property("enabled").toBool());
    settings->baseReceiverManufacturer()->setRawValue(GPS_AUTOMATIC_MANUFACTURER);
    QVERIFY(QMetaObject::invokeMethod(connect, "clicked"));
    QVERIFY(receiver.hasReceiver());
    QCOMPARE(panel.receiver.worker()->type(), GPSType::Automatic);

    GPSReceiverFactGroup& facts = *receiver.facts();
    facts.setLiveUpdates(true);
    auto status = panel.engine.create(GPSTest::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverStatus.qml")),
                                      {{QStringLiteral("receiver"), QVariant::fromValue(&receiver)},
                                       {QStringLiteral("facts"), QVariant::fromValue(&facts)}});
    QVERIFY2(status, qPrintable(panel.engine.lastError()));
    auto* text = status->findChild<QObject*>(QStringLiteral("rtkReceiverStatus"));
    auto* detected = status->findChild<QQuickItem*>(QStringLiteral("rtkDetectedReceiver"));
    QVERIFY(text && detected);
    QCOMPARE(text->property("text").toString(), QStringLiteral("Identifying receiver..."));
    QVERIFY(!detected->isVisible());
    panel.receiver.worker()->detected(GPSType::Unicore);
    QVERIFY(detected->isVisible());
    QCOMPARE(detected->property("labelText").toString(), QStringLiteral("Unicore"));
    QCOMPARE(text->property("text").toString(), QStringLiteral("Connecting to receiver..."));
    panel.receiver.worker()->ready();
    QVERIFY(text->property("text").toString().contains(QStringLiteral("averaging")));
    QVERIFY(detected->isVisible());
    receiver.disconnectReceiver();
    QVERIFY(!detected->isVisible());
}

void GPSReceiverSettingsTest::_automaticOffersQuectelConsent()
{
    Panel panel(GPS_AUTOMATIC_MANUFACTURER);
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* consent = panel.find("rtkPersistentChangesCheckBox");
    auto* consentWarning = panel.find("rtkPersistentConsentWarning");
    auto* restartNote = panel.find("rtkPersistentConfigurationWarning");
    auto* surveyNote = panel.find("rtkSurveySavesPosition");
    auto* connect = panel.find<QObject>("rtkConnectButton");
    QVERIFY(consent && consentWarning && restartNote && surveyNote && connect);
    // A factory LG290P needs this consent, so Automatic offers it and words it for an identified Quectel receiver.
    for (auto* item : {consent, consentWarning, restartNote, surveyNote}) {
        QVERIFY2(item->isVisible(), qPrintable(item->objectName()));
        QVERIFY2(item->property("text").toString().contains(QStringLiteral("if a Quectel receiver is identified"),
                                                            Qt::CaseInsensitive),
                 qPrintable(item->property("text").toString()));
    }
    QVERIFY(consent->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(consent, "click"));
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QCOMPARE(panel.receiver.workers.count(), 1);
    QCOMPARE(panel.receiver.worker()->type(), GPSType::Automatic);
    QVERIFY(panel.receiver.worker()->capturedConfig().allowPersistentChanges);
    QVERIFY(!consent->property("checked").toBool());
    panel.receiver.gps.disconnectReceiver();

    RTKSettings* const settings = panel.settings.settings;
    settings->baseReceiverManufacturer()->setRawValue(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY(!consent->isVisible());
    QVERIFY(!restartNote->isVisible());
    settings->baseReceiverManufacturer()->setRawValue(gpsReceiverManufacturerForType(GPSType::Quectel));
    QVERIFY(consent->isVisible());
    QCOMPARE(consent->property("text").toString(), QStringLiteral("Allow flash save and restart"));
}

void GPSReceiverSettingsTest::_compactCorrectionsToggle()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* toggle = panel.find("rtkCompactRtcm");
    QVERIFY(toggle);
    RTKSettings* const settings = panel.settings.settings;
    QVERIFY(toggle->isVisible());
    QVERIFY(toggle->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(toggle, "click"));
    QVERIFY(settings->compactRtcmCorrections()->rawValue().toBool());
    // Which families send compact corrections is the receiver presentation's; the toggle follows it.
    settings->baseReceiverManufacturer()->setRawValue(gpsReceiverManufacturerForType(GPSType::Septentrio));
    QVERIFY(!toggle->isVisible());
    settings->baseReceiverManufacturer()->setRawValue(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY(toggle->isVisible());
    QVERIFY(panel.receiver.gps.connectReceiver());
    QVERIFY(panel.receiver.worker()->capturedConfig().base.compactObservations);
    QVERIFY(!toggle->isEnabled());
}

void GPSReceiverSettingsTest::_recordingSwitch()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* toggle = panel.find("rtkRecordReceiverData");
    auto* file = panel.find("rtkRecordingFile");
    QVERIFY(toggle && file);
    GPSReceiver& receiver = panel.receiver.gps;
    QVERIFY(!toggle->property("checked").toBool());
    QVERIFY(!file->isVisible());

    // The session that already runs is not recorded, and the note says when recording starts.
    QVERIFY(receiver.connectReceiver());
    QVERIFY(QMetaObject::invokeMethod(toggle, "click"));
    QVERIFY(receiver.recordSessions());
    QVERIFY(file->isVisible());
    QCOMPARE(file->property("text").toString(),
             QCoreApplication::translate("GPSReceiverRecording", "Recording starts with the next connection."));

    receiver.disconnectReceiver();
    // The retired worker holds the serial port until it finishes.
    panel.receiver.workers.finishRetired();
    QVERIFY(receiver.connectReceiver());
    const QString recording = receiver.recordingFile();
    QVERIFY(recording.startsWith(SettingsReceiver::recordingDirectory()));
    QCOMPARE(file->property("text").toString(),
             QCoreApplication::translate("GPSReceiverRecording", "Recording file: %1").arg(recording));

    // The switch follows the receiver, and the session that started recorded stays so until it ends.
    receiver.setRecordSessions(false);
    QVERIFY(!toggle->property("checked").toBool());
    QVERIFY(file->property("text").toString().endsWith(recording));
    receiver.disconnectReceiver();
    QVERIFY(!file->isVisible());
}

void GPSReceiverSettingsTest::_consentIsOneUse()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::Quectel));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* consent = panel.find<QObject>("rtkPersistentChangesCheckBox");
    auto* connect = panel.find<QObject>("rtkConnectButton");
    QVERIFY(consent && connect);
    SettingsReceiver& receiver = panel.receiver;
    QVERIFY(!consent->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(consent, "click"));
    QVERIFY(consent->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QCOMPARE(receiver.workers.count(), 1);
    QVERIFY(receiver.worker()->capturedConfig().allowPersistentChanges);
    QVERIFY(!consent->property("checked").toBool());
    const auto failToOpen = [this, &receiver]() {
        expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Failed to open")));
        receiver.worker()->fail(GPSConnectionError::OpenFailed);
        verifyExpectedLogMessage();
        receiver.workers.finishRetired();
    };
    // A failed attempt uses up its consent.
    failToOpen();
    QVERIFY(!receiver.gps.hasReceiver());
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QCOMPARE(receiver.workers.count(), 2);
    QVERIFY(!receiver.worker()->capturedConfig().allowPersistentChanges);
    failToOpen();
    QVERIFY(!receiver.gps.hasReceiver());

    // Which settings withdraw the permission is GPSReceiverSettingsBindingTest's; the checkbox follows the receiver.
    RTKSettings* const settings = panel.settings.settings;
    QVERIFY(QMetaObject::invokeMethod(consent, "click"));
    QVERIFY(consent->property("checked").toBool());
    settings->serialDevice()->setRawValue(QStringLiteral("/test/other"));
    QVERIFY(!consent->property("checked").toBool());
    settings->baseReceiverManufacturer()->setRawValue(gpsReceiverManufacturerForType(GPSType::Unicore));
    QVERIFY(!consent->property("visible").toBool());
    settings->baseReceiverManufacturer()->setRawValue(gpsReceiverManufacturerForType(GPSType::Quectel));
    QVERIFY(QMetaObject::invokeMethod(consent, "click"));
    QVERIFY(receiver.gps.connectReceiver());
    QVERIFY(!consent->property("checked").toBool());
    QVERIFY(!consent->property("enabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QVERIFY(!receiver.gps.hasReceiver());
    QVERIFY(!consent->property("checked").toBool());
    // Leaving the panel withdraws a permission given for the configuration it showed.
    QVERIFY(QMetaObject::invokeMethod(consent, "click"));
    QVERIFY(receiver.gps.persistentChangesAllowed());
    panel.item.reset();
    QVERIFY(!receiver.gps.persistentChangesAllowed());
}

void GPSReceiverSettingsTest::_warningWidth_data()
{
    QTest::addColumn<int>("width");
    QTest::addColumn<bool>("longText");
    for (const int width : {320, 1200}) {
        QTest::addRow("width-%d", width) << width << false;
        QTest::addRow("translated-width-%d", width) << width << true;
    }
}

void GPSReceiverSettingsTest::_warningWidth()
{
    QFETCH(int, width);
    QFETCH(bool, longText);
    GPSTest::QmlWindowFixture window(width, 800);
    Panel panel(gpsReceiverManufacturerForType(GPSType::Quectel));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* item = qobject_cast<QQuickItem*>(panel.item.get());
    QVERIFY(item);
    item->setParentItem(window.contentItem());
    item->setWidth(width);
    QVERIFY(window.show());
    for (const auto* name : {"rtkPersistentConfigurationWarning", "rtkPersistentConsentWarning"}) {
        auto* warning = panel.find(name);
        QVERIFY(warning);
        if (longText) {
            const QString translated = QStringLiteral(
                "Empfänger-Konfiguration und dauerhafte Speicherung benötigen Ihre ausdrückliche "
                "Zustimmung. Änderungen können trotz fehlgeschlagener Verbindung gespeichert bleiben. ");
            QVERIFY(warning->setProperty("text", translated.repeated(4)));
        }
        // Text changes its content height before the enclosing layout's next polish.
        QTRY_VERIFY_WITH_TIMEOUT(warning->width() > 0 && warning->property("lineCount").toInt() > 1 &&
                                     warning->height() >= warning->property("contentHeight").toDouble(),
                                 TestTimeout::mediumMs());
        const auto bounds = warning->mapRectToItem(item, warning->boundingRect());
        QVERIFY2(bounds.left() >= 0 && bounds.right() <= item->width() + 1,
                 qPrintable(QStringLiteral("%1: text bounds %2..%3, panel width %4")
                                .arg(QString::fromLatin1(name))
                                .arg(bounds.left())
                                .arg(bounds.right())
                                .arg(item->width())));
        QVERIFY(item->implicitWidth() < 1200);
    }
}

void GPSReceiverSettingsTest::_serialSelectionTracksFacts()
{
#ifdef QGC_NO_SERIAL_LINK
    QSKIP("Serial selection requires serial link support");
#endif
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* device = panel.find<QObject>("rtkSerialDevice");
    auto* baud = panel.find<QObject>("rtkSerialBaudRate");
    auto* custom = panel.find<QObject>("rtkCustomBaudRate");
    QVERIFY(device && baud && custom);
    RTKSettings* const settings = panel.settings.settings;
    GPSReceiver& receiver = panel.receiver.gps;
    // The picker shows the labels of the receiver's serial port entries, which GPSReceiverTest::_serialPortEntries
    // covers, and stores their paths.
    QCOMPARE(device->property("currentText").toString(), QStringLiteral("/test/receiver"));
    QCOMPARE(baud->property("currentText").toString(), QStringLiteral("115200"));
    settings->serialDevice()->setRawValue(QStringLiteral("/test/missing"));
    QVERIFY(device->property("currentText").toString().contains(QStringLiteral("/test/missing")));
    QVERIFY(device->property("currentText").toString().contains(QStringLiteral("unavailable")));
    const QString label = QStringLiteral("Receiver – /test/missing");
    QVERIFY(panel.item->setProperty("serialPorts", portEntries(QStringLiteral("/test/missing"), label)));
    QCOMPARE(device->property("currentText").toString(), label);
    // With Connect automatically on, a configured base can leave the device to USB discovery.
    const QString anyReceiver = QCoreApplication::translate("GPSReceiverSettings", "Any RTK receiver on USB");
    panel.settings.autoConnect->setRawValue(true);
    QVERIFY(QMetaObject::invokeMethod(device, "activated", Q_ARG(int, 0)));
    QVERIFY(settings->serialDevice()->rawValue().toString().isEmpty());
    QCOMPARE(device->property("currentText").toString(), anyReceiver);
    panel.settings.autoConnect->setRawValue(false);
    QVERIFY(QMetaObject::invokeMethod(device, "activated", Q_ARG(int, 0)));
    QCOMPARE(settings->serialDevice()->rawValue().toString(), QStringLiteral("/test/missing"));
    settings->serialBaudRate()->setRawValue(123457);
    QVERIFY(custom->property("visible").toBool());
    QCOMPARE(custom->property("text").toString(), QStringLiteral("123457"));
    settings->serialBaudRate()->setRawValue(230400);
    QVERIFY(!custom->property("visible").toBool());
    QCOMPARE(baud->property("currentText").toString(), QStringLiteral("230400"));
    QVERIFY(QMetaObject::invokeMethod(baud, "activated", Q_ARG(int, 3)));
    QVERIFY(custom->property("visible").toBool());
    QVERIFY(custom->setProperty("text", QStringLiteral("250000")));
    QVERIFY(QMetaObject::invokeMethod(custom, "_onEditingFinished"));
    QCOMPARE(settings->serialBaudRate()->rawValue().toInt(), 250000);
    QVERIFY(QMetaObject::invokeMethod(baud, "activated", Q_ARG(int, 0)));
    QCOMPARE(settings->serialBaudRate()->rawValue().toInt(), 0);
    QCOMPARE(baud->property("currentText").toString(), QCoreApplication::translate("GPSReceiverSerialPort", "Auto"));
    // Passive roles need the receiver's existing rate, so Auto is not offered.
    settings->receiverRole()->setRawValue(RTKSettings::Passive);
    QVERIFY(custom->property("visible").toBool());
    settings->receiverRole()->setRawValue(RTKSettings::ConfiguredBase);
    settings->serialBaudRate()->setRawValue(230400);
    QVERIFY(receiver.connectReceiver());
    QCOMPARE(receiver.activeEndpoint(), QStringLiteral("/test/missing"));
    QCOMPARE(panel.receiver.worker()->capturedConfig().baudRate, 230400u);
    QVERIFY(!device->property("enabled").toBool());
    QVERIFY(!baud->property("enabled").toBool());
    QVERIFY(!custom->property("enabled").toBool());
}

void GPSReceiverSettingsTest::_serialSelectionSurvivesChoiceChanges()
{
#ifdef QGC_NO_SERIAL_LINK
    QSKIP("Serial selection requires serial link support");
#endif
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* device = panel.find<QObject>("rtkSerialDevice");
    auto* baud = panel.find<QObject>("rtkSerialBaudRate");
    QVERIFY(device && baud);
    RTKSettings* const settings = panel.settings.settings;
    const QString receiver = QStringLiteral("/test/receiver");
    const QString other = QStringLiteral("/test/other");
    const QString plugged = QStringLiteral("/test/plugged");
    const auto selection = [](const QObject* combo) {
        return QStringLiteral("%1: %2")
            .arg(combo->property("currentIndex").toInt())
            .arg(combo->property("currentText").toString());
    };
    // A combo box selects its first entry when its choices are replaced, so neither selection is the first one.
    QVERIFY(panel.item->setProperty("serialPorts", portEntries({receiver, other})));
    settings->serialDevice()->setRawValue(other);
    settings->serialBaudRate()->setRawValue(230400);
    QCOMPARE(selection(device), QStringLiteral("1: /test/other"));
    QCOMPARE(selection(baud), QStringLiteral("2: 230400"));

    // Plugging in a port lists it; unplugging the selected one keeps it, as unavailable.
    QVERIFY(panel.item->setProperty("serialPorts", portEntries({receiver, other, plugged})));
    QCOMPARE(selection(device), QStringLiteral("1: /test/other"));
    QVERIFY(panel.item->setProperty("serialPorts", portEntries({receiver, plugged})));
    QCOMPARE(selection(device), QStringLiteral("2: /test/other (unavailable)"));
    // Connect automatically lists USB discovery above the ports.
    panel.settings.autoConnect->setRawValue(true);
    QCOMPARE(selection(device), QStringLiteral("3: /test/other (unavailable)"));

    // A passive receiver is not offered Auto, and a receiver may list other rates.
    settings->receiverRole()->setRawValue(RTKSettings::Passive);
    QCOMPARE(selection(baud), QStringLiteral("1: 230400"));
    settings->receiverRole()->setRawValue(RTKSettings::ConfiguredBase);
    QCOMPARE(selection(baud), QStringLiteral("2: 230400"));
    QVERIFY(panel.item->setProperty("serialBaudRates", QVariant::fromValue(QList<int>{57600, 115200, 230400})));
    QCOMPARE(selection(baud), QStringLiteral("3: 230400"));
    QCOMPARE(settings->serialDevice()->rawValue().toString(), other);
    QCOMPARE(settings->serialBaudRate()->rawValue().toInt(), 230400);
}

void GPSReceiverSettingsTest::_horizontalAccuracyLabel()
{
    TestFixtures::SettingsFixture saved;
    Fact* const units = SettingsManager::instance()->unitsSettings()->horizontalDistanceUnits();
    saved.setFactValue(units, UnitsSettings::HorizontalDistanceUnitsMeters);
    GPSTest::QmlEngine engine;
    std::unique_ptr<QObject> panel =
        engine.create(GPSTest::sourceQmlUrl(QStringLiteral("GPS/Qml/GcsPositionStatus.qml")),
                      {{QStringLiteral("gcsPosition"), QVariant::fromValue(QGeoCoordinate(47.0, 8.0))},
                       {QStringLiteral("horizontalAccuracy"), 5.1}});
    QVERIFY2(panel, qPrintable(engine.lastError()));
    auto* accuracy = panel->findChild<QObject*>(QStringLiteral("gcsHorizontalAccuracy"));
    QVERIFY(accuracy);
    QVERIFY(accuracy->property("visible").toBool());
    QCOMPARE(accuracy->property("label").toString(), QStringLiteral("Horizontal accuracy"));
    QCOMPARE(accuracy->property("labelText").toString(), QStringLiteral("5.1 m"));
    QVERIFY(panel->setProperty("horizontalAccuracy", 0));
    QCOMPARE(accuracy->property("labelText").toString(), QStringLiteral("0.0 m"));
    // The accuracy follows the application's distance units.
    units->setRawValue(UnitsSettings::HorizontalDistanceUnitsFeet);
    QVERIFY(panel->setProperty("horizontalAccuracy", 5.1));
    QCOMPARE(accuracy->property("labelText").toString(), QStringLiteral("16.7 ft"));
    // An unknown accuracy is not shown.
    QVERIFY(panel->setProperty("horizontalAccuracy", qInf()));
    QVERIFY(!accuracy->property("visible").toBool());
}

UT_REGISTER_TEST(GPSReceiverSettingsTest, TestLabel::Unit)
