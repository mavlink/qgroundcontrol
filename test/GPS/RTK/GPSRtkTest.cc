#include "GPSRtkTest.h"

#include <limits>
#include <utility>

#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlExpression>
#include <QtTest/QSignalSpy>

#include "AutoConnectSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSManager.h"
#include "GPSRTKFactGroup.h"
#include "GPSRtk.h"
#include "GPSTransport.h"
#include "GpsTestHelpers.h"
#include "LogManager.h"
#include "QGCLoggingCategoryManager.h"
#include "QGroundControlQmlGlobal.h"
#include "RTCMMavlink.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

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

void GPSRtkTest::_testCountSatellitesClampsToMax()
{
    GPSSatelliteReport msg;
    msg.count = 250;

    const GPSRtk::SatelliteCounts counts = GPSRtk::countSatellites(msg);

    QCOMPARE(counts.inView, GPSSatelliteReport::MAX_SATELLITES);
    QVERIFY(!counts.used);
}

void GPSRtkTest::_testCountSatellitesCountsUsed()
{
    GPSSatelliteReport msg;
    msg.count = 6;
    for (uint16_t i = 0; i < msg.count; ++i) {
        msg.satellites[i].used = false;
    }
    msg.satellites[1].used = true;
    msg.satellites[3].used = true;
    msg.satellites[5].used = true;

    const GPSRtk::SatelliteCounts counts = GPSRtk::countSatellites(msg);

    QCOMPARE(static_cast<int>(counts.inView), 6);
    QCOMPARE(counts.used, std::optional<int>{3});
}

void GPSRtkTest::_testCountSatellitesIgnoresUsedBeyondCount()
{
    GPSSatelliteReport msg;
    msg.count = 2;
    msg.satellites[0].used = true;
    msg.satellites[1].used = false;
    msg.satellites[5].used = true;

    const GPSRtk::SatelliteCounts counts = GPSRtk::countSatellites(msg);

    QCOMPARE(static_cast<int>(counts.inView), 2);
    QCOMPARE(counts.used, std::optional<int>{1});
}

void GPSRtkTest::_snapshotUsageEvidence_data()
{
    QTest::addColumn<int>("count");
    QTest::addColumn<QList<int>>("flags");
    QTest::addColumn<int>("expectedUsage");
    QTest::newRow("all-unknown") << 3 << QList<int>{-1, -1, -1} << -1;
    QTest::newRow("partially-known") << 3 << QList<int>{0, 1, -1} << -1;
    QTest::newRow("known-zero") << 3 << QList<int>{0, 0, 0} << 0;
    QTest::newRow("known-used") << 3 << QList<int>{1, 0, 1} << 2;
    QTest::newRow("empty-clears") << 0 << QList<int>{} << 0;
}

void GPSRtkTest::_snapshotUsageEvidence()
{
    QFETCH(int, count);
    QFETCH(QList<int>, flags);
    QFETCH(int, expectedUsage);
    GPSSatelliteReport snapshot;
    snapshot.count = static_cast<uint16_t>(count);
    for (qsizetype i = 0; i < flags.size(); ++i) {
        if (flags[i] >= 0) {
            snapshot.satellites[i].used = flags[i] != 0;
        }
    }
    const auto counts = GPSRtk::countSatellites(snapshot);
    QCOMPARE(counts.inView, count);
    QCOMPARE(counts.used.value_or(-1), expectedUsage);

    GPSRtk receiver;
    auto* facts = qobject_cast<GPSRTKFactGroup*>(receiver.gpsRtkFactGroup());
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), -1);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), -1);
    receiver._satelliteInfoUpdate(snapshot);
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), count);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), expectedUsage);

    receiver._satelliteUsageUpdate({.usedCount = 12});
    receiver._satelliteInfoUpdate(snapshot);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), expectedUsage < 0 ? 12 : expectedUsage);
    receiver._satelliteUsageUpdate({.usedCount = 0});
    receiver._satelliteInfoUpdate(snapshot);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), expectedUsage < 0 ? 0 : expectedUsage);
    receiver._satelliteUsageUpdate({});
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), -1);
    receiver._onGPSDisconnect();
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), -1);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), -1);
}

void GPSRtkTest::_countOnlyUsagePreservesInView()
{
    GPSRtk receiver;
    auto* facts = qobject_cast<GPSRTKFactGroup*>(receiver.gpsRtkFactGroup());
    GPSSatelliteReport snapshot;
    snapshot.count = 15;
    receiver._satelliteInfoUpdate(snapshot);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), -1);
    for (const auto count : {std::optional<int>{12}, std::optional<int>{0}, std::optional<int>{}}) {
        receiver._satelliteUsageUpdate({.usedCount = count});
        QCOMPARE(facts->numSatellites()->rawValue().toInt(), 15);
        QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), count.value_or(-1));
    }
}

