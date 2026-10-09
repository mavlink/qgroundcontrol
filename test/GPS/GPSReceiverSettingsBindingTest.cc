#include "GPSReceiverSettingsBindingTest.h"

#include <chrono>

#include <QtCore/QMetaProperty>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionSettings.h"
#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "GPSSettingsBindings.h"
#include "GPSTransport.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "RTKSettings.h"
#include "Receiver/Support/GPSReceiverTestSupport.h"
#include "SettingsManager.h"
#include "Support/GPSTestHelpers.h"

using namespace std::chrono_literals;
using namespace GPSTest;

void GPSReceiverSettingsBindingTest::_capabilitiesFor_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<bool>("rtkBase");
    QTest::addColumn<bool>("receiverAveraging");
    QTest::addColumn<bool>("surveyIn");
    // Saved baseReceiverManufacturers values.
    QTest::newRow("automatic") << 0 << true << true << true;
    QTest::newRow("trimble") << 1 << true << false << true;
    QTest::newRow("septentrio") << 2 << true << false << true;
    QTest::newRow("femtomes") << 3 << true << false << true;
    QTest::newRow("ublox") << 4 << true << false << true;
    QTest::newRow("unicore") << 5 << true << true << false;
    QTest::newRow("quectel") << 6 << true << false << true;
    QTest::newRow("passive") << kPassiveManufacturer << false << false << false;
    QTest::newRow("invalid") << 99 << false << false << false;
}

void GPSReceiverSettingsBindingTest::_capabilitiesFor()
{
    QFETCH(int, manufacturer);
    QFETCH(bool, rtkBase);
    QFETCH(bool, receiverAveraging);
    QFETCH(bool, surveyIn);
    GPSReceiver receiver;
    // The passive family is selected by role, so a passive receiver's manufacturer does not matter.
    const bool passive = manufacturer == kPassiveManufacturer;
    const auto caps =
        receiver.capabilitiesFor(passive ? RTKSettings::Passive : RTKSettings::ConfiguredBase, manufacturer);
    QCOMPARE(caps.passive, passive);
    QCOMPARE(caps.rtkBase, rtkBase);
    QCOMPARE(caps.receiverAveraging, receiverAveraging);
    QCOMPARE(caps.surveyIn, surveyIn);
}

void GPSReceiverSettingsBindingTest::_receiverSettingsMapping_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<int>("baseMode");
    QTest::addColumn<bool>("accepted");
    // Each family's modes and limits are GPSBaseStationSettingsTest's; these rows show the receiver applies them.
    QTest::newRow("ublox-survey-in") << 4 << 0 << true;
    QTest::newRow("unicore-needs-explicit-mode") << 5 << 0 << false;
    // Automatic accepts what any family supports; the detected family's support is checked on the worker.
    for (const int mode : {0, 1, 2}) {
        QTest::addRow("automatic-mode-%d", mode) << 0 << mode << true;
    }
}

