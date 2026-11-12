#include "GPSSettingsBindingsTest.h"

#include <chrono>

#include <QtCore/QMetaProperty>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionSettings.h"
#include "GPSReceiver.h"
#include "GPSSettingsBindings.h"
#include "GPSTransport.h"
#include "ManualScheduler.h"
#include "NTRIP/Support/MockNTRIPTransport.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "PositionManager.h"
#include "PositionReceiverSettings.h"
#include "RTKSettings.h"
#include "Receiver/Support/GPSReceiverTestSupport.h"
#include "Receiver/Support/ScriptedBluetooth.h"
#include "SettingsManager.h"
#include "Support/GPSTestHelpers.h"

using namespace std::chrono_literals;
using namespace GPSTest;

void GPSSettingsBindingsTest::_receiverSettingsMapping_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<int>("baseMode");
    QTest::addColumn<bool>("accepted");
    // Each family's modes and limits are GPSReceiverSessionTest's; these rows show the receiver applies them.
    QTest::newRow("ublox-survey-in") << 4 << 0 << true;
    QTest::newRow("unicore-needs-explicit-mode") << 5 << 0 << false;
    // Automatic accepts what any family supports; the detected family's support is checked on the worker.
    for (const int mode : {0, 1, 2}) {
        QTest::addRow("automatic-mode-%d", mode) << 0 << mode << true;
    }
}

void GPSSettingsBindingsTest::_receiverSettingsMapping()
{
    QFETCH(int, manufacturer);
    QFETCH(int, baseMode);
    QFETCH(bool, accepted);
    RTKSettings settings;
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(settings.receiverRole(), RTKSettings::ConfiguredBase);
    saved.setFactValue(settings.baseReceiverManufacturer(), manufacturer);
    saved.setFactValue(settings.connectionType(), RTKSettings::Tcp);
    saved.setFactValue(settings.tcpHost(), QStringLiteral("rtk.test"));
    saved.setFactValue(settings.tcpPort(), 2101);
    saved.setFactValue(settings.autoConnect(), false);
    saved.setFactValue(settings.baseMode(), baseMode);
    saved.setFactValue(settings.fixedBasePositionLatitude(), 47.5);
    saved.setFactValue(settings.fixedBasePositionLongitude(), 8.25);
    saved.setFactValue(settings.fixedBasePositionAltitude(), 512.0);
    saved.setFactValue(settings.fixedBasePositionAccuracy(), 1.5);
    saved.setFactValue(settings.fixedBaseAntennaHeight(), 1.75);
    saved.setFactValue(settings.surveyInAccuracyLimit(), 1.75);
    saved.setFactValue(settings.surveyInMinObservationDuration(), 195);
    saved.setFactValue(settings.receiverAveragingDuration(), 321);
    saved.setFactValue(settings.compactRtcmCorrections(), true);
    // Bits no constellation has are not a selection.
    saved.setFactValue(settings.baseCorrectionConstellations(), RTKSettings::Galileo | RTKSettings::BeiDou | 0x40);
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
    QCOMPARE(config.baudRate, uint32_t(GPSTransport::BRIDGE_BAUD_RATE));
    QVERIFY(!config.allowPersistentChanges);
    // A fixed base is given where its antenna is: the antenna height above the saved position.
    const GPSBaseStationConfig::Mode expected =
        baseMode == 1 ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::Fixed{
                            .position = {.latitudeDegrees = 47.5, .longitudeDegrees = 8.25, .altitudeMeters = 513.75},
                            .accuracyMeters = 1.5f}}
        : baseMode == 2
            ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = 321s}}
            : GPSBaseStationConfig::Mode{GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1.75, .duration = 195s}};
    QVERIFY(config.base.mode == expected);
    // Automatic keeps the compact MSM4 option until detection, which falls back to MSM7 on receivers without it.
    QVERIFY(config.base.compactObservations);
    // So it keeps the constellations selected for the corrections.
    QCOMPARE(config.base.correctionConstellations,
             GPSCorrectionConstellation::Galileo | GPSCorrectionConstellation::BeiDou);
}

