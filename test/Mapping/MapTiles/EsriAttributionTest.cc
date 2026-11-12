#include "EsriAttributionTest.h"

#include <chrono>
#include <limits>

#include <QtCore/QRegularExpression>
#include <QtCore/QUrl>
#include <QtTest/QSignalSpy>

#include "EsriAttribution.h"
#include "QGCMapEngineManager.h"
#include "TileReplyNetworkMock.h"

namespace {

const QUrl kEsriAttributionTestListUrl(QStringLiteral("https://lists.example/attribution/Some_Service?f=json"));

/// Esri's server sends its lists with a UTF-8 byte order mark
const QByteArray kEsriAttributionTestByteOrderMark("\xEF\xBB\xBF");

/// A list in the form of Esri's, written by hand. A bbox is south, west, north, east.
/// - "Globe" has the whole world, at the low zoom levels only
/// - "Region" has two areas of different scores, and is the name of a second contributor as well
/// - "©Town, ©County" and "Survey" have the same area and score
/// - "West Of 180" and "East Of 180" lie at the antimeridian, and "Tropics" at their latitudes far from it
QByteArray esriAttributionTestList()
{
    return QByteArray(R"({"contributors":[
{"attribution":"Globe","coverageAreas":[{"zoomMax":9,"zoomMin":0,"score":50,"bbox":[-85,-180,85,180]}]},
{"attribution":"Region","coverageAreas":[
    {"zoomMax":23,"zoomMin":10,"score":40,"bbox":[46,8,48,10]},
    {"zoomMax":23,"zoomMin":10,"score":90,"bbox":[47,9,48,10]}]},
{"attribution":"\u00a9Town, \u00a9County","coverageAreas":[
    {"zoomMax":23,"zoomMin":12,"score":60,"bbox":[47.2,8.4,47.6,8.8]}]},
{"attribution":"Survey","coverageAreas":[{"zoomMax":23,"zoomMin":12,"score":60,"bbox":[47.2,8.4,47.6,8.8]}]},
{"attribution":"Region","coverageAreas":[{"zoomMax":23,"zoomMin":14,"score":100,"bbox":[47.3,8.5,47.4,8.6]}]},
{"attribution":"Tropics","coverageAreas":[{"zoomMax":23,"zoomMin":10,"score":95,"bbox":[-20,0,-10,10]}]},
{"attribution":"West Of 180","coverageAreas":[{"zoomMax":23,"zoomMin":0,"score":80,"bbox":[-20,170,-10,180]}]},
{"attribution":"East Of 180","coverageAreas":[{"zoomMax":23,"zoomMin":0,"score":70,"bbox":[-20,-180,-10,-170]}]}
]})");
}

/// Asks \a attribution about the list at \a listUrl, which starts its fetch, and has \a nam answer the request with
/// the body, the error and the status that it has. False when \a attribution did not ask \a nam for the list.
/// \a nam must hold its replies.
bool esriAttributionTestFetch(EsriAttribution& attribution, TileReplyMockNam& nam, const QUrl& listUrl)
{
    const qsizetype repliesBefore = nam.replies.size();
    (void) attribution.credits(listUrl, 0., 0., 0., 0., 0);
    if ((nam.replies.size() != (repliesBefore + 1)) || !nam.replies.constLast()) {
        return false;
    }
    nam.replies.constLast()->finishNow();
    return true;
}

}  // namespace

