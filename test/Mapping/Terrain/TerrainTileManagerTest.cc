#include "TerrainTileManagerTest.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>

#include "ElevationMapProvider.h"
#include "QGCMapEngine.h"
#include "TerrainQueryInterface.h"
#include "TerrainTileCopernicus.h"
#include "TerrainTileManager.h"
#include "TileReplyNetworkMock.h"
#include "UrlFactory.h"

namespace {

/// What the Copernicus server answers for a tile around \a center whose one cell has the height \a height
QByteArray terrainTileManagerTestServerTile(const QGeoCoordinate& center, double height)
{
    constexpr double halfTileDeg = TerrainTileCopernicus::kTileSizeDegrees / 2.0;

    QJsonArray row;
    row.append(height);
    QJsonArray carpet;
    carpet.append(row);

    const QJsonObject root{
        {"status", "success"},
        {"data",
         QJsonObject{
             {"bounds",
              QJsonObject{{"sw", QJsonArray{center.latitude() - halfTileDeg, center.longitude() - halfTileDeg}},
                          {"ne", QJsonArray{center.latitude() + halfTileDeg, center.longitude() + halfTileDeg}}}},
             {"stats", QJsonObject{{"min", height}, {"max", height}, {"avg", height}}},
             {"carpet", carpet},
         }},
    };
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

}  // namespace

// Asking the cache again gets the same bytes: the query would wait for ever if the server were not asked
void TerrainTileManagerTest::_testUndecodableCachedTileIsFetchedAgain()
{
    const QString type = QString::fromLatin1(CopernicusElevationProvider::kProviderKey);
    const QGeoCoordinate coordinate(-61.235, 97.415);  // in a tile no other test asks for
    constexpr double serverHeight = 25.0;

    const QString hash = UrlFactory::getTileHash(type, UrlFactory::long2tileX(type, coordinate.longitude(), 1),
                                                 UrlFactory::lat2tileY(type, coordinate.latitude(), 1), 1);
    QGCMapEngine* const engine = getQGCMapEngine();
    engine->ensureInitialized();
    engine->cacheTile(type, hash, QByteArrayLiteral("not a terrain tile"), QStringLiteral("bin"));

    TileReplyMockNam nam;  // must outlive the manager, which borrows it
    nam.body = terrainTileManagerTestServerTile(coordinate, serverHeight);
    TerrainTileManager manager(nullptr, &nam);
    TerrainQueryInterface query;
    QSignalSpy heightsSpy(&query, &TerrainQueryInterface::coordinateHeightsReceived);

    expectLogMessage("Terrain.TerrainTile", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Terrain tile binary data too small for TileInfo_t header")));
    manager.addCoordinateQuery(&query, {coordinate});
    QVERIFY_SIGNAL_COUNT_WAIT(heightsSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    const QVariantList arguments = heightsSpy.constFirst();
    QVERIFY(arguments.at(0).toBool());
    QCOMPARE(qvariant_cast<QList<double>>(arguments.at(1)), QList<double>{serverHeight});
    QCOMPARE(nam.requestCount, 1);
}

UT_REGISTER_TEST(TerrainTileManagerTest, TestLabel::Unit, TestLabel::Terrain)
