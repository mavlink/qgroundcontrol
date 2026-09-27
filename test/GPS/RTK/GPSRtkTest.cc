#include "GPSRtkTest.h"

#include <chrono>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>

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
#include "Driver/Support/ScriptedReceiver.h"
#include "GPSCorrectionManager.h"
#include "GPSManager.h"
#include "GPSPositionService.h"
#include "GPSRTKFactGroup.h"
#include "GPSRtk.h"
#include "GPSSettingsBindings.h"
#include "GpsTestHelpers.h"
#include "LogManager.h"
#include "ManualScheduler.h"
#include "MonotonicClock.h"
#include "NMEAUtils.h"
#include "QGCLoggingCategoryManager.h"
#include "QGroundControlQmlGlobal.h"
#include "RTKSettings.h"
#include "ScriptedProvider.h"
#ifndef QGC_NO_SERIAL_LINK
#include "GPSSerialPortManagerAdapter.h"
#include "SerialPortManager.h"
#endif

namespace {
GPSPositionReport fixReport(GPSFixQuality fixType)
{
    GPSPositionReport report;
    report.navigation.fixType = fixType;
    return report;
}

/// The descriptor ID of the passive family, which the settings select by role rather than by manufacturer.
const int kPassiveManufacturer = GPSRtk::manufacturerForType(GPSType::passive);

GPSRtk::Configuration receiverConfiguration(int manufacturer = GPSRtk::manufacturerForType(GPSType::ublox))
{
    GPSRtk::Configuration configuration;
    if (manufacturer == kPassiveManufacturer) {
        configuration.receiverRole = GPSRtk::Passive;
    } else {
        configuration.receiverRole = GPSRtk::ConfiguredBase;
        configuration.baseReceiverManufacturer = manufacturer;
    }
    return configuration;
}

GPSRtk::Configuration fixedConfiguration(int manufacturer = GPSRtk::manufacturerForType(GPSType::ublox))
{
    auto configuration = receiverConfiguration(manufacturer);
    configuration.baseMode = static_cast<int>(BaseModeDefinition::Mode::BaseFixed);
    configuration.fixedBasePositionLatitude = 47.5;
    configuration.fixedBasePositionLongitude = 8.25;
    configuration.fixedBasePositionAltitude = 512.0f;
    configuration.fixedBasePositionAccuracy = 1.5f;
    return configuration;
}

#ifndef QGC_NO_SERIAL_LINK
GPSRtk::Configuration serialConfiguration(int manufacturer, const QString& device, uint32_t baudRate)
{
    auto configuration = receiverConfiguration(manufacturer);
    configuration.connectionType = GPSRtk::Serial;
    configuration.serialDevice = device;
    configuration.serialBaudRate = baudRate;
    return configuration;
}
#endif

struct ScriptedRtkReceiver
{
    explicit ScriptedRtkReceiver(RuntimeScheduler* scheduler = nullptr)
        : receiver(nullptr, scheduler)
    {
        receiver.setProviderFactory(providers.providerFactory());
    }

    GPSRtk receiver;
    ScriptedProviderFactory providers;
};
}  // namespace

void GPSRtkTest::_currentBaseSaveValidity_data()
{
    QTest::addColumn<QString>("field");
    QTest::addColumn<double>("value");
    QTest::addColumn<bool>("expected");
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    QTest::newRow("known-zero-accuracy") << "currentAccuracy" << 0.0 << true;
    QTest::newRow("known-accuracy") << "currentAccuracy" << 1.25 << true;
    QTest::newRow("unavailable-accuracy") << "currentAccuracy" << nan << false;
    QTest::newRow("infinite-accuracy") << "currentAccuracy" << infinity << false;
    QTest::newRow("negative-accuracy") << "currentAccuracy" << -1.0 << false;
    QTest::newRow("wire-accuracy-overflow") << "currentAccuracy" << 429496.75 << false;
    QTest::newRow("float-accuracy-overflow") << "currentAccuracy" << 1e100 << false;
    QTest::newRow("unavailable-latitude") << "currentLatitude" << nan << false;
    QTest::newRow("invalid-latitude") << "currentLatitude" << 91.0 << false;
    QTest::newRow("unavailable-longitude") << "currentLongitude" << nan << false;
    QTest::newRow("invalid-longitude") << "currentLongitude" << -181.0 << false;
    QTest::newRow("unavailable-ellipsoid-altitude") << "currentAltitude" << nan << false;
    QTest::newRow("infinite-ellipsoid-altitude") << "currentAltitude" << infinity << false;
    QTest::newRow("wire-altitude-overflow") << "currentAltitude" << 21474838.0 << false;
}

void GPSRtkTest::_currentBaseSaveValidity()
{
    QFETCH(QString, field);
    QFETCH(double, value);
    QFETCH(bool, expected);
    GPSRTKFactGroup facts;
    facts.currentLatitude()->setRawValue(47.0);
    facts.currentLongitude()->setRawValue(8.0);
    facts.currentAltitude()->setRawValue(500.0f);
    facts.currentAccuracy()->setRawValue(1.0);
    QVERIFY(!facts.canSaveCurrentBasePosition());
    facts.valid()->setRawValue(true);
    QVERIFY(facts.canSaveCurrentBasePosition());
    QSignalSpy changes(&facts, &GPSRTKFactGroup::currentBasePositionChanged);
    Fact* const changed = facts.property(field.toUtf8().constData()).value<Fact*>();
    QVERIFY(changed);
    changed->setRawValue(value);
    QVERIFY(!changes.isEmpty());
    QCOMPARE(facts.canSaveCurrentBasePosition(), expected);
    QCOMPARE(facts.property("canSaveCurrentBasePosition").toBool(), expected);
    facts.valid()->setRawValue(false);
    QVERIFY(!facts.canSaveCurrentBasePosition());
}

void GPSRtkTest::_snapshotUsageEvidence_data()
{
    QTest::addColumn<int>("inViewValue");
    QTest::addColumn<int>("usedValue");
    QTest::addColumn<int>("expectedInView");
    QTest::addColumn<int>("expectedUsage");
    QTest::newRow("unavailable") << -1 << -1 << -1 << -1;
    QTest::newRow("count-only") << -1 << 7 << -1 << 7;
    QTest::newRow("unknown-usage") << 3 << -1 << 3 << -1;
    QTest::newRow("known-zero") << 3 << 0 << 3 << 0;
    QTest::newRow("known-used") << 3 << 2 << 3 << 2;
    QTest::newRow("empty") << 0 << 0 << 0 << 0;
}

void GPSRtkTest::_snapshotUsageEvidence()
{
    QFETCH(int, inViewValue);
    QFETCH(int, usedValue);
    QFETCH(int, expectedInView);
    QFETCH(int, expectedUsage);
    GPSSatelliteReport snapshot;
    snapshot.timestampUs = 1;
    if (inViewValue >= 0) {
        snapshot.inView = inViewValue;
    }
    if (usedValue >= 0) {
        snapshot.used = usedValue;
    }

    ScriptedRtkReceiver harness;
    auto& receiver = harness.receiver;
    QVERIFY(receiver.connectReceiver(GPSType::ublox, {}));
    auto* provider = harness.providers.current();
    QVERIFY(provider);
    QCOMPARE(receiver.status().numSatellites, -1);
    QCOMPARE(receiver.status().numSatellitesUsed, -1);
    provider->satellites(snapshot);
    QCOMPARE(receiver.status().numSatellites, expectedInView);
    QCOMPARE(receiver.status().numSatellitesUsed, expectedUsage);

    receiver.disconnectGPS();
    QCOMPARE(receiver.status().numSatellites, -1);
    QCOMPARE(receiver.status().numSatellitesUsed, -1);
}

