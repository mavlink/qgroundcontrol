#include "GPSAcceptedStateTest.h"

#include <memory>

#include "GPSIntegrityStore.h"
#include "GPSRelativePositionStore.h"
#include "GPSSatelliteStore.h"
#include "GPSSourceHealth.h"
#include "ManualScheduler.h"

void GPSAcceptedStateTest::_relativeExpiryAndSession()
{
    ManualScheduler scheduler;
    GPSRelativePositionStore store(nullptr, 100, &scheduler);
    store.beginSession(QStringLiteral("receiver"), 5);
    GPSRelativeObservation sample;
    sample.sessionId = 5;
    sample.monotonicTimestampUs = scheduler.nowUs();
    sample.headingDegrees = 42;
    store.updateObservation(sample);
    QVERIFY(store.fresh());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(50)));
    const auto currentStamp = scheduler.nowUs();
    sample.monotonicTimestampUs = currentStamp;
    sample.headingDegrees = 43;
    store.updateObservation(sample);
    sample.monotonicTimestampUs -= 1;
    sample.headingDegrees = 99;
    store.updateObservation(sample);
    QCOMPARE(store.observation().headingDegrees, std::optional<double>(43));
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(50)));
    QVERIFY(store.fresh());
    const auto connection =
        connect(&store, &GPSRelativePositionStore::observationChanged, &store, [&](const GPSRelativeObservation&) {
            if (!store.fresh()) {
                store.beginSession(QStringLiteral("replacement"), 6);
            }
        });
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(50)));
    QCOMPARE(store.sourceId(), QStringLiteral("replacement"));
    QCOMPARE(store.sessionId(), quint64(6));
    QVERIFY(!store.fresh());
    disconnect(connection);
    sample.monotonicTimestampUs = scheduler.nowUs();
    store.updateObservation(sample);
    QVERIFY(!store.fresh());
    sample.sessionId = 6;
    store.updateObservation(sample);
    QVERIFY(store.fresh());
    store.reset();
    QVERIFY(!store.fresh());
    QCOMPARE(scheduler.pendingCount(), qsizetype(0));
}

void GPSAcceptedStateTest::_integrityProvenanceExpiry()
{
    ManualScheduler scheduler;
    GPSIntegrityStore store(nullptr, &scheduler);
    store.beginSession(8);
    GPSIntegrityObservation sample;
    sample.sessionId = 8;
    sample.monotonicTimestampUs = scheduler.nowUs();
    sample.jammingState = 0;
    sample.systemErrors = 0;
    store.updateObservation(sample);
    QVERIFY(store.available());
    QVERIFY(store.systemErrorsKnown());
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(2)));
    const auto oldStamp = sample.monotonicTimestampUs;
    sample.monotonicTimestampUs = scheduler.nowUs();
    sample.provenance = GPSIntegrityProvenance{oldStamp, scheduler.nowUs(), 0, 0, 0};
    sample.spoofingState = 1;
    store.updateObservation(sample);
    sample.monotonicTimestampUs += 1000000;
    sample.spoofingState = 2;
    store.updateObservation(sample);
    QCOMPARE(store.observation().spoofingState, std::optional<int>(1));
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(3)));
    QVERIFY(!store.observation().jammingState);
    QVERIFY(store.observation().spoofingState);
    QVERIFY(store.systemErrorsKnown());
    sample.monotonicTimestampUs = scheduler.nowUs();
    sample.sessionId = 7;
    store.updateObservation(sample);
    QCOMPARE(store.observation().sessionId, quint64(8));
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(2)));
    QVERIFY(!store.available());
    QVERIFY(!store.systemErrorsKnown());
    QCOMPARE(scheduler.pendingCount(), qsizetype(0));

    sample.sessionId = 8;
    sample.monotonicTimestampUs = 1;
    sample.provenance->jammingTimestampUs = scheduler.nowUs();
    store.updateObservation(sample);
    QCOMPARE(store.observation().jammingState, std::optional<int>(0));
    QVERIFY(!store.observation().spoofingState);
    QVERIFY(store.available());
}

