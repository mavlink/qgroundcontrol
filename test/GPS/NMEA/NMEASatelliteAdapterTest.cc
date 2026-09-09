#include "NMEASatelliteAdapterTest.h"

#include <QtCore/QBuffer>
#include <QtPositioning/QGeoPositionInfoSource>
#include <QtPositioning/QNmeaSatelliteInfoSource>
#include <QtTest/QSignalSpy>

#include "GPSReadTimestamp.h"
#include "NMEADecoderSession.h"
#include "NMEASatelliteAdapter.h"
#include "NMEAUtils.h"

namespace {
class TimedBuffer : public QBuffer, public GPSReadTimestamp
{
public:
    quint64 receivedAtUs = 0;

    quint64 lastReadTimestampUs() const override { return receivedAtUs; }
};

void connectStore(NMEASatelliteAdapter& adapter, GPSSatelliteStore& store)
{
    store.beginSession(QStringLiteral("test"), 1);
    QObject::connect(&adapter, &NMEASatelliteAdapter::observationReceived, &store,
                     [&store](GPSSatelliteObservation observation) {
                         observation.sessionId = 1;
                         store.updateObservation(observation);
                     });
}

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

void NMEASatelliteAdapterTest::_modernConstellationsAndSignals()
{
    QBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEASatelliteAdapter adapter(&input);
    GPSSatelliteStore store;
    connectStore(adapter, store);
    QSignalSpy reports(&adapter, &NMEASatelliteAdapter::observationReceived);
    feed(input, {
                    "$GNGSA,A,3,44,02,,,,,,,,,,,1.0,0.8,0.6,1",
                    "$GNGSA,A,3,65,,,,,,,,,,,,1.0,0.8,0.6,2",
                    "$GNGSA,A,3,02,,,,,,,,,,,,1.0,0.8,0.6,3",
                    "$GNGSA,A,3,02,,,,,,,,,,,,1.0,0.8,0.6,4",
                    "$GPGSV,1,1,02,02,40,120,30,44,30,200,35,1",
                    "$GPGSV,1,1,01,07,10,100,,0",
                    "$GLGSV,1,1,01,65,40,100,30,1",
                    "$GAGSV,1,1,01,02,40,100,20,2",
                    "$GAGSV,2,1,05,02,40,100,40,03,50,150,31,04,20,150,25,05,25,170,30,7",
                    "$GAGSV,2,2,05,06,50,250,35,7",
                    "$GBGSV,1,1,01,02,40,100,30,B",
                    "$GNRMC,120001.00,A,3724.000,N,07918.000,W,0,0,090926,,,A",
                });
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellitesInViewCount(), 10, TestTimeout::shortMs());
    QCOMPARE(store.observation().satellitesInUseCount(), 5);
    const auto satellitesVisible = store.observation().satellites;
    int galileoSignals = 0;
    for (const auto& satellite : satellitesVisible) {
        if (satellite.constellation == GPSSatellite::Constellation::Galileo && satellite.id == 2) {
            ++galileoSignals;
            QCOMPARE(satellite.signalStrength, std::optional<int>(40));
        }
    }
    QCOMPARE(galileoSignals, 1);