void GPSRtkTest::_logsOnlyFixTransitions()
{
    GPSRtk receiver;
    const QString category = QStringLiteral("GPS.GPSRtk");
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
    GPSPositionReport report;
    report.fixType = GPSPositionReport::FixType::Fix3D;
    report.latitudeDegrees = 47.123456;
    report.longitudeDegrees = 8.654321;
    report.altitudeMslMeters = 512.5;
    expectLogMessage("GPS.GPSRtk", QtDebugMsg, QRegularExpression(QStringLiteral("Receiver fix changed:")));
    receiver._sensorGpsUpdate(report);
    verifyExpectedLogMessage();
    QCOMPARE(LogManager::capturedMessages(category).size(), initialCount + 1);
    for (int i = 0; i < 3; ++i) {
        report.latitudeDegrees += 0.1;
        receiver._sensorGpsUpdate(report);
    }
    QCOMPARE(LogManager::capturedMessages(category).size(), initialCount + 1);
    report.fixType = GPSPositionReport::FixType::NoFix;
    expectLogMessage("GPS.GPSRtk", QtDebugMsg, QRegularExpression(QStringLiteral("Receiver fix changed: 1")));
    receiver._sensorGpsUpdate(report);
    verifyExpectedLogMessage();
    receiver._sensorGpsUpdate(report);
    QCOMPARE(LogManager::capturedMessages(category).size(), initialCount + 2);
    receiver.disconnectGPS();
    expectLogMessage("GPS.GPSRtk", QtDebugMsg, QRegularExpression(QStringLiteral("Receiver fix changed: 1")));
    receiver._sensorGpsUpdate(report);
    verifyExpectedLogMessage();
    QCOMPARE(LogManager::capturedMessages(category).size(), initialCount + 3);
    for (const auto& entry : LogManager::capturedMessages(category)) {
        QVERIFY(entry.message.contains(QStringLiteral("Receiver fix changed:")));
        QVERIFY(!entry.message.contains(QStringLiteral("47.123")));
        QVERIFY(!entry.message.contains(QStringLiteral("8.654")));
        QVERIFY(!entry.message.contains(QStringLiteral("512.5")));
    }
}

UT_REGISTER_TEST(GPSRtkTest, TestLabel::Unit)

void GPSRtkTest::_testCoreAvailableWithoutReceiver()
{
    GPSRtk rtk;
    QVERIFY(!rtk.connected());
    auto* facts = qobject_cast<GPSRTKFactGroup*>(rtk.gpsRtkFactGroup());
    QVERIFY(facts);
    QVERIFY(!facts->connected()->rawValue().toBool());
    QVERIFY(QFile::exists(QStringLiteral(":/json/Vehicle/GPSRTKFact.json")));
    QVERIFY(QGroundControlQmlGlobal::staticMetaObject.indexOfProperty("gpsRtk") >= 0);
}

namespace {
struct BlockedOpen
{
    QSemaphore entered;
    QSemaphore release;
    std::atomic_bool sawCancellation = false;
};

GPSProvider::TransportFactory blockedFactory(const std::shared_ptr<BlockedOpen>& gate)
{
    return [gate](const std::atomic_bool& stop) {
        gate->entered.release();
        gate->release.acquire();
        gate->sawCancellation = stop.load();
        return std::unique_ptr<GPSTransport>{};
    };
}
}  // namespace

void GPSRtkTest::_failedOpenNeverConnects()
{
    TestFixtures::SettingsFixture saved;
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    GPSRtk receiver;
    auto* facts = qobject_cast<GPSRTKFactGroup*>(receiver.gpsRtkFactGroup());
    QSignalSpy connected(facts->connected(), &Fact::rawValueChanged);
    expectLogMessage("GPS.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport")));
    receiver.connectReceiver(GPSType::ublox, {});
    QVERIFY(!receiver.connected());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
    QVERIFY(!receiver.connected());
    QVERIFY(connected.isEmpty());
    QCOMPARE(facts->lastError()->rawValue().toInt(), static_cast<int>(GPSConnectionError::OpenFailed));
    verifyExpectedLogMessage();
}

