#include "LogViewerMapUITest.h"

#include <cstring>

#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QRectF>
#include <QtCore/QTemporaryDir>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlExpression>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include "QGCFileDialogController.h"
#include "QGCMapEngineManager.h"
#include "QGroundControlQmlGlobal.h"

UT_REGISTER_TEST(LogViewerMapUITest, TestLabel::Integration)

/// Writes a DataFlash log of POS messages, one a second. They carry an altitude so that the page shows its
/// altitude chart, whose cursor moves the dot on the map.
bool LogViewerMapUITest::_writePositionLog(const QString& filePath, const QList<QGeoCoordinate>& path)
{
    constexpr char kFormatType = static_cast<char>(128);
    constexpr char kPositionType = static_cast<char>(155);
    const QByteArray header = QByteArray::fromHex("a395");

    // FMT: type, length, name[4], format[16], columns[64]
    QByteArray format(86, '\0');
    format[0] = kPositionType;
    format[1] = 3 + 8 + 4 + 4 + 4;
    std::memcpy(format.data() + 2, "POS", 3);
    std::memcpy(format.data() + 6, "QLLf", 4);
    std::memcpy(format.data() + 22, "TimeUS,Lat,Lng,Alt", 18);
    QByteArray bytes = header + kFormatType + format;

    quint64 timeUs = 1000000;
    for (const QGeoCoordinate& coordinate : path) {
        const qint32 latitude = qRound(coordinate.latitude() * 1e7);
        const qint32 longitude = qRound(coordinate.longitude() * 1e7);
        const float altitude = 600.0f;
        QByteArray payload(20, '\0');
        std::memcpy(payload.data(), &timeUs, 8);
        std::memcpy(payload.data() + 8, &latitude, 4);
        std::memcpy(payload.data() + 12, &longitude, 4);
        std::memcpy(payload.data() + 16, &altitude, 4);
        bytes += header + kPositionType + payload;
        timeUs += 1000000;
    }

    QFile file(filePath);
    return file.open(QIODevice::WriteOnly) && (file.write(bytes) == bytes.size());
}

void LogViewerMapUITest::_addEngineRows()
{
    QTest::addColumn<bool>("geoMapEngine");

    QTest::newRow("QtLocation") << false;
    QTest::newRow("GeoMap") << true;
}

QList<QGeoCoordinate> LogViewerMapUITest::_logPath()
{
    return {
        QGeoCoordinate(-35.3632, 149.1652), QGeoCoordinate(-35.3600, 149.1652), QGeoCoordinate(-35.3600, 149.1700),
        QGeoCoordinate(-35.3632, 149.1700), QGeoCoordinate(-35.3650, 149.1676),
    };
}

QString LogViewerMapUITest::_mapName(bool geoMapEngine)
{
    return geoMapEngine ? QStringLiteral("logViewer_geoMap") : QStringLiteral("logViewer_flightMap");
}

void LogViewerMapUITest::_openLogViewer(bool geoMapEngine)
{
    setUseGeoMapFlyEngine(geoMapEngine);

    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }
    if (geoMapEngine) {
        QVERIFY2(expectSoftwareBackendWarnings(/*strict*/ false).has_value(),
                 "No renderer interface on the main window");
    }

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewAnalyze")));
    QVERIFY2(clickButton(QStringLiteral("analyzeButton_Log Viewer")), "Log Viewer analyze page button not found");
}

QQuickItem* LogViewerMapUITest::_openLog(const QString& logFile, bool geoMapEngine)
{
    QGCFileDialogController::setTestNextFileForAccept(logFile);
    if (!clickButton(QStringLiteral("logViewer_openBinButton"))) {
        QTest::qFail("button to open a log not found", __FILE__, __LINE__);
        return nullptr;
    }

    // The engine setting picks the map, and the other engine's map is not created
    const QString mapName = _mapName(geoMapEngine);
    const QString otherMapName = _mapName(!geoMapEngine);
    QQuickItem* const map = findItem(_rootItem, mapName);
    if (!map) {
        QTest::qFail(qPrintable(mapName + QStringLiteral(" not created")), __FILE__, __LINE__);
        return nullptr;
    }
    if (findItem(_rootItem, otherMapName)) {
        QTest::qFail(qPrintable(otherMapName + QStringLiteral(" was created")), __FILE__, __LINE__);
        return nullptr;
    }

    QQmlExpression logHasPath(qmlContext(map), map, QStringLiteral("_mapTab._hasPath"));
    if (!QTest::qWaitFor([&logHasPath] { return logHasPath.evaluate().toBool(); }, TestTimeout::mediumMs())) {
        QTest::qFail("the page has no path of the log", __FILE__, __LINE__);
        return nullptr;
    }
    return map;
}

