#include <QtTest/QTest>

#include "GPSSatelliteState.h"
#include "UnitTest.h"

namespace {

constexpr quint64 kStartUs = 10'000'000;
using Delta = GPSSatelliteObservation::UpdateMode;

GPSSatelliteObservation observation(Delta mode, QList<GPSSatelliteConstellation> constellations,
                                    quint64 timestampUs = 0)
{
    GPSSatelliteObservation result;
    result.updateMode = mode;
    result.constellations = std::move(constellations);
    result.monotonicTimestampUs = timestampUs;
    return result;
}

}  // namespace

class GPSSatelliteStateTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _constellationRetirement();
    void _viewAndUseExpireIndependently();
    void _unknownUsageRetiresPreviousCount();
    void _fullSnapshotsReplaceAndDeltasPreserve();
    void _independentRetirement_data();
    void _independentRetirement();
    void _normalization_data();
    void _normalization();
};

void GPSSatelliteStateTest::_constellationRetirement()
{
    GPSSatelliteState state(500);
    const quint64 now = kStartUs;
    auto raw =
        observation(Delta::ConstellationDelta, {{GPSConstellation::GPS, {now - 400000, 1}, {now - 400000, 1}},
                                                {GPSConstellation::Galileo, {now - 10000, 1}, {now - 10000, 0}}});
    state.updateObservation(raw, now);
    QCOMPARE(state.snapshot(now).satellitesInViewCount(), 2);
    QCOMPARE(state.snapshot(now).satellitesInUseCount(), 1);
    state.setFreshnessTimeoutMs(100);
    const auto retired = state.snapshot(now);
    QCOMPARE(retired.satellitesInViewCount(), 1);
    QCOMPARE(retired.constellations.first().constellation, GPSConstellation::Galileo);
    QCOMPARE(retired.satellitesInUseCount(), 0);
    QCOMPARE(retired.constellations.first().view.receivedAtUs, now - 10000);
    // A retired report cannot return, and a report from the future is rejected.
    state.setFreshnessTimeoutMs(500);
    state.updateObservation(raw, now);
    QCOMPARE(state.snapshot(now).satellitesInViewCount(), 1);
    raw.constellations[0].view.receivedAtUs = now + 1000000;
    state.updateObservation(raw, now);
    QCOMPARE(state.snapshot(now).satellitesInViewCount(), 1);
    raw.constellations[0].view.receivedAtUs = now;
    state.updateObservation(raw, now);
    QCOMPARE(state.snapshot(now).satellitesInViewCount(), 2);
}

void GPSSatelliteStateTest::_viewAndUseExpireIndependently()
{
    GPSSatelliteState state(500);
    const quint64 now = kStartUs;
    auto raw = observation(Delta::ConstellationDelta, {{GPSConstellation::GPS, {now - 10000, 1}, {now - 400000, 1}}});
    state.updateObservation(raw, now);
    state.setFreshnessTimeoutMs(100);
    QCOMPARE(state.snapshot(now).satellitesInViewCount(), 1);
    QCOMPARE(state.snapshot(now).satellitesInUseCount(), -1);
    raw.constellations[0].usage = {now, 0};
    state.updateObservation(raw, now);
    QCOMPARE(state.snapshot(now).satellitesInUseCount(), 0);
    // Clearing rejects every report received up to then.
    state.clear(now);
    state.updateObservation(raw, now);
    QCOMPARE(state.snapshot(now).satellitesInViewCount(), -1);
    QCOMPARE(state.snapshot(now).satellitesInUseCount(), -1);
}

void GPSSatelliteStateTest::_unknownUsageRetiresPreviousCount()
{
    GPSSatelliteState state(5000);
    quint64 now = kStartUs;
    const quint64 firstReceipt = now;
    auto report =
        observation(Delta::ConstellationDelta, {{GPSConstellation::GPS, {firstReceipt, 1}, {firstReceipt, 1}}});
    state.updateObservation(report, now);
    QCOMPARE(state.snapshot(now).satellitesInUseCount(), 1);

    report.constellations[0].usage = {};
    state.updateObservation(report, now);
    QCOMPARE(state.snapshot(now).satellitesInUseCount(), 1);

    now += 1000;
    report.constellations[0].usage.receivedAtUs = now;
    state.updateObservation(report, now);
    QCOMPARE(state.snapshot(now).satellitesInUseCount(), -1);
    QVERIFY(!state.snapshot(now).constellations.first().usage.count);

    report.constellations[0].usage = {firstReceipt, 1};
    state.updateObservation(report, now);
    QCOMPARE(state.snapshot(now).satellitesInUseCount(), -1);

    now += 1000;
    report.constellations[0].usage = {now, 0};
    state.updateObservation(report, now);
    QCOMPARE(state.snapshot(now).satellitesInUseCount(), 0);
}