void GPSReceiverSettingsBindingTest::_receiverSettingsMapping()
{
    QFETCH(int, manufacturer);
    QFETCH(int, baseMode);
    QFETCH(bool, accepted);
    RTKSettings settings;
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(settings.receiverRole(), RTKSettings::ConfiguredBase);
    saved.setFactValue(settings.baseReceiverManufacturers(), manufacturer);
    saved.setFactValue(settings.connectionType(), RTKSettings::Tcp);
    saved.setFactValue(settings.tcpHost(), QStringLiteral("rtk.test"));
    saved.setFactValue(settings.tcpPort(), 2101);
    saved.setFactValue(settings.autoConnect(), false);
    saved.setFactValue(settings.useFixedBasePosition(), baseMode);
    saved.setFactValue(settings.fixedBasePositionLatitude(), 47.5);
    saved.setFactValue(settings.fixedBasePositionLongitude(), 8.25);
    saved.setFactValue(settings.fixedBasePositionAltitude(), 512.0);
    saved.setFactValue(settings.fixedBasePositionAccuracy(), 1.5);
    saved.setFactValue(settings.surveyInAccuracyLimit(), 1.75);
    saved.setFactValue(settings.surveyInMinObservationDuration(), 195);
    saved.setFactValue(settings.receiverAveragingDuration(), 321);
    saved.setFactValue(settings.compactRtcmCorrections(), true);
    const auto type = gpsReceiverTypeForManufacturer(manufacturer);
    QVERIFY(type);
    ScriptedGPSReceiver harness;
    auto& receiver = harness.receiver;
    GPSSettingsBindings::bindReceiver(&settings, &receiver);
    QCOMPARE(receiver.connectReceiver(), accepted);
    QCOMPARE(receiver.hasReceiver(), accepted);
    QCOMPARE(receiver.errorMessage().isEmpty(), accepted);
    if (!accepted) {
        // A rejected configuration never creates a worker, so no transport is opened.
        QCOMPARE(harness.workers.count(), 0);
        return;
    }
    const auto& config = harness.workers.current()->capturedConfig();
    QCOMPARE(config.baudRate, uint32_t(GPSTransport::BRIDGE_BAUDRATE));
    QVERIFY(!config.allowPersistentChanges);
    const GPSBaseStationConfig::Mode expected =
        baseMode == 1 ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::Fixed{
                            .position = {.latitudeDegrees = 47.5, .longitudeDegrees = 8.25, .altitudeMeters = 512.0f},
                            .accuracyMeters = 1.5f}}
        : baseMode == 2
            ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = 321s}}
            : GPSBaseStationConfig::Mode{GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1.75, .duration = 195s}};
    QVERIFY(config.base.mode == expected);
    // Automatic keeps the compact MSM4 option until detection, which falls back to MSM7 on receivers without it.
    QVERIFY(config.base.compactObservations);
}

void GPSReceiverSettingsBindingTest::_configurationDiagnosticRetained_data()
{
    QTest::addColumn<QString>("session");
    QTest::addColumn<GPSConnectionError>("error");
    QTest::addColumn<QString>("detail");
    QTest::addColumn<QString>("expected");

    const struct
    {
        const char* name;
        GPSConnectionError error;
        QString detail;
    } cases[] = {{"provisioning-mismatch", GPSConnectionError::ConfigFailed,
                  QStringLiteral("Requested base settings differ from receiver readback.")},
                 {"possibly-persisted", GPSConnectionError::ConfigFailed,
                  QStringLiteral("Settings may have been saved, but reconnect failed.")},
                 {"empty-fallback", GPSConnectionError::ConfigFailed, QString()},
                 // Flash-save advice is for auto-connect; a manual connection reports a refusal like any failure.
                 {"consent-refused", GPSConnectionError::ConsentRequired,
                  QStringLiteral("LG290P role mismatch: save the requested base role externally")}};

    for (const auto& [name, error, detail] : cases) {
        const QString reported =
            detail.isEmpty()
                ? GPSReceiver::tr("Receiver configuration failed. Check the receiver type, baud rate, and base mode.")
                : GPSReceiver::tr("Receiver configuration failed: %1").arg(detail);
        // Only a connection that reached the receiver is retried, as Connect automatically is on by default.
        const QString retried =
            detail.isEmpty()
                ? GPSReceiver::tr("Receiver configuration failed. Reconnecting automatically.")
                : GPSReceiver::tr("Receiver configuration failed: %1. Reconnecting automatically.").arg(detail);
        QTest::addRow("first-attempt-%s", name) << QStringLiteral("first-attempt") << error << detail << reported;
        QTest::addRow("reconnecting-%s", name) << QStringLiteral("reconnecting") << error << detail << retried;
    }
}

void GPSReceiverSettingsBindingTest::_configurationDiagnosticRetained()
{
    QFETCH(QString, session);
    QFETCH(GPSConnectionError, error);
    QFETCH(QString, detail);
    QFETCH(QString, expected);
    const bool reconnecting = session == QStringLiteral("reconnecting");
    ScriptedGPSReceiver harness;
    auto& receiver = harness.receiver;
    auto configuration = receiverConfiguration(gpsReceiverManufacturerForType(GPSType::quectel));
    useTcp(configuration);
    receiver.setConfiguration(configuration);
    receiver.setPersistentChangesAllowed(true);
    QVERIFY(receiver.connectReceiver());
    auto* worker = harness.workers.current();
    QVERIFY(worker);
    if (reconnecting) {
        worker->ready();
    }
    receiver.setConnectionError(QStringLiteral("An earlier configuration error"));
    QSignalSpy messages(&receiver, &GPSReceiver::errorMessageChanged);
    expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg,
                     QRegularExpression(reconnecting ? QStringLiteral("GPS receiver session ended")
                                                     : QStringLiteral("GPS receiver did not accept configuration")));
    worker->fail(error, detail);
    verifyExpectedLogMessage();
    QCOMPARE(messages.size(), 1);
    QVERIFY(!receiver.facts()->telemetryAvailable());
    QCOMPARE(receiver.reconnecting(), reconnecting);
    QCOMPARE(receiver.errorMessage(), expected);
}