void GPSRtkTest::_retiredWorkerCannotUpdateReplacement()
{
    TestFixtures::SettingsFixture saved;
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    auto firstGate = std::make_shared<BlockedOpen>();
    auto secondGate = std::make_shared<BlockedOpen>();
    GPSCorrectionManager corrections;
    GPSRtk receiver;
    receiver.setCorrectionManager(&corrections);
    QSignalSpy routed(&corrections, &GPSCorrectionManager::correctionRouted);
    receiver._disconnectTimeoutMs = 0;
    const auto releaseWorkers = qScopeGuard([&]() {
        firstGate->release.release();
        secondGate->release.release();
        receiver._disconnectTimeoutMs = TestTimeout::mediumMs();
    });
    receiver.connectReceiver(GPSType::ublox, blockedFactory(firstGate), QStringLiteral("serial:test-base"));
    QTRY_VERIFY_WITH_TIMEOUT(firstGate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> first = receiver._gpsProvider;
    auto* facts = qobject_cast<GPSRTKFactGroup*>(receiver.gpsRtkFactGroup());
    QVERIFY(!receiver.connected());
    emit first->receiverReady();
    GPSSurveyInStatus survey{};
    survey.valid = true;
    survey.active = true;
    survey.coordinate = QGeoCoordinate(47.0, 8.0);
    survey.altitudeEllipsoidMeters = 500.0;
    survey.duration = std::chrono::seconds(4294967295LL);
    survey.meanAccuracyMeters = 1.5;
    emit first->surveyInStatus(survey);
    GPSSatelliteReport satellites;
    satellites.count = 2;
    satellites.satellites[0].used = true;
    emit first->satelliteInfoUpdate(satellites);
    emit first->satelliteUsageUpdate({.usedCount = 7});
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(receiver.connected());
    QVERIFY(facts->valid()->rawValue().toBool());
    QCOMPARE(facts->currentLatitude()->rawValue().toDouble(), 47.0);
    QCOMPARE(facts->currentLongitude()->rawValue().toDouble(), 8.0);
    QCOMPARE(facts->currentAltitude()->rawValue().toDouble(), 500.0);
    QCOMPARE(facts->currentAccuracy()->rawValue().toDouble(), 1.5);
    QCOMPARE(facts->currentDuration()->rawValue().toLongLong(), 4294967295LL);
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), 2);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), 7);

    const auto frame = GpsTestHelpers::buildRtcmFrame(1005);
    emit first->RTCMDataUpdate(frame, GPSCorrectionFrame::monotonicNowMs());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(routed.size(), 1);
    const auto original = qvariant_cast<GPSCorrectionFrame>(routed[0][0]);
    QCOMPARE(original.source, GPSCorrectionSource::LocalReceiver);
    QCOMPARE(original.sourceInstance, QStringLiteral("serial:test-base"));
    QVERIFY(original.validated);
    auto* rtcm = corrections.rtcmMavlink();
    const auto bytesBefore = rtcm->totalBytesSent();
    QCOMPARE(bytesBefore, quint64(frame.size()));
    // Retirement must reject callbacks already in the GUI queue.
    emit first->RTCMDataUpdate(frame, GPSCorrectionFrame::monotonicNowMs());
    emit first->surveyInStatus(survey);
    emit first->satelliteInfoUpdate(satellites);
    emit first->satelliteUsageUpdate({.usedCount = 12});
    emit first->sensorGpsUpdate(GPSPositionReport{});
    emit first->receiverReady();
    emit first->configurationError(QStringLiteral("Retired receiver configuration failure"));
    emit first->connectionError(GPSConnectionError::DeviceError);
    expectLogMessage(
        "GPS.GPSRtk", QtWarningMsg,
        QRegularExpression(QStringLiteral("GPS thread did not exit in time; deferring cleanup to finished")));
    receiver.connectReceiver(GPSType::ublox, blockedFactory(secondGate), QStringLiteral("serial:test-base"));
    verifyExpectedLogMessage();
    QVERIFY(!receiver.connected());
    QVERIFY(!facts->valid()->rawValue().toBool());
    QVERIFY(!facts->active()->rawValue().toBool());
    QVERIFY(qIsNaN(facts->currentLatitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(facts->currentAccuracy()->rawValue().toDouble()));
    QCOMPARE(facts->currentDuration()->rawValue().toInt(), 0);
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), -1);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), -1);
    QTRY_VERIFY_WITH_TIMEOUT(secondGate->entered.available() > 0, TestTimeout::mediumMs());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(!receiver.connected());
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), -1);
    QCOMPARE(facts->lastError()->rawValue().toInt(), static_cast<int>(GPSConnectionError::None));
    QVERIFY(receiver.errorMessage().isEmpty());
    QCOMPARE(rtcm->totalBytesSent(), bytesBefore);
    emit receiver._gpsProvider->receiverReady();
    const auto receivedAtMs = GPSCorrectionFrame::monotonicNowMs() - 10;
    emit receiver._gpsProvider->RTCMDataUpdate(frame, receivedAtMs);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(receiver.connected());
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

    expectLogMessage("GPS.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("GPS device error, connection lost")));
    emit receiver._gpsProvider->connectionError(GPSConnectionError::DeviceError);
    emit receiver._gpsProvider->RTCMDataUpdate(frame, GPSCorrectionFrame::monotonicNowMs());
    emit receiver._gpsProvider->receiverReady();
    emit receiver._gpsProvider->surveyInStatus(survey);
    emit receiver._gpsProvider->satelliteInfoUpdate(satellites);
    emit receiver._gpsProvider->satelliteUsageUpdate({.usedCount = 12});
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    verifyExpectedLogMessage();
    QVERIFY(!receiver.connected());
    QCOMPARE(facts->currentDuration()->rawValue().toLongLong(), 0);
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), -1);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), -1);
    QVERIFY(corrections.sourceInstances().isEmpty());
    QCOMPARE(routed.size(), 2);
    QCOMPARE(rtcm->totalBytesSent(), bytesBefore + frame.size());

    receiver._gpsProvider->stop();
    secondGate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
    QVERIFY(secondGate->sawCancellation);
    QVERIFY(!receiver.connected());
    QVERIFY(corrections.sourceInstances().isEmpty());
}