void EsriAttributionTest::_creditsOfAreaAndZoom_data()
{
    QTest::addColumn<bool>("byteOrderMark");
    QTest::addColumn<double>("south");
    QTest::addColumn<double>("west");
    QTest::addColumn<double>("north");
    QTest::addColumn<double>("east");
    QTest::addColumn<int>("zoom");
    QTest::addColumn<QString>("credits");

    QTest::newRow("zoom at the zoomMax of an area and below the zoomMin of others")
        << true << 47.3 << 8.5 << 47.4 << 8.6 << 9 << QStringLiteral("Globe");
    QTest::newRow("zoom at the zoomMin of an area and above the zoomMax of another")
        << true << 46.1 << 8.1 << 46.2 << 8.2 << 10 << QStringLiteral("Region");
    QTest::newRow("no area there") << true << 50. << 20. << 51. << 21. << 12 << QString();
    QTest::newRow("area that touches a corner of one")
        << true << 45.5 << 7.5 << 46. << 8. << 10 << QStringLiteral("Region");

    // "Region" comes before the others in the list. Its area in the first of these has the lowest score.
    const QString regionLast = QStringLiteral("©Town, ©County, Survey, Region");
    const QString regionFirst = QStringLiteral("Region, ©Town, ©County, Survey");
    QTest::newRow("best score first, order of the list for the same score")
        << true << 47.25 << 8.45 << 47.35 << 8.55 << 12 << regionLast;
    QTest::newRow("list without byte order mark") << false << 47.25 << 8.45 << 47.35 << 8.55 << 12 << regionLast;
    QTest::newRow("best score of the areas of a contributor there")
        << true << 47.25 << 8.45 << 47.35 << 9.05 << 12 << regionFirst;
    QTest::newRow("name of two contributors once, at the best score")
        << true << 47.31 << 8.51 << 47.39 << 8.59 << 14 << regionFirst;

    // "Tropics" has the latitudes of these areas, far from the antimeridian: only one of every longitude has it
    const QString bothSides = QStringLiteral("West Of 180, East Of 180");
    QTest::newRow("east of the antimeridian")
        << true << -15. << -179. << -14. << -178. << 12 << QStringLiteral("East Of 180");
    QTest::newRow("short of the antimeridian") << true << -15. << 160. << -14. << 169. << 12 << QString();
    QTest::newRow("across the antimeridian, east beyond 180")
        << true << -15. << 175. << -14. << 185. << 12 << bothSides;
    QTest::newRow("across the antimeridian, east less than west")
        << true << -15. << 175. << -14. << -175. << 12 << bothSides;
    QTest::newRow("across the antimeridian, west beyond -180")
        << true << -15. << -185. << -14. << -175. << 12 << bothSides;
    QTest::newRow("beyond 180 altogether")
        << true << -15. << 181. << -14. << 183. << 12 << QStringLiteral("East Of 180");
    const QString everyLongitude = QStringLiteral("Tropics, West Of 180, East Of 180");
    QTest::newRow("every longitude") << true << -15. << -180. << -14. << 180. << 12 << everyLongitude;
    QTest::newRow("wider than the world") << true << -15. << -270. << -14. << 270. << 12 << everyLongitude;
}

void EsriAttributionTest::_creditsOfAreaAndZoom()
{
    QFETCH(bool, byteOrderMark);
    QFETCH(double, south);
    QFETCH(double, west);
    QFETCH(double, north);
    QFETCH(double, east);
    QFETCH(int, zoom);
    QFETCH(QString, credits);

    TileReplyMockNam nam;
    nam.holdReplies = true;
    nam.body = (byteOrderMark ? kEsriAttributionTestByteOrderMark : QByteArray()) + esriAttributionTestList();
    EsriAttribution attribution(&nam);
    QVERIFY(esriAttributionTestFetch(attribution, nam, kEsriAttributionTestListUrl));

    QCOMPARE(attribution.credits(kEsriAttributionTestListUrl, south, west, north, east, zoom), credits);
}

void EsriAttributionTest::_entriesNotUnderstoodAreLeftOut()
{
    // The view is in the western hemisphere, at zoom level 5: an area that lacks an edge, a zoom level or a score
    // would be credited there if what it lacks counted as 0
    TileReplyMockNam nam;
    nam.holdReplies = true;
    nam.body = QByteArray(R"({"contributors":[
{"coverageAreas":[{"zoomMax":23,"zoomMin":0,"score":90,"bbox":[-85,-180,85,180]}]},
{"attribution":"No Areas"},
{"attribution":"Areas Not In A List","coverageAreas":7},
{"attribution":"Three Edges","coverageAreas":[{"zoomMax":23,"zoomMin":0,"score":80,"bbox":[-85,-180,85]}]},
{"attribution":"Zoom In A String","coverageAreas":[{"zoomMax":23,"zoomMin":"3","score":70,"bbox":[-85,-180,85,180]}]},
{"attribution":"No Score","coverageAreas":[{"zoomMax":23,"zoomMin":0,"bbox":[-85,-180,85,180]}]},
"not a contributor",
{"attribution":"Understood","coverageAreas":[17, {"zoomMax":23,"zoomMin":0,"score":10,"bbox":[-85,-180,85,180]}]}
]})");
    EsriAttribution attribution(&nam);
    QVERIFY(esriAttributionTestFetch(attribution, nam, kEsriAttributionTestListUrl));

    QCOMPARE(attribution.credits(kEsriAttributionTestListUrl, 10., -20., 20., -10., 5), QStringLiteral("Understood"));
}