void GPSAcceptedStateTest::_integrityRejectsOlderGroups()
{
    ManualScheduler scheduler;
    GPSIntegrityStore store(nullptr, &scheduler);
    store.beginSession(8);
    GPSIntegrityObservation sample;
    sample.sessionId = 8;
    const quint64 firstReceipt = scheduler.nowUs();
    sample.monotonicTimestampUs = firstReceipt;
    sample.jammingState = 1;
    sample.systemErrors = 0;
    store.updateObservation(sample);

    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    const quint64 jammerReceipt = scheduler.nowUs();
    sample.monotonicTimestampUs = jammerReceipt;
    sample.jammingState = 3;
    store.updateObservation(sample);

    sample.monotonicTimestampUs = firstReceipt;
    sample.jammingState = 1;
    store.updateObservation(sample);
    QCOMPARE(store.observation().jammingState, std::optional<int>(3));
    QCOMPARE(store.observation().provenance->jammingTimestampUs, jammerReceipt);

    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    const quint64 spoofReceipt = scheduler.nowUs();
    sample.spoofingState = 2;
    sample.provenance = GPSIntegrityProvenance{firstReceipt, spoofReceipt, 0, 0, 0};
    store.updateObservation(sample);
    QCOMPARE(store.observation().jammingState, std::optional<int>(3));
    QCOMPARE(store.observation().provenance->jammingTimestampUs, jammerReceipt);
    QCOMPARE(store.observation().spoofingState, std::optional<int>(2));
    QCOMPARE(store.observation().provenance->spoofingTimestampUs, spoofReceipt);

    QVERIFY(scheduler.advanceBy(std::chrono::seconds(4)));
    QVERIFY(!store.observation().jammingState);
    QCOMPARE(store.observation().spoofingState, std::optional<int>(2));
    store.updateObservation(sample);
    QVERIFY(!store.observation().jammingState);
    QCOMPARE(store.observation().spoofingState, std::optional<int>(2));

    // A newer complete report can explicitly clear fields absent from that snapshot.
    GPSIntegrityObservation cleared;
    cleared.sessionId = 8;
    cleared.monotonicTimestampUs = scheduler.nowUs();
    store.updateObservation(cleared);
    QVERIFY(!store.available());
    QCOMPARE(scheduler.pendingCount(), qsizetype(0));
}

void GPSAcceptedStateTest::_schedulerDestructionClearsAcceptedState()
{
    auto scheduler = std::make_unique<ManualScheduler>();
    GPSSatelliteStore satellites(nullptr, 5000, scheduler.get());
    GPSRelativePositionStore relative(nullptr, 5000, scheduler.get());
    GPSIntegrityStore integrity(nullptr, scheduler.get());
    GPSSourceHealth health(nullptr, scheduler.get());
    satellites.beginSession(QStringLiteral("receiver"), 1);
    relative.beginSession(QStringLiteral("receiver"), 1);
    GPSRelativeObservation baseline;
    baseline.sessionId = 1;
    baseline.monotonicTimestampUs = scheduler->nowUs();
    relative.updateObservation(baseline);
    GPSIntegrityObservation diagnostics;
    diagnostics.monotonicTimestampUs = scheduler->nowUs();
    diagnostics.systemErrors = 0;
    integrity.updateObservation(diagnostics);
    GPSSatelliteObservation report;
    report.sessionId = 1;
    report.monotonicTimestampUs = scheduler->nowUs();
    report.satellites = {GPSSatellite{}};
    satellites.updateObservation(report);
    GPSObservation fix;
    fix.monotonicTimestampUs = scheduler->nowUs();
    fix.position = QGeoPositionInfo(QGeoCoordinate(47, 8, 500), QDateTime::currentDateTimeUtc());
    fix.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
    health.updateObservation(fix);
    QVERIFY(relative.fresh());
    QVERIFY(integrity.available());
    QCOMPARE(satellites.observation().satellites.size(), 1);
    QVERIFY(health.usable());
    scheduler.reset();
    QVERIFY(!relative.fresh());
    QVERIFY(!integrity.available());
    QVERIFY(satellites.observation().satellites.isEmpty());
    QVERIFY(!health.usable());
    relative.updateObservation(baseline);
    integrity.updateObservation(diagnostics);
    satellites.updateObservation(report);
    satellites.clear();
    satellites.reset();
    satellites.beginSession(QStringLiteral("replacement"), 2);
    QVERIFY(!relative.fresh());
    QVERIFY(!integrity.available());
    QVERIFY(satellites.observation().satellites.isEmpty());
}

UT_REGISTER_TEST(GPSAcceptedStateTest, TestLabel::Unit)
