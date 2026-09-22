#include "NMEASatelliteAdapterTest.h"

#include <QtCore/QBuffer>
#include <QtPositioning/QGeoPositionInfoSource>
#include <QtPositioning/QNmeaSatelliteInfoSource>
#include <QtTest/QSignalSpy>

#include "ManualScheduler.h"
#include "MonotonicClock.h"
#include "NMEAConstellation.h"
#include "NMEADecoderSession.h"
#include "NMEASentence.h"
#include "NMEAStreamSplitter.h"
#include "NMEAUtils.h"
#include "QtRuntimeScheduler.h"
#include "ReadTimestamp.h"

namespace {
class TimedBuffer : public QBuffer, public ReadTimestamp
{
public:
    quint64 receivedAtUs = 0;

    quint64 lastReadTimestampUs() const override { return receivedAtUs; }
};

QByteArray gsa(const QByteArray& talker, const QList<QByteArray>& ids, const QByteArray& system = {})
{
    QList<QByteArray> fields{"$" + talker + "GSA", "A", "3"};
    for (int index = 0; index < 12; ++index) {
        fields.append(ids.value(index));
    }
    fields.append({"1.0", "0.8", "0.6"});
    if (!system.isEmpty()) {
        fields.append(system);
    }
    return fields.join(',');
}

void feed(QBuffer& input, const QList<QByteArray>& sentences)
{
    for (const auto& sentence : sentences) {
        input.buffer().append(NMEAUtils::repairChecksum(sentence));
    }
    emit input.readyRead();
}
}  // namespace

void NMEASatelliteAdapterTest::_gsvFields_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<bool>("accepted");
    QTest::addColumn<int>("satellites");
    QTest::addColumn<int>("signal");
    QTest::newRow("short") << QByteArray("$GPGSV,1,1,1,2,49,115,42") << true << 1 << -1;
    QTest::newRow("padded") << QByteArray("$GPGSV,1,1,1,2,49,115,42,,,,,,,,,,,,") << true << 1 << -1;
    QTest::newRow("padded-signal") << QByteArray("$GPGSV,1,1,1,2,49,115,42,,,,,,,,,,,,,B") << true << 1 << 11;
    QTest::newRow("partial-padding") << QByteArray("$GPGSV,1,1,1,2,49,115,42,,,,") << true << 1 << -1;
    QTest::newRow("zero") << QByteArray("$GPGSV,1,1,0") << true << 0 << -1;
    QTest::newRow("zero-padded") << QByteArray("$GPGSV,1,1,0,,,,,,,,,,,,,,,,") << true << 0 << -1;
    QTest::newRow("zero-padded-signal") << QByteArray("$GPGSV,1,1,0,,,,,,,,,,,,,,,,,1") << true << 0 << 1;
    QTest::newRow("last-page") << QByteArray("$GBGSV,2,2,7,205,,,,206,,,,207,,,33,,,,") << true << 3 << -1;
    QTest::newRow("nonempty-padding") << QByteArray("$GPGSV,1,1,1,2,49,115,42,3,,,") << false << 0 << -1;
    QTest::newRow("partial-slot") << QByteArray("$GPGSV,1,1,1,2,49,115,42,,") << false << 0 << -1;
    QTest::newRow("too-many-slots") << QByteArray("$GPGSV,1,1,0,,,,,,,,,,,,,,,,,,,,") << false << 0 << -1;
    QTest::newRow("truncated") << QByteArray("$GPGSV,1,1,2,2,49,115,42") << false << 0 << -1;
    QTest::newRow("invalid-signal") << QByteArray("$GPGSV,1,1,0,,,,,,,,,,,,,,,,,10") << false << 0 << -1;
    QTest::newRow("combined") << QByteArray("$GNGSV,1,1,2,1,40,100,30,65,30,200,25") << true << 2 << -1;
    QTest::newRow("combined-ambiguous201") << QByteArray("$GNGSV,1,1,2,1,40,100,30,201,30,200,25") << false << 0 << -1;
    QTest::newRow("combined-ambiguous202") << QByteArray("$GNGSV,1,1,1,202,30,200,25") << false << 0 << -1;
    QTest::newRow("combined-unknown") << QByteArray("$GNGSV,1,1,1,999,30,200,25") << false << 0 << -1;
    QTest::newRow("unknown-talker") << QByteArray("$XXGSV,1,1,1,1,30,200,25") << false << 0 << -1;
    QTest::newRow("navic") << QByteArray("$GIGSV,1,1,1,1,30,200,25") << true << 1 << -1;
}

void NMEASatelliteAdapterTest::_gsvFields()
{
    QFETCH(QByteArray, body);
    QFETCH(bool, accepted);
    QFETCH(int, satellites);
    QFETCH(int, signal);
    const auto bytes = NMEAUtils::repairChecksum(body);
    const auto sentence = NMEA::sentence(std::string_view(bytes.constData(), bytes.size()));
    QVERIFY(sentence);
    const auto page = NMEA::gsv(*sentence);
    QCOMPARE(page.has_value(), accepted);
    if (page) {
        QCOMPARE(page->satellites.size(), satellites);
        QCOMPARE(page->signal, signal);
    }
}

