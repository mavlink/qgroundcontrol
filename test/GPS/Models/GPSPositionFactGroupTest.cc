#include "GPSPositionFactGroupTest.h"

#include <QtCore/QScopeGuard>
#include <QtCore/QThread>

#include <memory>

#include "GPSPositionFactGroup.h"
#include "GPSReceiverFactGroup.h"
#include "GPSRelativePositionModel.h"
#include "ManualScheduler.h"

void GPSPositionFactGroupTest::_localObservation()
{
    GPSIntegrityStore store;
    GPSPositionFactGroup facts(store);
    GPSObservation fix;
    fix.position = QGeoPositionInfo(QGeoCoordinate(47.3, 8.54), QDateTime::currentDateTimeUtc());
    fix.position.setAttribute(QGeoPositionInfo::Direction, 90);
    fix.trueHeadingDegrees = 180;
    fix.horizontalDop = 1.6;
    fix.verticalDop = 2.1;
    fix.fixQuality = GPSObservation::FixQuality::RTKFixed;
    facts.updatePosition(fix);
    QCOMPARE(facts.lat()->rawValue().toDouble(), 47.3);
    QCOMPARE(facts.lon()->rawValue().toDouble(), 8.54);
    QCOMPARE(facts.hdop()->rawValue().toDouble(), 1.6);
    QCOMPARE(facts.vdop()->rawValue().toDouble(), 2.1);
    QCOMPARE(facts.courseOverGround()->rawValue().toDouble(), 90.0);
    QCOMPARE(facts.yaw()->rawValue().toDouble(), 180.0);
    QCOMPARE(facts.lock()->rawValue().toInt(), 6);
    fix.horizontalDop.reset();
    fix.trueHeadingDegrees.reset();
    fix.position.removeAttribute(QGeoPositionInfo::Direction);
    fix.fixQuality = GPSObservation::FixQuality::Extrapolated;
    facts.updatePosition(fix);
    QVERIFY(qIsNaN(facts.hdop()->rawValue().toDouble()));
    QVERIFY(qIsNaN(facts.courseOverGround()->rawValue().toDouble()));
    QVERIFY(qIsNaN(facts.yaw()->rawValue().toDouble()));
    QCOMPARE(facts.lock()->rawValue().toInt(), 0);
    facts.resetPosition();
    QVERIFY(qIsNaN(facts.lat()->rawValue().toDouble()));
    QVERIFY(!facts.telemetryAvailable());
}

void GPSPositionFactGroupTest::_resetDuringUpdate()
{
    GPSIntegrityStore store;
    GPSPositionFactGroup facts(store);
    bool reset = false;
    connect(facts.lat(), &Fact::rawValueChanged, this, [&]() {
        if (!reset) {
            reset = true;
            facts.resetPosition();
        }
    });
    GPSObservation fix;
    fix.position = QGeoPositionInfo(QGeoCoordinate(47.3, 8.54), QDateTime::currentDateTimeUtc());
    fix.fixQuality = GPSObservation::FixQuality::RTKFixed;
    facts.updatePosition(fix);
    QVERIFY(reset);
    QVERIFY(qIsNaN(facts.lat()->rawValue().toDouble()));
    QVERIFY(qIsNaN(facts.lon()->rawValue().toDouble()));
    QCOMPARE(facts.lock()->rawValue().toInt(), 0);
    QVERIFY(!facts.telemetryAvailable());
}