void GPSRtkTest::_logsFixTransitionsWithoutCoordinates()
{
    ScriptedRtkReceiver harness;
    auto& receiver = harness.receiver;
    QVERIFY(receiver.connectReceiver(GPSType::ublox, {}));
    auto* provider = harness.providers.current();
    QVERIFY(provider);
    const QString category = QStringLiteral("GPS.RTK.GPSRtk");
    auto* logging = QGCLoggingCategoryManager::instance();
    const bool wasEnabled = logging->isCategoryEnabled(category);
    if (!wasEnabled) {
        logging->setCategoryEnabled(category, true);
    }
    const auto restore = qScopeGuard([logging, category, wasEnabled] {
        if (!wasEnabled) {
            logging->setCategoryEnabled(category, false);
        }
    });
    const auto initialCount = LogManager::capturedMessages(category).size();
    expectLogMessage("GPS.RTK.GPSRtk", QtDebugMsg, QRegularExpression(QStringLiteral("Receiver fix changed:")));
    provider->position(fixReport(GPSFixQuality::Fix3D));
    verifyExpectedLogMessage();
    QCOMPARE(LogManager::capturedMessages(category).size(), initialCount + 1);
    expectLogMessage("GPS.RTK.GPSRtk", QtDebugMsg, QRegularExpression(QStringLiteral("Receiver fix changed: 1")));
    provider->position(fixReport(GPSFixQuality::NoFix));
    verifyExpectedLogMessage();
    QCOMPARE(LogManager::capturedMessages(category).size(), initialCount + 2);
    receiver.disconnectGPS();
    provider->position(fixReport(GPSFixQuality::NoFix));
    QCOMPARE(LogManager::capturedMessages(category).size(), initialCount + 2);
    for (const auto& entry : LogManager::capturedMessages(category)) {
        QVERIFY(QRegularExpression(QStringLiteral("^Receiver fix changed: [0-9]+$")).match(entry.message).hasMatch());
    }
}

void GPSRtkTest::_configurationDebugRedactsFixedBaseCoordinates()
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

UT_REGISTER_TEST(GPSRtkTest, TestLabel::Unit)

void GPSRtkTest::_silentReceiverClearsSolution()
{
    ManualScheduler scheduler(nullptr, MonotonicClock::nowUs() + 1000000);
    ScriptedRtkReceiver harness(&scheduler);
    auto& receiver = harness.receiver;
    QVERIFY(receiver.connectReceiver(GPSType::ublox, {}));
    auto* provider = harness.providers.current();
    QVERIFY(provider);
    const auto& status = receiver.status();
    GPSSatelliteReport satellites;
    satellites.inView = 12;
    satellites.used = 9;
    provider->satellites(satellites);
    auto report = fixReport(GPSFixQuality::Fix3D);
    report.integrity.jamming.state = GPSIntegrityReport::JammingState::Warning;
    provider->position(report);
    QCOMPARE(status.numSatellitesUsed, 9);
    QCOMPARE(status.fixType, GPSFixQuality::Fix3D);
    QCOMPARE(status.jammingState, GPSIntegrityReport::JammingState::Warning);

    // Passive and position-only links stay connected while silent; their last solution must not linger.
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(6)));
    QCOMPARE(status.fixType, GPSFixQuality::Unknown);
    QCOMPARE(status.numSatellites, -1);
    QCOMPARE(status.numSatellitesUsed, -1);
    QCOMPARE(status.jammingState, GPSIntegrityReport::JammingState::Unknown);
}

void GPSRtkTest::_testCoreAvailableWithoutReceiver()
{
    GPSRtk rtk;
    QVERIFY(!rtk.connected());
    QVERIFY(QFile::exists(QStringLiteral(":/json/Vehicle/GPSRTKFact.json")));
    GPSRTKFactGroup facts(&rtk);
    // The status without a receiver is what the Fact metadata declares as the default.
    for (const QString& name : facts.factNames()) {
        const Fact* fact = facts.getFact(name);
        const QVariant value = fact->rawValue();
        const QVariant expected = fact->rawDefaultValue();
        QVERIFY2(value == expected || (std::isnan(value.toDouble()) && std::isnan(expected.toDouble())),
                 qPrintable(name));
    }
    auto* manager = GPSManager::instance();
    QVERIFY(manager->gpsRtkFacts());
    QCOMPARE(manager->property("gpsRtkFacts").value<GPSRTKFactGroup*>(), manager->gpsRtkFacts());
    QCOMPARE(QGroundControlQmlGlobal::staticMetaObject.indexOfProperty("gpsRtk"), -1);
}

void GPSRtkTest::_rtkSettingsBinding()
{
    RTKSettings settings;
    GPSRtk receiver;
    GPSSettingsBindings::bindRtk(&settings, &receiver);

    settings.connectionType()->setRawValue(GPSRtk::Tcp);
    settings.tcpHost()->setRawValue(QStringLiteral("rtk.example"));
    settings.tcpPort()->setRawValue(2101);
    settings.serialDevice()->setRawValue(QStringLiteral("/test/receiver"));
    settings.serialBaudRate()->setRawValue(230400);
    settings.useFixedBasePosition()->setRawValue(1);
    settings.fixedBasePositionLatitude()->setRawValue(47.5);
    settings.autoConnect()->setRawValue(true);

    const auto& configuration = receiver.configuration();
    QCOMPARE(configuration.connectionType, GPSRtk::Tcp);
    QCOMPARE(configuration.tcpHost, QStringLiteral("rtk.example"));
    QCOMPARE(configuration.tcpPort, 2101U);
    QCOMPARE(configuration.serialDevice, QStringLiteral("/test/receiver"));
    QCOMPARE(configuration.serialBaudRate, 230400U);
    QCOMPARE(configuration.baseMode, 1);
    QCOMPARE(configuration.fixedBasePositionLatitude, 47.5);
    QVERIFY(configuration.autoConnect);

    receiver.disconnectConfiguredGPS();
    QVERIFY(!settings.autoConnect()->rawValue().toBool());

    settings.baseReceiverManufacturers()->setRawValue(GPSRtk::manufacturerForType(GPSType::quectel));
    auto gate = std::make_shared<BlockedTransportGate>();
    const auto releaseWorker = qScopeGuard([&] {
        gate->release.release();
        receiver.disconnectGPS();
    });
    QVERIFY(receiver.connectReceiver(GPSType::ublox, blockedTransportFactory(gate)));
    QCOMPARE(settings.baseReceiverManufacturers()->rawValue().toInt(), GPSRtk::manufacturerForType(GPSType::ublox));
}

void GPSRtkTest::_notificationsFollowCompletedConnection_data()
{
    QTest::addColumn<QString>("phase");
    QTest::addColumn<QString>("action");
    for (const QString& phase : {QStringLiteral("manufacturer"), QStringLiteral("error-message"),
                                 QStringLiteral("receiver"), QStringLiteral("status")}) {
        for (const QString& action :
             {QStringLiteral("replace"), QStringLiteral("disconnect"), QStringLiteral("delete")}) {
            QTest::newRow(qPrintable(phase + '-' + action)) << phase << action;
        }
    }
}

