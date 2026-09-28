#include "GPSReceiverConfigurationTest.h"

#include <chrono>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <stop_token>
#include <utility>
#include <variant>

#include <QtCore/QDebug>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtCore/QTimer>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QSignalSpy>

#include "BlockedTransportGate.h"
#include "GPSCorrectionManager.h"
#include "GPSManager.h"
#include "GPSPositionService.h"
#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "GPSSettingsBindings.h"
#include "GPSTestHelpers.h"
#include "LogManager.h"
#include "ManualScheduler.h"
#include "MonotonicClock.h"
#include "NMEAUtils.h"
#include "Protocols/Support/ScriptedReceiver.h"
#include "QGCLoggingCategoryManager.h"
#include "QGroundControlQmlGlobal.h"
#include "RTKSettings.h"
#include "ScriptedProvider.h"
#ifndef QGC_NO_SERIAL_LINK
#include "GPSSerialPortManagerAdapter.h"
#include "SerialPortManager.h"
#endif

using namespace std::chrono_literals;

#include "GPSReceiverTestSupport.h"

using namespace GPSReceiverTestSupport;

void GPSReceiverConfigurationTest::_manufacturerIds_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<int>("receiverType");
    QTest::newRow("automatic") << 0 << 7;
    QTest::newRow("trimble") << 1 << 1;
    QTest::newRow("septentrio") << 2 << 2;
    QTest::newRow("femtomes") << 3 << 3;
    QTest::newRow("ublox") << 4 << 0;
    QTest::newRow("unicore") << 5 << 4;
    QTest::newRow("quectel") << 6 << 5;
    QTest::newRow("passive") << 7 << 6;
    QTest::newRow("invalid") << 8 << -1;
}

void GPSReceiverConfigurationTest::_manufacturerIds()
{
    QFETCH(int, manufacturer);
    QFETCH(int, receiverType);
    RTKSettings settings;
    GPSReceiver receiver;
    const auto type = GPSReceiver::typeForManufacturer(manufacturer);
    const auto values = settings.baseReceiverManufacturers()->enumValues();
    // The passive family is selected by role, so it is not a settings manufacturer.
    QCOMPARE(values.size(), 7);
    for (int id = 0; id < values.size(); ++id) {
        QCOMPARE(values[id].toInt(), id);
    }
    QCOMPARE(type.has_value(), receiverType >= 0);
    if (type) {
        QCOMPARE(static_cast<int>(*type), receiverType);
        QCOMPARE(GPSReceiver::manufacturerForType(*type), manufacturer);
    }
    const auto caps = receiver.capabilitiesFor(
        manufacturer == kPassiveManufacturer ? GPSReceiver::Passive : GPSReceiver::ConfiguredBase, manufacturer);
    QCOMPARE(caps.recognized, manufacturer < 8);
    QCOMPARE(caps.passive, manufacturer == kPassiveManufacturer);
    QCOMPARE(caps.rtkBase, manufacturer < 7);
    QCOMPARE(caps.receiverAveraging, manufacturer == 0 || manufacturer == 5);
    QCOMPARE(caps.surveyIn, manufacturer < 7 && manufacturer != 5);
}

void GPSReceiverConfigurationTest::_receiverSettingsMapping_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<int>("baseMode");
    QTest::addColumn<bool>("accepted");
    for (const int manufacturer : {0, 4, 5, 6, 7}) {
        for (const int mode : {0, 1, 2}) {
            // Automatic accepts what any family supports; the detected family's support is checked on the worker.
            const bool accepted = manufacturer == 0 || manufacturer == 7 || mode == 1 ||
                                  (mode == 2 ? manufacturer == 5 : manufacturer != 5);
            QTest::newRow(qPrintable(QStringLiteral("receiver-%1-mode-%2").arg(manufacturer).arg(mode)))
                << manufacturer << mode << accepted;
        }
    }
}

