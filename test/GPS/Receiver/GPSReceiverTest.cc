#include "GPSReceiverTest.h"

#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtTest/QSignalSpy>

#include "Fixtures/RAIIFixtures.h"
#include "GPSBaseStationFactGroup.h"
#include "GPSBaseStationState.h"
#include "GPSDriverData.h"
#include "GPSManager.h"
#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "GPSReceiverTestProfile.h"
#include "GPSReplayScheduler.h"
#include "GPSTransport.h"
#include "PositionManager.h"
#include "QGroundControlQmlGlobal.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#include "TestGPSPositionSource.h"
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
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), -1);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), -1);
    QCOMPARE(facts->lastError()->rawValue().toUInt(), 0U);
    QCOMPARE(facts->getFact(QStringLiteral("lat")), facts->lat());
    QCOMPARE(facts->getFact(QStringLiteral("rtk.valid")), facts->rtk()->valid());
    QCOMPARE(facts->numSatellites(), facts->count());
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
    session.start(gpsReceiverTestProfile({}, GPSType::u_blox), {});
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
    GPSBaseStationState baseStationState(session, *receiver.facts()->rtk());
    auto* surveyFacts = receiver.facts()->rtk();
    const auto releaseWorkers = qScopeGuard([&]() {
        session.stop();
        firstGate->release.release();
        secondGate->release.release();
        session.shutdown();
    });
    session.start(gpsReceiverTestProfile({}, GPSType::u_blox), blockedFactory(firstGate));
    QTRY_VERIFY_WITH_TIMEOUT(firstGate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> first = receiver._session._provider;
    auto* facts = receiver.facts();
    QVERIFY(!receiver.connected());
    QSignalSpy positionUpdates(&session, &GPSReceiverSession::positionReceived);
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
    session.start(gpsReceiverTestProfile({}, GPSType::u_blox), blockedFactory(secondGate));
    QVERIFY(receiver.stopping());
    QVERIFY(!receiver.connected());
    QVERIFY(!surveyFacts->valid()->rawValue().toBool());
    QVERIFY(!surveyFacts->active()->rawValue().toBool());
    QVERIFY(qIsNaN(surveyFacts->currentLatitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(surveyFacts->currentAccuracy()->rawValue().toDouble()));
    QCOMPARE(surveyFacts->currentDuration()->rawValue().toInt(), 0);
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), -1);
    QTRY_VERIFY_WITH_TIMEOUT(secondGate->entered.available() > 0, TestTimeout::mediumMs());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(!receiver.connected());
    QCOMPARE(facts->lastError()->rawValue().toInt(), static_cast<int>(GPSConnectionError::None));
    QVERIFY(corrections.isEmpty());
    QVERIFY(positionUpdates.isEmpty());
    QVERIFY(!receiver.health()->observation().position.isValid());
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
    session.start(gpsReceiverTestProfile({}, GPSType::u_blox), blockedFactory(gate));
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
    session.start(gpsReceiverTestProfile({}, GPSType::u_blox), [gate](const std::atomic_bool& stop) {
        gate->entered.release();
        while (!stop && !gate->release.tryAcquire(1, 10)) {
        }
        gate->sawCancellation = stop.load();
        return std::unique_ptr<GPSTransport>();
    });

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
    session.start(gpsReceiverTestProfile({}, GPSType::u_blox), blockedFactory(gate));
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
    auto& session = *manager.receiverSession();
    const auto gate = std::make_shared<BlockedOpen>();
    const auto cleanup = qScopeGuard([&]() {
        session.stop();
        gate->release.release();
        manager.shutdown();
    });
    session.start(gpsReceiverTestProfile(), blockedFactory(gate));
    emit session._provider->receiverReady();
    QCoreApplication::sendPostedEvents(&session, QEvent::MetaCall);
    sensor_gps_s fix{};
    fix.fix_type = sensor_gps_s::FIX_TYPE_RTK_FIXED;
    fix.latitude_deg = 47;
    fix.longitude_deg = 8;
    fix.altitude_msl_m = 500;
    fix.eph = 0.1f;
    fix.epv = 0.2f;
    const auto publish = [&]() {
        auto observation = GPSDriverData::position(fix);
        observation.sessionId = session.sessionId();
        receiver->_sensorGpsUpdate(observation);
    };
    publish();
    QVERIFY(!position->gcsPosition().isValid());
    enabled->setRawValue(true);
    QVERIFY(!position->gcsPosition().isValid());
    publish();
    QCOMPARE(position->gcsPosition(), QGeoCoordinate(47, 8, 500));
    QVERIFY(receiver->connected());
    enabled->setRawValue(false);
    QVERIFY(receiver->connected());
    QVERIFY(!position->gcsPosition().isValid());
    publish();
    enabled->setRawValue(true);
    QVERIFY(!position->gcsPosition().isValid());
    publish();
    QVERIFY(position->gcsPosition().isValid());
    session.stop();
    QVERIFY(!position->gcsPosition().isValid());
    QVERIFY(!receiver->health()->observation().position.isValid());
    publish();
    QVERIFY(!position->gcsPosition().isValid());
}