void GPSRtkTest::_workerCanOutliveManager()
{
    TestFixtures::SettingsFixture saved;
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    auto gate = std::make_shared<BlockedOpen>();
    auto receiver = std::make_unique<GPSRtk>();
    receiver->_disconnectTimeoutMs = 0;
    const auto releaseWorker = qScopeGuard([&]() { gate->release.release(); });
    receiver->connectReceiver(GPSType::ublox, blockedFactory(gate));
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> provider = receiver->_gpsProvider;
    emit provider->receiverReady();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(receiver->connected());
    expectLogMessage(
        "GPS.GPSRtk", QtWarningMsg,
        QRegularExpression(QStringLiteral("GPS thread did not exit in time; deferring cleanup to finished")));
    receiver.reset();
    verifyExpectedLogMessage();
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
    TestFixtures::SettingsFixture saved;
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    auto gate = std::make_shared<BlockedOpen>();
    GPSCorrectionManager corrections;
    GPSRtk receiver;
    receiver.setCorrectionManager(&corrections);
    const auto releaseWorker = qScopeGuard([&]() { gate->release.release(); });
    receiver.connectReceiver(GPSType::ublox, blockedFactory(gate));
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    const auto receivedAtMs =
        GPSCorrectionFrame::monotonicNowMs() - (expired ? GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS : 0);
    QSignalSpy routed(&corrections, &GPSCorrectionManager::correctionRouted);
    emit receiver._gpsProvider->RTCMDataUpdate(frame, receivedAtMs);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    const auto stats = corrections.sources()[static_cast<int>(GPSCorrectionSource::LocalReceiver)].toMap();
    QCOMPARE(stats.value(QStringLiteral("receivedFrames")).toULongLong(), 1);
    QCOMPARE(stats.value(QStringLiteral("validatedFrames")).toULongLong(), valid ? 1 : 0);
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
    for (const auto* name : {"baseReceiverManufacturers", "serialDevice", "serialBaudRate", "useFixedBasePosition",
                             "surveyInAccuracyLimit", "surveyInMinObservationDuration", "receiverAveragingDuration",
                             "fixedBasePositionLatitude", "fixedBasePositionLongitude", "fixedBasePositionAltitude",
                             "fixedBasePositionAccuracy"}) {
        QTest::newRow(name) << QString::fromLatin1(name);
    }
}

