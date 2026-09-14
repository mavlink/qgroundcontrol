#include "GPSRtkTest.h"

#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtTest/QSignalSpy>

#include "Fixtures/RAIIFixtures.h"
#include "GPSRTKFactGroup.h"
#include "GPSRtk.h"
#include "GPSTransport.h"
#include "NTRIPManager.h"
#include "QGroundControlQmlGlobal.h"
#include "RTCMMavlink.h"
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
    expectLogMessage("GPS.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport")));
    receiver.connectReceiver(GPSType::u_blox, {});
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
    receiver._disconnectTimeoutMs = 0;
    const auto releaseWorkers = qScopeGuard([&]() {
        firstGate->release.release();
        secondGate->release.release();
        receiver._disconnectTimeoutMs = TestTimeout::mediumMs();
    });
    receiver.connectReceiver(GPSType::u_blox, blockedFactory(firstGate));
    QTRY_VERIFY_WITH_TIMEOUT(firstGate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> first = receiver._gpsProvider;
    auto* facts = qobject_cast<GPSRTKFactGroup*>(receiver.gpsRtkFactGroup());
    QVERIFY(!receiver.connected());
    emit first->receiverReady();
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
    QVERIFY(facts->valid()->rawValue().toBool());
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), 2);

    RTCMMavlink forwarder;
    auto* manager = NTRIPManager::instance();
    auto* previousForwarder = manager->rtcmMavlink();
    manager->setRtcmMavlink(&forwarder);
    const auto restoreForwarder = qScopeGuard([&]() { manager->setRtcmMavlink(previousForwarder); });
    auto* rtcm = &forwarder;
    const auto bytesBefore = rtcm->totalBytesSent();
    // These callbacks are queued before retirement, then delivered during the replacement session.
    emit first->RTCMDataUpdate(QByteArrayLiteral("stale corrections"));
    emit first->surveyInStatus(survey);
    emit first->satelliteInfoUpdate(satellites);
    emit first->receiverReady();
    emit first->connectionError(GPSConnectionError::DeviceError);
    expectLogMessage(
        "GPS.GPSRtk", QtWarningMsg,
        QRegularExpression(QStringLiteral("GPS thread did not exit in time; deferring cleanup to finished")));
    receiver.connectReceiver(GPSType::u_blox, blockedFactory(secondGate));
    verifyExpectedLogMessage();
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
    receiver->_disconnectTimeoutMs = 0;
    const auto releaseWorker = qScopeGuard([&]() { gate->release.release(); });
    receiver->connectReceiver(GPSType::u_blox, blockedFactory(gate));
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> provider = receiver->_gpsProvider;
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