void GPSReceiverSettingsBindingTest::_receiverSettingsBinding()
{
    RTKSettings settings;
    GPSReceiver receiver;
    GPSSettingsBindings::bindReceiver(&settings, &receiver);

    settings.connectionType()->setRawValue(RTKSettings::Tcp);
    settings.tcpHost()->setRawValue(QStringLiteral("rtk.example"));
    settings.tcpPort()->setRawValue(2101);
    settings.serialDevice()->setRawValue(QStringLiteral("/test/receiver"));
    settings.serialBaudRate()->setRawValue(230400);
    settings.useFixedBasePosition()->setRawValue(1);
    settings.fixedBasePositionLatitude()->setRawValue(47.5);
    settings.autoConnect()->setRawValue(true);

    const auto& configuration = receiver.configuration();
    QCOMPARE(configuration.connectionType, RTKSettings::Tcp);
    QCOMPARE(configuration.tcpHost, QStringLiteral("rtk.example"));
    QCOMPARE(configuration.tcpPort, 2101U);
    QCOMPARE(configuration.serialDevice, QStringLiteral("/test/receiver"));
    QCOMPARE(configuration.serialBaudRate, 230400U);
    const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&configuration.base.mode);
    QVERIFY(fixed);
    QCOMPARE(fixed->position.latitudeDegrees, 47.5);
    QVERIFY(configuration.autoConnect);

    // Disconnect only pauses automatic connection; the setting is the user's.
    receiver.disconnectReceiver();
    QVERIFY(settings.autoConnect()->rawValue().toBool());

    // Nothing writes the saved manufacturer, even when another family is detected.
    settings.baseReceiverManufacturers()->setRawValue(GPS_AUTOMATIC_MANUFACTURER);
    ScriptedReceiverWorkerFactory workers;
    receiver.setWorkerFactory(workers.workerFactory());
    QVERIFY(receiver.connectReceiver());
    workers.current()->detected(GPSType::ublox);
    QCOMPARE(receiver._activeManufacturer(), gpsReceiverManufacturerForType(GPSType::ublox));
    QCOMPARE(settings.baseReceiverManufacturers()->rawValue().toInt(), GPS_AUTOMATIC_MANUFACTURER);
}

void GPSReceiverSettingsBindingTest::_persistentConsentScope_data()
{
    QTest::addColumn<QString>("setting");
    QTest::addColumn<bool>("keepsConsent");
    // The permission covers one receiver, connection and base selection.
    for (const auto* setting : {"serial-device", "baud-rate", "base-mode", "manufacturer", "role", "connection-type",
                                "tcp-host", "tcp-port", "udp-port"}) {
        QTest::newRow(setting) << QString::fromLatin1(setting) << false;
    }
    QTest::newRow("survey-accuracy") << QStringLiteral("survey-accuracy") << true;
    QTest::newRow("auto-connect") << QStringLiteral("auto-connect") << true;
}