    // A new epoch must replace the previous signal lists, including an explicit no-fix report.
    reports.clear();
    feed(input, {"$GPGSV,1,1,00,0", "$GLGSV,1,1,00,0", "$GAGSV,1,1,00,0", "$GBGSV,1,1,00,0",
                 "$GNGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9,1", "$GNGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9,2",
                 "$GNGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9,3", "$GNGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9,4",
                 "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellitesInViewCount(), 0, TestTimeout::shortMs());
    QCOMPARE(store.observation().satellitesInUseCount(), 0);
}

void NMEASatelliteAdapterTest::_incompleteReportIsDiscarded()
{
    QBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEASatelliteAdapter adapter(&input);
    QSignalSpy reports(&adapter, &NMEASatelliteAdapter::observationReceived);
    // A missing first fragment and an interrupted report must never become a complete sky view.
    feed(input, {"$GPGSV,2,2,05,05,10,100,20,1", "$GAGSV,2,1,05,01,10,100,20,02,10,100,20,03,10,100,20,04,10,100,20,7",
                 "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QCoreApplication::processEvents();
    QVERIFY(reports.isEmpty());
    feed(input, {"$GPGSV,1,1,01,02,40,100,30,1", "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(reports.size(), 1, TestTimeout::shortMs());
    const auto report = reports.first().first().value<GPSSatelliteObservation>();
    QCOMPARE(report.satellites.size(), 1);
    QCOMPARE(report.satellites.first().constellation, GPSSatellite::Constellation::GPS);
    QCOMPARE(report.satellites.first().id, 2);
}

void NMEASatelliteAdapterTest::_idleBatchAndSourceClose()
{
    QBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEASatelliteAdapter adapter(&input);
    QSignalSpy ready(&adapter, &NMEASatelliteAdapter::observationReceived);
    // GSV-only sources have no position timestamps to delimit their batches.
    feed(input, {"$GPGSV,1,1,01,02,40,100,30"});
    QTRY_VERIFY_WITH_TIMEOUT(!ready.isEmpty(), TestTimeout::shortMs());
    QCOMPARE(ready.first().first().value<GPSSatelliteObservation>().satellites.size(), 1);
    input.close();
    QVERIFY(!adapter.isOpen());
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
    NMEASatelliteAdapter adapter(&input);
    QSignalSpy reports(&adapter, &NMEASatelliteAdapter::observationReceived);
    const quint64 oldTimestamp = GPSObservation::monotonicNowUs() - 2000000;
    input.receivedAtUs = oldTimestamp;
    feed(input, {"$GPGSV,1,1,01,02,40,100,30"});
    input.receivedAtUs = GPSObservation::monotonicNowUs();
    feed(input, {"$GAGSV,1,1,01,03,40,100,30", "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    // A newer constellation or position delimiter cannot freshen the older GPS report.
    QTRY_COMPARE_WITH_TIMEOUT(reports.size(), 1, TestTimeout::shortMs());
    const auto first = reports.first().first().value<GPSSatelliteObservation>();
    QCOMPARE(first.satellites.size(), 2);
    const auto gpsReport = std::find_if(first.provenance.cbegin(), first.provenance.cend(), [](const auto& report) {
        return report.constellation == GPSSatellite::Constellation::GPS;
    });
    QVERIFY(gpsReport != first.provenance.cend());
    QCOMPARE(gpsReport->inViewTimestampUs, oldTimestamp);
    const quint64 newTimestamp = GPSObservation::monotonicNowUs();
    input.receivedAtUs = newTimestamp;
    feed(input, {"$GAGSV,1,1,01,03,40,100,31", "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(reports.size(), 2, TestTimeout::shortMs());
    const auto second = reports.last().first().value<GPSSatelliteObservation>();
    QCOMPARE(second.provenance.size(), 1);
    QCOMPARE(second.provenance.first().constellation, GPSSatellite::Constellation::Galileo);
    QCOMPARE(second.provenance.first().inViewTimestampUs, newTimestamp);
    QCOMPARE(second.updateMode, GPSSatelliteObservation::UpdateMode::ConstellationDelta);
}

void NMEASatelliteAdapterTest::_decoderRejectsDelayedSatelliteBatch()
{
    TimedBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession session;
    QVERIFY(session.start(&input));
    QSignalSpy changes(&session, &NMEADecoderSession::satellitesReceived);
    input.receivedAtUs = GPSObservation::monotonicNowUs() - 6000000;
    feed(input, {"$GPGSV,1,1,01,02,40,100,30", "$GPGSA,A,3,02,,,,,,,,,,,,1.0,0.8,0.6"});
    input.receivedAtUs = GPSObservation::monotonicNowUs();
    feed(input, {"$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QTRY_VERIFY_WITH_TIMEOUT(!changes.isEmpty(), TestTimeout::shortMs());
    QCOMPARE(session.health()->satellitesInViewCount(), -1);
    QCOMPARE(session.health()->satellitesInUseCount(), -1);
}

void NMEASatelliteAdapterTest::_constellationsExpireIndependently()
{
    TimedBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEASatelliteAdapter adapter(&input);
    GPSSatelliteStore store(nullptr, 500);
    connectStore(adapter, store);
    const quint64 nowUs = GPSObservation::monotonicNowUs();
    input.receivedAtUs = nowUs - 400000;
    feed(input, {"$GPGSV,1,1,01,02,40,100,30", "$GPRMC,120000.00,V,,,,,,,090926,,,N"});
    input.receivedAtUs = nowUs;
    feed(input, {"$GAGSV,1,1,01,03,40,100,30", "$GPRMC,120001.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellites.size(), 2, TestTimeout::shortMs());
    store.setFreshnessTimeoutMs(100);
    QCOMPARE(store.observation().satellites.size(), 1);
    QCOMPARE(store.observation().satellites.first().constellation, GPSSatellite::Constellation::Galileo);
    QCOMPARE(store.observation().provenance.first().inViewTimestampUs, nowUs);
    QCOMPARE(store.observation().satellitesInUseCount(), -1);
}

void NMEASatelliteAdapterTest::_decoderKeepsFreshConstellation()
{
    TimedBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession session;
    QVERIFY(session.start(&input));
    input.receivedAtUs = GPSObservation::monotonicNowUs() - 3000000;
    feed(input, {"$GPGSV,1,1,01,02,40,100,30"});
    input.receivedAtUs = GPSObservation::monotonicNowUs();
    feed(input, {"$GAGSV,1,1,01,03,40,100,30", "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(session.health()->satellitesInViewCount(), 2, TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(session.health()->satellitesInViewCount(), 1, TestTimeout::mediumMs());
    QCOMPARE(session.satelliteObservation().satellites.size(), 1);
    QCOMPARE(session.satelliteObservation().satellites.first().constellation, GPSSatellite::Constellation::Galileo);
    QCOMPARE(session.health()->satellitesInUseCount(), -1);
}

void NMEASatelliteAdapterTest::_gsvDoesNotClearFreshUsedReport()
{
    TimedBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession session;
    QVERIFY(session.start(&input));
    const quint64 initialReceipt = GPSObservation::monotonicNowUs();
    input.receivedAtUs = initialReceipt;
    feed(input,
         {"$GPGSV,1,1,01,02,40,100,30", "$GPGSA,A,3,02,,,,,,,,,,,,1.0,0.8,0.6", "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(session.satelliteObservation().satellitesInUseCount(), 1, TestTimeout::shortMs());
    input.receivedAtUs = GPSObservation::monotonicNowUs();
    feed(input, {"$GPGSV,1,1,01,02,40,100,45", "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_VERIFY_WITH_TIMEOUT(!session.satelliteObservation().satellites.isEmpty() &&
                                 session.satelliteObservation().satellites.first().signalStrength == 45,
                             TestTimeout::mediumMs());
    QCOMPARE(session.satelliteObservation().satellitesInUseCount(), 1);
    QCOMPARE(session.satelliteObservation().satellites.first().used, std::optional<bool>(true));
    QCOMPARE(session.satelliteObservation().provenance.first().inUseTimestampUs, initialReceipt);
    QVERIFY(session.satelliteObservation().provenance.first().inViewTimestampUs > initialReceipt);
}

void NMEASatelliteAdapterTest::_identicalReportsAndGsaWithoutView()
{
    TimedBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession session;
    QVERIFY(session.start(&input));
    input.receivedAtUs = GPSObservation::monotonicNowUs() - 100000;
    feed(input, {"$GPGSA,A,3,02,,,,,,,,,,,,1.0,0.8,0.6", "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(session.satelliteObservation().satellitesInUseCount(), 1, TestTimeout::shortMs());
    QCOMPARE(session.satelliteObservation().satellitesInViewCount(), -1);
    const auto useReceipt = input.receivedAtUs;
    input.receivedAtUs = GPSObservation::monotonicNowUs() - 50000;
    feed(input, {"$GPGSV,1,1,02,02,0,0,0,03,,,", "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(session.satelliteObservation().satellites.size(), 2, TestTimeout::shortMs());
    const auto previous = session.satelliteObservation();
    QCOMPARE(previous.satellites[0].used, std::optional<bool>(true));
    QCOMPARE(previous.satellites[1].used, std::optional<bool>(false));
    QCOMPARE(previous.satellites[0].signalStrength, std::optional<int>(0));
    QCOMPARE(previous.satellites[0].elevationDegrees, std::optional<double>(0));
    QCOMPARE(previous.satellites[0].azimuthDegrees(), std::optional<double>(0));
    QVERIFY(!previous.satellites[1].signalStrength);
    QVERIFY(!previous.satellites[1].elevationDegrees);
    QVERIFY(!previous.satellites[1].azimuthDegrees());
    input.receivedAtUs = GPSObservation::monotonicNowUs();
    feed(input, {"$GPGSV,1,1,02,02,0,0,0,03,,,", "$GNRMC,120003.00,V,,,,,,,090926,,,N"});
    QTRY_VERIFY_WITH_TIMEOUT(session.satelliteObservation().revision > previous.revision, TestTimeout::shortMs());
    QCOMPARE(session.satelliteObservation().provenance.first().inViewTimestampUs, input.receivedAtUs);
    QCOMPARE(session.satelliteObservation().provenance.first().inUseTimestampUs, useReceipt);
    QSignalSpy publications(&session, &NMEADecoderSession::satellitesReceived);
    session.stop();
    const auto retired = session.sessionId();
    const auto count = publications.size();
    session.stop();
    QCOMPARE(session.sessionId(), retired);
    QCOMPARE(publications.size(), count);
}

void NMEASatelliteAdapterTest::_qtLegacyEquivalence()
{
    QBuffer input;
    QBuffer qtInput;
    QVERIFY(input.open(QIODevice::ReadOnly));
    QVERIFY(qtInput.open(QIODevice::ReadOnly));
    NMEASatelliteAdapter adapter(&input);
    GPSSatelliteStore store;
    connectStore(adapter, store);
    QNmeaSatelliteInfoSource qtDecoder(QNmeaSatelliteInfoSource::UpdateMode::RealTimeMode);
    qtDecoder.setDevice(&qtInput);
    QSignalSpy qtView(&qtDecoder, &QGeoSatelliteInfoSource::satellitesInViewUpdated);
    QSignalSpy qtUse(&qtDecoder, &QGeoSatelliteInfoSource::satellitesInUseUpdated);
    qtDecoder.requestUpdate(5000);
    const QList<QByteArray> sentences = {"$GPGSV,1,1,02,02,0,275,0,03,40,100,35",
                                         "$GPGSA,A,3,02,,,,,,,,,,,,1.0,0.8,0.6", "$GNRMC,120001.00,V,,,,,,,090926,,,N"};
    feed(input, sentences);
    feed(qtInput, sentences);
    QTRY_VERIFY_WITH_TIMEOUT(!qtView.isEmpty() && !qtUse.isEmpty() && store.observation().satellites.size() == 2,
                             TestTimeout::shortMs());
    const auto qtSatellites = qtView.last().first().value<QList<QGeoSatelliteInfo>>();
    QCOMPARE(store.observation().satellites.size(), qtSatellites.size());
    QCOMPARE(store.observation().satellitesInUseCount(), qtUse.last().first().value<QList<QGeoSatelliteInfo>>().size());
    const auto observation = store.observation();
    for (qsizetype index = 0; index < qtSatellites.size(); ++index) {
        const auto& actual = observation.satellites[index];
        const auto& reference = qtSatellites[index];
        QCOMPARE(actual.id, reference.satelliteIdentifier());
        QCOMPARE(actual.signalStrength, std::optional<int>(reference.signalStrength()));
        QCOMPARE(actual.elevationDegrees, std::optional<double>(reference.attribute(QGeoSatelliteInfo::Elevation)));
        QCOMPARE(actual.azimuthDegrees(), std::optional<double>(reference.attribute(QGeoSatelliteInfo::Azimuth)));
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
    connect(session.positionSource(), &QObject::destroyed, &session, [&]() {
        replaced = session.start(&replacementInput);
    });
    session.stop();
    QVERIFY(replaced);
    QVERIFY(session.positionSource());
    feed(replacementInput, {"$GPGSV,1,1,01,02,40,100,45", "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(session.satelliteObservation().satellitesInViewCount(), 1, TestTimeout::shortMs());
    QCOMPARE(session.satelliteObservation().satellites.first().signalStrength, std::optional<int>(45));
}

void NMEASatelliteAdapterTest::_identityResolution_data()
{
    using Constellation = GPSSatellite::Constellation;
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
    QFETCH(GPSSatellite::Constellation, expected);
    const std::optional<int> systemId = system < 0 ? std::nullopt : std::optional<int>(system);
    QCOMPARE(NMEAUtils::satelliteConstellation(talker, systemId, id), expected);
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
    NMEASatelliteAdapter adapter(&input);
    GPSSatelliteStore store;
    connectStore(adapter, store);
    // Synthetic accepted input: do not infer all IDs from the first ID or count 65 in two systems.
    feed(input, {gsa("GN", ids), gsa("GL", {"65"}), "$GPGSV,1,1,01,02,0,0,0", "$GLGSV,1,1,01,65,,,",
                 "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellites.size(), 2, TestTimeout::shortMs());
    QCOMPARE(store.observation().satellitesInUseCount(), 2);
    for (const auto& satellite : store.observation().satellites) {
        QCOMPARE(satellite.used, std::optional<bool>(true));
        if (satellite.id == 2) {
            QCOMPARE(satellite.constellation, GPSSatellite::Constellation::GPS);
            QCOMPARE(satellite.signalStrength, std::optional<int>(0));
        } else {
            QCOMPARE(satellite.id, 65);
            QCOMPARE(satellite.constellation, GPSSatellite::Constellation::GLONASS);
            QVERIFY(!satellite.signalStrength);
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
    NMEASatelliteAdapter adapter(&input);
    GPSSatelliteStore store;
    connectStore(adapter, store);
    input.receivedAtUs = GPSObservation::monotonicNowUs() - 100000;
    feed(input, {gsa("GN", ids), "$GQGSV,1,1,02,201,0,0,0,202,,,", "$BDGSV,1,1,01,201,0,0,0",
                 "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellites.size(), 3, TestTimeout::shortMs());
    QCOMPARE(store.observation().satellitesInUseCount(), -1);
    for (const auto& satellite : store.observation().satellites) {
        QVERIFY(!satellite.used.has_value());
    }
    input.receivedAtUs = GPSObservation::monotonicNowUs() - 50000;
    feed(input, {gsa("GN", {"201", "202"}, "5"), gsa("BD", {"201"}), "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(store.observation().satellitesInUseCount(), 3, TestTimeout::shortMs());
    const quint64 usedReceipt = input.receivedAtUs;
    input.receivedAtUs = GPSObservation::monotonicNowUs();
    QSignalSpy reports(&adapter, &NMEASatelliteAdapter::observationReceived);
    feed(input, {gsa("GN", ids), "$PQGSV,1,1,02,201,0,0,0,202,,,", "$GNRMC,120003.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(reports.size(), 1, TestTimeout::shortMs());
    QCOMPARE(store.observation().satellitesInUseCount(), 3);
    for (const auto& provenance : store.observation().provenance) {
        QCOMPARE(provenance.inUseTimestampUs, usedReceipt);
    }
}

void NMEASatelliteAdapterTest::_explicitZeroAndUnknownCoverage()
{
    QBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEASatelliteAdapter adapter(&input);
    QSignalSpy reports(&adapter, &NMEASatelliteAdapter::observationReceived);
    feed(input, {gsa("GN", {}), gsa("GN", {}, "0"), "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    // The following explicit GPS empty report is the only report with known coverage.
    feed(input, {gsa("GN", {}, "1"), "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_COMPARE_WITH_TIMEOUT(reports.size(), 1, TestTimeout::shortMs());
    const auto observation = reports.first().first().value<GPSSatelliteObservation>();
    QCOMPARE(observation.provenance.size(), 1);
    QCOMPARE(observation.provenance.first().constellation, GPSSatellite::Constellation::GPS);
    QCOMPARE(observation.provenance.first().satellitesUsed, std::optional<int>(0));
    QVERIFY(observation.provenance.first().usedSatelliteIds.has_value());
    QVERIFY(observation.provenance.first().usedSatelliteIds->isEmpty());
}