void EsriAttributionTest::_answerIsEmptyUntilListArrives()
{
    TileReplyMockNam nam;
    nam.holdReplies = true;
    nam.body = kEsriAttributionTestByteOrderMark + esriAttributionTestList();
    EsriAttribution attribution(&nam);
    QSignalSpy receivedSpy(&attribution, &EsriAttribution::listReceived);

    QVERIFY(attribution.credits(kEsriAttributionTestListUrl, 47.3, 8.5, 47.4, 8.6, 9).isEmpty());
    QCOMPARE(receivedSpy.count(), 0);
    QCOMPARE(nam.replies.size(), 1);
    QVERIFY(nam.replies.constFirst());

    nam.replies.constFirst()->finishNow();
    QCOMPARE(receivedSpy.count(), 1);
    QCOMPARE(receivedSpy.constFirst().constFirst().toUrl(), kEsriAttributionTestListUrl);
    QCOMPARE(attribution.credits(kEsriAttributionTestListUrl, 47.3, 8.5, 47.4, 8.6, 9), QStringLiteral("Globe"));
}

void EsriAttributionTest::_listIsFetchedOnce()
{
    TileReplyMockNam nam;
    nam.holdReplies = true;
    nam.body = kEsriAttributionTestByteOrderMark + esriAttributionTestList();
    EsriAttribution attribution(&nam);

    // Questions during the fetch
    QVERIFY(attribution.credits(kEsriAttributionTestListUrl, 47.3, 8.5, 47.4, 8.6, 9).isEmpty());
    QVERIFY(attribution.credits(kEsriAttributionTestListUrl, 46.1, 8.1, 46.2, 8.2, 10).isEmpty());
    QCOMPARE(nam.requestCount, 1);
    QCOMPARE(nam.requests.constFirst().url(), kEsriAttributionTestListUrl);
    QVERIFY(nam.replies.constFirst());
    nam.replies.constFirst()->finishNow();

    // Questions after it
    QCOMPARE(attribution.credits(kEsriAttributionTestListUrl, 47.3, 8.5, 47.4, 8.6, 9), QStringLiteral("Globe"));
    QCOMPARE(attribution.credits(kEsriAttributionTestListUrl, 46.1, 8.1, 46.2, 8.2, 10), QStringLiteral("Region"));
    QCOMPARE(nam.requestCount, 1);

    // Another list is a fetch of its own
    const QUrl otherListUrl(QStringLiteral("https://lists.example/attribution/Other_Service?f=json"));
    QVERIFY(attribution.credits(otherListUrl, 47.3, 8.5, 47.4, 8.6, 9).isEmpty());
    QCOMPARE(nam.requestCount, 2);
    QCOMPARE(nam.requests.constLast().url(), otherListUrl);
}

void EsriAttributionTest::_fetchWithoutList_data()
{
    QTest::addColumn<QNetworkReply::NetworkError>("networkError");
    QTest::addColumn<int>("httpStatus");
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<bool>("warns");
    QTest::addColumn<bool>("askedAgain");

    QTest::newRow("server not reached") << QNetworkReply::ConnectionRefusedError << 0 << QByteArray() << false << true;
    QTest::newRow("server error") << QNetworkReply::InternalServerError << 500 << QByteArray() << false << true;
    QTest::newRow("error status of a reply without an error")
        << QNetworkReply::NoError << 503 << esriAttributionTestList() << false << true;
    // What Esri's server answers for a map service that has no list
    QTest::newRow("no list on the server")
        << QNetworkReply::ContentNotFoundError << 404 << QByteArray("<html>Not Found</html>") << false << false;
    QTest::newRow("page in place of a list")
        << QNetworkReply::NoError << 200 << QByteArray("<html><body>Sign in to this network</body></html>") << true
        << true;
    QTest::newRow("JSON of another form")
        << QNetworkReply::NoError << 200 << QByteArray(R"({"error":{"code":498,"message":"Invalid token"}})") << true
        << true;
    QTest::newRow("contributors that are no list")
        << QNetworkReply::NoError << 200 << QByteArray(R"({"contributors":"Globe"})") << true << true;
    QTest::newRow("empty answer") << QNetworkReply::NoError << 200 << QByteArray() << true << true;
}