void GPSRtkTest::_runtimeSettingsDoNotRequireAppRestart()
{
    QFETCH(QString, name);
    auto* settings = SettingsManager::instance()->rtkSettings();
    auto* fact = settings->property(name.toUtf8().constData()).value<Fact*>();
    QVERIFY(fact);
    QVERIFY(!fact->qgcRebootRequired());
    QVERIFY(!fact->vehicleRebootRequired());
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
    QCOMPARE(values.size(), 8);
    for (int id = 0; id < values.size(); ++id) {
        QCOMPARE(values[id].toInt(), id);
    }
    QCOMPARE(type.has_value(), receiverType >= 0);
    if (type) {
        QCOMPARE(static_cast<int>(*type), receiverType);
        QCOMPARE(GPSRtk::manufacturerForType(*type), manufacturer);
    }
    const auto caps = receiver.capabilitiesForManufacturer(manufacturer);
    QCOMPARE(caps.value(QStringLiteral("recognized")).toBool(), manufacturer < 8);
    QCOMPARE(caps.value(QStringLiteral("passive")).toBool(), manufacturer == 7);
    QCOMPARE(caps.value(QStringLiteral("rtkBase")).toBool(), manufacturer < 7);
    QCOMPARE(caps.value(QStringLiteral("receiverAveraging")).toBool(), manufacturer == 0 || manufacturer == 5);
    QCOMPARE(caps.value(QStringLiteral("surveyIn")).toBool(), manufacturer < 7 && manufacturer != 5);
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
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->useFixedBasePosition(), baseMode);
    saved.setFactValue(settings->surveyInAccuracyLimit(), 1.75);
    saved.setFactValue(settings->surveyInMinObservationDuration(), 195);
    saved.setFactValue(settings->receiverAveragingDuration(), 321);
    saved.setFactValue(settings->fixedBasePositionLatitude(), 47.5);
    saved.setFactValue(settings->fixedBasePositionLongitude(), 8.25);
    saved.setFactValue(settings->fixedBasePositionAltitude(), 512.0);
    saved.setFactValue(settings->fixedBasePositionAccuracy(), 1.5);
    GPSReceiverConfig config;
    const auto type = GPSRtk::typeForManufacturer(manufacturer);
    QVERIFY(type);
    const auto error = GPSRtk::_receiverConfig(*type, settings, 230400, config);
    QCOMPARE(error.isEmpty(), accepted);
    QCOMPARE(config.baudRate, uint32_t(230400));
    QVERIFY(!config.allowPersistentChanges);
    if (manufacturer == 7) {
        QCOMPARE(config.role, GPSReceiverConfig::Role::Passive);
        QVERIFY(!config.base.useFixedBase);
        QCOMPARE(config.base.surveyInAccMeters, 0.0);
        QCOMPARE(config.base.surveyInDurationSecs, int64_t(0));
        QVERIFY(qIsNaN(config.base.fixedBaseLatitude));
        QVERIFY(qIsNaN(config.base.fixedBaseLongitude));
        QVERIFY(qIsNaN(config.base.fixedBaseAltitudeMeters));
        QCOMPARE(config.base.fixedBaseAccuracyMeters, 0.0f);
        QCOMPARE(config.base.surveyMode, GPSBaseStationConfig::SurveyMode::AccuracyControlled);
        QCOMPARE(config.base.receiverAveragingDurationSecs, uint32_t(60));
    } else {
        QCOMPARE(config.role, GPSReceiverConfig::Role::RTKBase);
        QCOMPARE(config.base.useFixedBase, baseMode == 1);
        if (baseMode == 1) {
            QCOMPARE(config.base.fixedBaseLatitude, 47.5);
            QCOMPARE(config.base.fixedBaseLongitude, 8.25);
            QCOMPARE(config.base.fixedBaseAltitudeMeters, 512.0f);
            QCOMPARE(config.base.fixedBaseAccuracyMeters, 1.5f);
        } else if (baseMode == 2) {
            QCOMPARE(config.base.surveyMode, GPSBaseStationConfig::SurveyMode::ReceiverManaged);
            QCOMPARE(config.base.receiverAveragingDurationSecs, uint32_t(321));
        } else {
            QCOMPARE(config.base.surveyMode, GPSBaseStationConfig::SurveyMode::AccuracyControlled);
            QCOMPARE(config.base.surveyInAccMeters, 1.75);
            QCOMPARE(config.base.surveyInDurationSecs, int64_t(195));
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
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->useFixedBasePosition(), mode);
    saved.setFactValue(settings->receiverAveragingDuration(), averagingDuration);
    GPSReceiverConfig config;
    const auto type = GPSRtk::typeForManufacturer(manufacturer);
    QVERIFY(type);
    QCOMPARE(GPSRtk::_receiverConfig(*type, settings, baud, config).isEmpty(), accepted);
    QCOMPARE(settings->receiverAveragingDuration()->rawMin().toUInt(), 1U);
    QCOMPARE(settings->receiverAveragingDuration()->rawMax().toUInt(), 3600U);
    QCOMPARE(settings->useFixedBasePosition()->enumValues(), (QVariantList{0, 1, 2}));
    if (!accepted) {
        GPSRtk receiver;
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
        QCOMPARE(receiver.gpsRtkFactGroup()->lastError()->rawValue().toInt(),
                 static_cast<int>(GPSConnectionError::ConfigFailed));
    }
}

#ifndef QGC_NO_SERIAL_LINK
void GPSRtkTest::_explicitSerialSelectionAndDisconnect_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<bool>("unplug");
    QTest::addColumn<bool>("allowPersistentChanges");
    QTest::newRow("unicore-disconnect") << 5 << false << false;
    QTest::newRow("quectel-disconnect") << 6 << false << false;
    QTest::newRow("quectel-one-use-consent") << 6 << false << true;
    QTest::newRow("passive-disconnect") << 7 << false << false;
    QTest::newRow("passive-unplug") << 7 << true << false;
}