void GPSRtkTest::_notificationsFollowCompletedConnection()
{
    QFETCH(QString, phase);
    QFETCH(QString, action);
    auto firstGate = std::make_shared<BlockedTransportGate>();
    auto replacementGate = std::make_shared<BlockedTransportGate>();
    auto receiver = std::make_unique<GPSRtk>();
    receiver->setConfiguration(receiverConfiguration(GPSRtk::manufacturerForType(GPSType::quectel)));
    receiver->_setError(GPSConnectionError::OpenFailed, QStringLiteral("previous failure"));
    QPointer<GPSProvider> first;
    QPointer<GPSProvider> replacement;
    QPointer<GPSRtk> deleted;
    bool handled = false;
    QObject observer;
    const auto cleanup = qScopeGuard([&] {
        for (const auto& provider : {first, replacement}) {
            if (provider) {
                provider->stop();
            }
        }
        if (receiver) {
            for (auto* provider : receiver->findChildren<GPSProvider*>()) {
                provider->stop();
            }
        }
        delete deleted.data();
        firstGate->release.release();
        replacementGate->release.release();
        if (receiver) {
            receiver->disconnectGPS();
        }
        for (const auto& provider : {first, replacement}) {
            if (provider) {
                QVERIFY(provider->wait(TestTimeout::mediumMs()));
            }
        }
    });
    const auto supersede = [&] {
        if (std::exchange(handled, true)) {
            return;
        }
        // Notifications arrive after the connection is installed.
        QVERIFY(receiver->hasReceiver());
        QCOMPARE(receiver->activeManufacturer(), GPSRtk::manufacturerForType(GPSType::ublox));
        QVERIFY(receiver->errorMessage().isEmpty());
        first = receiver->findChild<GPSProvider*>();
        QVERIFY(first);
        QVERIFY(firstGate->entered.tryAcquire(1, TestTimeout::mediumMs()));
        if (action == QStringLiteral("delete")) {
            deleted = receiver.release();
            deleted->deleteLater();
            return;
        }
        if (action == QStringLiteral("disconnect")) {
            receiver->disconnectGPS();
        } else {
            QVERIFY(receiver->connectReceiver(GPSType::passive, blockedTransportFactory(replacementGate),
                                              QStringLiteral("replacement"), 115200));
            replacement = receiver->findChild<GPSProvider*>();
            QVERIFY(replacement);
        }
        if (first) {
            QVERIFY(!first->parent());
        }
    };
    if (phase == QStringLiteral("manufacturer")) {
        connect(receiver.get(), &GPSRtk::baseManufacturerDetected, &observer, supersede);
    } else if (phase == QStringLiteral("error-message")) {
        connect(receiver.get(), &GPSRtk::errorMessageChanged, &observer, supersede);
    } else if (phase == QStringLiteral("status")) {
        connect(receiver.get(), &GPSRtk::statusChanged, &observer, supersede);
    } else {
        connect(receiver.get(), &GPSRtk::receiverChanged, &observer, supersede);
    }
    QVERIFY(receiver->connectReceiver(GPSType::ublox, blockedTransportFactory(firstGate)));
    QVERIFY(handled);
    if (action == QStringLiteral("delete")) {
        QVERIFY(deleted && deleted->hasReceiver());
        QTRY_VERIFY_WITH_TIMEOUT(!deleted, TestTimeout::shortMs());
        QVERIFY(first && !first->parent());
    } else if (action == QStringLiteral("replace")) {
        QVERIFY(receiver->hasReceiver());
        QCOMPARE(receiver->findChild<GPSProvider*>(), replacement);
        QCOMPARE(receiver->activeManufacturer(), 7);
        QVERIFY(!replacement->config().allowPersistentChanges);
        QCOMPARE(receiver->findChildren<GPSProvider*>().size(), 1);
    } else {
        QVERIFY(!receiver->hasReceiver());
        QCOMPARE(receiver->findChildren<GPSProvider*>().size(), 0);
    }
}

void GPSRtkTest::_factNotificationRetiresSession_data()
{
    QTest::addColumn<QString>("report");
    QTest::addColumn<QString>("action");
    for (const QString& report : {QStringLiteral("survey"), QStringLiteral("satellites"), QStringLiteral("ready"),
                                  QStringLiteral("disconnect"), QStringLiteral("fix")}) {
        for (const QString& action :
             {QStringLiteral("replace"), QStringLiteral("disconnect"), QStringLiteral("delete")}) {
            QTest::newRow(qPrintable(report + '-' + action)) << report << action;
        }
    }
}

void GPSRtkTest::_factNotificationRetiresSession()
{
    QFETCH(QString, report);
    QFETCH(QString, action);
    auto gate = std::make_shared<BlockedTransportGate>();
    auto replacementGate = std::make_shared<BlockedTransportGate>();
    auto receiver = std::make_unique<GPSRtk>();
    receiver->setConfiguration(receiverConfiguration());
    QPointer<GPSProvider> replacement;
    QPointer<GPSRtk> deleted;
    bool handled = false;
    QObject observer;
    QVERIFY(receiver->connectReceiver(GPSType::ublox, blockedTransportFactory(gate)));
    const QPointer<GPSProvider> first = receiver->findChild<GPSProvider*>();
    QVERIFY(first);
    const auto cleanup = qScopeGuard([&] {
        for (const auto& provider : {first, replacement}) {
            if (provider) {
                provider->stop();
            }
        }
        delete deleted.data();
        gate->release.release();
        replacementGate->release.release();
        if (receiver) {
            receiver->disconnectGPS();
        }
        for (const auto& provider : {first, replacement}) {
            if (provider) {
                QVERIFY(provider->wait(TestTimeout::mediumMs()));
            }
        }
    });
    QVERIFY(gate->entered.tryAcquire(1, TestTimeout::mediumMs()));
    GPSRTKFactGroup facts(receiver.get());
    if (report == QStringLiteral("disconnect")) {
        receiver->_onGPSConnect();
    }
    Fact* trigger = report == QStringLiteral("survey")       ? facts.currentDuration()
                    : report == QStringLiteral("satellites") ? facts.numSatellites()
                    : report == QStringLiteral("fix")        ? facts.fixType()
                                                             : facts.connected();
    connect(trigger, &Fact::rawValueChanged, &observer, [&] {
        if (std::exchange(handled, true)) {
            return;
        }
        if (action == QStringLiteral("delete")) {
            deleted = receiver.release();
            deleted->deleteLater();
        } else if (action == QStringLiteral("disconnect")) {
            receiver->disconnectGPS();
        } else {
            QVERIFY(receiver->connectReceiver(GPSType::passive, blockedTransportFactory(replacementGate), {}, 115200));
            replacement = receiver->findChild<GPSProvider*>();
            QVERIFY(replacement);
        }
    });
    if (report == QStringLiteral("survey")) {
        GPSSurveyReport survey;
        survey.duration = std::chrono::seconds(123);
        survey.valid = true;
        survey.active = true;
        survey.position = {.latitudeDegrees = 47, .longitudeDegrees = 8};
        survey.meanAccuracyMeters = 1.5;
        receiver->_onGPSSurveyReport(survey);
    } else if (report == QStringLiteral("satellites")) {
        GPSSatelliteReport satellites;
        satellites.timestampUs = 1;
        satellites.inView = 1;
        satellites.used = 1;
        receiver->_satelliteInfoUpdate(satellites);
    } else if (report == QStringLiteral("ready")) {
        receiver->_onGPSConnect();
    } else if (report == QStringLiteral("disconnect")) {
        receiver->disconnectGPS();
    } else {
        receiver->_positionUpdate(fixReport(GPSFixQuality::Fix3D));
    }
    QVERIFY(handled);
    if (action == QStringLiteral("delete")) {
        QVERIFY(deleted);
        QTRY_VERIFY_WITH_TIMEOUT(!deleted, TestTimeout::shortMs());
    }
    QVERIFY(first && !first->parent());
    if (receiver) {
        QCOMPARE(receiver->hasReceiver(), action == QStringLiteral("replace"));
        QVERIFY(!receiver->connected());
        QVERIFY(!receiver->status().valid);
        QCOMPARE(receiver->status().numSatellitesUsed, -1);
        QVERIFY(!facts.valid()->rawValue().toBool());
        QCOMPARE(facts.numSatellitesUsed()->rawValue().toInt(), -1);
        if (replacement) {
            QVERIFY(receiver->errorMessage().isEmpty());
            QCOMPARE(receiver->activeManufacturer(), 7);
        }
    }
}