void GPSReceiverConfigurationTest::_receiverSettingsMapping()
{
    QFETCH(int, manufacturer);
    QFETCH(int, baseMode);
    QFETCH(bool, accepted);
    auto configuration = receiverConfiguration(manufacturer);
    configuration.baseMode = baseMode;
    configuration.surveyInAccuracyLimit = 1.75;
    configuration.surveyInMinObservationDuration = 195s;
    configuration.receiverAveragingDuration = 321s;
    configuration.fixedBasePositionLatitude = 47.5;
    configuration.fixedBasePositionLongitude = 8.25;
    configuration.fixedBasePositionAltitude = 512.0f;
    configuration.fixedBasePositionAccuracy = 1.5f;
    configuration.compactRtcmCorrections = true;
    const auto type = GPSReceiver::typeForManufacturer(manufacturer);
    QVERIFY(type);
    ScriptedGPSReceiver harness;
    harness.receiver.setConfiguration(configuration);
    QCOMPARE(harness.receiver.connectReceiver(*type, {}, {}, 230400), accepted);
    if (!accepted) {
        QCOMPARE(harness.providers.count(), 0);
        return;
    }
    const auto& config = harness.providers.current()->capturedConfig();
    QCOMPARE(config.baudRate, uint32_t(230400));
    QVERIFY(!config.allowPersistentChanges);
    if (manufacturer == kPassiveManufacturer) {
        QCOMPARE(config.role, GPSReceiverConfig::Role::Passive);
        QCOMPARE(config.base, GPSBaseStationConfig{});
        return;
    }
    QCOMPARE(config.role, GPSReceiverConfig::Role::RTKBase);
    const GPSBaseStationConfig::Mode expected =
        baseMode == 1 ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::Fixed{
                            .position = {.latitudeDegrees = 47.5, .longitudeDegrees = 8.25, .altitudeMeters = 512.0f},
                            .accuracyMeters = 1.5f}}
        : baseMode == 2
            ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = 321s}}
            : GPSBaseStationConfig::Mode{GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1.75, .duration = 195s}};
    QVERIFY(config.base.mode == expected);
    // Of these receivers only u-blox sends compact MSM4 corrections; the others ignore the hidden option. Automatic
    // keeps it until detection, which falls back to MSM7.
    QCOMPARE(config.base.compactObservations, *type == GPSType::ublox || *type == GPSType::automatic);
}

void GPSReceiverConfigurationTest::_invalidReceiverSettings_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<int>("mode");
    QTest::addColumn<uint>("averagingDuration");
    QTest::addColumn<uint>("baud");
    QTest::addColumn<bool>("accepted");
    QTest::newRow("unicore-needs-explicit-mode") << 5 << 0 << 60U << 115200U << false;
    QTest::newRow("quectel-rejects-averaging") << 6 << 2 << 60U << 115200U << false;
    QTest::newRow("unknown-mode") << 6 << 42 << 60U << 115200U << false;
    QTest::newRow("zero-averaging") << 5 << 2 << 0U << 115200U << false;
    QTest::newRow("excess-averaging") << 5 << 2 << 3601U << 115200U << false;
    QTest::newRow("minimum-averaging") << 5 << 2 << 1U << 115200U << true;
    QTest::newRow("maximum-averaging") << 5 << 2 << 3600U << 115200U << true;
    QTest::newRow("passive-needs-baud") << 7 << 0 << 60U << 0U << false;
    QTest::newRow("passive-ignores-stale-base-settings") << 7 << 42 << 0U << 115200U << true;
}

void GPSReceiverConfigurationTest::_invalidReceiverSettings()
{
    QFETCH(int, manufacturer);
    QFETCH(int, mode);
    QFETCH(uint, averagingDuration);
    QFETCH(uint, baud);
    QFETCH(bool, accepted);
    auto configuration = receiverConfiguration(manufacturer);
    configuration.baseMode = mode;
    configuration.receiverAveragingDuration = std::chrono::seconds(averagingDuration);
    const auto type = GPSReceiver::typeForManufacturer(manufacturer);
    QVERIFY(type);
    RTKSettings settings;
    QCOMPARE(settings.receiverAveragingDuration()->rawMin().toUInt(), 1U);
    QCOMPARE(settings.receiverAveragingDuration()->rawMax().toUInt(), 3600U);
    QCOMPARE(settings.useFixedBasePosition()->enumValues(), (QVariantList{0, 1, 2}));
    ScriptedGPSReceiver harness;
    auto& receiver = harness.receiver;
    receiver.setConfiguration(configuration);
    QCOMPARE(receiver.connectReceiver(*type, {}, {}, baud), accepted);
    QCOMPARE(receiver.hasReceiver(), accepted);
    // A rejected configuration never creates a provider, so no transport is opened.
    QCOMPARE(harness.providers.count(), accepted ? 1 : 0);
    QCOMPARE(receiver.errorMessage().isEmpty(), accepted);
}

