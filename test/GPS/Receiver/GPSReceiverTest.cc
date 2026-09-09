#include "GPSReceiverTest.h"

#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtTest/QSignalSpy>

#include "Fixtures/RAIIFixtures.h"
#include "GPSDriverData.h"
#include "GPSManager.h"
#include "GPSRTKFactGroup.h"
#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "GPSReceiverPositionSource.h"
#include "GPSRtkState.h"
#include "GPSTransport.h"
#include "PositionManager.h"
#include "QGroundControlQmlGlobal.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#include "satellite_info.h"
#include "sensor_gps.h"

void GPSReceiverTest::_testCountSatellitesClampsToMax()
{
    satellite_info_s msg{};
    msg.count = 250;

    const GPSReceiver::SatelliteCounts counts = GPSReceiver::countSatellites(GPSDriverData::satellites(msg));

    QCOMPARE(static_cast<int>(counts.inView), static_cast<int>(satellite_info_s::SAT_INFO_MAX_SATELLITES));
    QCOMPARE(counts.used, 0);
}

void GPSReceiverTest::_testCountSatellitesCountsUsed()
{
    satellite_info_s msg{};
    msg.count = 6;
    msg.used[1] = 1;
    msg.used[3] = 1;
    msg.used[5] = 1;

    const GPSReceiver::SatelliteCounts counts = GPSReceiver::countSatellites(GPSDriverData::satellites(msg));

    QCOMPARE(static_cast<int>(counts.inView), 6);
    QCOMPARE(counts.used, 3);
}

void GPSReceiverTest::_testCountSatellitesIgnoresUsedBeyondCount()
{
    satellite_info_s msg{};
    msg.count = 2;
    msg.used[0] = 1;
    msg.used[5] = 1;

    const GPSReceiver::SatelliteCounts counts = GPSReceiver::countSatellites(GPSDriverData::satellites(msg));

    QCOMPARE(static_cast<int>(counts.inView), 2);
    QCOMPARE(counts.used, 1);
}

UT_REGISTER_TEST(GPSReceiverTest, TestLabel::Unit)

void GPSReceiverTest::_testCoreAvailableWithoutReceiver()
{
    GPSReceiverSession session;
    GPSReceiver receiver(session);
    QVERIFY(!receiver.connected());
    auto* facts = receiver.facts();
    QVERIFY(facts);
    QVERIFY(!facts->connected()->rawValue().toBool());
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), 0);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), 0);
    QCOMPARE(facts->lastError()->rawValue().toUInt(), 0U);
    auto factNames = facts->factNames();
    factNames.sort();
    QCOMPARE(factNames, QStringList({QStringLiteral("connected"), QStringLiteral("lastError"),
                                     QStringLiteral("numSatellites"), QStringLiteral("numSatellitesUsed")}));
    QVERIFY(QFile::exists(QStringLiteral(":/json/Vehicle/GPSReceiverFact.json")));
    QVERIFY(QGroundControlQmlGlobal::staticMetaObject.indexOfProperty("gpsReceiver") >= 0);
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

void GPSReceiverTest::_failedOpenNeverConnects()
{
    TestFixtures::SettingsFixture saved;
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    GPSReceiverSession session;
    GPSReceiver receiver(session);
    const auto cleanup = qScopeGuard([&]() { session.shutdown(); });
    auto* facts = receiver.facts();
    QSignalSpy connected(facts->connected(), &Fact::rawValueChanged);
    expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport")));
    session.start(GPSType::u_blox, {}, {});
    QVERIFY(!receiver.connected());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
    QVERIFY(!receiver.connected());
    QVERIFY(connected.isEmpty());
    QCOMPARE(facts->lastError()->rawValue().toInt(), static_cast<int>(GPSConnectionError::OpenFailed));
    verifyExpectedLogMessage();
}

