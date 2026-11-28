#include "MapCreditsTest.h"

#include "ElevationMapProvider.h"
#include "MapCredits.h"

void MapCreditsTest::_testMapAttribution_data()
{
    QTest::addColumn<QString>("mapType");
    QTest::addColumn<QString>("attribution");

    // The wording that the terms of these providers prescribe
    QTest::newRow("OpenStreetMap") << QStringLiteral("Street Map") << QStringLiteral("© OpenStreetMap contributors");
    QTest::newRow("Mapbox") << QStringLiteral("Mapbox Streets") << QStringLiteral("© Mapbox © OpenStreetMap");
    QTest::newRow("Mapbox satellite") << QStringLiteral("Mapbox Hybrid")
                                      << QStringLiteral("© Mapbox © OpenStreetMap © Maxar");
    QTest::newRow("Esri") << QStringLiteral("Esri World Satellite") << QStringLiteral("Powered by Esri");
    QTest::newRow("Kartverket") << QStringLiteral("Statkart Topo") << QStringLiteral("© Kartverket");
    QTest::newRow("Norwegian Polar Institute")
        << QStringLiteral("Svalbard Topo") << QStringLiteral("© Norwegian Polar Institute");

    QTest::newRow("GeoMap terrain") << QString::fromLatin1(TerrariumElevationProvider::kProviderKey)
                                    << QStringLiteral("Terrain: Mapzen, USGS, NOAA and other sources");
    QTest::newRow("source that the user sets") << QStringLiteral("CustomURL Custom") << QString();
    QTest::newRow("not a map") << QStringLiteral("No Such Map") << QString();
}

void MapCreditsTest::_testMapAttribution()
{
    QFETCH(QString, mapType);
    QFETCH(QString, attribution);

    QCOMPARE(MapCredits::mapAttribution(mapType), attribution);
}

void MapCreditsTest::_testTerrainAttribution()
{
    QCOMPARE(MapCredits::terrainAttribution(), QStringLiteral("Terrain: Mapzen, USGS, NOAA and other sources"));
}

UT_REGISTER_TEST(MapCreditsTest, TestLabel::Unit)
