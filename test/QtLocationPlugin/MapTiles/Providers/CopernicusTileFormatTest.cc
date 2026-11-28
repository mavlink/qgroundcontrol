#include "CopernicusTileFormatTest.h"

#include <QtCore/QRegularExpression>
#include <QtPositioning/QGeoCoordinate>

#include "CopernicusTileFormat.h"
#include "TerrainTile.h"

namespace {

const QByteArray kCopernicusBounds = QByteArrayLiteral(R"("bounds":{"sw":[47.39,8.54],"ne":[47.4,8.55]})");
const QByteArray kCopernicusStats = QByteArrayLiteral(R"("stats":{"min":460,"max":467,"avg":463.5})");
const QByteArray kCopernicusCarpet = QByteArrayLiteral(R"("carpet":[[460,461],[466,467]])");

/// The answer of the server with the members \a data as its data
QByteArray copernicusAnswer(const QByteArrayList& data)
{
    return QByteArrayLiteral(R"({"status":"success","data":{)") + data.join(',') + QByteArrayLiteral("}}");
}

}  // namespace

void CopernicusTileFormatTest::_testSerializedAnswerIsReadAsTile()
{
    const QByteArray tile =
        CopernicusTileFormat::serialize(copernicusAnswer({kCopernicusBounds, kCopernicusStats, kCopernicusCarpet}));
    QCOMPARE(tile.size(), static_cast<qsizetype>(sizeof(CopernicusTileFormat::Header) + (4 * sizeof(int16_t))));

    const TerrainTile terrainTile(tile);
    QVERIFY(terrainTile.isValid());
    QCOMPARE(terrainTile.minElevation(), 460.);
    QCOMPARE(terrainTile.maxElevation(), 467.);
    QCOMPARE(terrainTile.avgElevation(), 463.5);
    // The rows of the carpet run northwards from the south west corner, and the elevations of a row eastwards
    QCOMPARE(terrainTile.elevation(QGeoCoordinate(47.391, 8.541)), 460.);
    QCOMPARE(terrainTile.elevation(QGeoCoordinate(47.391, 8.549)), 461.);
    QCOMPARE(terrainTile.elevation(QGeoCoordinate(47.399, 8.541)), 466.);
    QCOMPARE(terrainTile.elevation(QGeoCoordinate(47.399, 8.549)), 467.);
}

void CopernicusTileFormatTest::_testAnswerThatIsNoTileGivesNone_data()
{
    QTest::addColumn<QByteArray>("answer");
    QTest::addColumn<QString>("warning");  // empty: none

    const QString missingKey = QStringLiteral("Error in reading json");
    QTest::newRow("page in place of an answer")
        << QByteArrayLiteral("<html>") << QStringLiteral("Terrain tile json doc parse error");
    QTest::newRow("JSON that is no object")
        << QByteArrayLiteral("[1]") << QStringLiteral("Terrain tile json doc is no object");
    QTest::newRow("answer without data") << QByteArrayLiteral(R"({"status":"success"})") << missingKey;
    QTest::newRow("answer of a server that refuses")
        << QByteArrayLiteral(R"({"status":"error","data":{}})") << QStringLiteral("Invalid terrain tile");
    QTest::newRow("data without the carpet") << copernicusAnswer({kCopernicusBounds, kCopernicusStats}) << missingKey;
    QTest::newRow("bounds without a corner")
        << copernicusAnswer({QByteArrayLiteral(R"("bounds":{"sw":[47.39,8.54]})"), kCopernicusStats, kCopernicusCarpet})
        << missingKey;
    QTest::newRow("corner without its longitude")
        << copernicusAnswer(
               {QByteArrayLiteral(R"("bounds":{"sw":[47.39],"ne":[47.4,8.55]})"), kCopernicusStats, kCopernicusCarpet})
        << QStringLiteral("Incomplete bounding location");
    QTest::newRow("stats without the average")
        << copernicusAnswer(
               {kCopernicusBounds, QByteArrayLiteral(R"("stats":{"min":460,"max":467})"), kCopernicusCarpet})
        << missingKey;
    QTest::newRow("carpet without a row")
        << copernicusAnswer({kCopernicusBounds, kCopernicusStats, QByteArrayLiteral(R"("carpet":[])")})
        << QStringLiteral("Empty carpet array");
    QTest::newRow("row without an elevation")
        << copernicusAnswer({kCopernicusBounds, kCopernicusStats, QByteArrayLiteral(R"("carpet":[[]])")})
        << QStringLiteral("Empty first row");
    // The tile would have elevations that the answer lacks
    QTest::newRow("row shorter than the first")
        << copernicusAnswer({kCopernicusBounds, kCopernicusStats, QByteArrayLiteral(R"("carpet":[[460,461],[466]])")})
        << QString();
}

void CopernicusTileFormatTest::_testAnswerThatIsNoTileGivesNone()
{
    QFETCH(QByteArray, answer);
    QFETCH(QString, warning);

    if (!warning.isEmpty()) {
        expectLogMessage("MapTiles.CopernicusTileFormat", QtWarningMsg, QRegularExpression(warning));
    }
    QVERIFY(CopernicusTileFormat::serialize(answer).isEmpty());
    if (!warning.isEmpty()) {
        verifyExpectedLogMessage();
    }
}

UT_REGISTER_TEST(CopernicusTileFormatTest, TestLabel::Unit)