void GPSSettingsBindingsTest::_receiverSettingsBinding()
{
    RTKSettings settings;
    GPSReceiver receiver;
    GPSSettingsBindings::bindReceiver(&settings, &receiver);

    settings.connectionType()->setRawValue(RTKSettings::Tcp);
    settings.tcpHost()->setRawValue(QStringLiteral("rtk.example"));
    settings.tcpPort()->setRawValue(2101);
    settings.serialDevice()->setRawValue(QStringLiteral("/test/receiver"));
    settings.serialBaudRate()->setRawValue(230400);
    settings.baseMode()->setRawValue(1);
    settings.fixedBasePositionLatitude()->setRawValue(47.5);
    settings.autoConnect()->setRawValue(true);

    const auto& configuration = receiver.configuration();
    QCOMPARE(configuration.connectionType, RTKSettings::Tcp);
    QCOMPARE(configuration.tcpHost, QStringLiteral("rtk.example"));
    QCOMPARE(configuration.tcpPort, 2101U);
    QCOMPARE(configuration.serialDevice, QStringLiteral("/test/receiver"));
    QCOMPARE(configuration.serialBaudRate, 230400U);
    settings.connectionType()->setRawValue(RTKSettings::Bluetooth);
    settings.bluetoothDeviceAddress()->setRawValue(QStringLiteral("00:11:22:AA:BB:CC"));
    settings.bluetoothDeviceName()->setRawValue(QStringLiteral("Survey receiver"));
    QCOMPARE(configuration.connectionType, RTKSettings::Bluetooth);
    QCOMPARE(configuration.bluetoothAddress, QStringLiteral("00:11:22:AA:BB:CC"));
    QCOMPARE(configuration.bluetoothName, QStringLiteral("Survey receiver"));
    settings.connectionType()->setRawValue(RTKSettings::Tcp);
    const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&configuration.base.mode);
    QVERIFY(fixed);
    QCOMPARE(fixed->position.latitudeDegrees, 47.5);
    QVERIFY(configuration.autoConnect);

    // Disconnect only pauses automatic connection; the setting is the user's.
    receiver.disconnectReceiver();
    QVERIFY(settings.autoConnect()->rawValue().toBool());

    // Nothing writes the saved manufacturer, even when another family is detected.
    settings.baseReceiverManufacturer()->setRawValue(GPS_AUTOMATIC_MANUFACTURER);
    ScriptedReceiverWorkerFactory workers;
    receiver.setWorkerFactory(workers.workerFactory());
    QVERIFY(receiver.connectReceiver());
    workers.current()->detected(GPSType::UBlox);
    QCOMPARE(receiver._activeManufacturer(), gpsReceiverManufacturerForType(GPSType::UBlox));
    QCOMPARE(settings.baseReceiverManufacturer()->rawValue().toInt(), GPS_AUTOMATIC_MANUFACTURER);
}

void GPSSettingsBindingsTest::_positionReceiverSettingsBinding()
{
    RTKSettings rtk;
    PositionReceiverSettings settings;
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    PositionManager positions;
    GPSSettingsBindings::bindPositionReceiver(&settings, &receiver);
    GPSSettingsBindings::bindPosition(&rtk, &settings, &positions);
    // Out of use until the user turns it on, which leaves the RTK receiver the receiver source.
    QVERIFY(!receiver.configuration().autoConnect);
    QCOMPARE(positions.configuration().receiver, PositionManager::Receiver::Rtk);

    settings.connectionType()->setRawValue(RTKSettings::Tcp);
    settings.tcpHost()->setRawValue(QStringLiteral("position.example"));
    settings.tcpPort()->setRawValue(2102);
    settings.udpPort()->setRawValue(14500);
    settings.serialDevice()->setRawValue(QStringLiteral("/test/position"));
    settings.serialBaudRate()->setRawValue(0);
    settings.enabled()->setRawValue(true);
    rtk.gcsPositionSource()->setRawValue(static_cast<int>(PositionManager::SourceMode::ReceiverOnly));

    // The connection is the settings'. The receiver is never configured and its RTCM never forwarded, whatever
    // the RTK receiver's settings say of theirs.
    const auto& configuration = receiver.configuration();
    QCOMPARE(configuration.receiverRole, RTKSettings::Passive);
    QVERIFY(!configuration.forwardReceiverRtcm);
    QCOMPARE(configuration.connectionType, RTKSettings::Tcp);
    QCOMPARE(configuration.tcpHost, QStringLiteral("position.example"));
    QCOMPARE(configuration.tcpPort, 2102U);
    QCOMPARE(configuration.udpPort, 14500U);
    QCOMPARE(configuration.serialDevice, QStringLiteral("/test/position"));
    QCOMPARE(configuration.serialBaudRate, 0U);
    QVERIFY(configuration.autoConnect);
    QCOMPARE(positions.configuration().receiver, PositionManager::Receiver::Position);
    QCOMPARE(positions.configuration().sourceMode, PositionManager::SourceMode::ReceiverOnly);

    // Out of use again, it is disconnected, where the RTK receiver's automatic connection only stops reconnecting.
    QVERIFY(receiver.connectReceiver());
    QCOMPARE(harness.workers.current()->type(), GPSType::Passive);
    settings.enabled()->setRawValue(false);
    QVERIFY(!receiver.hasReceiver());
    QCOMPARE(receiver.state(), GPSReceiver::State::Disconnected);
    QCOMPARE(positions.configuration().receiver, PositionManager::Receiver::Rtk);
    harness.workers.finishRetired();
}