void GPSRtkTest::_explicitSerialSelectionAndDisconnect()
{
    QFETCH(int, manufacturer);
    QFETCH(bool, unplug);
    QFETCH(bool, allowPersistentChanges);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    auto* autoConnect = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), manufacturer);
    saved.setFactValue(settings->useFixedBasePosition(), manufacturer == 5 ? 2 : 0);
    saved.setFactValue(settings->serialDevice(), QStringLiteral("/test/selected"));
    saved.setFactValue(settings->serialBaudRate(), 115200);
    saved.setFactValue(autoConnect->autoConnectRTKGPS(), true);
    const QList<SerialPortManager::Port> inventory{
        {QStringLiteral("/test/unselected"), QStringLiteral("unselected"), QGCSerialPortInfo::BoardTypeUnknown,
         QStringLiteral("USB serial")},
        {QStringLiteral("/test/selected"), QStringLiteral("selected"), QGCSerialPortInfo::BoardTypeUnknown,
         QStringLiteral("USB serial")},
    };
    SerialPortManager ports(nullptr, [&] { return inventory; });
    GPSRtk receiver;
    receiver.setSerialPortManager(&ports);
    auto gate = std::make_shared<BlockedOpen>();
    QString openedDevice;
    receiver._serialTransportFactory = [gate, &openedDevice](const QString& device, const std::atomic_bool& stop) {
        openedDevice = device;
        return blockedFactory(gate)(stop);
    };
    const auto releaseWorker = qScopeGuard([&] { gate->release.release(); });
    QSignalSpy manual(&receiver, &GPSRtk::manualConnectionRequested);
    QVERIFY(receiver.connectConfiguredGPS(allowPersistentChanges));
    QCOMPARE(manual.size(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QCOMPARE(openedDevice, QStringLiteral("/test/selected"));
    QCOMPARE(receiver.activeSerialDevice(), openedDevice);
    QCOMPARE(receiver.activeManufacturer(), manufacturer);
    QVERIFY(!autoConnect->autoConnectRTKGPS()->rawValue().toBool());
    QVERIFY(ports.isPortReserved(openedDevice));
    QVERIFY(!ports.isPortReserved(QStringLiteral("/test/unselected")));
    QVERIFY(!ports.reservePort(openedDevice));
    QPointer<GPSProvider> provider = receiver._gpsProvider;
    emit provider->receiverReady();
    GPSSurveyInStatus survey;
    survey.active = true;
    survey.valid = true;
    emit provider->surveyInStatus(survey);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(receiver.connected());
    QCOMPARE(receiver.gpsRtkFactGroup()->active()->rawValue().toBool(), manufacturer != 7);
    QCOMPARE(receiver.gpsRtkFactGroup()->valid()->rawValue().toBool(), manufacturer != 7);
    gate->release.release();
    if (unplug) {
        emit ports.portsEnumerated({QStringLiteral("/test/unselected")});
        QVERIFY(!receiver.errorMessage().isEmpty());
    } else {
        receiver.disconnectConfiguredGPS();
        QVERIFY(receiver.errorMessage().isEmpty());
    }
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(!receiver.connected());
    QVERIFY(!receiver.gpsRtkFactGroup()->active()->rawValue().toBool());
    QVERIFY(!receiver.gpsRtkFactGroup()->valid()->rawValue().toBool());
    QTRY_VERIFY_WITH_TIMEOUT(provider.isNull(), TestTimeout::mediumMs());
    QVERIFY(ports.canReservePort(openedDevice));
}

void GPSRtkTest::_manualSerialErrors_data()
{
    QTest::addColumn<QString>("reason");
    for (const auto* reason :
         {"all", "missing", "empty", "bootloader", "busy", "single-port", "invalid-baud", "unsupported-mode"}) {
        QTest::newRow(reason) << QString::fromLatin1(reason);
    }
}

void GPSRtkTest::_manualSerialErrors()
{
    QFETCH(QString, reason);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    auto* autoConnect = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), reason == QStringLiteral("all") ? 0 : 5);
    saved.setFactValue(settings->useFixedBasePosition(), reason == QStringLiteral("unsupported-mode") ? 0 : 2);
    saved.setFactValue(settings->serialDevice(),
                       reason == QStringLiteral("empty") ? QString() : QStringLiteral("/test/selected"));
    saved.setFactValue(settings->serialBaudRate(), reason == QStringLiteral("invalid-baud") ? 0 : 115200);
    saved.setFactValue(autoConnect->autoConnectRTKGPS(), false);
    SerialPortManager::Port port{
        QStringLiteral("/test/selected"), QStringLiteral("selected"), QGCSerialPortInfo::BoardTypeUnknown, {}};
    port.bootloader = reason == QStringLiteral("bootloader");
    SerialPortManager ports(nullptr, [&] {
        return reason == QStringLiteral("missing") ? QList<SerialPortManager::Port>{}
                                                   : QList<SerialPortManager::Port>{port};
    });
    SerialPortManager::ReservationPtr reservation;
    if (reason == QStringLiteral("busy")) {
        reservation = ports.reservePort(port.systemLocation);
    } else if (reason == QStringLiteral("single-port")) {
        ports.setSinglePortOnly(true);
        (void) ports.availablePorts();
        reservation = ports.reservePort(QStringLiteral("/test/mavlink"));
    }
    GPSRtk receiver;
    receiver.setSerialPortManager(&ports);
    std::atomic_bool opened = false;
    receiver._serialTransportFactory = [&opened](const QString&, const std::atomic_bool&) {
        opened = true;
        return std::unique_ptr<GPSTransport>{};
    };
    QVERIFY(!receiver.connectConfiguredGPS());
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(!receiver.connected());
    QVERIFY(!receiver.errorMessage().isEmpty());
    QVERIFY(!opened);
    QVERIFY(receiver.gpsRtkFactGroup()->lastError()->rawValue().toInt() != 0);
    if (reservation) {
        QVERIFY(ports.isPortReserved(reservation->systemLocation));
    } else {
        QVERIFY(ports.canReservePort(port.systemLocation));
    }
    QVERIFY(!receiver.connectGPS(port.systemLocation, QStringLiteral("USB serial"), 115200));
    QVERIFY(!opened);
}

