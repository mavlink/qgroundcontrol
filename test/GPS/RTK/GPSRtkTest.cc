#include "GPSRtkTest.h"

#include <limits>

#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtTest/QSignalSpy>

#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSRTKFactGroup.h"
#include "GPSRtk.h"
#include "GPSTransport.h"
#include "GpsTestHelpers.h"
#include "QGroundControlQmlGlobal.h"
#include "RTCMMavlink.h"
#include "RTKSettings.h"
#include "SettingsManager.h"

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