void EsriAttributionTest::_fetchWithoutList()
{
    QFETCH(QNetworkReply::NetworkError, networkError);
    QFETCH(int, httpStatus);
    QFETCH(QByteArray, body);
    QFETCH(bool, warns);
    QFETCH(bool, askedAgain);

    TileReplyMockNam nam;
    nam.holdReplies = true;
    nam.error = networkError;
    nam.httpStatus = httpStatus;
    nam.body = body;
    EsriAttribution attribution(&nam);
    QSignalSpy receivedSpy(&attribution, &EsriAttribution::listReceived);
    // Longer than the test takes
    attribution.setRetryDelay(std::chrono::hours(1));

    if (warns) {
        expectLogMessage("MapTiles.EsriAttribution", QtWarningMsg,
                         QRegularExpression(QStringLiteral("is not a list of contributors")));
    }
    QVERIFY(esriAttributionTestFetch(attribution, nam, kEsriAttributionTestListUrl));
    if (warns) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(receivedSpy.count(), 0);

    // The server has the list from here on. It is not asked before the delay has passed.
    nam.error = QNetworkReply::NoError;
    nam.httpStatus = 200;
    nam.body = kEsriAttributionTestByteOrderMark + esriAttributionTestList();
    QVERIFY(attribution.credits(kEsriAttributionTestListUrl, 47.3, 8.5, 47.4, 8.6, 9).isEmpty());
    QCOMPARE(nam.requestCount, 1);

    attribution.setRetryDelay(std::chrono::milliseconds(0));
    QVERIFY(attribution.credits(kEsriAttributionTestListUrl, 47.3, 8.5, 47.4, 8.6, 9).isEmpty());
    QCOMPARE(nam.requestCount, askedAgain ? 2 : 1);
    if (!askedAgain) {
        return;
    }

    QVERIFY(nam.replies.constLast());
    nam.replies.constLast()->finishNow();
    QCOMPARE(receivedSpy.count(), 1);
    QCOMPARE(attribution.credits(kEsriAttributionTestListUrl, 47.3, 8.5, 47.4, 8.6, 9), QStringLiteral("Globe"));
}

void EsriAttributionTest::_questionWithoutAnswerAsksNoServer_data()
{
    QTest::addColumn<QString>("mapType");
    QTest::addColumn<double>("latitude");
    QTest::addColumn<double>("width");

    QTest::newRow("map of another provider") << QStringLiteral("Bing Hybrid") << 47.35 << 800.;
    // Esri's server answers 404 for the list of this map's service
    QTest::newRow("Esri map without a list") << QStringLiteral("Esri Terrain") << 47.35 << 800.;
    QTest::newRow("not a map") << QStringLiteral("No Such Map") << 47.35 << 800.;
    QTest::newRow("view without a size") << QStringLiteral("Esri World Satellite") << 47.35 << 0.;
    QTest::newRow("view without a centre")
        << QStringLiteral("Esri World Satellite") << std::numeric_limits<double>::quiet_NaN() << 800.;
}

void EsriAttributionTest::_questionWithoutAnswerAsksNoServer()
{
    QFETCH(QString, mapType);
    QFETCH(double, latitude);
    QFETCH(double, width);

    TileReplyMockNam nam;
    nam.body = kEsriAttributionTestByteOrderMark + esriAttributionTestList();
    QGCMapEngineManager manager;
    manager.setAreaAttributionNetworkManager(&nam);

    QVERIFY(manager.areaAttribution(mapType, latitude, 8.55, 15., width, 600.).isEmpty());
    QCOMPARE(nam.requestCount, 0);
}