void GPSReceiverConfigurationTest::_configurationDiagnosticRetained_data()
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
        // Only a manual connection that reached the receiver once is retried.
        const QString retried =
            detail.isEmpty()
                ? GPSReceiver::tr("Receiver connection lost. Reconnecting automatically.")
                : GPSReceiver::tr("Receiver configuration failed: %1. Reconnecting automatically.").arg(detail);
        QTest::addRow("direct-%s", name) << QStringLiteral("direct") << error << detail << reported;
        QTest::addRow("first-attempt-%s", name) << QStringLiteral("first-attempt") << error << detail << reported;
        QTest::addRow("reconnecting-%s", name) << QStringLiteral("reconnecting") << error << detail << retried;
    }
}

void GPSReceiverConfigurationTest::_configurationDiagnosticRetained()
{
    QFETCH(QString, session);
    QFETCH(GPSConnectionError, error);
    QFETCH(QString, detail);
    QFETCH(QString, expected);
    const bool direct = session == QStringLiteral("direct");
    const bool reconnecting = session == QStringLiteral("reconnecting");
    ScriptedGPSReceiver harness;
    auto& receiver = harness.receiver;
    auto configuration = receiverConfiguration(GPSReceiver::manufacturerForType(GPSType::quectel));
    if (!direct) {
        configuration.connectionType = GPSReceiver::Tcp;
        configuration.tcpHost = QStringLiteral("rtk.test");
        configuration.tcpPort = 2101;
    }
    receiver.setConfiguration(configuration);
    QVERIFY(direct ? receiver.connectReceiver(GPSType::quectel, {}, {}, 115200, true)
                   : receiver.connectConfiguredGPS(true));
    auto* provider = harness.providers.current();
    QVERIFY(provider);
    if (reconnecting) {
        provider->ready();
    }
    receiver._setError(GPSConnectionError::ConfigFailed, QStringLiteral("An earlier configuration error"));
    QSignalSpy messages(&receiver, &GPSReceiver::errorMessageChanged);
    expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg,
                     QRegularExpression(reconnecting ? QStringLiteral("GPS receiver session ended")
                                                     : QStringLiteral("GPS receiver did not accept configuration")));
    provider->fail(error, detail);
    verifyExpectedLogMessage();
    QCOMPARE(messages.size(), 1);
    QVERIFY(!receiver.connected());
    QCOMPARE(receiver.reconnecting(), reconnecting);
    QCOMPARE(receiver.errorMessage(), expected);
}

