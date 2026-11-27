#include "MapCatalogTest.h"

#include <QtTest/QSignalSpy>

#include "ElevationMapProvider.h"
#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "FlightMapSettings.h"
#include "MapCatalog.h"
#include "MapProviderRegistry.h"
#include "SettingsManager.h"

void MapCatalogTest::_testIsSatelliteMap_data()
{
    QTest::addColumn<QString>("mapType");
    QTest::addColumn<bool>("satellite");

    // Satellite or aerial pictures whose names say neither "Satellite" nor "Hybrid"
    QTest::newRow("LINZ Basemap") << QStringLiteral("LINZ Basemap") << true;
    QTest::newRow("Japan-GSI Seamless") << QStringLiteral("Japan-GSI Seamless") << true;

    QTest::newRow("hybrid map") << QStringLiteral("Bing Hybrid") << true;
    QTest::newRow("street map") << QStringLiteral("Bing Road") << false;
    QTest::newRow("elevation provider") << QString::fromLatin1(CopernicusElevationProvider::kProviderKey) << false;
    QTest::newRow("not a map") << QStringLiteral("No Such Map Satellite") << false;
}

void MapCatalogTest::_testIsSatelliteMap()
{
    QFETCH(QString, mapType);
    QFETCH(bool, satellite);

    QCOMPARE(MapCatalog::isSatelliteMap(mapType), satellite);
}

void MapCatalogTest::_testIsDarkMap_data()
{
    QTest::addColumn<QString>("mapType");
    QTest::addColumn<bool>("dark");

    QTest::newRow("street map in the colors of the night") << QStringLiteral("Mapbox Dark") << true;
    QTest::newRow("satellite map") << QStringLiteral("Bing Satellite") << true;
    QTest::newRow("hybrid map") << QStringLiteral("Bing Hybrid") << true;
    QTest::newRow("street map") << QStringLiteral("Mapbox Light") << false;
    QTest::newRow("not a map") << QStringLiteral("No Such Map Dark") << false;
}

void MapCatalogTest::_testIsDarkMap()
{
    QFETCH(QString, mapType);
    QFETCH(bool, dark);

    QCOMPARE(MapCatalog::isDarkMap(mapType), dark);
}

void MapCatalogTest::_testIsImageryMap_data()
{
    QTest::addColumn<QString>("mapType");
    QTest::addColumn<bool>("imagery");

    QTest::newRow("map") << QStringLiteral("Bing Hybrid") << true;
    QTest::newRow("map without a type") << QStringLiteral("OpenAIP") << true;
    QTest::newRow("elevation provider") << QString::fromLatin1(CopernicusElevationProvider::kProviderKey) << false;
    QTest::newRow("elevation provider that the user cannot select")
        << QString::fromLatin1(TerrariumElevationProvider::kProviderKey) << false;
    // A set of an earlier version can name a map that its provider has no more
    QTest::newRow("no map of a provider that has maps") << QStringLiteral("Bing Removed") << false;
    QTest::newRow("no name") << QString() << false;
}

void MapCatalogTest::_testIsImageryMap()
{
    QFETCH(QString, mapType);
    QFETCH(bool, imagery);

    QCOMPARE(MapCatalog::isImageryMap(mapType), imagery);
}

void MapCatalogTest::_testMaxZoom_data()
{
    QTest::addColumn<QString>("mapType");
    QTest::addColumn<int>("maxZoom");

    QTest::newRow("map whose deepest zoom is known") << QStringLiteral("VWorld Satellite Map") << 19;
    // "Add New Set" offers the elevation providers too, with a zoom slider that ends here
    QTest::newRow("elevation provider with zoom levels")
        << QString::fromLatin1(TerrariumElevationProvider::kProviderKey) << TerrariumElevationProvider::kMaxZoom;
    QTest::newRow("not a map") << QStringLiteral("No Such Map") << QGC_MAX_MAP_ZOOM;
}

void MapCatalogTest::_testMaxZoom()
{
    QFETCH(QString, mapType);
    QFETCH(int, maxZoom);

    QCOMPARE(MapCatalog::maxZoom(mapType), maxZoom);
}

// The settings select a map by provider and type: the providers and their types must lead to every map, and to
// nothing else. The provider of OpenStreetMap, "Street", is also a word of other providers' map names, and the map
// of OpenAIP has no type.
void MapCatalogTest::_testProviderAndTypeSelectEveryMap()
{
    QCOMPARE(MapCatalog::mapTypeList(QStringLiteral("Street")), QStringList{QStringLiteral("Map")});

    QStringList selectableMaps;
    for (const QString& provider : MapCatalog::mapProviderList()) {
        const QStringList types = MapCatalog::mapTypeList(provider);
        if (types.isEmpty()) {
            selectableMaps.append(MapCatalog::mapName(provider, QStringLiteral("type of another provider")));
        }
        for (const QString& type : types) {
            selectableMaps.append(MapCatalog::mapName(provider, type));
        }
    }

    QStringList maps = MapCatalog::mapList();
    for (const QString& elevationProvider : MapProviderRegistry::elevationProviderTypes()) {
        (void) maps.removeAll(elevationProvider);
    }
    selectableMaps.sort();
    maps.sort();
    QCOMPARE(selectableMaps, maps);
}