void GPSRtkTest::_failedOpenNeverConnects()
{
    GPSRtk receiver;
    receiver.setConfiguration(receiverConfiguration());
    GPSRTKFactGroup facts(&receiver);
    QSignalSpy connected(facts.connected(), &Fact::rawValueChanged);
    expectLogMessage("GPS.RTK.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport")));
    receiver.connectReceiver(GPSType::ublox, {});
    QVERIFY(!receiver.connected());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
    QVERIFY(!receiver.connected());
    QVERIFY(connected.isEmpty());
    verifyExpectedLogMessage();
}

void GPSRtkTest::_receiverPublishesGcsPosition()
{
    GPSPositionService positions;
    ScriptedRtkReceiver harness;
    auto& receiver = harness.receiver;
    receiver.setPositionService(&positions);
    QVERIFY(receiver.connectReceiver(GPSType::ublox, {}, QStringLiteral("serial:test-base")));
    auto* provider = harness.providers.current();
    QVERIFY(provider);
    GPSPositionReport report = fixReport(GPSFixQuality::RTKFixed);
    report.navigation.latitudeDegrees = 47.25;
    report.navigation.longitudeDegrees = 8.5;
    report.navigation.altitudeMslMeters = 450;
    report.navigation.horizontalAccuracyMeters = 0.02f;
    provider->position(report);
    // Registration waits until the receiver is configured.
    QCOMPARE(positions.selectedSource(), GPSPositionService::SelectedSource::None);

    provider->ready();
    provider->position(report);
    QCOMPARE(positions.selectedSource(), GPSPositionService::SelectedSource::Receiver);
    QCOMPARE(positions.gcsPosition().latitude(), 47.25);
    QCOMPARE(positions.gcsPositionHorizontalAccuracy(), qreal(0.02f));
    QCOMPARE(receiver.status().fixType, GPSFixQuality::RTKFixed);

    receiver.disconnectGPS();
    QVERIFY(!positions.gcsPosition().isValid());
    QCOMPARE(positions.selectedSource(), GPSPositionService::SelectedSource::None);
    QVERIFY(!receiver.acceptedPositionObservation(GPSObservation::PositionUse::Gga));
    provider->position(report);
    QVERIFY(!receiver.acceptedPositionObservation(GPSObservation::PositionUse::Gga));
}

void GPSRtkTest::_fixedBasePositionIsGcsPosition()
{
    GPSPositionService positions;
    ScriptedRtkReceiver harness;
    auto& receiver = harness.receiver;
    auto configuration = receiverConfiguration();
    configuration.baseMode = static_cast<int>(BaseModeDefinition::Mode::BaseFixed);
    configuration.fixedBasePositionLatitude = 47.5;
    configuration.fixedBasePositionLongitude = 8.25;
    configuration.fixedBasePositionAltitude = 480.0f;
    configuration.fixedBasePositionAccuracy = 0.0f;
    receiver.setConfiguration(configuration);
    receiver.setPositionService(&positions);
    QVERIFY(receiver.connectReceiver(GPSType::ublox, {}));
    auto* provider = harness.providers.current();
    QVERIFY(provider);
    provider->ready();
    // Fixed-mode receivers report only a time fix.
    provider->position(fixReport(GPSFixQuality::NoFix));
    QCOMPARE(positions.selectedSource(), GPSPositionService::SelectedSource::Receiver);
    QCOMPARE(positions.gcsPosition(), QGeoCoordinate(47.5, 8.25));
    QCOMPARE(positions.gcsPositionHorizontalAccuracy(), 0.01);
    const auto remoteId = receiver.acceptedPositionObservation(GPSObservation::PositionUse::RemoteID);
    QVERIFY(remoteId);
    QCOMPARE(remoteId->altitudeDatum, GPSAltitudeDatum::Ellipsoid);
    QCOMPARE(remoteId->position.coordinate().altitude(), 480.0);
    const auto gga = receiver.acceptedPositionObservation(GPSObservation::PositionUse::Gga);
    QVERIFY(gga);
    QVERIFY(gga->altitudeDatum != GPSAltitudeDatum::MeanSeaLevel);
    QCOMPARE(receiver.status().fixType, GPSFixQuality::NoFix);
    receiver.disconnectGPS();
    QVERIFY(!positions.gcsPosition().isValid());
}

void GPSRtkTest::_receiverIntegrityFacts()
{
    ScriptedRtkReceiver harness;
    auto& receiver = harness.receiver;
    QVERIFY(receiver.connectReceiver(GPSType::ublox, {}));
    auto* provider = harness.providers.current();
    QVERIFY(provider);
    GPSRTKFactGroup facts(&receiver);
    GPSPositionReport report = fixReport(GPSFixQuality::Fix3D);
    report.integrity.jamming.state = GPSIntegrityReport::JammingState::Warning;
    report.integrity.spoofing.state = GPSIntegrityReport::SpoofingState::Indicated;
    provider->position(report);
    QCOMPARE(facts.jammingState()->rawValue().toInt(), 2);
    QCOMPARE(facts.spoofingState()->rawValue().toInt(), 2);
    QCOMPARE(facts.jammingState()->enumStringValue(), QStringLiteral("Warning"));
    QVERIFY(facts.interferenceWarning());
    report.integrity = {};
    provider->position(report);
    QCOMPARE(facts.jammingState()->rawValue().toInt(), 0);
    QVERIFY(!facts.interferenceWarning());
    report.integrity.jamming.state = GPSIntegrityReport::JammingState::Ok;
    report.integrity.spoofing.state = GPSIntegrityReport::SpoofingState::None;
    provider->position(report);
    QVERIFY(!facts.interferenceWarning());
    QSignalSpy interference(&facts, &GPSRTKFactGroup::interferenceWarningChanged);
    report.integrity.jamming.state = GPSIntegrityReport::JammingState::Critical;
    provider->position(report);
    QVERIFY(facts.interferenceWarning());
    QVERIFY(!interference.isEmpty());
    receiver.disconnectGPS();
    QCOMPARE(facts.jammingState()->rawValue().toInt(), 0);
    QCOMPARE(facts.spoofingState()->rawValue().toInt(), 0);
    QVERIFY(!facts.interferenceWarning());
}