void GPSPositionFactGroupTest::_integrityExpiryAndReentrancy()
{
    GPSIntegrityStore store;
    GPSIntegrityFactGroup facts(store);
    GPSIntegrityObservation observation;
    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs() - 4900000;
    observation.jammingState = 3;
    store.updateObservation(observation);
    QVERIFY(facts.available());
    QTRY_VERIFY_WITH_TIMEOUT(!facts.available(), 2000);
    QCOMPARE(facts.jammingState()->rawValue().toInt(), 255);
    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs() - 6000000;
    store.updateObservation(observation);
    QVERIFY(!facts.available());
    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    observation.spoofingState = 3;
    bool reset = false;
    connect(facts.spoofingState(), &Fact::rawValueChanged, this, [&]() {
        if (!reset) {
            reset = true;
            store.reset();
        }
    });
    store.updateObservation(observation);
    QVERIFY(reset);
    QVERIFY(!facts.available());
    QCOMPARE(facts.jammingState()->rawValue().toInt(), 255);
}

void GPSPositionFactGroupTest::_independentIntegrityReports()
{
    GPSIntegrityStore store;
    GPSIntegrityFactGroup facts(store);
    GPSIntegrityObservation observation;
    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    observation.jammingState = 3;
    observation.spoofingState = 1;
    observation.correctionsUsed = 2;
    observation.provenance = GPSIntegrityProvenance{
        .jammingTimestampUs = observation.monotonicTimestampUs - 6000000,
        .spoofingTimestampUs = observation.monotonicTimestampUs,
        .correctionsTimestampUs = observation.monotonicTimestampUs - 6000000,
    };
    store.updateObservation(observation);
    QVERIFY(facts.available());
    QCOMPARE(facts.jammingState()->rawValue().toInt(), 255);
    QCOMPARE(facts.spoofingState()->rawValue().toInt(), 1);
    QCOMPARE(facts.correctionsUsed()->rawValue().toInt(), 255);
    for (int i = 0; i < 10; ++i) {
        observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
        store.updateObservation(observation);
        QCOMPARE(facts.jammingState()->rawValue().toInt(), 255);
        QCOMPARE(facts.correctionsUsed()->rawValue().toInt(), 255);
    }
    observation.provenance->jammingTimestampUs = GPSObservation::monotonicNowUs() - 4900000;
    store.updateObservation(observation);
    QCOMPARE(facts.jammingState()->rawValue().toInt(), 3);
    QTRY_COMPARE_WITH_TIMEOUT(facts.jammingState()->rawValue().toInt(), 255, 2000);
    QCOMPARE(facts.spoofingState()->rawValue().toInt(), 1);
    observation.provenance->jammingTimestampUs = GPSObservation::monotonicNowUs();
    // An unchanged report renews its own freshness even when another diagnostic group is stale.
    observation.monotonicTimestampUs -= 6000000;
    store.updateObservation(observation);
    QCOMPARE(facts.jammingState()->rawValue().toInt(), 3);
    QCOMPARE(facts.correctionsUsed()->rawValue().toInt(), 255);
    store.reset();
    QVERIFY(!facts.available());
}

void GPSPositionFactGroupTest::_receiverMetadata()
{
    ManualScheduler scheduler;
    GPSIntegrityStore store(nullptr, &scheduler);
    GPSReceiverFactGroup receiver(store);
    QCOMPARE(receiver.numSatellites(), receiver.count());
    QCOMPARE(receiver.numSatellites()->rawValue().toInt(), -1);
    QCOMPARE(receiver.numSatellitesUsed()->rawValue().toInt(), -1);
    QCOMPARE(receiver.lat()->metaData()->parent(), &receiver);
    QCOMPARE(receiver.integrity()->jammingState()->metaData()->parent(), receiver.integrity());
    QCOMPARE(receiver.rtk()->currentAccuracy()->metaData()->parent(), receiver.rtk());
    QCOMPARE(receiver.getFact(QStringLiteral("rtk.currentAccuracy")), receiver.rtk()->currentAccuracy());
    receiver.rtk()->currentAccuracy()->setRawValue(0.25);
    QCOMPARE(receiver.rtk()->currentAccuracy()->rawValue().toDouble(), 0.25);
    GPSObservation fix;
    fix.position = QGeoPositionInfo(QGeoCoordinate(47.3, 8.54), QDateTime::currentDateTimeUtc());
    fix.fixQuality = GPSObservation::FixQuality::Fix3D;
    receiver.updatePosition(fix);
    QCOMPARE(receiver.lat()->rawValue().toDouble(), 47.3);
    QCOMPARE(receiver.rtk()->currentAccuracy()->rawValue().toDouble(), 0.25);
}