void LogViewerMapUITest::_showsLogPath_data()
{
    _addEngineRows();
}

void LogViewerMapUITest::_showsLogPath()
{
    QFETCH(bool, geoMapEngine);

    const QList<QGeoCoordinate> path = _logPath();
    QTemporaryDir logDir;
    QVERIFY(logDir.isValid());
    const QString logFile = logDir.filePath(QStringLiteral("track.bin"));
    QVERIFY(_writePositionLog(logFile, path));

    _openLogViewer(geoMapEngine);
    if (QTest::currentTestFailed()) {
        return;
    }
    // Read here: the Fly View map, shown at startup, settles the shared view to what its window can show
    const QGeoCoordinate sharedPosition = QGroundControlQmlGlobal::flightMapPosition();
    const double sharedZoom = QGroundControlQmlGlobal::flightMapZoom();
    QQuickItem* const map = _openLog(logFile, geoMapEngine);
    if (!map) {
        return;
    }

    // The log is read before the tab of the map is shown for the first time: the map is asked for the path while
    // it has no size, and shows it once it has one
    QVERIFY2(map->width() <= 0 || map->height() <= 0, "the map has a size before its tab was shown");
    QVERIFY(clickButton(QStringLiteral("logViewer_mapTabButton")));
    QVERIFY2(findVisibleItem(_rootItem, _mapName(geoMapEngine), TestTimeout::mediumMs()),
             "the map of the log is not visible");

    QTRY_VERIFY2_WITH_TIMEOUT(mapShowsCoordinates(map, path), "the map does not show the whole path",
                              TestTimeout::mediumMs());

    // The map credits its imagery, under its scale
    const QString mapCredit = QGCMapEngineManager::mapAttribution(QGCMapEngineManager::instance()->chosenMap());
    QVERIFY2(!mapCredit.isEmpty(), "the chosen map has no credit");
    QQuickItem* const credit = findVisibleItem(map, QStringLiteral("mapAttribution"), TestTimeout::shortMs());
    QVERIFY2(credit, "the map shows no credit");
    QCOMPARE(credit->property("text").toString(), mapCredit);
    QQuickItem* const scale = findVisibleItem(map, QStringLiteral("logViewer_mapScale"));
    QVERIFY2(scale, "map scale not visible");
    const auto rectInMap = [map](const QQuickItem* item) {
        return item->mapRectToItem(map, QRectF(0, 0, item->width(), item->height()));
    };
    // The scale ends where the credit begins: half a pixel is for the rounding of the two positions
    QTRY_COMPARE_LE_WITH_TIMEOUT(rectInMap(scale).bottom(), rectInMap(credit).top() + 0.5, TestTimeout::shortMs());

    if (geoMapEngine) {
        QQuickItem* const track = findItem(map, QStringLiteral("logViewer_geoMapTrack"));
        QVERIFY2(track, "track not found");
        QTRY_COMPARE_WITH_TIMEOUT(track->property("pointCount").toInt(), path.count(), TestTimeout::shortMs());
    }

    // Loading a log puts the cursor of the charts mid-log, and the dot on the path point of that time
    QQuickItem* const dot = findVisibleItem(map, QStringLiteral("logViewer_positionDot"), TestTimeout::mediumMs());
    QVERIFY2(dot, "position dot not visible");
    const auto dotCenter = [dot, map] { return dot->mapToItem(map, QPointF(dot->width() / 2, dot->height() / 2)); };
    QTRY_COMPARE_LT_WITH_TIMEOUT((dotCenter() - mapPoint(map, path[2])).manhattanLength(), 3.0, TestTimeout::shortMs());

    // The log's place stays the log viewer's: Fly View and Plan View keep theirs
    QCOMPARE(QGroundControlQmlGlobal::flightMapPosition(), sharedPosition);
    QCOMPARE(QGroundControlQmlGlobal::flightMapZoom(), sharedZoom);

    // Leaving the Analyze view unloads the page and its map while the window stays. A GeoMap must take its 3D
    // scene along: Qt warns of one left behind when the window closes, which fails the test.
    const QPointer<QQuickItem> unloadedMap(map);
    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewFly")));
    QTRY_VERIFY_WITH_TIMEOUT(unloadedMap.isNull(), TestTimeout::shortMs());

    stopUI();
}