void GPSRtkTest::_surveyedBasePositionIsGcsPosition()
{
    GPSPositionService positions;
    ScriptedRtkReceiver harness;
    auto& receiver = harness.receiver;
    auto configuration = receiverConfiguration();
    configuration.baseMode = static_cast<int>(BaseModeDefinition::Mode::BaseSurveyIn);
    configuration.surveyInAccuracyLimit = 2.0;
    receiver.setConfiguration(configuration);
    receiver.setPositionService(&positions);
    QVERIFY(receiver.connectReceiver(GPSType::ublox, {}));
    auto* provider = harness.providers.current();
    QVERIFY(provider);
    const auto deliver = [&](const GPSPositionReport& report) { provider->position(report); };
    GPSPositionReport navigating = fixReport(GPSFixQuality::Fix3D);
    navigating.navigation.latitudeDegrees = 10;
    navigating.navigation.longitudeDegrees = 20;
    navigating.navigation.horizontalAccuracyMeters = 3;
    provider->ready();
    deliver(navigating);
    QCOMPARE(positions.gcsPosition().latitude(), 10.0);

    GPSSurveyReport survey;
    survey.active = false;
    survey.valid = true;
    survey.position = {.latitudeDegrees = 11, .longitudeDegrees = 21, .altitudeMeters = 400};
    survey.meanAccuracyMeters = 1.5;
    provider->survey(survey);
    deliver(fixReport(GPSFixQuality::NoFix));
    QCOMPARE(positions.gcsPosition(), QGeoCoordinate(11, 21));
    QCOMPARE(positions.gcsPositionHorizontalAccuracy(), 1.5);

    survey.meanAccuracyMeters.reset();
    provider->survey(survey);
    deliver(fixReport(GPSFixQuality::NoFix));
    QCOMPARE(positions.gcsPositionHorizontalAccuracy(), 2.0);

    survey.valid = false;
    survey.active = true;
    provider->survey(survey);
    deliver(fixReport(GPSFixQuality::NoFix));
    QVERIFY(!positions.gcsPosition().isValid());
}

void GPSRtkTest::_retiredWorkerCannotUpdateReplacement()
{
    auto firstGate = std::make_shared<BlockedTransportGate>();
    auto secondGate = std::make_shared<BlockedTransportGate>();
    GPSCorrectionManager corrections;
    GPSRtk receiver;
    receiver.setConfiguration(receiverConfiguration());
    receiver.setCorrectionManager(&corrections);
    QSignalSpy routed(&corrections.router(), &GPSCorrectionRouter::frameRouted);
    const auto releaseWorkers = qScopeGuard([&]() {
        firstGate->release.release();
        secondGate->release.release();
    });
    receiver.connectReceiver(GPSType::ublox, blockedTransportFactory(firstGate), QStringLiteral("serial:test-base"));
    QTRY_VERIFY_WITH_TIMEOUT(firstGate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> first = receiver.findChild<GPSProvider*>();
    QVERIFY(first);
    const auto& status = receiver.status();
    QVERIFY(!receiver.connected());
    emit first->receiverReady();
    GPSSurveyReport survey{};
    survey.valid = true;
    survey.active = true;
    survey.position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500};
    survey.duration = std::chrono::seconds(4294967295LL);
    survey.meanAccuracyMeters = 1.5;
    emit first->surveyInStatus(survey);
    GPSSatelliteReport satellites;
    satellites.timestampUs = 1;
    satellites.inView = 2;
    satellites.used = 7;
    emit first->satelliteInfoUpdate(satellites);
    QTRY_VERIFY_WITH_TIMEOUT(receiver.connected(), TestTimeout::shortMs());
    QVERIFY(status.valid);
    QCOMPARE(status.currentLatitude, 47.0);
    QCOMPARE(status.currentLongitude, 8.0);
    QCOMPARE(status.currentAltitude, 500.0f);
    QCOMPARE(status.currentAccuracy, 1.5);
    QCOMPARE(status.currentDuration.count(), 4294967295LL);
    QCOMPARE(status.numSatellites, 2);
    QCOMPARE(status.numSatellitesUsed, 7);
    const auto frame = GpsTestHelpers::buildRtcmFrame(1005);
    emit first->RTCMDataUpdate(frame, GPSCorrectionFrame::monotonicNowMs());
    QTRY_COMPARE_WITH_TIMEOUT(routed.size(), 1, TestTimeout::shortMs());
    const auto original = qvariant_cast<GPSCorrectionFrame>(routed[0][0]);
    QCOMPARE(original.source, GPSCorrectionSource::LocalReceiver);
    QCOMPARE(original.sourceInstance, QStringLiteral("serial:test-base"));
    QVERIFY(original.validated);
    auto* rtcm = corrections.rtcmMavlink();
    const auto bytesBefore = rtcm->totalBytesSent();
    QCOMPARE(bytesBefore, quint64(frame.size()));
    // Retirement must reject callbacks already in the GUI queue.
    satellites.used = 12;
    emit first->RTCMDataUpdate(frame, GPSCorrectionFrame::monotonicNowMs());
    emit first->surveyInStatus(survey);
    emit first->satelliteInfoUpdate(satellites);
    emit first->positionUpdate(fixReport(GPSFixQuality::Unknown));
    emit first->receiverReady();
    emit first->connectionError(GPSConnectionError::ConfigFailed,
                                QStringLiteral("Retired receiver configuration failure"));
    emit first->connectionError(GPSConnectionError::DeviceError);
    receiver.connectReceiver(GPSType::ublox, blockedTransportFactory(secondGate), QStringLiteral("serial:test-base"));
    QVERIFY(!receiver.connected());
    QVERIFY(!status.valid);
    QVERIFY(!status.active);
    QVERIFY(qIsNaN(status.currentLatitude));
    QVERIFY(qIsNaN(status.currentAccuracy));
    QCOMPARE(status.currentDuration.count(), 0);
    QCOMPARE(status.numSatellites, -1);
    QCOMPARE(status.numSatellitesUsed, -1);
    QTRY_VERIFY_WITH_TIMEOUT(secondGate->entered.available() > 0, TestTimeout::mediumMs());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(!receiver.connected());
    QCOMPARE(status.numSatellitesUsed, -1);
    QVERIFY(receiver.errorMessage().isEmpty());
    QCOMPARE(rtcm->totalBytesSent(), bytesBefore);
    auto* activeProvider = receiver.findChild<GPSProvider*>();
    QVERIFY(activeProvider);
    emit activeProvider->receiverReady();
    const auto receivedAtMs = GPSCorrectionFrame::monotonicNowMs() - 10;
    emit activeProvider->RTCMDataUpdate(frame, receivedAtMs);
    QTRY_VERIFY_WITH_TIMEOUT(receiver.connected(), TestTimeout::shortMs());
    QCOMPARE(rtcm->totalBytesSent(), bytesBefore + frame.size());
    QCOMPARE(routed.size(), 2);
    const auto replacement = qvariant_cast<GPSCorrectionFrame>(routed[1][0]);
    QVERIFY(replacement.session != original.session);
    QCOMPARE(replacement.sourceInstance, original.sourceInstance);
    QCOMPARE(replacement.receivedAtMs, receivedAtMs);
    firstGate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(first.isNull(), TestTimeout::mediumMs());
    QVERIFY(firstGate->sawCancellation);
    QVERIFY(receiver.connected());

    expectLogMessage("GPS.RTK.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("GPS device error, connection lost")));
    const QPointer<GPSProvider> second = receiver.findChild<GPSProvider*>();
    QVERIFY(second);
    emit second->connectionError(GPSConnectionError::DeviceError);
    emit second->RTCMDataUpdate(frame, GPSCorrectionFrame::monotonicNowMs());
    emit second->receiverReady();
    emit second->surveyInStatus(survey);
    emit second->satelliteInfoUpdate(satellites);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    verifyExpectedLogMessage();
    QVERIFY(!receiver.connected());
    QCOMPARE(status.currentDuration.count(), 0);
    QCOMPARE(status.numSatellites, -1);
    QCOMPARE(status.numSatellitesUsed, -1);
    QVERIFY(corrections.sourceInstances().isEmpty());
    QCOMPARE(routed.size(), 2);
    QCOMPARE(rtcm->totalBytesSent(), bytesBefore + frame.size());

    secondGate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(second.isNull(), TestTimeout::mediumMs());
    QVERIFY(secondGate->sawCancellation);
    QVERIFY(!receiver.connected());
    QVERIFY(corrections.sourceInstances().isEmpty());
}

void GPSRtkTest::_workerCanOutliveManager()
{
    auto gate = std::make_shared<BlockedTransportGate>();
    auto receiver = std::make_unique<GPSRtk>();
    receiver->setConfiguration(receiverConfiguration());
    const auto releaseWorker = qScopeGuard([&]() { gate->release.release(); });
    receiver->connectReceiver(GPSType::ublox, blockedTransportFactory(gate));
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> provider = receiver->findChild<GPSProvider*>();
    QVERIFY(provider);
    emit provider->receiverReady();
    QTRY_VERIFY_WITH_TIMEOUT(receiver->connected(), TestTimeout::shortMs());
    receiver.reset();
    QVERIFY(provider);
    QVERIFY(!provider->parent());
    gate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(provider.isNull(), TestTimeout::mediumMs());
    QVERIFY(gate->sawCancellation);
}

void GPSRtkTest::_receiverFramesAreValidated_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<bool>("expired");
    const auto good = GpsTestHelpers::buildRtcmFrame(1005);
    auto badCrc = good;
    badCrc.back() ^= 1;
    auto badHeader = good;
    badHeader[1] |= 0x80;
    QTest::newRow("valid") << good << true << false;
    QTest::newRow("bad-crc") << badCrc << false << false;
    QTest::newRow("reserved-header-bits") << badHeader << false << false;
    QTest::newRow("truncated") << good.first(good.size() - 1) << false << false;
    QTest::newRow("unframed") << QByteArrayLiteral("corrections") << false << false;
    QTest::newRow("expired-before-dequeue") << good << true << true;
}