void GPSSettingsBindingsTest::_bluetoothOfferedWhereReceiversConnectOverIt_data()
{
    QTest::addColumn<bool>("supported");
    QTest::newRow("bluetooth") << true;
    // A build without Bluetooth sockets, on any build.
    QTest::newRow("no-bluetooth") << false;
}

void GPSSettingsBindingsTest::_bluetoothOfferedWhereReceiversConnectOverIt()
{
    QFETCH(bool, supported);
    RTKSettings settings;
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(settings.connectionType(), RTKSettings::Bluetooth);
    saved.setFactValue(settings.autoConnect(), false);
    const auto offered = [&settings] {
        return settings.connectionType()->enumStrings().contains(QStringLiteral("Bluetooth"));
    };
    QVERIFY(offered());
    ScriptedBluetooth bluetooth;
    bluetooth.supported = supported;
    GPSReceiver receiver(nullptr, nullptr, {.bluetooth = bluetooth.side()});
    QSignalSpy enumsChanged(settings.connectionType(), &Fact::enumsChanged);
    GPSSettingsBindings::bindReceiver(&settings, &receiver);

    // The connection is offered only to a receiver that has it, and a saved selection of it is kept only there.
    QCOMPARE(offered(), supported);
    QCOMPARE(enumsChanged.size(), supported ? 0 : 1);
    QCOMPARE(settings.connectionType()->enumStrings().size(), settings.connectionType()->enumValues().size());
    QCOMPARE(settings.connectionType()->rawValue().toInt(),
             supported ? int(RTKSettings::Bluetooth) : int(RTKSettings::Serial));
    QCOMPARE(receiver.configuration().connectionType, supported ? RTKSettings::Bluetooth : RTKSettings::Serial);
}

void GPSSettingsBindingsTest::_runtimeSettingsDoNotRequireAppRestart()
{
    RTKSettings settings;
    PositionReceiverSettings positionReceiver;
    // The bindings apply every receiver setting to the running receiver.
    for (const Fact* fact :
         GPSSettingsBindings::_boundFacts(&settings) + GPSSettingsBindings::_boundFacts(&positionReceiver)) {
        QVERIFY2(!fact->qgcRebootRequired(), qPrintable(fact->name()));
        QVERIFY2(!fact->vehicleRebootRequired(), qPrintable(fact->name()));
    }
}

