#include "LogViewerMapUITest.h"

#include <cstring>

#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlExpression>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include "Fact.h"
#include "FlyViewSettings.h"
#include "QGCFileDialogController.h"
#include "QGroundControlQmlGlobal.h"
#include "SettingsManager.h"

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

/// Where the map draws a coordinate, in the map's own pixels. Both engines' maps have fromCoordinate.
QPointF LogViewerMapUITest::_mapPoint(QQuickItem* map, const QGeoCoordinate& coordinate)
{
    QQmlExpression expression(qmlContext(map), map,
                              QStringLiteral("fromCoordinate(QtPositioning.coordinate(%1, %2), false)")
                                  .arg(coordinate.latitude(), 0, 'f', 7)
                                  .arg(coordinate.longitude(), 0, 'f', 7));
    return expression.evaluate().toPointF();
}

/// True once the map shows the whole path, and large: a path far away or at world scale does not count
bool LogViewerMapUITest::_pathFitsMap(QQuickItem* map, const QList<QGeoCoordinate>& path)
{
    const QRectF mapRect(0, 0, map->width(), map->height());
    if (mapRect.isEmpty()) {
        return false;  // the tab was just shown and is not laid out yet
    }
    QRectF pathRect;
    for (const QGeoCoordinate& coordinate : path) {
        const QPointF point = _mapPoint(map, coordinate);
        if (!mapRect.adjusted(-1, -1, 1, 1).contains(point)) {
            return false;
        }
        pathRect = pathRect.isNull() ? QRectF(point, QSizeF(1, 1)) : pathRect.united(QRectF(point, QSizeF(1, 1)));
    }
    return qMax(pathRect.width(), pathRect.height()) > (qMin(mapRect.width(), mapRect.height()) / 4);
}

void LogViewerMapUITest::_showsLogPath_data()
{
    QTest::addColumn<bool>("geoMapEngine");

    QTest::newRow("QtLocation") << false;
    QTest::newRow("GeoMap") << true;
}

void LogViewerMapUITest::_showsLogPath()
{
    QFETCH(bool, geoMapEngine);

    Fact* const geoEngineFact = SettingsManager::instance()->flyViewSettings()->useGeoMapEngine();
    const QVariant savedEngine = geoEngineFact->rawValue();
    const auto restoreEngine = qScopeGuard([geoEngineFact, savedEngine] { geoEngineFact->setRawValue(savedEngine); });
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    geoEngineFact->setRawValue(geoMapEngine);

    // A place of its own, away from the map view the other views share
    const QList<QGeoCoordinate> path = {
        QGeoCoordinate(-35.3632, 149.1652), QGeoCoordinate(-35.3600, 149.1652), QGeoCoordinate(-35.3600, 149.1700),
        QGeoCoordinate(-35.3632, 149.1700), QGeoCoordinate(-35.3650, 149.1676),
    };
    QTemporaryDir logDir;
    QVERIFY(logDir.isValid());
    const QString logFile = logDir.filePath(QStringLiteral("track.bin"));
    QVERIFY(_writePositionLog(logFile, path));

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
    // Read here: the Fly View map, shown at startup, settles the shared view to what its window can show
    const QGeoCoordinate sharedPosition = QGroundControlQmlGlobal::flightMapPosition();
    const double sharedZoom = QGroundControlQmlGlobal::flightMapZoom();
    QGCFileDialogController::setTestNextFileForAccept(logFile);
    QVERIFY(clickButton(QStringLiteral("logViewer_openBinButton")));
    QVERIFY(clickButton(QStringLiteral("logViewer_mapTabButton")));

    // The engine setting picks the map, and the other engine's map is not created
    const QString mapName = geoMapEngine ? QStringLiteral("logViewer_geoMap") : QStringLiteral("logViewer_flightMap");
    const QString otherMapName =
        geoMapEngine ? QStringLiteral("logViewer_flightMap") : QStringLiteral("logViewer_geoMap");
    QQuickItem* const map = findVisibleItem(_rootItem, mapName, TestTimeout::mediumMs());
    QVERIFY2(map, qPrintable(mapName + QStringLiteral(" not visible")));
    QVERIFY2(!findItem(_rootItem, otherMapName), qPrintable(otherMapName + QStringLiteral(" was created")));

    QTRY_VERIFY2_WITH_TIMEOUT(_pathFitsMap(map, path), "the map does not show the whole path", TestTimeout::mediumMs());
    if (geoMapEngine) {
        QQuickItem* const track = findItem(map, QStringLiteral("logViewer_geoMapTrack"));
        QVERIFY2(track, "track not found");
        QTRY_COMPARE_WITH_TIMEOUT(track->property("pointCount").toInt(), path.count(), TestTimeout::shortMs());
    }

    // Loading a log puts the cursor of the charts mid-log, and the dot on the path point of that time
    QQuickItem* const dot = findVisibleItem(map, QStringLiteral("logViewer_positionDot"), TestTimeout::mediumMs());
    QVERIFY2(dot, "position dot not visible");
    const QPointF midPathPoint = _mapPoint(map, path[2]);
    const auto dotCenter = [dot, map] { return dot->mapToItem(map, QPointF(dot->width() / 2, dot->height() / 2)); };
    QTRY_COMPARE_LT_WITH_TIMEOUT((dotCenter() - midPathPoint).manhattanLength(), 3.0, TestTimeout::shortMs());

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