void GPSReceiverConfigurationTest::_rtkSettingsBinding()
{
    RTKSettings settings;
    GPSReceiver receiver;
    GPSSettingsBindings::bindRtk(&settings, &receiver);

    settings.connectionType()->setRawValue(GPSReceiver::Tcp);
    settings.tcpHost()->setRawValue(QStringLiteral("rtk.example"));
    settings.tcpPort()->setRawValue(2101);
    settings.serialDevice()->setRawValue(QStringLiteral("/test/receiver"));
    settings.serialBaudRate()->setRawValue(230400);
    settings.useFixedBasePosition()->setRawValue(1);
    settings.fixedBasePositionLatitude()->setRawValue(47.5);
    settings.autoConnect()->setRawValue(true);

    const auto& configuration = receiver.configuration();
    QCOMPARE(configuration.connectionType, GPSReceiver::Tcp);
    QCOMPARE(configuration.tcpHost, QStringLiteral("rtk.example"));
    QCOMPARE(configuration.tcpPort, 2101U);
    QCOMPARE(configuration.serialDevice, QStringLiteral("/test/receiver"));
    QCOMPARE(configuration.serialBaudRate, 230400U);
    QCOMPARE(configuration.baseMode, 1);
    QCOMPARE(configuration.fixedBasePositionLatitude, 47.5);
    QVERIFY(configuration.autoConnect);

    receiver.disconnectConfiguredGPS();
    QVERIFY(!settings.autoConnect()->rawValue().toBool());

    // Nothing writes the saved manufacturer, even when another family connects.
    settings.baseReceiverManufacturers()->setRawValue(GPSReceiver::manufacturerForType(GPSType::quectel));
    auto gate = std::make_shared<BlockedTransportGate>();
    const auto releaseWorker = qScopeGuard([&] {
        gate->release.release();
        receiver.disconnectGPS();
    });
    QVERIFY(receiver.connectReceiver(GPSType::ublox, blockedTransportFactory(gate)));
    QCOMPARE(receiver.activeManufacturer(), GPSReceiver::manufacturerForType(GPSType::ublox));
    QCOMPARE(settings.baseReceiverManufacturers()->rawValue().toInt(),
             GPSReceiver::manufacturerForType(GPSType::quectel));
}

void GPSReceiverConfigurationTest::_persistentConsentScope()
{
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    auto configuration = receiverConfiguration(GPSReceiver::manufacturerForType(GPSType::quectel));
    receiver.setConfiguration(configuration);
    QSignalSpy changed(&receiver, &GPSReceiver::persistentChangesAllowedChanged);
    receiver.setPersistentChangesAllowed(true);
    QVERIFY(receiver.persistentChangesAllowed());
    QCOMPARE(changed.size(), 1);

    // Settings outside the connection and base selection keep the permission.
    configuration.surveyInAccuracyLimit = 5.;
    receiver.setConfiguration(configuration);
    QVERIFY(receiver.persistentChangesAllowed());

    configuration.serialDevice = QStringLiteral("/test/other");
    receiver.setConfiguration(configuration);
    QVERIFY(!receiver.persistentChangesAllowed());
    QCOMPARE(changed.size(), 2);

    receiver.setPersistentChangesAllowed(true);
    configuration.baseMode = static_cast<int>(BaseModeDefinition::Mode::BaseFixed);
    receiver.setConfiguration(configuration);
    QVERIFY(!receiver.persistentChangesAllowed());
}

void GPSReceiverConfigurationTest::_persistentConsentIsSpentOnConnect_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<bool>("granted");
    QTest::addColumn<bool>("expected");
    const int quectel = GPSReceiver::manufacturerForType(GPSType::quectel);
    const int ublox = GPSReceiver::manufacturerForType(GPSType::ublox);
    QTest::newRow("quectel-granted") << quectel << true << true;
    QTest::newRow("quectel-withheld") << quectel << false << false;
    // Families that cannot save settings never receive the permission.
    QTest::newRow("ublox-granted") << ublox << true << false;
}

void GPSReceiverConfigurationTest::_persistentConsentIsSpentOnConnect()
{
    QFETCH(int, manufacturer);
    QFETCH(bool, granted);
    QFETCH(bool, expected);
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    auto configuration = receiverConfiguration(manufacturer);
    configuration.connectionType = GPSReceiver::Tcp;
    configuration.tcpHost = QStringLiteral("rtk.example");
    configuration.tcpPort = 2101;
    receiver.setConfiguration(configuration);
    receiver.setPersistentChangesAllowed(granted);

    QVERIFY(receiver.connectSelectedReceiver());
    QVERIFY(harness.providers.current());
    QCOMPARE(harness.providers.current()->capturedConfig().allowPersistentChanges, expected);
    QVERIFY(!receiver.persistentChangesAllowed());

    // A session change or disconnect withdraws a permission granted while connected.
    receiver.setPersistentChangesAllowed(true);
    receiver.disconnectConfiguredGPS();
    QVERIFY(!receiver.persistentChangesAllowed());
}