void GPSRtkTest::_serialReservationSurvivesDelayedStop()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), 7);
    SerialPortManager ports(nullptr, [] { return QList<SerialPortManager::Port>{}; });
    auto gate = std::make_shared<BlockedOpen>();
    GPSRtk receiver;
    receiver.setSerialPortManager(&ports);
    receiver._disconnectTimeoutMs = 0;
    receiver._serialTransportFactory = [gate](const QString&, const std::atomic_bool& stop) {
        return blockedFactory(gate)(stop);
    };
    const auto releaseWorker = qScopeGuard([&] { gate->release.release(); });
    QVERIFY(receiver.connectGPS(QStringLiteral("/test/selected"), QStringLiteral("passive"), 115200));
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> provider = receiver._gpsProvider;
    expectLogMessage(
        "GPS.GPSRtk", QtWarningMsg,
        QRegularExpression(QStringLiteral("GPS thread did not exit in time; deferring cleanup to finished")));
    receiver.disconnectGPS();
    verifyExpectedLogMessage();
    QVERIFY(!receiver.hasReceiver());
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

class PassiveTestTransport : public GPSTransport
{
public:
    PassiveTestTransport(const std::atomic_bool& stop, std::shared_ptr<PassiveTransportState> state)
        : GPSTransport(stop)
        , _state(std::move(state))
    {}

    GPSOpenResult open() override { return {GPSOpenStatus::Opened}; }

    bool fatalError() const override { return false; }

    bool setBaudrate(unsigned baud) override
    {
        _state->baud = baud;
        ++_state->baudChanges;
        return true;
    }

    GPSReadResult read(uint8_t*, int, int) override
    {
        _state->reading.release();
        _state->releaseRead.acquire();
        return {GPSReadStatus::Cancelled};
    }

    GPSWriteResult writeBounded(const uint8_t*, int, QDeadlineTimer) override
    {
        ++_state->writes;
        return {GPSWriteStatus::Error};
    }

private:
    std::shared_ptr<PassiveTransportState> _state;
};
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
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), 7);
    saved.setFactValue(settings->useFixedBasePosition(), 2);
    saved.setFactValue(settings->serialDevice(), QStringLiteral("/test/passive"));
    saved.setFactValue(settings->serialBaudRate(), baud);
    saved.setFactValue(SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS(), false);
    SerialPortManager ports(nullptr, [] {
        return QList<SerialPortManager::Port>{
            {QStringLiteral("/test/passive"), QStringLiteral("passive"), QGCSerialPortInfo::BoardTypeUnknown, {}}};
    });
    auto state = std::make_shared<PassiveTransportState>();
    GPSRtk receiver;
    receiver.setSerialPortManager(&ports);
    receiver._serialTransportFactory = [state](const QString&, const std::atomic_bool& stop) {
        return std::make_unique<PassiveTestTransport>(stop, state);
    };
    const auto releaseWorker = qScopeGuard([&] { state->releaseRead.release(); });
    QVERIFY(receiver.connectConfiguredGPS());
    QTRY_VERIFY_WITH_TIMEOUT(receiver.connected() && state->reading.available() > 0, TestTimeout::mediumMs());
    QCOMPARE(state->baud.load(), baud);
    QCOMPARE(state->baudChanges.load(), 1U);
    QCOMPARE(state->writes.load(), 0U);
    QVERIFY(!receiver.gpsRtkFactGroup()->active()->rawValue().toBool());
    QVERIFY(!receiver.gpsRtkFactGroup()->valid()->rawValue().toBool());
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
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->useFixedBasePosition(), 1);
    saved.setFactValue(settings->fixedBasePositionLatitude(), 47.5);
    saved.setFactValue(settings->fixedBasePositionLongitude(), 8.25);
    saved.setFactValue(settings->fixedBasePositionAltitude(), 512.0);
    saved.setFactValue(settings->fixedBasePositionAccuracy(), 1.5);
    const auto type = GPSRtk::typeForManufacturer(manufacturer);
    QVERIFY(type);
    GPSReceiverConfig config;
    const auto diagnostic = GPSRtk::_receiverConfig(*type, settings, 115200, config, allowPersistentChanges);
    QCOMPARE(diagnostic.isEmpty(), !allowPersistentChanges || manufacturer == 6);
    QCOMPARE(config.allowPersistentChanges, allowPersistentChanges);
    QVERIFY(GPSRtk::_receiverConfig(*type, settings, 115200, config).isEmpty());
    QVERIFY(!config.allowPersistentChanges);
}