void GPSRtkTest::_receiverFramesAreValidated()
{
    QFETCH(QByteArray, frame);
    QFETCH(bool, valid);
    QFETCH(bool, expired);
    GPSCorrectionManager corrections;
    ScriptedRtkReceiver harness;
    auto& receiver = harness.receiver;
    receiver.setCorrectionManager(&corrections);
    QVERIFY(receiver.connectReceiver(GPSType::ublox, {}));
    auto* provider = harness.providers.current();
    QVERIFY(provider);
    const auto receivedAtMs =
        GPSCorrectionFrame::monotonicNowMs() - (expired ? GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS : 0);
    QSignalSpy routed(&corrections.router(), &GPSCorrectionRouter::frameRouted);
    provider->rtcm(frame, receivedAtMs);
    const auto stats = corrections.sourceDiagnostics()[static_cast<int>(GPSCorrectionSource::LocalReceiver)];
    QCOMPARE(stats.receivedFrames, 1);
    QCOMPARE(stats.validatedFrames, valid ? 1 : 0);
    QCOMPARE(routed.size(), valid && !expired ? 1 : 0);
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSent(), valid && !expired ? quint64(frame.size()) : 0);
    if (!routed.isEmpty()) {
        const auto correction = qvariant_cast<GPSCorrectionFrame>(routed[0][0]);
        QCOMPARE(correction.data, frame);
        QCOMPARE(correction.messageId, 1005);
        QVERIFY(correction.validated);
        QCOMPARE(correction.receivedAtMs, receivedAtMs);
    }
}

void GPSRtkTest::_runtimeSettingsDoNotRequireAppRestart_data()
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

void GPSRtkTest::_runtimeSettingsDoNotRequireAppRestart()
{
    QFETCH(QString, name);
    RTKSettings settings;
    auto* fact = settings.property(name.toUtf8().constData()).value<Fact*>();
    QVERIFY(fact);
    QVERIFY(!fact->qgcRebootRequired());
    QVERIFY(!fact->vehicleRebootRequired());
}

void GPSRtkTest::_compactCorrectionsFollowReceiverSupport()
{
    auto configuration = receiverConfiguration();
    configuration.baseMode = static_cast<int>(BaseModeDefinition::Mode::BaseSurveyIn);
    configuration.surveyInAccuracyLimit = 2.0;
    configuration.surveyInMinObservationDuration = 60;
    for (const bool compact : {false, true}) {
        configuration.compactRtcmCorrections = compact;
        GPSReceiverConfig config;
        QVERIFY(GPSRtk::_receiverConfig(GPSType::ublox, configuration, 115200, config).isEmpty());
        QCOMPARE(config.base.compactObservations, compact);
        // Receivers without MSM4 support ignore the hidden option instead of failing to connect.
        QVERIFY(GPSRtk::_receiverConfig(GPSType::septentrio, configuration, 115200, config).isEmpty());
        QVERIFY(!config.base.compactObservations);
    }
}

void GPSRtkTest::_udpPositionOnlyReceiver()
{
    QUdpSocket probe;
    QVERIFY(probe.bind(QHostAddress::LocalHost, 0));
    const quint16 port = probe.localPort();
    probe.close();
    GPSCorrectionManager corrections;
    GPSRtk receiver;
    auto configuration = receiverConfiguration(kPassiveManufacturer);
    configuration.receiverRole = GPSRtk::PositionOnly;
    configuration.connectionType = GPSRtk::Udp;
    configuration.udpPort = port;
    receiver.setConfiguration(configuration);
    receiver.setCorrectionManager(&corrections);
    QVERIFY(receiver.connectConfiguredGPS());
    QCOMPARE(receiver.activeRole(), GPSRtk::PositionOnly);
    QCOMPARE(receiver.activeEndpoint(), QStringLiteral("UDP port %1").arg(port));
    // A silent position-only link waits for data instead of reconnecting.
    auto* provider = receiver.findChild<GPSProvider*>();
    QVERIFY(provider);
    QVERIFY(!provider->endsWhenIdle());
    QTRY_VERIFY_WITH_TIMEOUT(receiver.connected(), TestTimeout::mediumMs());

    QByteArray stream;
    for (const QByteArray& body : {QByteArray("$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A"),
                                   QByteArray("$GPGST,092750.000,1,1,1,0,1,1,2"),
                                   QByteArray("$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,")}) {
        stream += NMEAUtils::repairChecksum(body);
    }
    stream += GpsTestHelpers::buildRtcmFrame(1005, 20);
    QUdpSocket sender;
    QTRY_VERIFY_WITH_TIMEOUT(sender.writeDatagram(stream, QHostAddress::LocalHost, port) == stream.size() &&
                                 receiver.acceptedPositionObservation(GPSObservation::PositionUse::GroundStation),
                             TestTimeout::mediumMs());
    const auto observation = receiver.acceptedPositionObservation(GPSObservation::PositionUse::Gga);
    QVERIFY(observation);
    QCOMPARE(observation->altitudeDatum, GPSAltitudeDatum::MeanSeaLevel);
    // Position-only receivers never contribute corrections.
    QVERIFY(corrections.sourceInstances().isEmpty());
    QCOMPARE(corrections.sourceDiagnostics()[static_cast<int>(GPSCorrectionSource::LocalReceiver)].validatedFrames,
             0ULL);
    receiver.disconnectConfiguredGPS();
    QVERIFY(!receiver.hasReceiver());
}

void GPSRtkTest::_manufacturerIds_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<int>("receiverType");
    QTest::newRow("all-is-not-a-receiver") << 0 << -1;
    QTest::newRow("trimble") << 1 << 1;
    QTest::newRow("septentrio") << 2 << 2;
    QTest::newRow("femtomes") << 3 << 3;
    QTest::newRow("ublox") << 4 << 0;
    QTest::newRow("unicore") << 5 << 4;
    QTest::newRow("quectel") << 6 << 5;
    QTest::newRow("passive") << 7 << 6;
    QTest::newRow("invalid") << 8 << -1;
}