// The map of the tiles that the user imported is one to look at, and none to make an offline set of: no server has
// tiles of it to download
void MapCatalogTest::_testMapWithoutServerIsNotDownloadable()
{
    const QString importedMap = QStringLiteral("MBTiles Imported");
    QStringList maps = MapCatalog::mapList();
    QVERIFY(MapCatalog::isImageryMap(importedMap));
    QCOMPARE(maps.removeAll(importedMap), 1);
    QCOMPARE(MapCatalog::downloadableMapList(), maps);
}

void MapCatalogTest::_testChosenMap_data()
{
    // The settings that the catalog is created with
    QTest::addColumn<QString>("savedProvider");
    QTest::addColumn<QString>("savedType");
    // The settings and the chosen map once it is created
    QTest::addColumn<QString>("provider");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("map");
    // What the settings page sets then, the provider first, and the chosen map after each. Empty: it is not set.
    QTest::addColumn<QString>("newProvider");
    QTest::addColumn<QString>("mapOfNewProvider");
    QTest::addColumn<QString>("newType");
    QTest::addColumn<QString>("mapOfNewType");

    const QString bing = QStringLiteral("Bing");
    const QString hybrid = QStringLiteral("Hybrid");
    const QString bingHybrid = QStringLiteral("Bing Hybrid");
    const QString notSet;

    // The provider and the type name no map in between: the map chosen before stays
    QTest::newRow("provider, then type") << bing << hybrid << bing << hybrid << bingHybrid
                                         << QStringLiteral("CustomURL") << bingHybrid << QStringLiteral("Custom")
                                         << QStringLiteral("CustomURL Custom");
    // The map of OpenAIP has no type: its provider selects it, whatever type is left of the provider before
    QTest::newRow("provider without types")
        << bing << hybrid << bing << hybrid << bingHybrid << QStringLiteral("OpenAIP") << QStringLiteral("OpenAIP")
        << notSet << notSet;

    // Saved settings that name no map are repaired. Earlier versions saved this pair for OpenStreetMap: it gets the
    // type of that map.
    QTest::newRow("saved type that its provider lacks")
        << QStringLiteral("Street") << QStringLiteral("Street Map") << QStringLiteral("Street") << QStringLiteral("Map")
        << QStringLiteral("Street Map") << notSet << notSet << notSet << notSet;
    // A map that is offered no more: the settings get the first type that its provider still offers
    QTest::newRow("saved map that is offered no more")
        << QStringLiteral("Statkart") << QStringLiteral("Basemap") << QStringLiteral("Statkart")
        << QStringLiteral("Topo") << QStringLiteral("Statkart Topo") << notSet << notSet << notSet << notSet;
    // No type makes a map of this provider: the settings get the default map
    QTest::newRow("saved provider that no map has")
        << QStringLiteral("No Such Provider") << QStringLiteral("Custom") << bing << hybrid << bingHybrid << notSet
        << notSet << notSet << notSet;

    // An elevation provider is in the map list, and is no map that a map engine can show
    QTest::newRow("saved provider that is an elevation provider")
        << QString::fromLatin1(CopernicusElevationProvider::kProviderKey) << hybrid << bing << hybrid << bingHybrid
        << notSet << notSet << notSet << notSet;
    QTest::newRow("elevation provider set as the provider")
        << bing << hybrid << bing << hybrid << bingHybrid
        << QString::fromLatin1(TerrariumElevationProvider::kProviderKey) << bingHybrid << notSet << notSet;
}

void MapCatalogTest::_testChosenMap()
{
    QFETCH(QString, savedProvider);
    QFETCH(QString, savedType);
    QFETCH(QString, provider);
    QFETCH(QString, type);
    QFETCH(QString, map);
    QFETCH(QString, newProvider);
    QFETCH(QString, mapOfNewProvider);
    QFETCH(QString, newType);
    QFETCH(QString, mapOfNewType);

    Fact* const providerFact = SettingsManager::instance()->flightMapSettings()->mapProvider();
    Fact* const typeFact = SettingsManager::instance()->flightMapSettings()->mapType();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(providerFact, savedProvider);
    saved.setFactValue(typeFact, savedType);

    const MapCatalog catalog;
    QCOMPARE(providerFact->rawValue().toString(), provider);
    QCOMPARE(typeFact->rawValue().toString(), type);
    QCOMPARE(catalog.chosenMap(), map);

    // The signal tells of a change of the map, and of nothing else
    QSignalSpy chosenMapSpy(&catalog, &MapCatalog::chosenMapChanged);
    if (!newProvider.isEmpty()) {
        providerFact->setRawValue(newProvider);
        QCOMPARE(catalog.chosenMap(), mapOfNewProvider);
        QCOMPARE(chosenMapSpy.count(), (mapOfNewProvider == map) ? 0 : 1);
    }
    if (!newType.isEmpty()) {
        chosenMapSpy.clear();
        typeFact->setRawValue(newType);
        QCOMPARE(catalog.chosenMap(), mapOfNewType);
        QCOMPARE(chosenMapSpy.count(), (mapOfNewType == mapOfNewProvider) ? 0 : 1);
    }
}

UT_REGISTER_TEST(MapCatalogTest, TestLabel::Unit)