void GPSReceiverTest::_retiredWorkerCannotUpdateReplacement()
{
    TestFixtures::SettingsFixture saved;
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    auto firstGate = std::make_shared<BlockedOpen>();
    auto secondGate = std::make_shared<BlockedOpen>();
    GPSReceiverSession session;
    GPSReceiver receiver(session);
    GPSRtkState rtkState(session);
    auto* surveyFacts = rtkState.facts();
    const auto releaseWorkers = qScopeGuard([&]() {
        session.stop();
        firstGate->release.release();
        secondGate->release.release();
        session.shutdown();
    });
    session.start(GPSType::u_blox, blockedFactory(firstGate), {});
    QTRY_VERIFY_WITH_TIMEOUT(firstGate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> first = receiver._session._provider;
    auto* facts = receiver.facts();
    QVERIFY(!receiver.connected());
    receiver.positionSource()->startUpdates();
    QSignalSpy positionUpdates(receiver.positionSource(), &QGeoPositionInfoSource::positionUpdated);
    sensor_gps_s fix{};
    fix.fix_type = sensor_gps_s::FIX_TYPE_3D;
    fix.latitude_deg = 47;
    fix.longitude_deg = 8;
    fix.eph = 1;
    emit first->receiverReady();
    emit first->sensorGpsUpdate(GPSDriverData::position(fix));
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
    emit first->satelliteInfoUpdate(GPSDriverData::satellites(satellites));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(receiver.connected());
    QCOMPARE(positionUpdates.size(), 1);
    positionUpdates.clear();
    QVERIFY(surveyFacts->valid()->rawValue().toBool());
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), 2);

    QSignalSpy corrections(&session, &GPSReceiverSession::rtcmReceived);
    // These callbacks are queued before retirement, then delivered during the replacement session.
    emit first->sensorGpsUpdate(GPSDriverData::position(fix));
    emit first->RTCMDataUpdate(QByteArrayLiteral("stale corrections"));
    emit first->surveyInStatus(survey);
    emit first->satelliteInfoUpdate(GPSDriverData::satellites(satellites));
    emit first->receiverReady();
    emit first->connectionError(GPSConnectionError::DeviceError);
    session.start(GPSType::u_blox, blockedFactory(secondGate), {});
    QVERIFY(receiver.stopping());
    QVERIFY(!receiver.connected());
    QVERIFY(!surveyFacts->valid()->rawValue().toBool());
    QVERIFY(!surveyFacts->active()->rawValue().toBool());
    QVERIFY(qIsNaN(surveyFacts->currentLatitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(surveyFacts->currentAccuracy()->rawValue().toDouble()));
    QCOMPARE(surveyFacts->currentDuration()->rawValue().toInt(), 0);
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), 0);
    QTRY_VERIFY_WITH_TIMEOUT(secondGate->entered.available() > 0, TestTimeout::mediumMs());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(!receiver.connected());
    QCOMPARE(facts->lastError()->rawValue().toInt(), static_cast<int>(GPSConnectionError::None));
    QVERIFY(corrections.isEmpty());
    QVERIFY(positionUpdates.isEmpty());
    QVERIFY(!receiver.positionSource()->lastKnownPosition().isValid());
    emit receiver._session._provider->receiverReady();
    emit receiver._session._provider->RTCMDataUpdate(QByteArrayLiteral("new"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(receiver.connected());
    QCOMPARE(corrections.size(), 1);
    QCOMPARE(corrections.first().first().toByteArray(), QByteArrayLiteral("new"));
    firstGate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(first.isNull(), TestTimeout::mediumMs());
    QVERIFY(firstGate->sawCancellation);
    QVERIFY(receiver.connected());

    receiver._session._provider->stop();
    secondGate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
    QVERIFY(secondGate->sawCancellation);
    QVERIFY(!receiver.connected());
}

void GPSReceiverTest::_facadeDestructionDoesNotStopSession()
{
    TestFixtures::SettingsFixture saved;
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    auto gate = std::make_shared<BlockedOpen>();
    GPSReceiverSession session;
    auto receiver = std::make_unique<GPSReceiver>(session);
    const auto releaseWorker = qScopeGuard([&]() {
        session.stop();
        gate->release.release();
        session.shutdown();
    });
    session.start(GPSType::u_blox, blockedFactory(gate), {});
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> provider = receiver->_session._provider;
    emit provider->receiverReady();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(receiver->connected());
    receiver.reset();
    QVERIFY(provider);
    QVERIFY(session.hasReceiver());
    QCOMPARE(provider->parent(), &session);
    QVERIFY(!session.stopping());
    QVERIFY(!gate->sawCancellation);
    GPSReceiver replacement(session);
    QVERIFY(replacement.connected());
    session.stop();
    QVERIFY(!replacement.connected());
    gate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(provider.isNull(), TestTimeout::mediumMs());
    QVERIFY(gate->sawCancellation);
}

void GPSReceiverTest::_shutdownWithoutEventLoop_data()
{
    QTest::addColumn<QString>("phase");
    for (const char* phase : {"running", "retired", "finished", "awaiting-deletion", "before-start"}) {
        QTest::newRow(phase) << QString::fromLatin1(phase);
    }
}

void GPSReceiverTest::_shutdownWithoutEventLoop()
{
    QFETCH(QString, phase);
    auto gate = std::make_shared<BlockedOpen>();
    GPSReceiverSession session;
    GPSReceiver receiver(session);
    const auto releaseWorker = qScopeGuard([&]() {
        session.stop();
        gate->release.release();
        session.shutdown();
    });
    QPointer<GPSProvider> provider;
    if (phase == QStringLiteral("before-start")) {
        connect(&receiver, &GPSReceiver::receiverTypeChanged, &receiver, [&]() {
            provider = receiver._session._provider;
            session.shutdown();
        });
    }
    session.start(GPSType::u_blox,
                  [gate](const std::atomic_bool& stop) {
                      gate->entered.release();
                      while (!stop && !gate->release.tryAcquire(1, 10)) {
                      }
                      gate->sawCancellation = stop.load();
                      return std::unique_ptr<GPSTransport>();
                  },
                  {});

    if (phase != QStringLiteral("before-start")) {
        provider = receiver._session._provider;
        QVERIFY(provider);
        QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
        if (phase == QStringLiteral("retired")) {
            session.stop();
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
        session.shutdown();
        QVERIFY(gate->sawCancellation);
    }
    QVERIFY(provider.isNull());
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(!receiver.stopping());
    session.shutdown();
    session.start(GPSType::u_blox, blockedFactory(gate), {});
    QVERIFY(!receiver.hasReceiver());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(!receiver.stopping());
}

void GPSReceiverTest::_positionSourceSelection()
{
    TestFixtures::SettingsFixture saved;
    auto* enabled = SettingsManager::instance()->rtkSettings()->useReceiverPosition();
    saved.setFactValue(enabled, false);
    GPSManager manager;
    auto* receiver = manager.receiver();
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
    receiver->_sensorGpsUpdate(GPSDriverData::position(fix));
    QVERIFY(updates.isEmpty());
    enabled->setRawValue(true);
    QVERIFY(!position->gcsPosition().isValid());
    receiver->_sensorGpsUpdate(GPSDriverData::position(fix));
    QCOMPARE(updates.size(), 1);
    QCOMPARE(position->gcsPosition(), QGeoCoordinate(47, 8, 500));
    QVERIFY(receiver->connected());
    enabled->setRawValue(false);
    QVERIFY(receiver->connected());
    QVERIFY(!position->gcsPosition().isValid());
    receiver->_sensorGpsUpdate(GPSDriverData::position(fix));
    QCOMPARE(updates.size(), 1);
    enabled->setRawValue(true);
    QVERIFY(!position->gcsPosition().isValid());
    receiver->_sensorGpsUpdate(GPSDriverData::position(fix));
    QVERIFY(position->gcsPosition().isValid());
    receiver->_session.stop();
    QVERIFY(!position->gcsPosition().isValid());
    QVERIFY(!receiver->positionSource()->lastKnownPosition().isValid());
    receiver->_sensorGpsUpdate(GPSDriverData::position(fix));
    QVERIFY(!position->gcsPosition().isValid());
}

void GPSReceiverTest::_sourceHealthIndependentOfSurvey()
{
    GPSReceiverSession session;
    GPSReceiver receiver(session);
    GPSRtkState rtkState(session);
    session._provider = new GPSProvider({}, GPSType::u_blox, {}, {}, &session);
    session._capabilities = GPSReceiverCapabilities::forType(GPSType::u_blox);
    auto* facts = receiver.facts();
    receiver._onGPSConnect();
    QVERIFY(receiver.connected());
    QCOMPARE(receiver.health()->state(), GPSSourceHealth::NoData);
    GPSSurveyInStatus survey{};
    survey.valid = true;
    emit session.surveyInReceived(survey);
    QVERIFY(rtkState.facts()->valid()->rawValue().toBool());
    QVERIFY(!receiver.health()->usable());
    sensor_gps_s fix{};
    fix.fix_type = sensor_gps_s::FIX_TYPE_3D;
    fix.latitude_deg = 47;
    fix.longitude_deg = 8;
    fix.eph = 1;
    receiver._sensorGpsUpdate(GPSDriverData::position(fix));
    QVERIFY(receiver.health()->usable());
    fix.fix_type = sensor_gps_s::FIX_TYPE_NONE;
    receiver._sensorGpsUpdate(GPSDriverData::position(fix));
    QCOMPARE(receiver.health()->state(), GPSSourceHealth::Invalid);
    QVERIFY(receiver.connected());
    QVERIFY(rtkState.facts()->valid()->rawValue().toBool());
    satellite_info_s satellites{};
    satellites.count = 2;
    satellites.used[0] = 1;
    receiver._satelliteInfoUpdate(GPSDriverData::satellites(satellites));
    QCOMPARE(receiver.health()->satellitesInViewCount(), 2);
    QCOMPARE(receiver.health()->satellitesInUseCount(), 1);
    satellites.timestamp = 1;
    receiver._satelliteInfoUpdate(GPSDriverData::satellites(satellites));
    QCOMPARE(receiver.health()->satellitesInViewCount(), -1);
    QCOMPARE(receiver.health()->satellitesInUseCount(), -1);
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), 0);
    session.stop();
    QCOMPARE(receiver.health()->state(), GPSSourceHealth::NoData);
}