void NMEASatelliteAdapterTest::_combinedTalkerReports_data()
{
    QTest::addColumn<bool>("dedicatedFirst");
    QTest::newRow("combined-first") << false;
    QTest::newRow("dedicated-first") << true;
}

void NMEASatelliteAdapterTest::_combinedTalkerReports()
{
    QFETCH(bool, dedicatedFirst);
    TimedBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession adapter;
    QVERIFY(adapter.start(&input));
    auto& store = adapter._satellites;
    QSignalSpy reports(&adapter, &NMEADecoderSession::satellitesReceived);
    const quint64 firstReceipt = MonotonicClock::nowUs() - 1000000;
    input.receivedAtUs = firstReceipt;
    const QList<QByteArray> dedicated{"$GPGSV,1,1,01,01,40,100,45,1"};
    if (dedicatedFirst)
        feed(input, dedicated);
    feed(input, {"$GNGSV,2,1,06,01,40,100,30,65,30,100,30,301,20,100,30,401,10,100,30,1"});
    input.receivedAtUs = firstReceipt + 100000;
    feed(input, {"$GNGSV,2,2,06,193,40,100,30,33,30,100,30,,,,,,,,,1", "$GNGSV,1,1,02,01,40,100,40,301,20,100,50,7"});
    if (!dedicatedFirst)
        feed(input, dedicated);
    feed(input, {gsa("GN", {"01", "65", "301", "401", "193", "33"}), "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(reports.size(), 1, TestTimeout::shortMs());
    const auto observation = store.observation();
    QCOMPARE(observation.satellitesInViewCount(), 6);
    QCOMPARE(observation.satellitesInUseCount(), 6);
    QCOMPARE(observation.constellations.size(), 6);
    for (const auto& constellation : observation.constellations) {
        QVERIFY(constellation.constellation != GPSConstellation::Unknown);
        QCOMPARE(constellation.view.receivedAtUs, firstReceipt);
        for (const auto& satellite : constellation.view.satellites) {
            QCOMPARE(satellite.constellation, constellation.constellation);
            QCOMPARE(satellite.id, constellation.constellation == GPSConstellation::SBAS ? 120 : 1);
            QCOMPARE(satellite.used, std::optional<bool>(true));
            if (constellation.constellation == GPSConstellation::GPS) {
                QCOMPARE(satellite.signalStrength, std::optional<int>(45));
            }
            if (constellation.constellation == GPSConstellation::Galileo) {
                QCOMPARE(satellite.signalStrength, std::optional<int>(50));
            }
        }
    }
}

void NMEASatelliteAdapterTest::_modernConstellationsAndSignals()
{
    QBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession adapter;
    QVERIFY(adapter.start(&input));
    auto& store = adapter._satellites;
    QSignalSpy reports(&adapter, &NMEADecoderSession::satellitesReceived);
    feed(input, {
                    "$GNGSA,A,3,44,02,,,,,,,,,,,1.0,0.8,0.6,1",
                    "$GNGSA,A,3,65,,,,,,,,,,,,1.0,0.8,0.6,2",
                    "$GNGSA,A,3,02,,,,,,,,,,,,1.0,0.8,0.6,3",
                    "$GNGSA,A,3,02,,,,,,,,,,,,1.0,0.8,0.6,4",
                    "$GNGSA,A,3,02,,,,,,,,,,,,1.0,0.8,0.6,6",
                    "$GPGSV,1,1,02,02,40,120,30,44,30,200,35,1",
                    "$GPGSV,1,1,01,07,10,100,,0",
                    "$GLGSV,1,1,01,65,40,100,30,1",
                    "$GAGSV,1,1,01,02,40,100,20,2",
                    "$GAGSV,2,1,05,02,40,100,40,03,50,150,31,04,20,150,25,05,25,170,30,7",
                    "$GAGSV,2,2,05,06,50,250,35,,,,,,,,,,,,,7",
                    "$GBGSV,1,1,01,02,40,100,30,B",
                    "$GIGSV,1,1,01,02,40,100,30,1",
                    "$GNRMC,120001.00,A,3724.000,N,07918.000,W,0,0,090926,,,A",
                });
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellitesInViewCount(), 11, TestTimeout::shortMs());
    QCOMPARE(store.observation().satellitesInUseCount(), 6);
    const auto observation = store.observation();
    int galileoSignals = 0;
    for (const auto& constellation : observation.constellations) {
        if (constellation.constellation == GPSConstellation::Galileo) {
            for (const auto& satellite : constellation.view.satellites) {
                if (satellite.id == 2) {
                    ++galileoSignals;
                    QCOMPARE(satellite.signalStrength, std::optional<int>(40));
                }
            }
        }
    }
    QCOMPARE(galileoSignals, 1);

    // A new epoch must replace the previous signal lists, including an explicit no-fix report.
    reports.clear();
    feed(input, {"$GPGSV,1,1,00,,,,,,,,,,,,,,,,", "$GLGSV,1,1,00,0", "$GAGSV,1,1,00,0", "$GBGSV,1,1,00,0",
                 "$GIGSV,1,1,00,,,,,,,,,,,,,,,,,1", "$GNGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9,1",
                 "$GNGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9,2", "$GNGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9,3",
                 "$GNGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9,4", "$GNGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9,6",
                 "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellitesInViewCount(), 0, TestTimeout::shortMs());
    QCOMPARE(store.observation().satellitesInUseCount(), 0);
}

void NMEASatelliteAdapterTest::_interleavedDeadlineReports()
{
    ManualScheduler scheduler;
    NMEADecoderSession adapter(nullptr, &scheduler);
    QBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    QVERIFY(adapter.start(&input));
    auto& store = adapter._satellites;
    QSignalSpy reports(&adapter, &NMEADecoderSession::satellitesReceived);
    const auto ingest = [&](const QByteArray& body) {
        const auto sentence = NMEASentenceEnvelope::parse(NMEAUtils::repairChecksum(body), scheduler.nowUs());
        QVERIFY(sentence);
        adapter._ingestSatellites(*sentence);
    };
    const auto firstReceipt = scheduler.nowUs();
    ingest("$GPGSV,2,1,05,01,10,20,30,02,20,30,40,03,30,40,50,04,40,50,60,1");
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(25)));
    ingest("$GLGSV,1,1,01,65,10,20,30,1");
    ingest("$GPGSV,2,2,05,05,50,60,70,1");
    ingest("$GPGSV,1,1,01,07,10,20,45,7");
    ingest("$GNRMC,120001.00,V,,,,,,,999999,,,N");
    QVERIFY(reports.isEmpty());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(149)));
    QVERIFY(reports.isEmpty());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(reports.size(), 1);
    QCOMPARE(store.observation().satellitesInViewCount(), 7);
    QCOMPARE(store.observation().constellations.first().view.receivedAtUs, firstReceipt);
    QVERIFY(scheduler.advanceToUs(firstReceipt + 5'000'000));
    QCOMPARE(store.observation().satellitesInViewCount(), 1);
    QCOMPARE(store.observation().constellations.first().constellation, GPSConstellation::GLONASS);
}