UT_REGISTER_TEST(GPSPositionFactGroupTest, TestLabel::Unit)

void GPSPositionFactGroupTest::_reentrantAvailabilityNotification_data()
{
    QTest::addColumn<bool>("viaTelemetry");
    QTest::newRow("fact-change") << false;
    QTest::newRow("telemetry-change") << true;
}

void GPSPositionFactGroupTest::_reentrantAvailabilityNotification()
{
    QFETCH(bool, viaTelemetry);
    ManualScheduler scheduler;
    GPSIntegrityStore store(nullptr, &scheduler);
    GPSIntegrityFactGroup facts(store);
    QSignalSpy changed(&facts, &GPSIntegrityFactGroup::availabilityChanged);
    GPSIntegrityObservation observation;
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.systemErrors = 0;
    observation.spoofingState = 1;
    bool replaced = false;
    const auto replace = [&]() {
        if (!replaced) {
            replaced = true;
            observation.spoofingState = 2;
            store.updateObservation(observation);
        }
    };
    if (viaTelemetry) {
        connect(&facts, &FactGroup::telemetryAvailableChanged, this, replace);
    } else {
        connect(facts.spoofingState(), &Fact::rawValueChanged, this, replace);
    }
    store.updateObservation(observation);
    QVERIFY(replaced);
    QVERIFY(facts.available());
    QVERIFY(facts.systemErrorsKnown());
    QCOMPARE(facts.spoofingState()->rawValue().toInt(), 2);
    QCOMPARE(changed.count(), 1);
    store.updateObservation(observation);
    QCOMPARE(changed.count(), 1);
}

void GPSPositionFactGroupTest::_foreignStoresRejected()
{
    QThread worker;
    GPSIntegrityStore integrity;
    GPSRelativePositionStore relative;
    const auto owner = QThread::currentThread();
    QVERIFY(integrity.moveToThread(&worker));
    QVERIFY(relative.moveToThread(&worker));
    worker.start();
    const auto cleanup = qScopeGuard([&]() {
        QMetaObject::invokeMethod(&integrity, [&]() { integrity.moveToThread(owner); }, Qt::BlockingQueuedConnection);
        QMetaObject::invokeMethod(&relative, [&]() { relative.moveToThread(owner); }, Qt::BlockingQueuedConnection);
        worker.quit();
        worker.wait();
    });
    const QRegularExpression warning(QStringLiteral("Store must share the projection thread"));
    expectLogMessage("GPS.Models.GPSIntegrityFactGroup", QtWarningMsg, warning);
    GPSIntegrityFactGroup facts(integrity);
    verifyExpectedLogMessage();
    QVERIFY(!facts.available());
    expectLogMessage("GPS.Models.GPSRelativePositionModel", QtWarningMsg, warning);
    GPSRelativePositionModel model(relative);
    verifyExpectedLogMessage();
    QVERIFY(!model.fresh());
}

void GPSPositionFactGroupTest::_integrityStoreDestruction()
{
    ManualScheduler scheduler;
    auto store = std::make_unique<GPSIntegrityStore>(nullptr, &scheduler);
    GPSIntegrityFactGroup facts(*store);
    GPSIntegrityObservation observation;
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.jammingState = 1;
    store->updateObservation(observation);
    QVERIFY(facts.available());
    QSignalSpy changed(&facts, &GPSIntegrityFactGroup::availabilityChanged);
    store.reset();
    QVERIFY(!facts.available());
    QCOMPARE(facts.jammingState()->rawValue().toInt(), 255);
    QCOMPARE(changed.count(), 1);
}
