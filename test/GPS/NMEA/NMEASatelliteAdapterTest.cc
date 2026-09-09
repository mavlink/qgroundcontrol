#include "NMEASatelliteAdapterTest.h"

#include <QtCore/QBuffer>
#include <QtPositioning/QNmeaSatelliteInfoSource>

#include "NMEASatelliteAdapter.h"
#include "NMEAUtils.h"

namespace {
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
    QNmeaSatelliteInfoSource decoder(QNmeaSatelliteInfoSource::UpdateMode::RealTimeMode);
    decoder.setDevice(&adapter);
    QSignalSpy used(&decoder, &QGeoSatelliteInfoSource::satellitesInUseUpdated);
    QSignalSpy view(&decoder, &QGeoSatelliteInfoSource::satellitesInViewUpdated);
    decoder.requestUpdate(5000);
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
    QTRY_VERIFY_WITH_TIMEOUT(!used.isEmpty() && !view.isEmpty(), TestTimeout::shortMs());
    const auto satellitesUsed = used.last().first().value<QList<QGeoSatelliteInfo>>();
    const auto satellitesVisible = view.last().first().value<QList<QGeoSatelliteInfo>>();
    QCOMPARE(satellitesUsed.size(), 5);
    QCOMPARE(satellitesVisible.size(), 10);
    int galileoSignals = 0;
    for (const auto& satellite : satellitesVisible) {
        if (satellite.satelliteSystem() == QGeoSatelliteInfo::GALILEO && satellite.satelliteIdentifier() == 2) {
            ++galileoSignals;
            QCOMPARE(satellite.signalStrength(), 40);
        }
    }
    QCOMPARE(galileoSignals, 1);

    // A new epoch must replace the previous signal lists, including an explicit no-fix report.
    used.clear();
    view.clear();
    decoder.requestUpdate(5000);
    feed(input, {"$GPGSV,1,1,00,0", "$GLGSV,1,1,00,0", "$GAGSV,1,1,00,0", "$GBGSV,1,1,00,0",
                 "$GNGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9,1", "$GNGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9,2",
                 "$GNGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9,3", "$GNGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9,4",
                 "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    QTRY_VERIFY_WITH_TIMEOUT(!used.isEmpty() && !view.isEmpty(), TestTimeout::shortMs());
    QVERIFY(used.last().first().value<QList<QGeoSatelliteInfo>>().isEmpty());
    QVERIFY(view.last().first().value<QList<QGeoSatelliteInfo>>().isEmpty());
}

void NMEASatelliteAdapterTest::_incompleteReportIsDiscarded()
{
    QBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEASatelliteAdapter adapter(&input);
    // A missing first fragment and an interrupted report must never become a complete sky view.
    feed(input, {"$GPGSV,2,2,05,05,10,100,20,1", "$GAGSV,2,1,05,01,10,100,20,02,10,100,20,03,10,100,20,04,10,100,20,7",
                 "$GNRMC,120001.00,V,,,,,,,090926,,,N"});
    QVERIFY(adapter.readAll().isEmpty());
    feed(input, {"$GPGSV,1,1,01,02,40,100,30,1", "$GNRMC,120002.00,V,,,,,,,090926,,,N"});
    const QByteArray output = adapter.readAll();
    QVERIFY(output.contains("$GPGSV,1,1,1,02,40,100,30*"));
    QVERIFY(!output.contains("$GAGSV"));
}

void NMEASatelliteAdapterTest::_idleBatchAndSourceClose()
{
    QBuffer input;
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEASatelliteAdapter adapter(&input);
    QSignalSpy ready(&adapter, &QIODevice::readyRead);
    // GSV-only sources have no position timestamps to delimit their batches.
    feed(input, {"$GPGSV,1,1,01,02,40,100,30"});
    QTRY_VERIFY_WITH_TIMEOUT(!ready.isEmpty(), TestTimeout::shortMs());
    QVERIFY(adapter.canReadLine());
    input.close();
    QVERIFY(!adapter.isOpen());
    QCOMPARE(adapter.bytesAvailable(), 0);
}

UT_REGISTER_TEST(NMEASatelliteAdapterTest, TestLabel::Unit)