void GPSReceiverTest::_sourceHealthIndependentOfSurvey()
{
    GPSReceiverSession session;
    GPSReceiver receiver(session);
    GPSBaseStationState baseStationState(session, *receiver.facts()->rtk());
    const auto profile = gpsReceiverTestProfile();
    session._attempt = {++session._generation,
                        std::make_shared<const GPSReceiverProfile>(profile),
                        GPSReceiverAttempt::Phase::Ready,
                        GPSConnectionError::None,
                        {}};
    session._provider = new GPSProvider({}, GPSType::u_blox, profile.receiver, {}, &session);
    receiver._satellites.beginSession(QStringLiteral("nativeReceiver"), session.sessionId());
    session._capabilities = GPSReceiverCapabilities::forType(GPSType::u_blox);
    auto* facts = receiver.facts();
    receiver._onGPSConnect();
    QVERIFY(receiver.connected());
    QCOMPARE(receiver.health()->state(), GPSSourceHealth::NoData);
    GPSSurveyInStatus survey{};
    survey.valid = true;
    emit session.surveyInReceived(survey);
    QVERIFY(receiver.facts()->rtk()->valid()->rawValue().toBool());
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
    QVERIFY(receiver.facts()->rtk()->valid()->rawValue().toBool());
    satellite_info_s satellites{};
    satellites.count = 2;
    satellites.used[0] = 1;
    auto satelliteObservation = GPSDriverData::satellites(satellites);
    satelliteObservation.sessionId = session.sessionId();
    receiver._satelliteInfoUpdate(satelliteObservation);
    QCOMPARE(receiver.health()->satellitesInViewCount(), 2);
    QCOMPARE(receiver.health()->satellitesInUseCount(), 1);
    satellites.timestamp = 1;
    satelliteObservation = GPSDriverData::satellites(satellites);
    satelliteObservation.sessionId = session.sessionId();
    receiver._satelliteInfoUpdate(satelliteObservation);
    QCOMPARE(receiver.health()->satellitesInViewCount(), 2);
    receiver._satellites.setFreshnessTimeoutMs(1);
    QTRY_COMPARE_WITH_TIMEOUT(receiver.health()->satellitesInViewCount(), -1, TestTimeout::shortMs());
    QCOMPARE(receiver.health()->satellitesInUseCount(), -1);
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), -1);
    session.stop();
    QCOMPARE(receiver.health()->state(), GPSSourceHealth::NoData);
}

void GPSReceiverTest::_liveFactsFollowHealth()
{
    GPSReceiverSession session;
    GPSReceiver receiver(session);
    const auto gate = std::make_shared<BlockedOpen>();
    const auto cleanup = qScopeGuard([&]() {
        session.stop();
        gate->release.release();
        session.shutdown();
    });
    session.start(gpsReceiverTestProfile(), blockedFactory(gate));
    emit session._provider->receiverReady();
    QCoreApplication::sendPostedEvents(&session, QEvent::MetaCall);
    auto* facts = receiver.facts();
    facts->rtk()->currentLatitude()->setRawValue(48.0);
    facts->rtk()->valid()->setRawValue(true);
    GPSObservation fix;
    fix.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    fix.position = QGeoPositionInfo(QGeoCoordinate(47.3, 8.54), QDateTime::currentDateTimeUtc());
    fix.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
    fix.position.setAttribute(QGeoPositionInfo::Direction, 90);
    fix.trueHeadingDegrees = 180;
    fix.horizontalDop = 1.6;
    fix.fixQuality = GPSObservation::FixQuality::RTKFixed;
    receiver._sensorGpsUpdate(fix);
    QCOMPARE(facts->lat()->rawValue().toDouble(), 47.3);
    QCOMPARE(facts->lock()->rawValue().toInt(), 6);
    QCOMPARE(facts->courseOverGround()->rawValue().toDouble(), 90.0);
    QCOMPARE(facts->yaw()->rawValue().toDouble(), 180.0);
    GPSSatelliteObservation satelliteReport;
    satelliteReport.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    satelliteReport.sessionId = session.sessionId();
    satelliteReport.provenance = {{GPSSatellite::Constellation::GPS, satelliteReport.monotonicTimestampUs,
                                   satelliteReport.monotonicTimestampUs, 17}};
    for (int i = 0; i < 20; ++i) {
        GPSSatellite satellite;
        satellite.id = i + 1;
        satellite.constellation = GPSSatellite::Constellation::GPS;
        satellite.used = i < 17;
        satelliteReport.satellites.append(satellite);
    }
    receiver._satelliteInfoUpdate(satelliteReport);
    QCOMPARE(facts->count()->rawValue().toInt(), 20);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), 17);
    fix.monotonicTimestampUs = GPSObservation::monotonicNowUs() - 6000000;
    receiver._sensorGpsUpdate(fix);
    QCOMPARE(receiver.health()->state(), GPSSourceHealth::Stale);
    QVERIFY(qIsNaN(facts->lat()->rawValue().toDouble()));
    QCOMPARE(facts->lock()->rawValue().toInt(), 0);
    QVERIFY(!facts->telemetryAvailable());
    QCOMPARE(facts->rtk()->currentLatitude()->rawValue().toDouble(), 48.0);
    QVERIFY(facts->rtk()->valid()->rawValue().toBool());
    fix.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    receiver._sensorGpsUpdate(fix);
    QVERIFY(facts->telemetryAvailable());
    session.stop();
    QVERIFY(qIsNaN(facts->lat()->rawValue().toDouble()));
    QCOMPARE(facts->count()->rawValue().toInt(), -1);
    QVERIFY(!facts->telemetryAvailable());
}