void GPSRtkTest::_manufacturerIds()
{
    QFETCH(int, manufacturer);
    QFETCH(int, receiverType);
    RTKSettings settings;
    GPSRtk receiver;
    const auto type = GPSRtk::typeForManufacturer(manufacturer);
    const auto values = settings.baseReceiverManufacturers()->enumValues();
    // The passive family is selected by role, so it is not a settings manufacturer.
    QCOMPARE(values.size(), 7);
    for (int id = 0; id < values.size(); ++id) {
        QCOMPARE(values[id].toInt(), id);
    }
    QCOMPARE(type.has_value(), receiverType >= 0);
    if (type) {
        QCOMPARE(static_cast<int>(*type), receiverType);
        QCOMPARE(GPSRtk::manufacturerForType(*type), manufacturer);
    }
    const auto caps = receiver.capabilitiesFor(
        manufacturer == kPassiveManufacturer ? GPSRtk::Passive : GPSRtk::ConfiguredBase, manufacturer);
    QCOMPARE(caps.recognized, manufacturer < 8);
    QCOMPARE(caps.passive, manufacturer == kPassiveManufacturer);
    QCOMPARE(caps.rtkBase, manufacturer < 7);
    QCOMPARE(caps.receiverAveraging, manufacturer == 0 || manufacturer == 5);
    QCOMPARE(caps.surveyIn, manufacturer < 7 && manufacturer != 5);
}

void GPSRtkTest::_receiverSettingsMapping_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<int>("baseMode");
    QTest::addColumn<bool>("accepted");
    for (const int manufacturer : {4, 5, 6, 7}) {
        for (const int mode : {0, 1, 2}) {
            const bool accepted = manufacturer == 7 || mode == 1 || (mode == 2 ? manufacturer == 5 : manufacturer != 5);
            QTest::newRow(qPrintable(QStringLiteral("receiver-%1-mode-%2").arg(manufacturer).arg(mode)))
                << manufacturer << mode << accepted;
        }
    }
}

void GPSRtkTest::_receiverSettingsMapping()
{
    QFETCH(int, manufacturer);
    QFETCH(int, baseMode);
    QFETCH(bool, accepted);
    auto configuration = receiverConfiguration(manufacturer);
    configuration.baseMode = baseMode;
    configuration.surveyInAccuracyLimit = 1.75;
    configuration.surveyInMinObservationDuration = 195;
    configuration.receiverAveragingDuration = 321;
    configuration.fixedBasePositionLatitude = 47.5;
    configuration.fixedBasePositionLongitude = 8.25;
    configuration.fixedBasePositionAltitude = 512.0f;
    configuration.fixedBasePositionAccuracy = 1.5f;
    GPSReceiverConfig config;
    const auto type = GPSRtk::typeForManufacturer(manufacturer);
    QVERIFY(type);
    const auto error = GPSRtk::_receiverConfig(*type, configuration, 230400, config);
    QCOMPARE(error.isEmpty(), accepted);
    QCOMPARE(config.baudRate, uint32_t(230400));
    QVERIFY(!config.allowPersistentChanges);
    if (manufacturer == 7) {
        QCOMPARE(config.role, GPSReceiverConfig::Role::Passive);
        QCOMPARE(config.base, GPSBaseStationConfig{});
    } else {
        QCOMPARE(config.role, GPSReceiverConfig::Role::RTKBase);
        QCOMPARE(std::holds_alternative<GPSBaseStationConfig::Fixed>(config.base.mode), baseMode == 1);
        if (baseMode == 1) {
            const auto& fixed = std::get<GPSBaseStationConfig::Fixed>(config.base.mode);
            QCOMPARE(fixed.position.latitudeDegrees, 47.5);
            QCOMPARE(fixed.position.longitudeDegrees, 8.25);
            QCOMPARE(fixed.position.altitudeMeters, 512.0f);
            QCOMPARE(fixed.accuracyMeters, 1.5f);
        } else if (baseMode == 2) {
            QVERIFY(std::holds_alternative<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode));
            QCOMPARE(std::get<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode).maximumDurationSecs,
                     uint32_t(321));
        } else {
            QVERIFY(std::holds_alternative<GPSBaseStationConfig::SurveyIn>(config.base.mode));
            const auto& survey = std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode);
            QCOMPARE(survey.accuracyMeters, 1.75);
            QCOMPARE(survey.durationSecs, int64_t(195));
        }
    }
}

void GPSRtkTest::_invalidReceiverSettings_data()
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

void GPSRtkTest::_invalidReceiverSettings()
{
    QFETCH(int, manufacturer);
    QFETCH(int, mode);
    QFETCH(uint, averagingDuration);
    QFETCH(uint, baud);
    QFETCH(bool, accepted);
    auto configuration = receiverConfiguration(manufacturer);
    configuration.baseMode = mode;
    configuration.receiverAveragingDuration = averagingDuration;
    GPSReceiverConfig config;
    const auto type = GPSRtk::typeForManufacturer(manufacturer);
    QVERIFY(type);
    QCOMPARE(GPSRtk::_receiverConfig(*type, configuration, baud, config).isEmpty(), accepted);
    RTKSettings settings;
    QCOMPARE(settings.receiverAveragingDuration()->rawMin().toUInt(), 1U);
    QCOMPARE(settings.receiverAveragingDuration()->rawMax().toUInt(), 3600U);
    QCOMPARE(settings.useFixedBasePosition()->enumValues(), (QVariantList{0, 1, 2}));
    if (!accepted) {
        GPSRtk receiver;
        receiver.setConfiguration(configuration);
        bool opened = false;
        QVERIFY(!receiver.connectReceiver(
            *type,
            [&opened](const std::atomic_bool&) {
                opened = true;
                return std::unique_ptr<GPSTransport>{};
            },
            {}, baud));
        QVERIFY(!opened);
        QVERIFY(!receiver.hasReceiver());
        QVERIFY(!receiver.errorMessage().isEmpty());
    }
}

#ifndef QGC_NO_SERIAL_LINK

void GPSRtkTest::_serialReservationSurvivesDelayedStop()
{
    SerialPortManager ports(nullptr, [] { return QList<SerialPortManager::Port>{}; });
    GPSSerialPortManagerAdapter serialPorts(&ports);
    auto gate = std::make_shared<BlockedTransportGate>();
    GPSRtk receiver;
    receiver.setConfiguration(receiverConfiguration(kPassiveManufacturer));
    receiver.setSerialPorts(&serialPorts);
    receiver._serialTransportFactory = [gate](const QString&, const std::atomic_bool& stop) {
        return blockedTransportFactory(gate)(stop);
    };
    const auto releaseWorker = qScopeGuard([&] { gate->release.release(); });
    QVERIFY(receiver.connectSerial(QStringLiteral("/test/selected"), GPSType::passive, 115200, false));
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> provider = receiver.findChild<GPSProvider*>();
    QVERIFY(provider);
    emit provider->receiverReady();
    QTRY_VERIFY_WITH_TIMEOUT(receiver.connected(), TestTimeout::shortMs());
    bool heartbeat = false;
    QTimer::singleShot(0, &receiver, [&heartbeat] { heartbeat = true; });
    QElapsedTimer retirementTime;
    retirementTime.start();
    receiver.disconnectGPS();
    QVERIFY2(retirementTime.elapsed() < 500, "Retiring a worker must not wait for its transport on the GUI thread");
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(!receiver.connected());
    QTRY_VERIFY_WITH_TIMEOUT(heartbeat, TestTimeout::shortMs());
    QVERIFY(provider && provider->isRunning());
    QVERIFY(ports.isPortReserved(QStringLiteral("/test/selected")));
    gate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(provider.isNull(), TestTimeout::mediumMs());
    QVERIFY(ports.canReservePort(QStringLiteral("/test/selected")));
}