void GPSReceiverSettingsBindingTest::_persistentConsentScope()
{
    QFETCH(QString, setting);
    QFETCH(bool, keepsConsent);
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    auto configuration = receiverConfiguration(gpsReceiverManufacturerForType(GPSType::quectel));
    receiver.setConfiguration(configuration);
    QSignalSpy changed(&receiver, &GPSReceiver::persistentChangesAllowedChanged);
    receiver.setPersistentChangesAllowed(true);
    QVERIFY(receiver.persistentChangesAllowed());
    QCOMPARE(changed.size(), 1);

    if (setting == QStringLiteral("serial-device")) {
        configuration.serialDevice = QStringLiteral("/test/other");
    } else if (setting == QStringLiteral("baud-rate")) {
        configuration.serialBaudRate = 230400;
    } else if (setting == QStringLiteral("base-mode")) {
        configuration.base.mode = GPSBaseStationConfig::Fixed{};
    } else if (setting == QStringLiteral("manufacturer")) {
        configuration.baseReceiverManufacturer = gpsReceiverManufacturerForType(GPSType::ublox);
    } else if (setting == QStringLiteral("role")) {
        configuration.receiverRole = RTKSettings::Passive;
    } else if (setting == QStringLiteral("connection-type")) {
        configuration.connectionType = RTKSettings::Tcp;
    } else if (setting == QStringLiteral("tcp-host")) {
        configuration.tcpHost = QStringLiteral("other.example");
    } else if (setting == QStringLiteral("tcp-port")) {
        configuration.tcpPort = 2102;
    } else if (setting == QStringLiteral("udp-port")) {
        configuration.udpPort = 14402;
    } else if (setting == QStringLiteral("survey-accuracy")) {
        std::get<GPSBaseStationConfig::SurveyIn>(configuration.base.mode).accuracyMeters = 5.;
    } else {
        configuration.autoConnect = !configuration.autoConnect;
    }
    receiver.setConfiguration(configuration);
    QCOMPARE(receiver.persistentChangesAllowed(), keepsConsent);
    QCOMPARE(changed.size(), keepsConsent ? 1 : 2);
}

void GPSReceiverSettingsBindingTest::_persistentConsentIsSpentOnConnect_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<bool>("granted");
    QTest::addColumn<bool>("expected");
    const int quectel = gpsReceiverManufacturerForType(GPSType::quectel);
    const int ublox = gpsReceiverManufacturerForType(GPSType::ublox);
    QTest::newRow("quectel-granted") << quectel << true << true;
    QTest::newRow("quectel-withheld") << quectel << false << false;
    // Families that cannot save settings never receive the permission.
    QTest::newRow("ublox-granted") << ublox << true << false;
}

void GPSReceiverSettingsBindingTest::_persistentConsentIsSpentOnConnect()
{
    QFETCH(int, manufacturer);
    QFETCH(bool, granted);
    QFETCH(bool, expected);
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    auto configuration = receiverConfiguration(manufacturer);
    useTcp(configuration);
    receiver.setConfiguration(configuration);
    receiver.setPersistentChangesAllowed(granted);

    QVERIFY(receiver.connectReceiver());
    QVERIFY(harness.workers.current());
    QCOMPARE(harness.workers.current()->capturedConfig().allowPersistentChanges, expected);
    QVERIFY(!receiver.persistentChangesAllowed());

    // A session change or disconnect withdraws a permission granted while connected.
    receiver.setPersistentChangesAllowed(true);
    receiver.disconnectReceiver();
    QVERIFY(!receiver.persistentChangesAllowed());
}

void GPSReceiverSettingsBindingTest::_effectiveConnection_data()
{
    QTest::addColumn<int>("role");
    QTest::addColumn<int>("saved");
    QTest::addColumn<int>("effective");
    QTest::addColumn<bool>("supported");
#ifdef QGC_NO_SERIAL_LINK
    const int serial = RTKSettings::Tcp;
#else
    const int serial = RTKSettings::Serial;
#endif
    QTest::newRow("base-serial") << int(RTKSettings::ConfiguredBase) << int(RTKSettings::Serial) << serial << true;
    QTest::newRow("base-tcp") << int(RTKSettings::ConfiguredBase) << int(RTKSettings::Tcp) << int(RTKSettings::Tcp)
                              << true;
    QTest::newRow("base-udp") << int(RTKSettings::ConfiguredBase) << int(RTKSettings::Udp) << int(RTKSettings::Udp)
                              << false;
    QTest::newRow("passive-udp") << int(RTKSettings::Passive) << int(RTKSettings::Udp) << int(RTKSettings::Udp) << true;
    QTest::newRow("unknown-type") << int(RTKSettings::ConfiguredBase) << 7 << serial << true;
}

