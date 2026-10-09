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

#ifndef QGC_NO_SERIAL_LINK
    // Every device the tests select; the panel's own port list decides which ones it shows as available.
    TestSerialPorts serial{{serialPort(QStringLiteral("/test/receiver")), serialPort(QStringLiteral("/test/other")),
                            serialPort(QStringLiteral("/test/missing"))}};
    GPSTest::ScriptedGPSReceiver scripted{nullptr, {.serialPorts = &serial.manager}};
#else
    GPSTest::ScriptedGPSReceiver scripted;
#endif
    GPSReceiver& gps = scripted.receiver;
    ScriptedReceiverWorkerFactory& workers = scripted.workers;
};

/// A serial device picker entry, labelled with its path unless @a label is given.
QVariant portEntries(const QString& path, const QString& label = {})
{
    return QVariant::fromValue(QList<GPSSerialPortEntry>{{.value = path, .label = label.isEmpty() ? path : label}});
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
}  // namespace

void GPSReceiverSettingsTest::_surveySaveWorkflow()
{
    // How each family maps the saved base settings is GPSReceiverSettingsBindingTest's; this is the panel's workflow.
    Panel panel(gpsReceiverManufacturerForType(GPSType::ublox));
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
    QCOMPARE(settings->useFixedBasePosition()->rawValue().toInt(), 0);

    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QVERIFY(!panel.receiver.gps.hasReceiver());
    workers.finishRetired();
    QVERIFY(fixed->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(fixed, "click"));
    QCOMPARE(settings->useFixedBasePosition()->rawValue().toInt(), 1);
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QCOMPARE(workers.count(), 2);
    // The saved position starts the fixed base.
    const GPSBaseStationConfig::Mode savedPosition =
        GPSBaseStationConfig::Fixed{.position = kSurveyedPosition, .accuracyMeters = 0.75f};
    QVERIFY(workers.current()->capturedConfig().base.mode == savedPosition);
}

void GPSReceiverSettingsTest::_surveyCompletePrompt()
{
    RTKSettingsFixture settings(gpsReceiverManufacturerForType(GPSType::ublox));
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

void GPSReceiverSettingsTest::_reconnectingOffersDisconnect()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::ublox));
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
    Panel panel(gpsReceiverManufacturerForType(GPSType::ublox));
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
    settings->baseReceiverManufacturers()->setRawValue(gpsReceiverManufacturerForType(GPSType::quectel));
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
    Panel panel(gpsReceiverManufacturerForType(GPSType::ublox));
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
    QCOMPARE(panel.receiver.worker()->type(), GPSType::passive);
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
    settings->useFixedBasePosition()->setRawValue(2);
    QVERIFY(connect->property("enabled").toBool());
    settings->baseReceiverManufacturers()->setRawValue(gpsReceiverManufacturerForType(GPSType::ublox));
    QVERIFY(!explanation->isVisible());
    QVERIFY(!connect->property("enabled").toBool());
    settings->baseReceiverManufacturers()->setRawValue(GPS_AUTOMATIC_MANUFACTURER);
    QVERIFY(QMetaObject::invokeMethod(connect, "clicked"));
    QVERIFY(receiver.hasReceiver());
    QCOMPARE(panel.receiver.worker()->type(), GPSType::automatic);

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
    panel.receiver.worker()->detected(GPSType::unicore);
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
    QCOMPARE(panel.receiver.worker()->type(), GPSType::automatic);
    QVERIFY(panel.receiver.worker()->capturedConfig().allowPersistentChanges);
    QVERIFY(!consent->property("checked").toBool());
    panel.receiver.gps.disconnectReceiver();

    RTKSettings* const settings = panel.settings.settings;
    settings->baseReceiverManufacturers()->setRawValue(gpsReceiverManufacturerForType(GPSType::ublox));
    QVERIFY(!consent->isVisible());
    QVERIFY(!restartNote->isVisible());
    settings->baseReceiverManufacturers()->setRawValue(gpsReceiverManufacturerForType(GPSType::quectel));
    QVERIFY(consent->isVisible());
    QCOMPARE(consent->property("text").toString(), QStringLiteral("Allow flash save and restart"));
}

void GPSReceiverSettingsTest::_compactCorrectionsToggle()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::ublox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* toggle = panel.find("rtkCompactRtcm");
    QVERIFY(toggle);
    RTKSettings* const settings = panel.settings.settings;
    QVERIFY(toggle->isVisible());
    QVERIFY(toggle->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(toggle, "click"));
    QVERIFY(settings->compactRtcmCorrections()->rawValue().toBool());
    // Which families send compact corrections is the receiver presentation's; the toggle follows it.
    settings->baseReceiverManufacturers()->setRawValue(gpsReceiverManufacturerForType(GPSType::septentrio));
    QVERIFY(!toggle->isVisible());
    settings->baseReceiverManufacturers()->setRawValue(gpsReceiverManufacturerForType(GPSType::ublox));
    QVERIFY(toggle->isVisible());
    QVERIFY(panel.receiver.gps.connectReceiver());
    QVERIFY(panel.receiver.worker()->capturedConfig().base.compactObservations);
    QVERIFY(!toggle->isEnabled());
}

void GPSReceiverSettingsTest::_consentIsOneUse()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::quectel));
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
    settings->baseReceiverManufacturers()->setRawValue(gpsReceiverManufacturerForType(GPSType::unicore));
    QVERIFY(!consent->property("visible").toBool());
    settings->baseReceiverManufacturers()->setRawValue(gpsReceiverManufacturerForType(GPSType::quectel));
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
    Panel panel(gpsReceiverManufacturerForType(GPSType::quectel));
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
    Panel panel(gpsReceiverManufacturerForType(GPSType::ublox));
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