namespace {
struct PassiveTransportState
{
    std::atomic_uint baud = 0;
    std::atomic_uint baudChanges = 0;
    std::atomic_uint writes = 0;
    QSemaphore reading;
    QSemaphore releaseRead;
};

std::unique_ptr<GPSTransport> makePassiveTestTransport(const std::atomic_bool& stop,
                                                       const std::shared_ptr<PassiveTransportState>& state)
{
    auto transport = std::make_unique<ScriptedReceiver>(stop);
    transport->setBaudrateHandler([state](unsigned baud) -> std::optional<bool> {
        state->baud = baud;
        ++state->baudChanges;
        return true;
    });
    transport->setReadHandler([state](uint8_t*, int, int) -> std::optional<GPSReadResult> {
        state->reading.release();
        state->releaseRead.acquire();
        return GPSReadResult{GPSReadStatus::Cancelled};
    });
    transport->setWriteHandler(
        [state](const QByteArray&, const ScriptedReceiver::WriteContext&) -> std::optional<GPSWriteResult> {
            ++state->writes;
            return GPSWriteResult{GPSWriteStatus::Error};
        });
    return transport;
}
}  // namespace

void GPSRtkTest::_manualPassiveBaudPreserved_data()
{
    QTest::addColumn<uint>("baud");
    QTest::newRow("minimum") << 1200U;
    QTest::newRow("usb-serial") << 230400U;
    QTest::newRow("maximum") << 4000000U;
}

void GPSRtkTest::_manualPassiveBaudPreserved()
{
    QFETCH(uint, baud);
    SerialPortManager ports(nullptr, [] {
        return QList<SerialPortManager::Port>{
            {QStringLiteral("/test/passive"), QStringLiteral("passive"), QGCSerialPortInfo::BoardTypeUnknown, {}}};
    });
    GPSSerialPortManagerAdapter serialPorts(&ports);
    auto state = std::make_shared<PassiveTransportState>();
    GPSRtk receiver;
    receiver.setConfiguration(serialConfiguration(kPassiveManufacturer, QStringLiteral("/test/passive"), baud));
    receiver.setSerialPorts(&serialPorts);
    receiver._serialTransportFactory = [state](const QString&, const std::atomic_bool& stop) {
        return makePassiveTestTransport(stop, state);
    };
    const auto releaseWorker = qScopeGuard([&] { state->releaseRead.release(); });
    QVERIFY(receiver.connectSerial(QStringLiteral("/test/passive"), GPSType::passive, baud, false));
    QTRY_VERIFY_WITH_TIMEOUT(receiver.connected() && state->reading.available() > 0, TestTimeout::mediumMs());
    QCOMPARE(state->baud.load(), baud);
    QCOMPARE(state->baudChanges.load(), 1U);
    QCOMPARE(state->writes.load(), 0U);
    QVERIFY(!receiver.status().active);
    QVERIFY(!receiver.status().valid);
    QVERIFY(ports.isPortReserved(QStringLiteral("/test/passive")));
    state->releaseRead.release();
    receiver.disconnectConfiguredGPS();
    QVERIFY(!receiver.hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(ports.canReservePort(QStringLiteral("/test/passive")), TestTimeout::mediumMs());
}
#endif

void GPSRtkTest::_persistentConsentMapping_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<bool>("allowPersistentChanges");
    for (const int manufacturer : {4, 5, 6, 7}) {
        for (const bool allow : {false, true}) {
            QTest::newRow(qPrintable(QStringLiteral("receiver-%1-consent-%2").arg(manufacturer).arg(allow)))
                << manufacturer << allow;
        }
    }
}

void GPSRtkTest::_persistentConsentMapping()
{
    QFETCH(int, manufacturer);
    QFETCH(bool, allowPersistentChanges);
    const auto configuration = fixedConfiguration(manufacturer);
    const auto type = GPSRtk::typeForManufacturer(manufacturer);
    QVERIFY(type);
    GPSReceiverConfig config;
    const auto diagnostic = GPSRtk::_receiverConfig(*type, configuration, 115200, config, allowPersistentChanges);
    QCOMPARE(diagnostic.isEmpty(), !allowPersistentChanges || manufacturer == 6);
    QCOMPARE(config.allowPersistentChanges, allowPersistentChanges);
    QVERIFY(GPSRtk::_receiverConfig(*type, configuration, 115200, config).isEmpty());
    QVERIFY(!config.allowPersistentChanges);
}

void GPSRtkTest::_configurationDiagnosticRetained_data()
{
    QTest::addColumn<QString>("session");
    QTest::addColumn<QString>("detail");
    QTest::addColumn<QString>("expected");

    const struct
    {
        const char* name;
        QString detail;
    } cases[] = {{"provisioning-mismatch", QStringLiteral("Requested base settings differ from receiver readback.")},
                 {"possibly-persisted", QStringLiteral("Settings may have been saved, but reconnect failed.")},
                 {"empty-fallback", QString()}};

    for (const auto& [name, detail] : cases) {
        const QString reported =
            detail.isEmpty()
                ? GPSRtk::tr("Receiver configuration failed. Check the receiver type, baud rate, and base mode.")
                : GPSRtk::tr("Receiver configuration failed: %1").arg(detail);
        // Only a manual connection that reached the receiver once is retried.
        const QString retried =
            detail.isEmpty() ? GPSRtk::tr("Receiver connection lost. Reconnecting automatically.")
                             : GPSRtk::tr("Receiver configuration failed: %1. Reconnecting automatically.").arg(detail);
        QTest::addRow("direct-%s", name) << QStringLiteral("direct") << detail << reported;
        QTest::addRow("first-attempt-%s", name) << QStringLiteral("first-attempt") << detail << reported;
        QTest::addRow("reconnecting-%s", name) << QStringLiteral("reconnecting") << detail << retried;
    }
}

void GPSRtkTest::_configurationDiagnosticRetained()
{
    QFETCH(QString, session);
    QFETCH(QString, detail);
    QFETCH(QString, expected);
    const bool direct = session == QStringLiteral("direct");
    const bool reconnecting = session == QStringLiteral("reconnecting");
    ScriptedRtkReceiver harness;
    auto& receiver = harness.receiver;
    auto configuration = receiverConfiguration(GPSRtk::manufacturerForType(GPSType::quectel));
    if (!direct) {
        configuration.connectionType = GPSRtk::Tcp;
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
    QSignalSpy messages(&receiver, &GPSRtk::errorMessageChanged);
    expectLogMessage("GPS.RTK.GPSRtk", QtWarningMsg,
                     QRegularExpression(reconnecting ? QStringLiteral("GPS receiver session ended")
                                                     : QStringLiteral("GPS receiver did not accept configuration")));
    provider->fail(GPSConnectionError::ConfigFailed, detail);
    verifyExpectedLogMessage();
    QCOMPARE(messages.size(), 1);
    QVERIFY(!receiver.connected());
    QCOMPARE(receiver.reconnecting(), reconnecting);
    QCOMPARE(receiver.errorMessage(), expected);
}