void GPSReceiverSettingsBindingTest::_effectiveConnection()
{
    QFETCH(int, role);
    QFETCH(int, saved);
    QFETCH(int, effective);
    QFETCH(bool, supported);
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    QSignalSpy changed(&receiver, &GPSReceiver::configurationChanged);
    auto configuration = receiverConfiguration();
    configuration.receiverRole = static_cast<RTKSettings::ReceiverRole>(role);
    configuration.connectionType = static_cast<RTKSettings::ConnectionType>(saved);
    receiver.setConfiguration(configuration);
    QCOMPARE(int(receiver.effectiveConnectionType()), effective);
    QCOMPARE(receiver.connectionSupported(), supported);
    QCOMPARE(receiver.property("effectiveConnectionType").toInt(), effective);
    QVERIFY(changed.size() <= 1);
}

void GPSReceiverSettingsBindingTest::_runtimeSettingsDoNotRequireAppRestart()
{
    RTKSettings settings;
    // The bindings apply every receiver setting to the running receiver.
    for (const Fact* fact : GPSSettingsBindings::_boundFacts(&settings)) {
        QVERIFY2(!fact->qgcRebootRequired(), qPrintable(fact->name()));
        QVERIFY2(!fact->vehicleRebootRequired(), qPrintable(fact->name()));
    }
}

void GPSReceiverSettingsBindingTest::_ntripSettingsBinding()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerConnectEnabled(), false);

    const NTRIPManager::Configuration expected{
        .connection = {.host = QStringLiteral("caster.example.com"),
                       .port = 443,
                       .username = QStringLiteral("user"),
                       .password = QStringLiteral("private-password"),
                       .mountpoint = QStringLiteral("MOUNT"),
                       .useTls = true,
                       .allowSelfSignedCerts = true,
                       .pinnedCertificate = QStringLiteral("caster.example.com:443|00ff")},
        .filter = {.whitelist = QStringLiteral("1005,1077")},
        .gga = {.source = NTRIPGgaReporter::PositionSource::GCSPosition, .interval = std::chrono::seconds(7)}};
    saved.setFactValue(settings->ntripServerHostAddress(), expected.connection.host);
    saved.setFactValue(settings->ntripServerPort(), expected.connection.port);
    saved.setFactValue(settings->ntripUsername(), expected.connection.username);
    saved.setFactValue(settings->ntripPassword(), expected.connection.password);
    saved.setFactValue(settings->ntripMountpoint(), expected.connection.mountpoint);
    saved.setFactValue(settings->ntripUseTls(), expected.connection.useTls);
    saved.setFactValue(settings->ntripAllowSelfSignedCerts(), expected.connection.allowSelfSignedCerts);
    saved.setFactValue(settings->ntripPinnedCertificate(), expected.connection.pinnedCertificate);
    saved.setFactValue(settings->ntripWhitelist(), expected.filter.whitelist);
    saved.setFactValue(settings->ntripGgaPositionSource(), static_cast<int>(expected.gga.source));
    saved.setFactValue(settings->ntripGgaIntervalSec(), 7);

    NTRIPManager manager;
    GPSSettingsBindings::bindNtrip(settings, &manager);
    const NTRIPManager::Configuration configuration = manager.configuration();
    QVERIFY(!configuration.enabled);
    QCOMPARE(configuration, expected);
    settings->ntripServerConnectEnabled()->setRawValue(true);
    QVERIFY(manager.configuration().enabled);
}

void GPSReceiverSettingsBindingTest::_settingsLogRedactsSecrets_data()
{
    QTest::addColumn<Fact*>("fact");
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QVariant>("changed");
    QTest::addColumn<bool>("redacted");
    NTRIPSettings* const ntrip = SettingsManager::instance()->ntripSettings();
    RTKSettings* const rtk = SettingsManager::instance()->rtkSettings();
    QTest::newRow("ntrip-password") << ntrip->ntripPassword() << QVariant(QStringLiteral("private-password"))
                                    << QVariant(QStringLiteral("changed-password")) << true;
    QTest::newRow("ntrip-mountpoint") << ntrip->ntripMountpoint() << QVariant(QStringLiteral("MOUNT"))
                                      << QVariant(QStringLiteral("OTHER")) << false;
    QTest::newRow("base-latitude") << rtk->fixedBasePositionLatitude() << QVariant(12.3456789) << QVariant(12.5678)
                                   << true;
    QTest::newRow("base-longitude") << rtk->fixedBasePositionLongitude() << QVariant(98.7654321) << QVariant(98.1234)
                                    << true;
    QTest::newRow("base-altitude") << rtk->fixedBasePositionAltitude() << QVariant(543.21) << QVariant(432.1) << true;
    QTest::newRow("base-mode") << rtk->useFixedBasePosition()
                               << QVariant(static_cast<int>(BaseModeDefinition::Mode::BaseFixed))
                               << QVariant(static_cast<int>(BaseModeDefinition::Mode::BaseSurveyIn)) << false;
}