void EsriAttributionTest::_viewOfMapIsCreditedForItsArea_data()
{
    QTest::addColumn<QString>("mapType");
    QTest::addColumn<QString>("listUrl");
    QTest::addColumn<double>("latitude");
    QTest::addColumn<double>("longitude");
    QTest::addColumn<double>("zoomLevel");
    QTest::addColumn<double>("width");
    QTest::addColumn<double>("height");
    QTest::addColumn<QString>("credits");

    const QString satellite = QStringLiteral("Esri World Satellite");
    const QString satelliteList = QStringLiteral("https://static.arcgis.com/attribution/World_Imagery?f=json");
    const QString regionOnly = QStringLiteral("Region");
    const QString regionLast = QStringLiteral("©Town, ©County, Survey, Region");
    const QString regionFirst = QStringLiteral("Region, ©Town, ©County, Survey");

    QTest::newRow("satellite map") << satellite << satelliteList << 47.35 << 8.55 << 15. << 800. << 600. << regionFirst;
    QTest::newRow("street map") << QStringLiteral("Esri World Street")
                                << QStringLiteral("https://static.arcgis.com/attribution/World_Street_Map?f=json")
                                << 47.35 << 8.55 << 15. << 800. << 600. << regionFirst;

    // At zoom level 12 a pixel is 0.00034 degrees of longitude, and 0.00023 degrees of latitude at 47 degrees north.
    // The area of the town begins at 8.4 degrees east and at 47.2 degrees north.
    QTest::newRow("view that ends 0.03 degrees west of an area")
        << satellite << satelliteList << 47.4 << 8.2 << 12. << 1000. << 200. << regionOnly;
    QTest::newRow("wider view that reaches the area")
        << satellite << satelliteList << 47.4 << 8.2 << 12. << 1200. << 200. << regionLast;
    QTest::newRow("view that ends 0.01 degrees south of an area")
        << satellite << satelliteList << 47. << 8.6 << 12. << 200. << 1600. << regionOnly;
    QTest::newRow("higher view that reaches the area")
        << satellite << satelliteList << 47. << 8.6 << 12. << 200. << 1800. << regionLast;

    // The second "Region" is credited from zoom level 14 on
    QTest::newRow("zoom level between two tile zoom levels")
        << satellite << satelliteList << 47.35 << 8.55 << 13.99 << 400. << 300. << regionLast;
    QTest::newRow("zoom level that round-off left below a tile zoom level")
        << satellite << satelliteList << 47.35 << 8.55 << (14. - 1e-9) << 400. << 300. << regionFirst;

    QTest::newRow("view across the antimeridian") << satellite << satelliteList << -15. << 179.9 << 12. << 1000. << 200.
                                                  << QStringLiteral("West Of 180, East Of 180");
    // At zoom level 1 the world is 512 pixels wide
    QTest::newRow("view of the whole world") << satellite << satelliteList << 0. << 0. << 1. << 2000. << 2000.
                                             << QStringLiteral("West Of 180, East Of 180, Globe");
    QTest::newRow("view of half the world")
        << satellite << satelliteList << 0. << 0. << 1. << 256. << 256. << QStringLiteral("Globe");
}

void EsriAttributionTest::_viewOfMapIsCreditedForItsArea()
{
    QFETCH(QString, mapType);
    QFETCH(QString, listUrl);
    QFETCH(double, latitude);
    QFETCH(double, longitude);
    QFETCH(double, zoomLevel);
    QFETCH(double, width);
    QFETCH(double, height);
    QFETCH(QString, credits);

    TileReplyMockNam nam;
    nam.holdReplies = true;
    nam.body = kEsriAttributionTestByteOrderMark + esriAttributionTestList();
    QGCMapEngineManager manager;
    manager.setAreaAttributionNetworkManager(&nam);
    QSignalSpy changedSpy(&manager, &QGCMapEngineManager::areaAttributionChanged);
    const int revision = manager.areaAttributionRevision();

    // The first question about a map starts the fetch of the list of its map service
    QVERIFY(manager.areaAttribution(mapType, latitude, longitude, zoomLevel, width, height).isEmpty());
    QCOMPARE(nam.requestCount, 1);
    QCOMPARE(nam.requests.constFirst().url(), QUrl(listUrl));
    QCOMPARE(changedSpy.count(), 0);

    // The revision tells what asked that the answer is another one
    QVERIFY(nam.replies.constFirst());
    nam.replies.constFirst()->finishNow();
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE_NE(manager.areaAttributionRevision(), revision);
    QCOMPARE(manager.areaAttribution(mapType, latitude, longitude, zoomLevel, width, height), credits);
    QCOMPARE(nam.requestCount, 1);
}

UT_REGISTER_TEST(EsriAttributionTest, TestLabel::Unit)
