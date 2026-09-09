#include "GPSRtkTest.h"

#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtTest/QSignalSpy>

#include "Fixtures/RAIIFixtures.h"
#include "GPSManager.h"
#include "GPSRTKFactGroup.h"
#include "GPSRtk.h"
#include "GPSTransport.h"
#include "PositionManager.h"
#include "QGroundControlQmlGlobal.h"
#include "RTCMMavlink.h"
#include "RTKPositionSource.h"
#include "RTKSettings.h"
#include "SettingsManager.h"

void GPSRtkTest::_testCountSatellitesClampsToMax()
{
    satellite_info_s msg{};
    msg.count = 250;

    const GPSRtk::SatelliteCounts counts = GPSRtk::countSatellites(msg);

    QCOMPARE(static_cast<int>(counts.inView), static_cast<int>(satellite_info_s::SAT_INFO_MAX_SATELLITES));
    QCOMPARE(counts.used, 0);
}

void GPSRtkTest::_testCountSatellitesCountsUsed()
{
    satellite_info_s msg{};
    msg.count = 6;
    msg.used[1] = 1;
    msg.used[3] = 1;
    msg.used[5] = 1;

    const GPSRtk::SatelliteCounts counts = GPSRtk::countSatellites(msg);

    QCOMPARE(static_cast<int>(counts.inView), 6);
    QCOMPARE(counts.used, 3);
}

