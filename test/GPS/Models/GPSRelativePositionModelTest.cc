#include "GPSRelativePositionModelTest.h"

#include <QtTest/QSignalSpy>

#include <cmath>
#include <limits>
#include <memory>

#include "GPSRelativePositionModel.h"
#include "ManualScheduler.h"

void GPSRelativePositionModelTest::_validityAndZeroBaseline()
{
    GPSRelativePositionStore store;
    GPSRelativePositionModel model(store);
    QVERIFY(std::isnan(model.north()));
    QVERIFY(std::isnan(model.heading()));
    QCOMPARE(model.referenceStationId(), -1);
    store.beginSession(QStringLiteral("nativeReceiver"), 4);
    GPSRelativeObservation report;
    report.sessionId = 4;
    report.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    report.referenceStationId = 12;
    report.fixValid = true;
    report.positionValid = true;
    report.carrierFixed = true;
    report.normalized = true;
    report.positionNedMeters = {0, -1, 2};
    report.accuracyNedMeters = {0, 0.02, 0.03};
    report.headingDegrees = 0;
    report.headingAccuracyDegrees = 0;
    store.updateObservation(report);
    QVERIFY(model.fresh());
    QCOMPARE(model.north(), 0.0);
    QCOMPARE(model.east(), -1.0);
    QCOMPARE(model.down(), 2.0);
    QCOMPARE(model.northAccuracy(), 0.0);
    QCOMPARE(model.heading(), 0.0);
    QCOMPARE(model.headingAccuracy(), 0.0);
    QVERIFY(model.carrierFixed());
    QVERIFY(model.normalized().toBool());
    QCOMPARE(model.referenceStationId(), 12);
    report.positionValid = false;
    report.headingDegrees.reset();
    store.updateObservation(report);
    QVERIFY(std::isnan(model.north()));
    QVERIFY(std::isnan(model.northAccuracy()));
    QVERIFY(std::isnan(model.heading()));
    QVERIFY(std::isnan(model.headingAccuracy()));
    report.positionValid = true;
    report.positionNedMeters[0] = std::numeric_limits<double>::infinity();
    report.accuracyNedMeters[1] = -1;
    store.updateObservation(report);
    QVERIFY(std::isnan(model.north()));
    QVERIFY(std::isnan(model.eastAccuracy()));
}

void GPSRelativePositionModelTest::_freshnessAndSessionIsolation()
{
    GPSRelativePositionStore store(nullptr, 100);
    GPSRelativePositionModel model(store);
    store.beginSession(QStringLiteral("nativeReceiver"), 1);
    GPSRelativeObservation report;
    report.sessionId = 1;
    report.monotonicTimestampUs = GPSObservation::monotonicNowUs() - 10000;
    report.fixValid = report.positionValid = true;
    report.movingBase = true;
    report.positionNedMeters[0] = 5;
    store.updateObservation(report);
    QVERIFY(model.fresh());
    report.monotonicTimestampUs -= 1000;
    report.positionNedMeters[0] = 99;
    store.updateObservation(report);
    QCOMPARE(model.north(), 5.0);
    report.monotonicTimestampUs = GPSObservation::monotonicNowUs() + 1000000;
    store.updateObservation(report);
    QCOMPARE(model.north(), 5.0);
    QTRY_VERIFY_WITH_TIMEOUT(!model.fresh(), 1000);
    QVERIFY(std::isnan(model.north()));
    QVERIFY(!model.movingBase().isValid());
    QCOMPARE(model.referenceStationId(), -1);
    store.beginSession(QStringLiteral("nativeReceiver"), 2);
    report.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    store.updateObservation(report);
    QVERIFY(!model.fresh());
    report.sessionId = 2;
    store.updateObservation(report);
    QCOMPARE(model.north(), 99.0);
    store.reset();
    QVERIFY(!model.fresh());
    QVERIFY(model.sourceId().isEmpty());
}

void GPSRelativePositionModelTest::_reentrantReplacement()
{
    GPSRelativePositionStore store;
    GPSRelativePositionModel model(store);
    store.beginSession(QStringLiteral("nativeReceiver"), 1);
    connect(&model, &GPSRelativePositionModel::stateChanged, &model, [&]() {
        if (model.fresh() && model.sessionId() == 1) {
            store.beginSession(QStringLiteral("replacement"), 2);
        }
    });
    GPSRelativeObservation report;
    report.sessionId = 1;
    report.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    store.updateObservation(report);
    QCOMPARE(model.sessionId(), 2ULL);
    QVERIFY(!model.fresh());
    QVERIFY(std::isnan(model.length()));
}

UT_REGISTER_TEST(GPSRelativePositionModelTest, TestLabel::Unit)

void GPSRelativePositionModelTest::_virtualExpiryAndUnchangedPublication()
{
    ManualScheduler scheduler;
    GPSRelativePositionStore store(nullptr, 100, &scheduler);
    GPSRelativePositionModel model(store);
    store.beginSession(QStringLiteral("receiver"), 2);
    GPSRelativeObservation report;
    report.sessionId = 2;
    report.monotonicTimestampUs = scheduler.nowUs();
    report.fixValid = true;
    report.positionValid = true;
    report.positionNedMeters = {0, 1, 2};
    store.updateObservation(report);
    QSignalSpy changed(&model, &GPSRelativePositionModel::stateChanged);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(50)));
    report.monotonicTimestampUs = scheduler.nowUs();
    store.updateObservation(report);
    QCOMPARE(changed.count(), 0);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(99)));
    QVERIFY(model.fresh());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QVERIFY(!model.fresh());
    QCOMPARE(changed.count(), 1);
    QVERIFY(std::isnan(model.north()));
    QVERIFY(!model.movingBase().isValid());
    store.updateObservation(report);
    QCOMPARE(changed.count(), 1);
}

void GPSRelativePositionModelTest::_borrowedStoreSurvivesPresentation()
{
    ManualScheduler scheduler;
    auto store = std::make_unique<GPSRelativePositionStore>(nullptr, 100, &scheduler);
    store->beginSession(QStringLiteral("receiver"), 1);
    GPSRelativeObservation observation;
    observation.sessionId = 1;
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.fixValid = true;
    observation.positionValid = true;
    observation.positionNedMeters = {1, 2, 3};
    store->updateObservation(observation);
    {
        GPSRelativePositionModel first(*store);
        QCOMPARE(first.north(), 1.0);
    }
    QVERIFY(store->fresh());
    GPSRelativePositionModel replacement(*store);
    QCOMPARE(replacement.east(), 2.0);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(100)));
    QVERIFY(!replacement.fresh());
    QVERIFY(std::isnan(replacement.north()));
    store.reset();
    QVERIFY(replacement.sourceId().isEmpty());
    QCOMPARE(replacement.sessionId(), 0ULL);
}
