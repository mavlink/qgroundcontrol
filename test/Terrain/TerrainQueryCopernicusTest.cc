#include "TerrainQueryCopernicusTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QSslConfiguration>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>

#include "TerrainQueryCopernicus.h"
#include "TileReplyNetworkMock.h"

namespace {

/// The answer of the server to the request for a carpet of three by three elevations
const QByteArray kCarpetAnswer =
    QByteArrayLiteral(R"({"data":{"bounds":{"ne":[47.3905,8.5405],"sw":[47.39,8.54]},)"
                      R"("carpet":[[464.0,462.0,464.0],[460.0,463.0,467.0],[462.0,464.0,467.0]],)"
                      R"("stats":{"avg":463.6666564941406,"max":467.0,"min":460.0}},"status":"success"})");

const QGeoCoordinate kSouthWest(47.39, 8.54);
const QGeoCoordinate kNorthEast(47.3905, 8.5405);

/// Asks through a network manager that answers itself
class CopernicusQueryOfMock : public TerrainQueryCopernicus
{
public:
    CopernicusQueryOfMock()
    {
        delete _networkManager;
        _networkManager = &network;
    }

    TileReplyMockNam network;
};

}  // namespace

void TerrainQueryCopernicusTest::_testCarpetHeights_data()
{
    QTest::addColumn<bool>("statsOnly");
    QTest::addColumn<QList<QList<double>>>("carpet");
    QTest::newRow("carpet") << false
                            << QList<QList<double>>{{464., 462., 464.}, {460., 463., 467.}, {462., 464., 467.}};
    QTest::newRow("stats only") << true << QList<QList<double>>();
}

void TerrainQueryCopernicusTest::_testCarpetHeights()
{
    QFETCH(bool, statsOnly);
    QFETCH(QList<QList<double>>, carpet);

    CopernicusQueryOfMock query;
    query.network.body = kCarpetAnswer;
    QSignalSpy spy(&query, &TerrainQueryInterface::carpetHeightsReceived);

    query.requestCarpetHeights(kSouthWest, kNorthEast, statsOnly);
    QVERIFY(spy.wait(TestTimeout::mediumMs()));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toBool(), true);
    QCOMPARE(spy.first().at(1).toDouble(), 460.);
    QCOMPARE(spy.first().at(2).toDouble(), 467.);
    QCOMPARE(spy.first().at(3).value<QList<QList<double>>>(), carpet);

    QCOMPARE(query.network.requestCount, 1);
    const QNetworkRequest request = query.network.requests.constFirst();
    // The certificate of the server is checked, in a debug build as in any other
    QVERIFY(request.sslConfiguration().peerVerifyMode() != QSslSocket::VerifyNone);
    const QUrl url = request.url();
    QCOMPARE(url.path(), QStringLiteral("/api/v1/carpet"));
    QCOMPARE(QUrlQuery(url).queryItemValue(QStringLiteral("points")),
             QStringLiteral("47.3900000000,8.5400000000,47.3905000000,8.5405000000"));
}

void TerrainQueryCopernicusTest::_testCarpetAnswerThatIsNoCarpetFails_data()
{
    QTest::addColumn<QByteArray>("answer");
    QTest::addColumn<QString>("category");
    QTest::addColumn<QString>("warning");
    QTest::newRow("no JSON") << QByteArrayLiteral("<html>") << QStringLiteral("MapTiles.CopernicusTileFormat")
                             << QStringLiteral("Terrain tile json doc parse error");
    QTest::newRow("refused") << QByteArrayLiteral(R"({"data":{},"status":"error"})")
                             << QStringLiteral("MapTiles.CopernicusTileFormat")
                             << QStringLiteral("Invalid terrain tile");
    QTest::newRow("stats without elevations")
        << QByteArrayLiteral(R"({"data":{"bounds":{},"carpet":[[1.0]],"stats":{}},"status":"success"})")
        << QStringLiteral("Terrain.TerrainQueryCopernicus")
        << QStringLiteral("Carpet without its lowest and highest elevation");
}

void TerrainQueryCopernicusTest::_testCarpetAnswerThatIsNoCarpetFails()
{
    QFETCH(QByteArray, answer);
    QFETCH(QString, category);
    QFETCH(QString, warning);

    CopernicusQueryOfMock query;
    query.network.body = answer;
    QSignalSpy spy(&query, &TerrainQueryInterface::carpetHeightsReceived);

    const QByteArray categoryName = category.toLatin1();
    expectLogMessage(categoryName.constData(), QtWarningMsg, QRegularExpression(warning));
    query.requestCarpetHeights(kSouthWest, kNorthEast, false);
    QVERIFY(spy.wait(TestTimeout::mediumMs()));
    verifyExpectedLogMessage();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toBool(), false);
    QCOMPARE(spy.first().at(3).value<QList<QList<double>>>(), QList<QList<double>>());
}

void TerrainQueryCopernicusTest::_testCarpetRequestThatFailsFails()
{
    CopernicusQueryOfMock query;
    query.network.error = QNetworkReply::OperationNotImplementedError;
    query.network.httpStatus = 501;
    QSignalSpy spy(&query, &TerrainQueryInterface::carpetHeightsReceived);

    // Told when the error occurs, and when the reply finishes
    expectLogMessage("Terrain.TerrainQueryInterface", QtWarningMsg,
                     QRegularExpression(QStringLiteral("error:url:data")));
    expectLogMessage("Terrain.TerrainQueryCopernicus", QtWarningMsg,
                     QRegularExpression(QStringLiteral("error:url:data")));
    query.requestCarpetHeights(kSouthWest, kNorthEast, false);
    QVERIFY(spy.wait(TestTimeout::mediumMs()));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toBool(), false);
}

void TerrainQueryCopernicusTest::_testQueriesTheServerLacksFailWithoutRequest()
{
    CopernicusQueryOfMock query;
    QSignalSpy coordinateSpy(&query, &TerrainQueryInterface::coordinateHeightsReceived);
    QSignalSpy pathSpy(&query, &TerrainQueryInterface::pathHeightsReceived);

    expectLogMessage("Terrain.TerrainQueryCopernicus", QtWarningMsg,
                     QRegularExpression(QStringLiteral("no query for the heights of coordinates")));
    query.requestCoordinateHeights({kSouthWest, kNorthEast});
    verifyExpectedLogMessage();
    QCOMPARE(coordinateSpy.count(), 1);
    QCOMPARE(coordinateSpy.first().at(0).toBool(), false);

    expectLogMessage("Terrain.TerrainQueryCopernicus", QtWarningMsg,
                     QRegularExpression(QStringLiteral("no query for the heights of a path")));
    query.requestPathHeights(kSouthWest, kNorthEast);
    verifyExpectedLogMessage();
    QCOMPARE(pathSpy.count(), 1);
    QCOMPARE(pathSpy.first().at(0).toBool(), false);

    QCOMPARE(query.network.requestCount, 0);
}

UT_REGISTER_TEST(TerrainQueryCopernicusTest, TestLabel::Unit, TestLabel::Terrain)
