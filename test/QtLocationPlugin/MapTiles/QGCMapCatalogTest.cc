#include "QGCMapCatalogTest.h"

#include <QtTest/QSignalSpy>

#include "ElevationMapProvider.h"
#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "FlightMapSettings.h"
#include "QGCMapCatalog.h"
#include "SettingsManager.h"
#include "UrlFactory.h"

void QGCMapCatalogTest::_testIsSatelliteMap_data()
{
    QTest::addColumn<QString>("mapType");
    QTest::addColumn<bool>("satellite");

    // Satellite or aerial pictures whose names say neither "Satellite" nor "Hybrid"
    QTest::newRow("MapQuest Sat") << QStringLiteral("MapQuest Sat") << true;
    QTest::newRow("LINZ Basemap") << QStringLiteral("LINZ Basemap") << true;
    QTest::newRow("Japan-GSI Seamless") << QStringLiteral("Japan-GSI Seamless") << true;

    QTest::newRow("hybrid map") << QStringLiteral("Bing Hybrid") << true;
    QTest::newRow("street map") << QStringLiteral("Bing Road") << false;
    QTest::newRow("elevation provider") << QString::fromLatin1(CopernicusElevationProvider::kProviderKey) << false;
    QTest::newRow("not a map") << QStringLiteral("No Such Map Satellite") << false;
}

void QGCMapCatalogTest::_testIsSatelliteMap()
{
    QFETCH(QString, mapType);
    QFETCH(bool, satellite);

    QCOMPARE(QGCMapCatalog::isSatelliteMap(mapType), satellite);
}

void QGCMapCatalogTest::_testIsImageryMap_data()
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

void QGCMapCatalogTest::_testIsImageryMap()
{
    QFETCH(QString, mapType);
    QFETCH(bool, imagery);

    QCOMPARE(QGCMapCatalog::isImageryMap(mapType), imagery);
}

// The settings select a map by provider and type: the providers and their types must lead to every map, and to
// nothing else. The provider of OpenStreetMap, "Street", is also a word of other providers' map names, and the map
// of OpenAIP has no type.
void QGCMapCatalogTest::_testProviderAndTypeSelectEveryMap()
{
    QCOMPARE(QGCMapCatalog::mapTypeList(QStringLiteral("Street")), QStringList{QStringLiteral("Map")});

    QStringList selectableMaps;
    for (const QString& provider : QGCMapCatalog::mapProviderList()) {
        const QStringList types = QGCMapCatalog::mapTypeList(provider);
        if (types.isEmpty()) {
            selectableMaps.append(QGCMapCatalog::mapName(provider, QStringLiteral("type of another provider")));
        }
        for (const QString& type : types) {
            selectableMaps.append(QGCMapCatalog::mapName(provider, type));
        }
    }

    QStringList maps = QGCMapCatalog::mapList();
    for (const QString& elevationProvider : UrlFactory::getElevationProviderTypes()) {
        (void) maps.removeAll(elevationProvider);
    }
    selectableMaps.sort();
    maps.sort();
    QCOMPARE(selectableMaps, maps);
}

void QGCMapCatalogTest::_testChosenMap_data()
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

void QGCMapCatalogTest::_testChosenMap()
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

    const QGCMapCatalog catalog;
    QCOMPARE(providerFact->rawValue().toString(), provider);
    QCOMPARE(typeFact->rawValue().toString(), type);
    QCOMPARE(catalog.chosenMap(), map);

    // The signal tells of a change of the map, and of nothing else
    QSignalSpy chosenMapSpy(&catalog, &QGCMapCatalog::chosenMapChanged);
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

UT_REGISTER_TEST(QGCMapCatalogTest, TestLabel::Unit)