void GPSSettingsBindingsTest::_ntripSettingsBinding()
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
        .gga = {.source = NTRIPGgaReporter::PositionSource::GCSPosition, .interval = std::chrono::seconds(7)},
        .version1Casters = QStringLiteral("http://caster.example.com:2101/")};
    saved.setFactValue(settings->ntripServerHostAddress(), expected.connection.host);
    saved.setFactValue(settings->ntripServerPort(), expected.connection.port);
    saved.setFactValue(settings->ntripUsername(), expected.connection.username);
    saved.setFactValue(settings->ntripPassword(), expected.connection.password);
    saved.setFactValue(settings->ntripMountpoint(), expected.connection.mountpoint);
    saved.setFactValue(settings->ntripUseTls(), expected.connection.useTls);
    saved.setFactValue(settings->ntripAllowSelfSignedCerts(), expected.connection.allowSelfSignedCerts);
    saved.setFactValue(settings->ntripPinnedCertificate(), expected.connection.pinnedCertificate);
    saved.setFactValue(settings->ntripGgaPositionSource(), static_cast<int>(expected.gga.source));
    saved.setFactValue(settings->ntripGgaIntervalSec(), 7);
    saved.setFactValue(settings->ntripVersion1Casters(), expected.version1Casters);
    saved.setFactValue(settings->ntripFailoverEnabled(), false);
    saved.setFactValue(settings->ntripSavedCasters(), QString());
    saved.setFactValue(settings->ntripRequireVehicle(), false);
    saved.setFactValue(settings->ntripDataCapMB(), 0);

    NTRIPManager manager;
    GPSSettingsBindings::bindNtrip(settings, &manager);
    const NTRIPManager::Configuration configuration = manager.configuration();
    QVERIFY(!configuration.enabled);
    QCOMPARE(configuration, expected);
    settings->ntripServerConnectEnabled()->setRawValue(true);
    QVERIFY(manager.configuration().enabled);

    // The manager reports the casters it found to take NTRIP version 1 requests only, which the next run starts with.
    const QString casters = QStringLiteral("http://caster.example.com:2101/ https://other.example.com:443/");
    emit manager.version1CastersChanged(casters);
    deliverQueuedCalls();
    QCOMPARE(settings->ntripVersion1Casters()->rawValue().toString(), casters);
    QCOMPARE(manager.configuration().version1Casters, casters);

    // The manager is given the saved casters to go on to, as they are when the list changes, and whether it may.
    settings->ntripServerConnectEnabled()->setRawValue(false);
    QVERIFY(settings->saveConnectionAsCaster(QStringLiteral("Backup")));
    settings->ntripFailoverEnabled()->setRawValue(true);
    QVERIFY(manager.configuration().failover);
    QCOMPARE(manager.configuration().savedCasters.size(), 1);
    const NTRIPManager::SavedCaster backup = manager.configuration().savedCasters.constFirst();
    QCOMPARE(backup.name, QStringLiteral("Backup"));
    QCOMPARE(backup.connection, expected.connection);
    QVERIFY(settings->removeSavedCaster(backup.name));
    QVERIFY(manager.configuration().savedCasters.isEmpty());
    settings->ntripRequireVehicle()->setRawValue(true);
    QVERIFY(manager.configuration().requireVehicle);
    settings->ntripDataCapMB()->setRawValue(250);
    QCOMPARE(manager.configuration().dataCapMB, quint32(250));

    // A stored source that names none would report no position at all.
    expectLogMessage("GPS.GPSSettingsBindings", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid GGA position source")));
    settings->ntripGgaPositionSource()->setRawValue(9);
    verifyExpectedLogMessage();
    QCOMPARE(manager.configuration().gga.source, NTRIPGgaReporter::PositionSource::Auto);
}

void GPSSettingsBindingsTest::_ntripActionsFollowConnectionSetting()
{
    using Status = NTRIPManager::ConnectionStatus;
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    Fact* const enabled = settings->ntripServerConnectEnabled();
    saved.setFactValue(enabled, false);
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("caster.example.com"));
    saved.setFactValue(settings->ntripServerPort(), 2101);
    saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("TEST"));
    saved.setFactValue(settings->ntripUseTls(), false);
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    GPSSettingsBindings::bindNtrip(settings, &manager);
    MockNTRIPTransport* transport = injectMockTransport(manager, true);
    manager.init();
    QCOMPARE(manager.connectionStatus(), Status::Disconnected);
    // The binding stores a request once the manager's operation has returned, and the manager applies the stored
    // setting after its debounce.
    const auto settle = [&scheduler] {
        deliverQueuedCalls();
        return scheduler.advanceBy(1s);
    };

    // Connect and Disconnect change the setting, and the manager follows the setting as for any other change of it.
    manager.connectCaster();
    QVERIFY(settle());
    QVERIFY(enabled->rawValue().toBool());
    QCOMPARE(manager.connectionStatus(), Status::Connected);
    manager.disconnectCaster();
    QVERIFY(settle());
    QVERIFY(!enabled->rawValue().toBool());
    QCOMPARE(manager.connectionStatus(), Status::Disconnected);
    transport = injectMockTransport(manager);
    enabled->setRawValue(true);
    QVERIFY(settle());
    QCOMPARE(manager.connectionStatus(), Status::Connecting);
    QCOMPARE(transport->startCount, 1);

    // After an error the setting is still on, so Connect cannot act by changing it: it starts the new attempt itself.
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg, QRegularExpression(QStringLiteral("NTRIP error:")));
    transport->simulateError(NTRIPError::AuthFailed, QStringLiteral("rejected"));
    deliverQueuedCalls();
    verifyExpectedLogMessage();
    QCOMPARE(manager.connectionStatus(), Status::Error);
    QVERIFY(enabled->rawValue().toBool());
    transport = injectMockTransport(manager);
    manager.connectCaster();
    QCOMPARE(transport->startCount, 1);
    QVERIFY(settle());
    QVERIFY(enabled->rawValue().toBool());
    QCOMPARE(manager.connectionStatus(), Status::Connecting);
    enabled->setRawValue(false);
    QVERIFY(settle());
    QCOMPARE(manager.connectionStatus(), Status::Disconnected);
}