void GPSRtkTest::_testCountSatellitesIgnoresUsedBeyondCount()
{
    satellite_info_s msg{};
    msg.count = 2;
    msg.used[0] = 1;
    msg.used[5] = 1;

    const GPSRtk::SatelliteCounts counts = GPSRtk::countSatellites(msg);

    QCOMPARE(static_cast<int>(counts.inView), 2);
    QCOMPARE(counts.used, 1);
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
    expectLogMessage("GPS.RTK.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport")));
    receiver.connectReceiver(GPSType::u_blox, {}, {});
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
    GPSRtk receiver;
    const auto releaseWorkers = qScopeGuard([&]() {
        firstGate->release.release();
        secondGate->release.release();
    });
    receiver.connectReceiver(GPSType::u_blox, blockedFactory(firstGate), {});
    QTRY_VERIFY_WITH_TIMEOUT(firstGate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> first = receiver._gpsProvider;
    auto* facts = qobject_cast<GPSRTKFactGroup*>(receiver.gpsRtkFactGroup());
    QVERIFY(!receiver.connected());
    receiver.positionSource()->startUpdates();
    QSignalSpy positionUpdates(receiver.positionSource(), &QGeoPositionInfoSource::positionUpdated);
    sensor_gps_s fix{};
    fix.fix_type = sensor_gps_s::FIX_TYPE_3D;
    fix.latitude_deg = 47;
    fix.longitude_deg = 8;
    fix.eph = 1;
    emit first->receiverReady();
    emit first->sensorGpsUpdate(fix);
    GPSSurveyInStatus survey{};
    survey.valid = true;
    survey.active = true;
    survey.latitude = 47.0;
    survey.longitude = 8.0;
    survey.altitude = 500.0;
    survey.durationSecs = 20;
    survey.meanAccuracyMM = 1500;
    emit first->surveyInStatus(survey);
    satellite_info_s satellites{};
    satellites.count = 2;
    satellites.used[0] = 1;
    emit first->satelliteInfoUpdate(satellites);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(receiver.connected());
    QCOMPARE(positionUpdates.size(), 1);
    positionUpdates.clear();
    QVERIFY(facts->valid()->rawValue().toBool());
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), 2);

    RTCMMavlink forwarder;
    connect(&receiver, &GPSRtk::rtcmDataReceived, &forwarder, &RTCMMavlink::RTCMDataUpdate);
    auto* rtcm = &forwarder;
    const auto bytesBefore = rtcm->totalBytesSent();
    // These callbacks are queued before retirement, then delivered during the replacement session.
    emit first->sensorGpsUpdate(fix);
    emit first->RTCMDataUpdate(QByteArrayLiteral("stale corrections"));
    emit first->surveyInStatus(survey);
    emit first->satelliteInfoUpdate(satellites);
    emit first->receiverReady();
    emit first->connectionError(GPSConnectionError::DeviceError);
    receiver.connectReceiver(GPSType::u_blox, blockedFactory(secondGate), {});
    QVERIFY(receiver.stopping());
    QVERIFY(!receiver.connected());
    QVERIFY(!facts->valid()->rawValue().toBool());
    QVERIFY(!facts->active()->rawValue().toBool());
    QVERIFY(qIsNaN(facts->currentLatitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(facts->currentAccuracy()->rawValue().toDouble()));
    QCOMPARE(facts->currentDuration()->rawValue().toInt(), 0);
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), 0);
    QTRY_VERIFY_WITH_TIMEOUT(secondGate->entered.available() > 0, TestTimeout::mediumMs());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(!receiver.connected());
    QCOMPARE(facts->lastError()->rawValue().toInt(), static_cast<int>(GPSConnectionError::None));
    QCOMPARE(rtcm->totalBytesSent(), bytesBefore);
    QVERIFY(positionUpdates.isEmpty());
    QVERIFY(!receiver.positionSource()->lastKnownPosition().isValid());
    emit receiver._gpsProvider->receiverReady();
    emit receiver._gpsProvider->RTCMDataUpdate(QByteArrayLiteral("new"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(receiver.connected());
    QCOMPARE(rtcm->totalBytesSent(), bytesBefore + 3);
    firstGate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(first.isNull(), TestTimeout::mediumMs());
    QVERIFY(firstGate->sawCancellation);
    QVERIFY(receiver.connected());

    receiver._gpsProvider->stop();
    secondGate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
    QVERIFY(secondGate->sawCancellation);
    QVERIFY(!receiver.connected());
}

void GPSRtkTest::_workerCanOutliveManager()
{
    TestFixtures::SettingsFixture saved;
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    auto gate = std::make_shared<BlockedOpen>();
    auto receiver = std::make_unique<GPSRtk>();
    const auto releaseWorker = qScopeGuard([&]() { gate->release.release(); });
    receiver->connectReceiver(GPSType::u_blox, blockedFactory(gate), {});
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> provider = receiver->_gpsProvider;
    receiver.reset();
    QVERIFY(provider);
    QVERIFY(!provider->parent());
    gate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(provider.isNull(), TestTimeout::mediumMs());
    QVERIFY(gate->sawCancellation);
}

void GPSRtkTest::_shutdownWithoutEventLoop_data()
{
    QTest::addColumn<QString>("phase");
    for (const char* phase : {"running", "retired", "finished", "awaiting-deletion", "before-start"}) {
        QTest::newRow(phase) << QString::fromLatin1(phase);
    }
}

void GPSRtkTest::_shutdownWithoutEventLoop()
{
    QFETCH(QString, phase);
    auto gate = std::make_shared<BlockedOpen>();
    const auto releaseWorker = qScopeGuard([&]() { gate->release.release(); });
    GPSRtk receiver;
    QPointer<GPSProvider> provider;
    if (phase == QStringLiteral("before-start")) {
        connect(&receiver, &GPSRtk::receiverTypeChanged, &receiver, [&]() {
            provider = receiver._gpsProvider;
            receiver.shutdown();
        });
    }
    receiver.connectReceiver(GPSType::u_blox,
                             [gate](const std::atomic_bool& stop) {
                                 gate->entered.release();
                                 while (!stop && !gate->release.tryAcquire(1, 10)) {
                                 }
                                 gate->sawCancellation = stop.load();
                                 return std::unique_ptr<GPSTransport>();
                             },
                             {});

    if (phase != QStringLiteral("before-start")) {
        provider = receiver._gpsProvider;
        QVERIFY(provider);
        QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
        if (phase == QStringLiteral("retired")) {
            receiver.disconnectGPS();
            QVERIFY(receiver.stopping());
        } else if (phase == QStringLiteral("finished") || phase == QStringLiteral("awaiting-deletion")) {
            provider->stop();
            QVERIFY(provider->wait(TestTimeout::mediumMs()));
            if (phase == QStringLiteral("awaiting-deletion")) {
                QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
                QVERIFY(!receiver.hasReceiver());
                QVERIFY(provider);
            }
        }
        // No event processing is allowed between shutdown and the destruction check.
        receiver.shutdown();
        QVERIFY(gate->sawCancellation);
    }
    QVERIFY(provider.isNull());
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(!receiver.stopping());
    receiver.shutdown();
    receiver.connectReceiver(GPSType::u_blox, blockedFactory(gate), {});
    QVERIFY(!receiver.hasReceiver());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(!receiver.stopping());
}

void GPSRtkTest::_positionSourceSelection()
{
    TestFixtures::SettingsFixture saved;
    auto* enabled = SettingsManager::instance()->rtkSettings()->useReceiverPosition();
    saved.setFactValue(enabled, false);
    GPSManager manager;
    auto* receiver = manager.gpsRtk();
    auto* position = QGCPositionManager::instance();
    const auto cleanup = qScopeGuard([&]() { manager.shutdown(); });
    receiver->_onGPSConnect();
    QSignalSpy updates(receiver->positionSource(), &QGeoPositionInfoSource::positionUpdated);
    sensor_gps_s fix{};
    fix.fix_type = sensor_gps_s::FIX_TYPE_RTK_FIXED;
    fix.latitude_deg = 47;
    fix.longitude_deg = 8;
    fix.altitude_msl_m = 500;
    fix.eph = 0.1f;
    fix.epv = 0.2f;
    receiver->_sensorGpsUpdate(fix);
    QVERIFY(updates.isEmpty());
    enabled->setRawValue(true);
    QVERIFY(!position->gcsPosition().isValid());
    receiver->_sensorGpsUpdate(fix);
    QCOMPARE(updates.size(), 1);
    QCOMPARE(position->gcsPosition(), QGeoCoordinate(47, 8, 500));
    QVERIFY(receiver->connected());
    enabled->setRawValue(false);
    QVERIFY(receiver->connected());
    QVERIFY(!position->gcsPosition().isValid());
    receiver->_sensorGpsUpdate(fix);
    QCOMPARE(updates.size(), 1);
    enabled->setRawValue(true);
    QVERIFY(!position->gcsPosition().isValid());
    receiver->_sensorGpsUpdate(fix);
    QVERIFY(position->gcsPosition().isValid());
    receiver->disconnectGPS();
    QVERIFY(!position->gcsPosition().isValid());
    QVERIFY(!receiver->positionSource()->lastKnownPosition().isValid());
    receiver->_sensorGpsUpdate(fix);
    QVERIFY(!position->gcsPosition().isValid());
}

void GPSRtkTest::_sourceHealthIndependentOfSurvey()
{
    GPSRtk receiver;
    auto* facts = static_cast<GPSRTKFactGroup*>(receiver.gpsRtkFactGroup());
    receiver._onGPSConnect();
    QVERIFY(receiver.connected());
    QCOMPARE(receiver.health()->state(), GPSSourceHealth::NoData);
    GPSSurveyInStatus survey{};
    survey.valid = true;
    receiver._onGPSSurveyInStatus(survey);
    QVERIFY(facts->valid()->rawValue().toBool());
    QVERIFY(!receiver.health()->usable());
    sensor_gps_s fix{};
    fix.fix_type = sensor_gps_s::FIX_TYPE_3D;
    fix.latitude_deg = 47;
    fix.longitude_deg = 8;
    fix.eph = 1;
    receiver._sensorGpsUpdate(fix);
    QVERIFY(receiver.health()->usable());
    fix.fix_type = sensor_gps_s::FIX_TYPE_NONE;
    receiver._sensorGpsUpdate(fix);
    QCOMPARE(receiver.health()->state(), GPSSourceHealth::Invalid);
    QVERIFY(receiver.connected());
    QVERIFY(facts->valid()->rawValue().toBool());
    satellite_info_s satellites{};
    satellites.count = 2;
    satellites.used[0] = 1;
    receiver._satelliteInfoUpdate(satellites);
    QCOMPARE(receiver.health()->satellitesInViewCount(), 2);
    QCOMPARE(receiver.health()->satellitesInUseCount(), 1);
    satellites.timestamp = 1;
    receiver._satelliteInfoUpdate(satellites);
    QCOMPARE(receiver.health()->satellitesInViewCount(), -1);
    QCOMPARE(receiver.health()->satellitesInUseCount(), -1);
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), 0);
    receiver.disconnectGPS();
    QCOMPARE(receiver.health()->state(), GPSSourceHealth::NoData);
}