void GPSRtkTest::_configurationDiagnosticRetained_data()
{
    QTest::addColumn<QString>("detail");
    QTest::newRow("provisioning-mismatch") << QStringLiteral("Requested base settings differ from receiver readback.");
    QTest::newRow("possibly-persisted") << QStringLiteral("Settings may have been saved, but reconnect failed.");
    QTest::newRow("empty-fallback") << QString();
}

void GPSRtkTest::_configurationDiagnosticRetained()
{
    QFETCH(QString, detail);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), 6);
    saved.setFactValue(settings->useFixedBasePosition(), 0);
    auto gate = std::make_shared<BlockedOpen>();
    GPSRtk receiver;
    const auto releaseWorker = qScopeGuard([&] { gate->release.release(); });
    QVERIFY(receiver.connectReceiver(GPSType::quectel, blockedFactory(gate), {}, 115200, true));
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> provider = receiver._gpsProvider;
    emit provider->configurationError(detail);
    emit provider->connectionError(GPSConnectionError::ConfigFailed);
    expectLogMessage("GPS.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("GPS receiver did not accept configuration")));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    verifyExpectedLogMessage();
    QVERIFY(!receiver.connected());
    QCOMPARE(receiver.gpsRtkFactGroup()->lastError()->rawValue().toInt(),
             static_cast<int>(GPSConnectionError::ConfigFailed));
    QVERIFY(!receiver.errorMessage().isEmpty());
    if (!detail.isEmpty()) {
        QVERIFY(receiver.errorMessage().contains(detail));
    }
    const QString message = receiver.errorMessage();
    provider->stop();
    gate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
    QCOMPARE(receiver.errorMessage(), message);
}

void GPSRtkTest::_qmlConsentIsOneUse()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), 6);
    saved.setFactValue(settings->useFixedBasePosition(), 0);
    saved.setFactValue(settings->serialDevice(), QStringLiteral("/test/absent"));
    auto* receiver = GPSManager::instance()->gpsRtk();
    QVERIFY(!receiver->hasReceiver());
    const auto originalError =
        static_cast<GPSConnectionError>(receiver->gpsRtkFactGroup()->lastError()->rawValue().toInt());
    const QString originalMessage = receiver->errorMessage();
#ifndef QGC_NO_SERIAL_LINK
    SerialPortManager ports(nullptr, [] { return QList<SerialPortManager::Port>{}; });
    const auto originalPorts = receiver->_serialPorts;
    receiver->setSerialPortManager(&ports);
#endif
    const auto restore = qScopeGuard([&] {
        receiver->_setError(originalError, originalMessage);
#ifndef QGC_NO_SERIAL_LINK
        receiver->setSerialPortManager(originalPorts);
#endif
    });
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/QGroundControl/Toolbar/GPSIndicatorPage.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QVERIFY(root->setProperty("expanded", true));
    auto* checkbox = root->findChild<QObject*>(QStringLiteral("rtkPersistentChangesCheckBox"));
    QVERIFY(checkbox);
    QVERIFY(!checkbox->property("checked").toBool());
    QCOMPARE(checkbox->property("visible").toBool(), receiver->serialSupported());
    QVERIFY(root->setProperty("_allowPersistentChanges", true));
    QVERIFY(checkbox->property("checked").toBool());
    settings->serialDevice()->setRawValue(QStringLiteral("/test/other"));
    QVERIFY(!root->property("_allowPersistentChanges").toBool());
    QVERIFY(root->setProperty("_allowPersistentChanges", true));
    settings->useFixedBasePosition()->setRawValue(1);
    QVERIFY(!root->property("_allowPersistentChanges").toBool());
    QVERIFY(root->setProperty("_allowPersistentChanges", true));
    settings->baseReceiverManufacturers()->setRawValue(5);
    QVERIFY(!root->property("_allowPersistentChanges").toBool());
    QVERIFY(!checkbox->property("visible").toBool());
    settings->baseReceiverManufacturers()->setRawValue(6);
    QVERIFY(root->setProperty("_allowPersistentChanges", true));
    QQmlExpression attempt(qmlContext(root.get()), root.get(), QStringLiteral("connectSelectedReceiver()"));
    QVERIFY(!attempt.evaluate().toBool());
    QVERIFY2(!attempt.hasError(), qPrintable(attempt.error().toString()));
    QVERIFY(!root->property("_allowPersistentChanges").toBool());
    QVERIFY(!receiver->hasReceiver());
    QVERIFY(!receiver->errorMessage().isEmpty());
    QVERIFY(root->setProperty("_allowPersistentChanges", true));
    root.reset(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QVERIFY(!root->property("_allowPersistentChanges").toBool());
}