void GPSReceiverTest::_projectionHandlesReentrantIntegrity_data()
{
    QTest::addColumn<QString>("action");
    for (const auto& action : {"reset", "replace", "disconnect", "destroy", "replace-during-reset"}) {
        QTest::newRow(action) << QString::fromLatin1(action);
    }
}

void GPSReceiverTest::_projectionHandlesReentrantIntegrity()
{
    QFETCH(QString, action);
    GPSReceiverSession session;
    auto receiver = std::make_unique<GPSReceiver>(session);
    GPSObservation observation;
    observation.position = QGeoPositionInfo(QGeoCoordinate(47, 8), QDateTime::currentDateTimeUtc());
    observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    observation.jammingState = 1;
    if (action == QStringLiteral("replace-during-reset")) {
        receiver->health()->updateObservation(observation);
    }
    bool applied = false;
    connect(receiver->facts()->integrity()->jammingState(), &Fact::rawValueChanged, &session, [&]() {
        if (applied) {
            return;
        }
        applied = true;
        if (action == QStringLiteral("destroy")) {
            receiver.reset();
        } else if (action == QStringLiteral("disconnect")) {
            receiver->_onGPSDisconnect();
        } else if (action.startsWith(QStringLiteral("replace"))) {
            auto replacement = observation;
            replacement.position.setCoordinate(QGeoCoordinate(48, 9));
            replacement.jammingState = 2;
            receiver->health()->updateObservation(replacement);
        } else {
            receiver->health()->reset();
        }
    });
    if (action == QStringLiteral("replace-during-reset")) {
        receiver->health()->reset();
    } else {
        receiver->health()->updateObservation(observation);
    }
    QVERIFY(applied);
    if (action == QStringLiteral("destroy")) {
        QVERIFY(!receiver);
    } else if (action.startsWith(QStringLiteral("replace"))) {
        QCOMPARE(receiver->facts()->lat()->rawValue().toDouble(), 48.0);
        QCOMPARE(receiver->facts()->lon()->rawValue().toDouble(), 9.0);
        QCOMPARE(receiver->facts()->integrity()->jammingState()->rawValue().toInt(), 2);
    } else {
        QVERIFY(qIsNaN(receiver->facts()->lat()->rawValue().toDouble()));
        QVERIFY(!receiver->facts()->telemetryAvailable());
    }
}

void GPSReceiverTest::_retainedPositionFactsExpire()
{
    GPSReplayScheduler scheduler;
    GPSReceiverSession session;
    GPSReceiver receiver(session, nullptr, &scheduler);
    receiver.health()->setFreshnessTimeoutMs(100);
    GPSObservation observation;
    observation.position = QGeoPositionInfo(QGeoCoordinate(47, 8), QDateTime::currentDateTimeUtc());
    observation.monotonicTimestampUs = scheduler.nowUs();
    receiver.health()->updateObservation(observation);
    QCOMPARE(receiver.health()->state(), GPSSourceHealth::Invalid);
    QCOMPARE(receiver.facts()->lat()->rawValue().toDouble(), 47.0);
    QVERIFY(receiver.facts()->telemetryAvailable());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(100)));
    QCOMPARE(receiver.health()->state(), GPSSourceHealth::Stale);
    QVERIFY(qIsNaN(receiver.facts()->lat()->rawValue().toDouble()));
    QVERIFY(!receiver.facts()->telemetryAvailable());
}