void GPSReceiverSettingsBindingTest::_settingsLogRedactsSecrets()
{
    QFETCH(Fact*, fact);
    QFETCH(QVariant, value);
    QFETCH(QVariant, changed);
    QFETCH(bool, redacted);
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->ntripSettings()->ntripServerConnectEnabled(), false);
    saved.setFactValue(fact, value);
    const QString boundValue = fact->rawValue().toString();
    const QString category = QStringLiteral("GPS.GPSSettingsBindings");
    const TestFixtures::LoggingCategoryFixture logging(category);
    ignoreLogMessage("GPS.GPSSettingsBindings", QtDebugMsg, QRegularExpression(QStringLiteral(".*")));
    NTRIPManager ntrip;
    GPSSettingsBindings::bindNtrip(SettingsManager::instance()->ntripSettings(), &ntrip);
    GPSReceiver receiver;
    GPSSettingsBindings::bindReceiver(SettingsManager::instance()->rtkSettings(), &receiver);
    fact->setRawValue(changed);
    const QString changedValue = fact->rawValue().toString();

    // The bindings log each setting when bound and when it changes, a redacted one only as set or empty.
    const QStringList logged = GPSTest::debugMessages(category);
    const auto line = [&fact](const QString& shown) { return QStringLiteral("%1 = %2").arg(fact->name(), shown); };
    if (redacted) {
        QCOMPARE(logged.count(line(QStringLiteral("<set>"))), 2);
        const QString all = logged.join(QLatin1Char('\n'));
        QVERIFY2(!all.contains(boundValue) && !all.contains(changedValue), qPrintable(all));
    } else {
        QCOMPARE(logged.count(line(boundValue)), 1);
        QCOMPARE(logged.count(line(changedValue)), 1);
    }
}

void GPSReceiverSettingsBindingTest::_settingsBindingsCoverEveryFact_data()
{
    QTest::addColumn<SettingsGroup*>("group");
    QTest::addColumn<QList<Fact*>>("bound");

    // A new setting must be bound.
    SettingsManager* const settings = SettingsManager::instance();
    QTest::newRow("RTK") << static_cast<SettingsGroup*>(settings->rtkSettings())
                         << GPSSettingsBindings::_boundFacts(settings->rtkSettings());
    QTest::newRow("NTRIP") << static_cast<SettingsGroup*>(settings->ntripSettings())
                           << GPSSettingsBindings::_boundFacts(settings->ntripSettings());
    QTest::newRow("GPSCorrection") << static_cast<SettingsGroup*>(settings->gpsCorrectionSettings())
                                   << GPSSettingsBindings::_boundFacts(settings->gpsCorrectionSettings());
}

void GPSReceiverSettingsBindingTest::_settingsBindingsCoverEveryFact()
{
    QFETCH(SettingsGroup*, group);
    QFETCH(QList<Fact*>, bound);

    const QMetaObject* const metaObject = group->metaObject();
    for (int i = SettingsGroup::staticMetaObject.propertyCount(); i < metaObject->propertyCount(); ++i) {
        const QMetaProperty property = metaObject->property(i);
        if (property.metaType() != QMetaType::fromType<Fact*>()) {
            continue;
        }
        QVERIFY2(bound.count(property.read(group).value<Fact*>()) == 1,
                 qPrintable(QStringLiteral("%1 must be bound once").arg(QString::fromLatin1(property.name()))));
    }
}

UT_REGISTER_TEST(GPSReceiverSettingsBindingTest, TestLabel::Unit)