void GPSReceiverConfigurationTest::_effectiveConnection_data()
{
    QTest::addColumn<int>("role");
    QTest::addColumn<int>("saved");
    QTest::addColumn<int>("effective");
    QTest::addColumn<bool>("supported");
#ifdef QGC_NO_SERIAL_LINK
    const int serial = GPSReceiver::Tcp;
#else
    const int serial = GPSReceiver::Serial;
#endif
    QTest::newRow("base-serial") << int(GPSReceiver::ConfiguredBase) << int(GPSReceiver::Serial) << serial << true;
    QTest::newRow("base-tcp") << int(GPSReceiver::ConfiguredBase) << int(GPSReceiver::Tcp) << int(GPSReceiver::Tcp)
                              << true;
    QTest::newRow("base-udp") << int(GPSReceiver::ConfiguredBase) << int(GPSReceiver::Udp) << int(GPSReceiver::Udp)
                              << false;
    QTest::newRow("passive-udp") << int(GPSReceiver::Passive) << int(GPSReceiver::Udp) << int(GPSReceiver::Udp) << true;
    QTest::newRow("unknown-type") << int(GPSReceiver::ConfiguredBase) << 7 << serial << true;
}

void GPSReceiverConfigurationTest::_effectiveConnection()
{
    QFETCH(int, role);
    QFETCH(int, saved);
    QFETCH(int, effective);
    QFETCH(bool, supported);
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    QSignalSpy changed(&receiver, &GPSReceiver::configurationChanged);
    auto configuration = receiverConfiguration();
    configuration.receiverRole = static_cast<GPSReceiver::ReceiverRole>(role);
    configuration.connectionType = static_cast<GPSReceiver::ConnectionType>(saved);
    receiver.setConfiguration(configuration);
    QCOMPARE(int(receiver.effectiveConnectionType()), effective);
    QCOMPARE(receiver.connectionSupported(), supported);
    QCOMPARE(receiver.property("effectiveConnectionType").toInt(), effective);
    QVERIFY(changed.size() <= 1);
}

void GPSReceiverConfigurationTest::_configurationDebugRedactsFixedBaseCoordinates()
{
    auto configuration = fixedConfiguration();
    configuration.fixedBasePositionLatitude = 12.3456789;
    configuration.fixedBasePositionLongitude = 98.7654321;
    configuration.fixedBasePositionAltitude = 543.21f;

    QString output;
    {
        QDebug debug(&output);
        debug << configuration;
    }

    QVERIFY(output.contains(QStringLiteral("baseMode=1")));
    QVERIFY(!output.contains(QStringLiteral("12.345")));
    QVERIFY(!output.contains(QStringLiteral("98.765")));
    QVERIFY(!output.contains(QStringLiteral("543.21")));
}

void GPSReceiverConfigurationTest::_runtimeSettingsDoNotRequireAppRestart_data()
{
    QTest::addColumn<QString>("name");
    for (const auto* name :
         {"baseReceiverManufacturers", "serialDevice", "serialBaudRate", "useFixedBasePosition",
          "surveyInAccuracyLimit", "surveyInMinObservationDuration", "receiverAveragingDuration",
          "fixedBasePositionLatitude", "fixedBasePositionLongitude", "fixedBasePositionAltitude",
          "fixedBasePositionAccuracy", "compactRtcmCorrections", "connectionType", "tcpHost", "tcpPort"}) {
        QTest::newRow(name) << QString::fromLatin1(name);
    }
}

void GPSReceiverConfigurationTest::_runtimeSettingsDoNotRequireAppRestart()
{
    QFETCH(QString, name);
    RTKSettings settings;
    auto* fact = settings.property(name.toUtf8().constData()).value<Fact*>();
    QVERIFY(fact);
    QVERIFY(!fact->qgcRebootRequired());
    QVERIFY(!fact->vehicleRebootRequired());
}

UT_REGISTER_TEST(GPSReceiverConfigurationTest, TestLabel::Unit)