void LogViewerMapUITest::_chartCursorMovesPositionDot_data()
{
    _addEngineRows();
}

void LogViewerMapUITest::_chartCursorMovesPositionDot()
{
    QFETCH(bool, geoMapEngine);

    const QList<QGeoCoordinate> path = _logPath();
    QTemporaryDir logDir;
    QVERIFY(logDir.isValid());
    const QString logFile = logDir.filePath(QStringLiteral("track.bin"));
    QVERIFY(_writePositionLog(logFile, path));

    _openLogViewer(geoMapEngine);
    if (QTest::currentTestFailed()) {
        return;
    }
    QQuickItem* const map = _openLog(logFile, geoMapEngine);
    if (!map) {
        return;
    }
    QVERIFY(clickButton(QStringLiteral("logViewer_mapTabButton")));
    QTRY_VERIFY2_WITH_TIMEOUT(mapShowsCoordinates(map, path), "the map does not show the whole path",
                              TestTimeout::mediumMs());

    // The log has a point of the path for every second. The dot is on the point whose time is nearest to that of
    // the cursor of the charts, which the page keeps in seconds: -1 while it has no cursor.
    QQuickItem* const dot = findVisibleItem(map, QStringLiteral("logViewer_positionDot"), TestTimeout::mediumMs());
    QVERIFY2(dot, "position dot not visible");
    const auto dotCenter = [dot, map] { return dot->mapToItem(map, QPointF(dot->width() / 2, dot->height() / 2)); };
    QQmlExpression secondsAfterFirstPoint(qmlContext(map), map,
                                          QStringLiteral("_mapTab._sharedCursorT - logParser.minTimestamp"));
    const auto cursorPoint = [&secondsAfterFirstPoint, &path]() -> int {
        const double seconds = secondsAfterFirstPoint.evaluate().toDouble();
        return qIsNaN(seconds) ? -1 : qBound(0, qRound(seconds), int(path.size()) - 1);
    };

    // A log that was just loaded has the cursor in its middle
    constexpr int kMiddlePoint = 2;
    QTRY_COMPARE_WITH_TIMEOUT(cursorPoint(), kMiddlePoint, TestTimeout::shortMs());
    QTRY_COMPARE_LT_WITH_TIMEOUT((dotCenter() - mapPoint(map, path[kMiddlePoint])).manhattanLength(), 3.0,
                                 TestTimeout::shortMs());

    // A click in the altitude chart under the map puts the cursor there: near the end of the log
    const QString altitudeChart = QStringLiteral("logViewer_altChart");
    QVERIFY(clickItemFraction(altitudeChart, 0.9, 0.5));
    QTRY_COMPARE_GT_WITH_TIMEOUT(cursorPoint(), kMiddlePoint, TestTimeout::shortMs());
    const int laterPoint = cursorPoint();
    QTRY_COMPARE_LT_WITH_TIMEOUT((dotCenter() - mapPoint(map, path[laterPoint])).manhattanLength(), 3.0,
                                 TestTimeout::shortMs());

    // And near its start
    QVERIFY(clickItemFraction(altitudeChart, 0.1, 0.5));
    QTRY_VERIFY_WITH_TIMEOUT((cursorPoint() >= 0) && (cursorPoint() < kMiddlePoint), TestTimeout::shortMs());
    const int earlierPoint = cursorPoint();
    QTRY_COMPARE_LT_WITH_TIMEOUT((dotCenter() - mapPoint(map, path[earlierPoint])).manhattanLength(), 3.0,
                                 TestTimeout::shortMs());

    stopUI();
}