void GPSSatelliteStateTest::_fullSnapshotsReplaceAndDeltasPreserve()
{
    GPSSatelliteState state;
    const quint64 now = kStartUs;
    auto full = observation(Delta::FullSnapshot,
                            {{GPSConstellation::GPS, {now - 10000, 1}, {now - 10000, 1}},
                             {GPSConstellation::Galileo, {now - 10000, 1}, {now - 10000, 1}}},
                            now - 10000);
    state.updateObservation(full, now);
    QCOMPARE(state.snapshot(now).satellitesInViewCount(), 2);
    auto delta =
        observation(Delta::ConstellationDelta, {{GPSConstellation::GPS, {now - 9000, 1}, {now - 9000, 1}}}, now - 9000);
    state.updateObservation(delta, now);
    QCOMPARE(state.snapshot(now).satellitesInViewCount(), 2);
    QCOMPARE(state.snapshot(now).constellations[0].view.receivedAtUs, now - 9000);
    full.monotonicTimestampUs = now - 8000;
    full.constellations = {{GPSConstellation::GPS, {now - 8000, 1}, {}}};
    state.updateObservation(full, now);
    QCOMPARE(state.snapshot(now).satellitesInViewCount(), 1);
    QCOMPARE(state.snapshot(now).constellations[0].view.receivedAtUs, now - 8000);
    QCOMPARE(state.snapshot(now).satellitesInUseCount(), -1);
    // A retired constellation cannot be restored by an older queued delta.
    delta.constellations = {{GPSConstellation::Galileo, {now - 9000, 1}, {now - 9000, 1}}};
    state.updateObservation(delta, now);
    QCOMPARE(state.snapshot(now).satellitesInViewCount(), 1);
    full.monotonicTimestampUs = now - 7000;
    full.constellations = {{GPSConstellation::Unknown, {now - 7000, 0}, {now - 7000, 0}}};
    state.updateObservation(full, now);
    QCOMPARE(state.snapshot(now).satellitesInViewCount(), 0);
    QCOMPARE(state.snapshot(now).satellitesInUseCount(), 0);
    // Full snapshots also cannot overwrite a newer delta already accepted for that constellation.
    delta.monotonicTimestampUs = now - 5000;
    delta.constellations[0].view.receivedAtUs = now - 5000;
    delta.constellations[0].usage.receivedAtUs = now - 5000;
    state.updateObservation(delta, now);
    full.monotonicTimestampUs = now - 6000;
    full.constellations[0].view.receivedAtUs = now - 6000;
    full.constellations[0].usage.receivedAtUs = now - 6000;
    state.updateObservation(full, now);
    QCOMPARE(state.snapshot(now).satellitesInViewCount(), 1);
    QCOMPARE(state.snapshot(now).constellations.last().constellation, GPSConstellation::Galileo);
}

void GPSSatelliteStateTest::_independentRetirement_data()
{
    QTest::addColumn<bool>("retireView");
    QTest::addColumn<bool>("fullSnapshot");
    QTest::newRow("expired-view") << true << false;
    QTest::newRow("expired-usage") << false << false;
    QTest::newRow("omitted-view") << true << true;
    QTest::newRow("omitted-usage") << false << true;
}

void GPSSatelliteStateTest::_independentRetirement()
{
    QFETCH(bool, retireView);
    QFETCH(bool, fullSnapshot);
    GPSSatelliteState state(5000);
    quint64 now = kStartUs;
    const auto initial = observation(Delta::ConstellationDelta, {{GPSConstellation::GPS, {now, 1}, {now, 1}}});
    state.updateObservation(initial, now);
    now += 1'000'000;
    const auto retained = observation(fullSnapshot ? Delta::FullSnapshot : Delta::ConstellationDelta,
                                      {retireView ? GPSSatelliteConstellation{GPSConstellation::GPS, {}, {now, 1}}
                                                  : GPSSatelliteConstellation{GPSConstellation::GPS, {now, 1}, {}}});
    state.updateObservation(retained, now);
    if (!fullSnapshot) {
        now += 4'000'000;
    }
    const auto verifyRetained = [&]() {
        const auto accepted = state.snapshot(now);
        QCOMPARE(accepted.satellitesInViewCount(), retireView ? -1 : 1);
        QCOMPARE(accepted.satellitesInUseCount(), retireView ? 1 : -1);
    };
    verifyRetained();
    state.setFreshnessTimeoutMs(10000);
    state.updateObservation(initial, now);
    verifyRetained();
}

void GPSSatelliteStateTest::_normalization_data()
{
    QTest::addColumn<bool>("empty");
    QTest::addColumn<bool>("unknownUsage");
    QTest::newRow("empty") << true << false;
    QTest::newRow("known") << false << false;
    QTest::newRow("partly-unknown") << false << true;
}

void GPSSatelliteStateTest::_normalization()
{
    QFETCH(bool, empty);
    QFETCH(bool, unknownUsage);
    GPSSatelliteState state(1000);
    quint64 now = kStartUs;
    const auto report =
        observation(Delta::FullSnapshot,
                    empty ? QList<GPSSatelliteConstellation>{{GPSConstellation::Unknown, {now, 0}, {now, 0}}}
                          : QList<GPSSatelliteConstellation>{{GPSConstellation::GPS, {now, 1}, {now, 0}},
                                                             {GPSConstellation::Galileo,
                                                              {now, 1},
                                                              {unknownUsage ? 0 : now,
                                                               unknownUsage ? std::nullopt : std::optional<int>(1)}}},
                    now);
    state.updateObservation(report, now);
    const auto actual = state.snapshot(now);
    QCOMPARE(actual.satellitesInViewCount(), empty ? 0 : 2);
    QCOMPARE(actual.satellitesInUseCount(), empty || unknownUsage ? 0 : 1);
    QCOMPARE(actual.constellations.size(), report.constellations.size());
    for (qsizetype index = 0; index < actual.constellations.size(); ++index) {
        const auto& accepted = actual.constellations[index];
        const auto& input = report.constellations[index];
        QCOMPARE(accepted.view.receivedAtUs, input.view.receivedAtUs);
        QCOMPARE(accepted.view.count, input.view.count);
        QCOMPARE(accepted.usage.receivedAtUs, input.usage.receivedAtUs);
        QCOMPARE(accepted.usage.count, input.usage.count);
    }
    now += 1'000'000;
    QCOMPARE(state.snapshot(now).satellitesInViewCount(), -1);
    QCOMPARE(state.snapshot(now).satellitesInUseCount(), -1);
}

UT_REGISTER_TEST(GPSSatelliteStateTest, TestLabel::Unit)

#include "GPSSatelliteStateTest.moc"