void NMEASatelliteAdapterTest::_incompleteReportIsDiscarded_data()
{
    QTest::addColumn<QByteArray>("talker");
    QTest::newRow("specific") << QByteArray("GP");
    QTest::newRow("combined") << QByteArray("GN");
}

void NMEASatelliteAdapterTest::_incompleteReportIsDiscarded()
{
    QFETCH(QByteArray, talker);
    QBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession adapter;
    QVERIFY(adapter.start(&input));
    QSignalSpy reports(&adapter, &NMEADecoderSession::satellitesReceived);
    // A missing first fragment and an interrupted report must never become a complete sky view.
    feed(input, {"$" + talker + "GSV,2,2,05,05,10,100,20,1",
                 "$" + talker + "GSV,2,1,05,01,10,100,20,02,10,100,20,03,10,100,20,04,10,100,20,7",
                 "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QCoreApplication::processEvents();
    QVERIFY(reports.isEmpty());
    feed(input, {"$GPGSV,1,1,01,02,40,100,30,1", "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(reports.size(), 1, TestTimeout::shortMs());
    const auto report = reports.first().first().value<GPSSatelliteObservation>();
    QCOMPARE(report.satellitesInViewCount(), 1);
    QCOMPARE(report.constellations.first().constellation, GPSConstellation::GPS);
    QCOMPARE(report.constellations.first().view.satellites.first().id, 2);
}

void NMEASatelliteAdapterTest::_epochTimeNormalization_data()
{
    QTest::addColumn<QByteArray>("time");
    QTest::addColumn<int>("expectedCount");
    QTest::newRow("same-precision") << QByteArray("120000.0") << 5;
    QTest::newRow("different-precision") << QByteArray("120000.00") << 5;
    QTest::newRow("integer-seconds") << QByteArray("120000") << 5;
    QTest::newRow("malformed") << QByteArray("invalid") << 5;
    QTest::newRow("out-of-range") << QByteArray("246000") << 5;
    QTest::newRow("next-epoch") << QByteArray("120001.0") << 0;
}

void NMEASatelliteAdapterTest::_epochTimeNormalization()
{
    QFETCH(QByteArray, time);
    QFETCH(int, expectedCount);
    QBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession adapter;
    QVERIFY(adapter.start(&input));
    QSignalSpy reports(&adapter, &NMEADecoderSession::satellitesReceived);
    feed(input,
         {"$GPRMC,120000.0,V,,,,,,,140926,,,N", "$GPGSV,2,1,05,01,10,100,30,02,20,100,30,03,30,100,30,04,40,100,30",
          "$GPGGA," + time + ",,,,,0,0,,,,,,,", "$GPGSV,2,2,05,05,50,100,30", "$GPRMC,120002.0,V,,,,,,,140926,,,N",
          "$GLGSV,1,1,00", "$GPRMC,120003.0,V,,,,,,,140926,,,N"});
    // The final GLONASS report proves queued delivery completed even when GPS was discarded.
    QTRY_COMPARE_WITH_TIMEOUT(reports.size(), expectedCount ? 2 : 1, TestTimeout::shortMs());
    const auto finalReport = reports.last().first().value<GPSSatelliteObservation>();
    QVERIFY(std::any_of(
        finalReport.constellations.cbegin(), finalReport.constellations.cend(),
        [](const auto& constellation) { return constellation.constellation == GPSConstellation::GLONASS; }));
    if (expectedCount) {
        const auto observation = reports.first().first().value<GPSSatelliteObservation>();
        QCOMPARE(observation.satellitesInViewCount(), expectedCount);
    }
}

void NMEASatelliteAdapterTest::_idleBatchAndSourceClose()
{
    QBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession adapter;
    QVERIFY(adapter.start(&input));
    QSignalSpy ready(&adapter, &NMEADecoderSession::satellitesReceived);
    // GSV-only sources have no position timestamps to delimit their batches.
    feed(input, {"$GPGSV,1,1,01,02,40,100,30"});
    QTRY_VERIFY_WITH_TIMEOUT(!ready.isEmpty(), TestTimeout::shortMs());
    QCOMPARE(ready.first().first().value<GPSSatelliteObservation>().satellitesInViewCount(), 1);
    input.close();
    QVERIFY(!adapter._satellitesOpen);
    QCoreApplication::processEvents();
    QVERIFY(!adapter.positionSource());
    const auto publications = ready.size();
    feed(input, {"$GPGSV,1,1,01,02,40,100,40", "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QCoreApplication::processEvents();
    QCOMPARE(ready.size(), publications);
}

UT_REGISTER_TEST(NMEASatelliteAdapterTest, TestLabel::Unit)

void NMEASatelliteAdapterTest::_preservesReceiptAgeAcrossReports()
{
    TimedBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession adapter;
    QVERIFY(adapter.start(&input));
    QSignalSpy reports(&adapter, &NMEADecoderSession::satellitesReceived);
    const quint64 oldTimestamp = MonotonicClock::nowUs() - 2000000;
    input.receivedAtUs = oldTimestamp;
    feed(input, {"$GPGSV,1,1,01,02,40,100,30"});
    input.receivedAtUs = MonotonicClock::nowUs();
    feed(input, {"$GAGSV,1,1,01,03,40,100,30", "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    // A newer constellation or position delimiter cannot freshen the older GPS report.
    QTRY_COMPARE_WITH_TIMEOUT(reports.size(), 1, TestTimeout::shortMs());
    const auto first = reports.first().first().value<GPSSatelliteObservation>();
    QCOMPARE(first.satellitesInViewCount(), 2);
    const auto gpsReport =
        std::find_if(first.constellations.cbegin(), first.constellations.cend(),
                     [](const auto& report) { return report.constellation == GPSConstellation::GPS; });
    QVERIFY(gpsReport != first.constellations.cend());
    QCOMPARE(gpsReport->view.receivedAtUs, oldTimestamp);
    const quint64 newTimestamp = MonotonicClock::nowUs();
    input.receivedAtUs = newTimestamp;
    feed(input, {"$GAGSV,1,1,01,03,40,100,31", "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(reports.size(), 2, TestTimeout::shortMs());
    const auto second = reports.last().first().value<GPSSatelliteObservation>();
    QCOMPARE(second.constellations.size(), first.constellations.size());
    const auto retainedGps =
        std::find_if(second.constellations.cbegin(), second.constellations.cend(),
                     [](const auto& report) { return report.constellation == GPSConstellation::GPS; });
    QVERIFY(retainedGps != second.constellations.cend());
    QCOMPARE(retainedGps->view.receivedAtUs, oldTimestamp);
    const auto galileo = std::find_if(
        second.constellations.cbegin(), second.constellations.cend(),
        [](const auto& constellation) { return constellation.constellation == GPSConstellation::Galileo; });
    QVERIFY(galileo != second.constellations.cend());
    QCOMPARE(galileo->view.receivedAtUs, newTimestamp);
    QCOMPARE(second.updateMode, GPSSatelliteObservation::UpdateMode::FullSnapshot);
}

void NMEASatelliteAdapterTest::_decoderRejectsDelayedSatelliteBatch()
{
    TimedBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession session;
    QVERIFY(session.start(&input));
    QSignalSpy changes(&session, &NMEADecoderSession::satellitesReceived);
    input.receivedAtUs = MonotonicClock::nowUs() - 6000000;
    feed(input, {"$GPGSV,1,1,01,02,40,100,30", "$GPGSA,A,3,02,,,,,,,,,,,,1.0,0.8,0.6"});
    input.receivedAtUs = MonotonicClock::nowUs();
    feed(input, {"$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QTRY_VERIFY_WITH_TIMEOUT(!changes.isEmpty(), TestTimeout::shortMs());
    QCOMPARE(session.health()->satellitesInViewCount(), -1);
    QCOMPARE(session.health()->satellitesInUseCount(), -1);
}

void NMEASatelliteAdapterTest::_constellationsExpireIndependently()
{
    TimedBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession adapter;
    QVERIFY(adapter.start(&input));
    auto& store = adapter._satellites;
    store.setFreshnessTimeoutMs(500);
    const quint64 nowUs = MonotonicClock::nowUs();
    input.receivedAtUs = nowUs - 400000;
    feed(input, {"$GPGSV,1,1,01,02,40,100,30", "$GPRMC,120000.00,V,,,,,,,090926,,,N"});
    input.receivedAtUs = nowUs;
    feed(input, {"$GAGSV,1,1,01,03,40,100,30", "$GPRMC,120001.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellitesInViewCount(), 2, TestTimeout::shortMs());
    store.setFreshnessTimeoutMs(100);
    QCOMPARE(store.observation().satellitesInViewCount(), 1);
    QCOMPARE(store.observation().constellations.first().constellation, GPSConstellation::Galileo);
    QCOMPARE(store.observation().constellations.first().view.receivedAtUs, nowUs);
    QCOMPARE(store.observation().satellitesInUseCount(), -1);
}

void NMEASatelliteAdapterTest::_decoderKeepsFreshConstellation()
{
    TimedBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession session;
    QVERIFY(session.start(&input));
    input.receivedAtUs = MonotonicClock::nowUs() - 3000000;
    feed(input, {"$GPGSV,1,1,01,02,40,100,30"});
    input.receivedAtUs = MonotonicClock::nowUs();
    feed(input, {"$GAGSV,1,1,01,03,40,100,30", "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(session.health()->satellitesInViewCount(), 2, TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(session.health()->satellitesInViewCount(), 1, TestTimeout::mediumMs());
    QCOMPARE(session.satelliteObservation().satellitesInViewCount(), 1);
    QCOMPARE(session.satelliteObservation().constellations.first().constellation, GPSConstellation::Galileo);
    QCOMPARE(session.health()->satellitesInUseCount(), -1);
}

void NMEASatelliteAdapterTest::_gsvDoesNotClearFreshUsedReport()
{
    TimedBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession session;
    QVERIFY(session.start(&input));
    const quint64 initialReceipt = MonotonicClock::nowUs();
    input.receivedAtUs = initialReceipt;
    feed(input,
         {"$GPGSV,1,1,01,02,40,100,30", "$GPGSA,A,3,02,,,,,,,,,,,,1.0,0.8,0.6", "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(session.satelliteObservation().satellitesInUseCount(), 1, TestTimeout::shortMs());
    input.receivedAtUs = MonotonicClock::nowUs();
    feed(input, {"$GPGSV,1,1,01,02,40,100,45", "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_VERIFY_WITH_TIMEOUT(
        session.satelliteObservation().satellitesInViewCount() > 0 &&
            session.satelliteObservation().constellations.first().view.satellites.first().signalStrength == 45,
        TestTimeout::mediumMs());
    QCOMPARE(session.satelliteObservation().satellitesInUseCount(), 1);
    QCOMPARE(session.satelliteObservation().constellations.first().view.satellites.first().used,
             std::optional<bool>(true));
    QCOMPARE(session.satelliteObservation().constellations.first().usage.receivedAtUs, initialReceipt);
    QVERIFY(session.satelliteObservation().constellations.first().view.receivedAtUs > initialReceipt);
}

void NMEASatelliteAdapterTest::_identicalReportsAndGsaWithoutView()
{
    TimedBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession session;
    QVERIFY(session.start(&input));
    input.receivedAtUs = MonotonicClock::nowUs() - 100000;
    feed(input, {"$GPGSA,A,3,02,,,,,,,,,,,,1.0,0.8,0.6", "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(session.satelliteObservation().satellitesInUseCount(), 1, TestTimeout::shortMs());
    QCOMPARE(session.satelliteObservation().satellitesInViewCount(), -1);
    const auto useReceipt = input.receivedAtUs;
    input.receivedAtUs = MonotonicClock::nowUs() - 50000;
    feed(input, {"$GPGSV,1,1,02,02,0,0,0,03,,,", "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(session.satelliteObservation().satellitesInViewCount(), 2, TestTimeout::shortMs());
    const auto previous = session.satelliteObservation();
    const auto& view = previous.constellations.first().view.satellites;
    QCOMPARE(view[0].used, std::optional<bool>(true));
    QCOMPARE(view[1].used, std::optional<bool>(false));
    QCOMPARE(view[0].signalStrength, std::optional<int>(0));
    QCOMPARE(view[0].elevationDegrees, std::optional<double>(0));
    QCOMPARE(view[0].azimuthDegrees(), std::optional<double>(0));
    QVERIFY(!view[1].signalStrength);
    QVERIFY(!view[1].elevationDegrees);
    QVERIFY(!view[1].azimuthDegrees());
    input.receivedAtUs = MonotonicClock::nowUs();
    feed(input, {"$GPGSV,1,1,02,02,0,0,0,03,,,", "$GNRMC,120003.00,V,,,,,,,090926,,,N"});
    QTRY_VERIFY_WITH_TIMEOUT(session.satelliteObservation().revision > previous.revision, TestTimeout::shortMs());
    QCOMPARE(session.satelliteObservation().constellations.first().view.receivedAtUs, input.receivedAtUs);
    QCOMPARE(session.satelliteObservation().constellations.first().usage.receivedAtUs, useReceipt);
    QSignalSpy publications(&session, &NMEADecoderSession::satellitesReceived);
    session.stop();
    const auto retired = session.sessionId();
    const auto count = publications.size();
    session.stop();
    QCOMPARE(session.sessionId(), retired);
    QCOMPARE(publications.size(), count);
}

void NMEASatelliteAdapterTest::_qtLegacyEquivalence_data()
{
    QTest::addColumn<bool>("mixed");
    QTest::addColumn<bool>("gsaFirst");
    QTest::addColumn<bool>("padded");
    QTest::newRow("gps-view-first") << false << false << false;
    QTest::newRow("gps-use-first") << false << true << false;
    QTest::newRow("gps-glonass-view-first") << true << false << false;
    QTest::newRow("gps-glonass-use-first") << true << true << false;
    QTest::newRow("padded-gps") << false << false << true;
    QTest::newRow("padded-gps-glonass") << true << true << true;
}

void NMEASatelliteAdapterTest::_qtLegacyEquivalence()
{
    QFETCH(bool, mixed);
    QFETCH(bool, gsaFirst);
    QFETCH(bool, padded);
    QBuffer input;
    QBuffer qtInput;
    QVERIFY(input.open(QIODevice::ReadOnly));
    QVERIFY(qtInput.open(QIODevice::ReadOnly));
    NMEADecoderSession adapter;
    QVERIFY(adapter.start(&input));
    auto& store = adapter._satellites;
    QNmeaSatelliteInfoSource qtDecoder(QNmeaSatelliteInfoSource::UpdateMode::RealTimeMode);
    qtDecoder.setDevice(&qtInput);
    QSignalSpy qtView(&qtDecoder, &QGeoSatelliteInfoSource::satellitesInViewUpdated);
    QSignalSpy qtUse(&qtDecoder, &QGeoSatelliteInfoSource::satellitesInUseUpdated);
    qtDecoder.requestUpdate(5000);
    QList<QByteArray> view{"$GPGSV,1,1,02,02,0,275,0,03,40,100,35"};
    QList<QByteArray> use{gsa("GP", {"02"})};
    if (mixed) {
        view.append("$GLGSV,1,1,02,65,30,110,40,66,50,180,25");
        use.append(gsa("GL", {"65"}));
    }
    if (padded) {
        for (auto& sentence : view)
            sentence.append(",,,,,,,,");
    }
    auto sentences = gsaFirst ? use + view : view + use;
    sentences.append("$GNRMC,120001.00,V,,,,,,,090926,,,N");
    feed(input, sentences);
    feed(qtInput, sentences);
    QTRY_VERIFY_WITH_TIMEOUT(
        !qtView.isEmpty() && !qtUse.isEmpty() && store.observation().satellitesInViewCount() == (mixed ? 4 : 2),
        TestTimeout::shortMs());
    const auto qtSatellites = qtView.last().first().value<QList<QGeoSatelliteInfo>>();
    QCOMPARE(store.observation().satellitesInViewCount(), qtSatellites.size());
    QCOMPARE(store.observation().satellitesInUseCount(), qtUse.last().first().value<QList<QGeoSatelliteInfo>>().size());
    const auto observation = store.observation();
    qsizetype index = 0;
    for (const auto& group : observation.constellations) {
        for (const auto& actual : group.view.satellites) {
            const auto& reference = qtSatellites[index++];
            const auto constellation = reference.satelliteSystem() == QGeoSatelliteInfo::GPS
                                           ? GPSConstellation::GPS
                                           : GPSConstellation::GLONASS;
            QCOMPARE(group.constellation, constellation);
            QCOMPARE(actual.constellation, constellation);
            // QGC stores constellation-local IDs; Qt retains the legacy GLONASS offset.
            QCOMPARE(actual.id, NMEA::satelliteId(constellation, reference.satelliteIdentifier()));
            QCOMPARE(actual.signalStrength, std::optional<int>(reference.signalStrength()));
            QCOMPARE(actual.elevationDegrees, std::optional<double>(reference.attribute(QGeoSatelliteInfo::Elevation)));
            QCOMPARE(actual.azimuthDegrees(), std::optional<double>(reference.attribute(QGeoSatelliteInfo::Azimuth)));
        }
    }
}

void NMEASatelliteAdapterTest::_reentrantStopKeepsReplacement()
{
    QBuffer firstInput;
    QBuffer replacementInput;
    QVERIFY(firstInput.open(QIODevice::ReadOnly));
    QVERIFY(replacementInput.open(QIODevice::ReadOnly));
    NMEADecoderSession session;
    QVERIFY(session.start(&firstInput));
    bool replaced = false;
    connect(session.positionSource(), &QObject::destroyed, &session,
            [&]() { replaced = session.start(&replacementInput); });
    session.stop();
    QVERIFY(replaced);
    QVERIFY(session.positionSource());
    feed(replacementInput, {"$GPGSV,1,1,01,02,40,100,45", "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(session.satelliteObservation().satellitesInViewCount(), 1, TestTimeout::shortMs());
    QCOMPARE(session.satelliteObservation().constellations.first().view.satellites.first().signalStrength,
             std::optional<int>(45));
}

void NMEASatelliteAdapterTest::_identityResolution_data()
{
    using Constellation = GPSConstellation;
    QTest::addColumn<QByteArray>("talker");
    QTest::addColumn<int>("system");
    QTest::addColumn<int>("id");
    QTest::addColumn<Constellation>("expected");
    QTest::newRow("gps") << QByteArray("GN") << -1 << 2 << Constellation::GPS;
    QTest::newRow("glonass") << QByteArray("GN") << -1 << 65 << Constellation::GLONASS;
    QTest::newRow("qzss") << QByteArray("GN") << -1 << 200 << Constellation::QZSS;
    QTest::newRow("ambiguous201") << QByteArray("GN") << -1 << 201 << Constellation::Unknown;
    QTest::newRow("ambiguous202") << QByteArray("GN") << -1 << 202 << Constellation::Unknown;
    QTest::newRow("legacy-beidou") << QByteArray("GN") << -1 << 203 << Constellation::BeiDou;
    QTest::newRow("extended-beidou") << QByteArray("GN") << -1 << 401 << Constellation::BeiDou;
    QTest::newRow("extended-galileo") << QByteArray("GN") << -1 << 301 << Constellation::Galileo;
    QTest::newRow("zero-id") << QByteArray("GN") << -1 << 0 << Constellation::Unknown;
    QTest::newRow("unrecognized-id") << QByteArray("GN") << -1 << 999 << Constellation::Unknown;
    QTest::newRow("invalid-system") << QByteArray("GN") << 0 << 2 << Constellation::Unknown;
    QTest::newRow("unknown-system") << QByteArray("GN") << 99 << 2 << Constellation::Unknown;
    QTest::newRow("explicit-qzss") << QByteArray("GN") << 5 << 201 << Constellation::QZSS;
    QTest::newRow("explicit-beidou") << QByteArray("GN") << 4 << 201 << Constellation::BeiDou;
    QTest::newRow("modern-galileo") << QByteArray("GN") << 3 << 2 << Constellation::Galileo;
    QTest::newRow("modern-navic") << QByteArray("GN") << 6 << 2 << Constellation::NavIC;
    QTest::newRow("navic-talker") << QByteArray("GI") << -1 << 2 << Constellation::NavIC;
    QTest::newRow("gps-talker") << QByteArray("GP") << -1 << 2 << Constellation::GPS;
    QTest::newRow("beidou-talker") << QByteArray("GB") << -1 << 201 << Constellation::BeiDou;
    QTest::newRow("beidou-alias") << QByteArray("BD") << -1 << 201 << Constellation::BeiDou;
    QTest::newRow("qzss-talker") << QByteArray("GQ") << -1 << 201 << Constellation::QZSS;
    QTest::newRow("qzss-alias") << QByteArray("QZ") << -1 << 201 << Constellation::QZSS;
    QTest::newRow("qzss-proprietary-alias") << QByteArray("PQ") << -1 << 201 << Constellation::QZSS;
    QTest::newRow("unsupported-talker") << QByteArray("XX") << 1 << 2 << Constellation::Unknown;
}

void NMEASatelliteAdapterTest::_identityResolution()
{
    QFETCH(QByteArray, talker);
    QFETCH(int, system);
    QFETCH(int, id);
    QFETCH(GPSConstellation, expected);
    const std::optional<int> systemId = system < 0 ? std::nullopt : std::optional<int>(system);
    QCOMPARE(NMEA::satelliteConstellation(std::string_view(talker.constData(), talker.size()), systemId, id), expected);
}

void NMEASatelliteAdapterTest::_mixedLegacyIdentities_data()
{
    QTest::addColumn<QList<QByteArray>>("ids");
    QTest::newRow("gps-first") << QList<QByteArray>{"02", "65", "65"};
    QTest::newRow("glonass-first") << QList<QByteArray>{"65", "02", "02"};
}

void NMEASatelliteAdapterTest::_mixedLegacyIdentities()
{
    QFETCH(QList<QByteArray>, ids);
    QBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession adapter;
    QVERIFY(adapter.start(&input));
    auto& store = adapter._satellites;
    // Synthetic accepted input: do not infer all IDs from the first ID or count 65 in two systems.
    feed(input, {gsa("GN", ids), gsa("GL", {"65"}), "$GPGSV,1,1,01,02,0,0,0", "$GLGSV,1,1,01,65,,,",
                 "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellitesInViewCount(), 2, TestTimeout::shortMs());
    QCOMPARE(store.observation().satellitesInUseCount(), 2);
    for (const auto& constellation : store.observation().constellations) {
        for (const auto& satellite : constellation.view.satellites) {
            QCOMPARE(satellite.used, std::optional<bool>(true));
            if (satellite.id == 2) {
                QCOMPARE(satellite.constellation, GPSConstellation::GPS);
                QCOMPARE(satellite.signalStrength, std::optional<int>(0));
            } else {
                QCOMPARE(satellite.id, 1);
                QCOMPARE(satellite.constellation, GPSConstellation::GLONASS);
                QVERIFY(!satellite.signalStrength);
            }
        }
    }
}

void NMEASatelliteAdapterTest::_ambiguousLegacyIdentities_data()
{
    QTest::addColumn<QList<QByteArray>>("ids");
    QTest::newRow("ambiguous-pair") << QList<QByteArray>{"201", "202"};
    QTest::newRow("partial-gps") << QList<QByteArray>{"02", "201"};
    QTest::newRow("partial-beidou") << QList<QByteArray>{"203", "202"};
}

void NMEASatelliteAdapterTest::_ambiguousLegacyIdentities()
{
    QFETCH(QList<QByteArray>, ids);
    TimedBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession adapter;
    QVERIFY(adapter.start(&input));
    auto& store = adapter._satellites;
    input.receivedAtUs = MonotonicClock::nowUs() - 100000;
    feed(input, {gsa("GN", ids), "$GQGSV,1,1,02,201,0,0,0,202,,,", "$BDGSV,1,1,01,201,0,0,0",
                 "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellitesInViewCount(), 3, TestTimeout::shortMs());
    QCOMPARE(store.observation().satellitesInUseCount(), -1);
    for (const auto& constellation : store.observation().constellations) {
        for (const auto& satellite : constellation.view.satellites) {
            QVERIFY(!satellite.used.has_value());
        }
    }
    input.receivedAtUs = MonotonicClock::nowUs() - 50000;
    feed(input, {gsa("GN", {"201", "202"}, "5"), gsa("BD", {"201"}), "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellitesInUseCount(), 3, TestTimeout::shortMs());
    const quint64 usedReceipt = input.receivedAtUs;
    input.receivedAtUs = MonotonicClock::nowUs();
    QSignalSpy reports(&adapter, &NMEADecoderSession::satellitesReceived);
    feed(input, {gsa("GN", ids), "$PQGSV,1,1,02,201,0,0,0,202,,,", "$GNRMC,120003.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(reports.size(), 1, TestTimeout::shortMs());
    QCOMPARE(store.observation().satellitesInUseCount(), 3);
    for (const auto& constellation : store.observation().constellations) {
        QCOMPARE(constellation.usage.receivedAtUs, usedReceipt);
    }
}

void NMEASatelliteAdapterTest::_explicitZeroAndUnknownCoverage()
{
    QBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession adapter;
    QVERIFY(adapter.start(&input));
    QSignalSpy reports(&adapter, &NMEADecoderSession::satellitesReceived);
    feed(input,
         {gsa("GN", {}), gsa("GN", {}, "0"), "$GNGSV,1,1,00,,,,,,,,,,,,,,,,", "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    // The following explicit GPS empty report is the only report with known coverage.
    feed(input, {gsa("GN", {}, "1"), "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(reports.size(), 1, TestTimeout::shortMs());
    const auto observation = reports.first().first().value<GPSSatelliteObservation>();
    QCOMPARE(observation.constellations.size(), 2);
    QCOMPARE(observation.constellations.last().constellation, GPSConstellation::SBAS);
    QCOMPARE(observation.constellations.last().usage.count, std::optional<int>(0));
    QCOMPARE(observation.constellations.first().constellation, GPSConstellation::GPS);
    QCOMPARE(observation.constellations.first().usage.count, std::optional<int>(0));
    QVERIFY(observation.constellations.first().usage.ids.has_value());
    QVERIFY(observation.constellations.first().usage.ids->isEmpty());
}

void NMEASatelliteAdapterTest::_canonicalIdentities()
{
    QBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession adapter;
    QVERIFY(adapter.start(&input));
    auto& store = adapter._satellites;
    feed(input, {"$GLGSV,1,1,01,01,30,100,40", "$GNGSA,A,3,65,,,,,,,,,,,,1.0,0.8,0.6,2", "$GAGSV,1,1,01,301,30,100,40",
                 "$GAGSA,A,3,01,,,,,,,,,,,,1.0,0.8,0.6", "$GBGSV,1,1,01,401,30,100,40",
                 "$GBGSA,A,3,201,,,,,,,,,,,,1.0,0.8,0.6", "$GIGSV,1,1,01,01,30,100,40", gsa("GI", {"01"}),
                 "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellitesInViewCount(), 4, TestTimeout::shortMs());
    QCOMPARE(store.observation().satellitesInUseCount(), 4);
    for (const auto& constellation : store.observation().constellations) {
        for (const auto& satellite : constellation.view.satellites) {
            QCOMPARE(satellite.id, 1);
            QCOMPARE(satellite.used, std::optional<bool>(true));
            QVERIFY(satellite.prn > 0);
        }
    }
}

void NMEASatelliteAdapterTest::_schedulerCanBeDestroyed()
{
    QBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    auto scheduler = std::make_unique<QtRuntimeScheduler>();
    NMEADecoderSession adapter(nullptr, scheduler.get());
    QVERIFY(adapter.start(&input));
    QSignalSpy updates(&adapter, &NMEADecoderSession::satellitesReceived);
    feed(input, {"$GPGSV,1,1,01,01,45,100,30"});
    scheduler.reset();
    QVERIFY(!adapter.positionSource());
    for (const auto& update : updates) {
        const auto observation = update.first().value<GPSSatelliteObservation>();
        for (const auto& constellation : observation.constellations) {
            QVERIFY(constellation.view.satellites.isEmpty());
        }
    }
    const auto count = updates.size();
    feed(input, {"$GPGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9"});
    adapter.stop();
    QCOMPARE(updates.size(), count);
}