void GPSSettingsBindingsTest::_settingsLogRedactsSecrets_data()
{
    QTest::addColumn<Fact*>("fact");
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QVariant>("changed");
    QTest::addColumn<bool>("redacted");
    NTRIPSettings* const ntrip = SettingsManager::instance()->ntripSettings();
    RTKSettings* const rtk = SettingsManager::instance()->rtkSettings();
    QTest::newRow("ntrip-password") << ntrip->ntripPassword() << QVariant(QStringLiteral("private-password"))
                                    << QVariant(QStringLiteral("changed-password")) << true;
    // The saved casters are stored with their passwords.
    const auto casters = [](const char* password) {
        return QVariant(QStringLiteral(R"([{"name":"Backup","host":"backup.example.com","port":2101,"password":"%1"}])")
                            .arg(QLatin1String(password)));
    };
    QTest::newRow("ntrip-saved-casters") << ntrip->ntripSavedCasters() << casters("private-password")
                                         << casters("changed-password") << true;
    QTest::newRow("ntrip-mountpoint") << ntrip->ntripMountpoint() << QVariant(QStringLiteral("MOUNT"))
                                      << QVariant(QStringLiteral("OTHER")) << false;
    QTest::newRow("base-latitude") << rtk->fixedBasePositionLatitude() << QVariant(12.3456789) << QVariant(12.5678)
                                   << true;
    QTest::newRow("base-longitude") << rtk->fixedBasePositionLongitude() << QVariant(98.7654321) << QVariant(98.1234)
                                    << true;
    QTest::newRow("base-altitude") << rtk->fixedBasePositionAltitude() << QVariant(543.21) << QVariant(432.1) << true;
    QTest::newRow("base-mode") << rtk->baseMode() << QVariant(static_cast<int>(BaseModeDefinition::Mode::BaseFixed))
                               << QVariant(static_cast<int>(BaseModeDefinition::Mode::BaseSurveyIn)) << false;
}

void GPSSettingsBindingsTest::_settingsLogRedactsSecrets()
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

void GPSSettingsBindingsTest::_settingsBindingsCoverEveryFact_data()
{
    QTest::addColumn<SettingsGroup*>("group");
    QTest::addColumn<QList<Fact*>>("bound");
    QTest::addColumn<QList<Fact*>>("unbound");

    // A new setting must be bound, unless no service reads it: RTKSettings itself keeps the saved base sites and the
    // receivers detected on each link, and only the form reads how it shows the fixed base position.
    SettingsManager* const settings = SettingsManager::instance();
    QTest::newRow("RTK") << static_cast<SettingsGroup*>(settings->rtkSettings())
                         << GPSSettingsBindings::_boundFacts(settings->rtkSettings())
                         << QList<Fact*>{settings->rtkSettings()->savedBaseSites(),
                                         settings->rtkSettings()->detectedReceivers(),
                                         settings->rtkSettings()->fixedBasePositionFormat()};
    QTest::newRow("NTRIP") << static_cast<SettingsGroup*>(settings->ntripSettings())
                           << GPSSettingsBindings::_boundFacts(settings->ntripSettings()) << QList<Fact*>{};
    QTest::newRow("GPSCorrection") << static_cast<SettingsGroup*>(settings->gpsCorrectionSettings())
                                   << GPSSettingsBindings::_boundFacts(settings->gpsCorrectionSettings())
                                   << QList<Fact*>{};
    QTest::newRow("PositionReceiver") << static_cast<SettingsGroup*>(settings->positionReceiverSettings())
                                      << GPSSettingsBindings::_boundFacts(settings->positionReceiverSettings())
                                      << QList<Fact*>{};
}

void GPSSettingsBindingsTest::_settingsBindingsCoverEveryFact()
{
    QFETCH(SettingsGroup*, group);
    QFETCH(QList<Fact*>, bound);
    QFETCH(QList<Fact*>, unbound);

    const QMetaObject* const metaObject = group->metaObject();
    for (int i = SettingsGroup::staticMetaObject.propertyCount(); i < metaObject->propertyCount(); ++i) {
        const QMetaProperty property = metaObject->property(i);
        if (property.metaType() != QMetaType::fromType<Fact*>()) {
            continue;
        }
        Fact* const fact = property.read(group).value<Fact*>();
        QVERIFY2(bound.count(fact) == (unbound.contains(fact) ? 0 : 1),
                 qPrintable(QStringLiteral("%1 must be bound once").arg(QString::fromLatin1(property.name()))));
    }
}

UT_REGISTER_TEST(GPSSettingsBindingsTest, TestLabel::Unit)
