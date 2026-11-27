#include "OfflineMapEditorUITest.h"

#include <algorithm>

#include <QtLocation/private/qdeclarativegeomap_p.h>
#include <QtLocation/private/qgeomap_p.h>
#include <QtLocation/private/qgeomaptype_p.h>
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>
#include <QtLocation/private/qgeotilefetcher_p.h>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "CachedTileSet.h"
#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "FlightMapSettings.h"
#include "MapCatalog.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "OfflineMapsSettings.h"
#include "QGCFormat.h"
#include "SettingsManager.h"
#include "TileCache.h"
#include "UnitTestTileGenerator.h"

// "Clear Cache" in the view of the default set, once it is confirmed
void OfflineMapEditorUITest::_testClearCacheDeletesCachedTiles()
{
    constexpr int kTileCount = 5;
    const SharedMapProvider provider =
        MapProviderRegistry::getMapProviderFromProviderType(UnitTestTileGenerator::mapProviderType());
    QVERIFY(provider);
    for (int x = 0; x < kTileCount; x++) {
        TileCache::instance()->cacheTile(provider->mapId(), provider->getTileHash(x, 1, 12), QByteArray(100, 'T'),
                                         QStringLiteral("png"));
    }
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());

    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewSettings")));
    QVERIFY(clickButtonScrolled(QStringLiteral("settingsButton_Maps"), QStringLiteral("settings_buttonList")));
    QQuickItem* const setRow =
        findVisibleItemScrolled(QStringLiteral("offlineMaps_tileSet"), QStringLiteral("settingsPageFlickable"));
    QVERIFY2(setRow, "No tile set on the Maps page");
    const auto* const defaultSet = qobject_cast<CachedTileSet*>(setRow->property("tileSet").value<QObject*>());
    QVERIFY(defaultSet);
    QVERIFY(defaultSet->defaultSet());
    QTRY_COMPARE_GE_WITH_TIMEOUT(defaultSet->savedTileCount(), static_cast<quint32>(kTileCount),
                                 TestTimeout::mediumMs());
    QVERIFY(QMetaObject::invokeMethod(setRow, "clicked"));

    QVERIFY(clickButton(QStringLiteral("offlineMapEditor_clearCache")));
    QVERIFY(waitForDialog(QStringLiteral("Confirm Clear")));
    QVERIFY(acceptDialog());
    // No test of this class makes an offline set: the cache has no tile left
    QTRY_COMPARE_WITH_TIMEOUT(defaultSet->savedTileCount(), 0U, TestTimeout::mediumMs());
    QQuickItem* const tileCount = findVisibleItem(_rootItem, QStringLiteral("offlineMapEditor_defaultSetTileCount"));
    QVERIFY2(tileCount, "The editor closed");
    QTRY_COMPARE_WITH_TIMEOUT(tileCount->property("text").toString(), QGC::numberToString(0), TestTimeout::mediumMs());

    stopUI();
}

UT_REGISTER_TEST(OfflineMapEditorUITest, TestLabel::Integration)

void OfflineMapEditorUITest::_testZoomPreviewsShowMapOfEditor()
{
    // The sliders of the editor set these: a later run starts with the zoom levels that this one found
    TestFixtures::SettingsFixture settings;
    OfflineMapsSettings* const offlineMapsSettings = SettingsManager::instance()->offlineMapsSettings();
    settings.setFactValue(offlineMapsSettings->minZoomLevelDownload(),
                          offlineMapsSettings->minZoomLevelDownload()->rawValue());
    settings.setFactValue(offlineMapsSettings->maxZoomLevelDownload(),
                          offlineMapsSettings->maxZoomLevelDownload()->rawValue());

    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }

    // Settings > Maps > Add New Set opens the editor over the window
    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewSettings")));
    QVERIFY(clickButtonScrolled(QStringLiteral("settingsButton_Maps"), QStringLiteral("settings_buttonList")));
    QQuickItem* const addNewSet =
        findVisibleItemScrolled(QStringLiteral("offlineMaps_addNewSet"), QStringLiteral("settingsPageFlickable"));
    QVERIFY2(addNewSet, "No Add New Set row on the Maps page");
    // The row has the button
    QVERIFY(QMetaObject::invokeMethod(addNewSet, "clicked"));

    auto* const editor =
        qobject_cast<QDeclarativeGeoMap*>(findVisibleItem(_rootItem, QStringLiteral("offlineMapEditor")));
    auto* const minPreview = qobject_cast<QDeclarativeGeoMap*>(
        findVisibleItem(_rootItem, QStringLiteral("offlineMapEditor_minZoomPreview")));
    auto* const maxPreview = qobject_cast<QDeclarativeGeoMap*>(
        findVisibleItem(_rootItem, QStringLiteral("offlineMapEditor_maxZoomPreview")));
    QVERIFY2(editor, "The editor did not open");
    QVERIFY2(minPreview && maxPreview, "The editor shows no previews of its zoom levels");
    QTRY_VERIFY_WITH_TIMEOUT(editor->map() && minPreview->map() && maxPreview->map(), TestTimeout::mediumMs());

    // The maps of QGC share one plugin: the previews show tiles from the engine, and from the cache, of the map of
    // the editor
    auto* const engine = qobject_cast<QGeoTiledMappingManagerEngine*>(editor->map()->parent());
    QVERIFY(engine);
    QCOMPARE(minPreview->map()->parent(), engine);
    QCOMPARE(maxPreview->map()->parent(), engine);

    // The previews show the map that the set is of
    const QString streetMap = QStringLiteral("Street Map");
    QCOMPARE_NE(editor->activeMapType().name(), streetMap);
    QVERIFY(editor->setProperty("mapType", streetMap));
    QTRY_COMPARE_WITH_TIMEOUT(editor->activeMapType().name(), streetMap, TestTimeout::mediumMs());
    QCOMPARE(minPreview->activeMapType().name(), streetMap);
    QCOMPARE(maxPreview->activeMapType().name(), streetMap);

    // Each preview is at the zoom level of its slider, and fetches the tiles of that level
    QObject* const addNewSetView = editor->property("_addNewSetViewObject").value<QObject*>();
    QVERIFY(addNewSetView);
    QObject* const sliderMinZoom = addNewSetView->property("sliderMinZoom").value<QObject*>();
    QObject* const sliderMaxZoom = addNewSetView->property("sliderMaxZoom").value<QObject*>();
    QVERIFY(sliderMinZoom && sliderMaxZoom);

    // The sliders stop at the deepest zoom level of which the provider of the map has tiles
    const int deepestZoom = MapCatalog::maxZoom(streetMap);
    QCOMPARE_LT(deepestZoom, 20);
    QCOMPARE(sliderMaxZoom->property("to").toInt(), deepestZoom);
    QVERIFY(sliderMaxZoom->setProperty("value", 20));
    QCOMPARE(sliderMaxZoom->property("value").toInt(), deepestZoom);
    constexpr int kMinZoom = 5;
    constexpr int kMaxZoom = 9;
    QCOMPARE_NE(sliderMinZoom->property("value").toInt(), kMinZoom);
    QCOMPARE_NE(sliderMaxZoom->property("value").toInt(), kMaxZoom);
    QCOMPARE_NE(qRound(editor->zoomLevel()), kMinZoom);
    QCOMPARE_NE(qRound(editor->zoomLevel()), kMaxZoom);
    const QSignalSpy tileSpy(engine->tileFetcher(), &QGeoTileFetcher::tileFinished);
    QVERIFY(sliderMinZoom->setProperty("value", kMinZoom));
    QVERIFY(sliderMaxZoom->setProperty("value", kMaxZoom));
    QCOMPARE(minPreview->zoomLevel(), qreal(kMinZoom));
    QCOMPARE(maxPreview->zoomLevel(), qreal(kMaxZoom));
    const auto fetchedZoom = [&tileSpy](int zoom) {
        return std::any_of(tileSpy.cbegin(), tileSpy.cend(), [zoom](const QList<QVariant>& arguments) {
            return arguments.at(0).value<QGeoTileSpec>().zoom() == zoom;
        });
    };
    QTRY_VERIFY2_WITH_TIMEOUT(fetchedZoom(kMinZoom), "The preview of the lowest zoom level fetched no tile",
                              TestTimeout::longMs());
    QTRY_VERIFY2_WITH_TIMEOUT(fetchedZoom(kMaxZoom), "The preview of the highest zoom level fetched no tile",
                              TestTimeout::longMs());

    stopUI();
}

// A new set downloads its tiles. The map of the tiles that the user imported has none to download: when it is the map
// that the user looks at, the editor opens with the first map of its list, which does not offer that one.
void OfflineMapEditorUITest::_testNewSetIsOfMapWithServer()
{
    TestFixtures::SettingsFixture settings;
    FlightMapSettings* const flightMapSettings = SettingsManager::instance()->flightMapSettings();
    settings.setFactValue(flightMapSettings->mapProvider(), QStringLiteral("MBTiles"));
    settings.setFactValue(flightMapSettings->mapType(), QStringLiteral("Imported"));
    const QString importedMap = QStringLiteral("MBTiles Imported");
    QCOMPARE(MapCatalog::instance()->chosenMap(), importedMap);

    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewSettings")));
    QVERIFY(clickButtonScrolled(QStringLiteral("settingsButton_Maps"), QStringLiteral("settings_buttonList")));
    QQuickItem* const addNewSet =
        findVisibleItemScrolled(QStringLiteral("offlineMaps_addNewSet"), QStringLiteral("settingsPageFlickable"));
    QVERIFY2(addNewSet, "No Add New Set row on the Maps page");
    QVERIFY(QMetaObject::invokeMethod(addNewSet, "clicked"));

    const QQuickItem* const editor = findVisibleItem(_rootItem, QStringLiteral("offlineMapEditor"));
    const QQuickItem* const mapCombo = findVisibleItem(_rootItem, QStringLiteral("offlineMapEditor_mapCombo"));
    QVERIFY2(editor, "The editor did not open");
    QVERIFY2(mapCombo, "The editor has no list of maps");
    const QStringList maps = MapCatalog::downloadableMapList();
    QVERIFY(!maps.contains(importedMap));
    QCOMPARE(mapCombo->property("model").toStringList(), maps);
    QCOMPARE(editor->property("mapType").toString(), maps.constFirst());
    QCOMPARE(mapCombo->property("currentText").toString(), maps.constFirst());

    stopUI();
}

// The editor writes the count and the size of the tiles of a set as the locale does, with its digit grouping
void OfflineMapEditorUITest::_testInfoShowsTilesOfDefaultSet()
{
    // Enough tiles for a count with a digit group. The default set has every tile of the cache.
    constexpr int kTileCount = 1200;
    const SharedMapProvider provider =
        MapProviderRegistry::getMapProviderFromProviderType(UnitTestTileGenerator::mapProviderType());
    QVERIFY(provider);
    for (int x = 0; x < kTileCount; x++) {
        TileCache::instance()->cacheTile(provider->mapId(), provider->getTileHash(x, 0, 12), QByteArray(100, 'T'),
                                         QStringLiteral("png"));
    }
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());

    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }

    // Settings > Maps lists the default set first, and a click on a set opens the editor with its info
    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewSettings")));
    QVERIFY(clickButtonScrolled(QStringLiteral("settingsButton_Maps"), QStringLiteral("settings_buttonList")));
    QQuickItem* const setRow =
        findVisibleItemScrolled(QStringLiteral("offlineMaps_tileSet"), QStringLiteral("settingsPageFlickable"));
    QVERIFY2(setRow, "No tile set on the Maps page");
    const auto* const defaultSet = qobject_cast<CachedTileSet*>(setRow->property("tileSet").value<QObject*>());
    QVERIFY(defaultSet);
    QVERIFY(defaultSet->defaultSet());
    QTRY_COMPARE_GE_WITH_TIMEOUT(defaultSet->savedTileCount(), static_cast<quint32>(kTileCount),
                                 TestTimeout::mediumMs());
    QVERIFY(QMetaObject::invokeMethod(setRow, "clicked"));

    QQuickItem* const tileCount = findVisibleItem(_rootItem, QStringLiteral("offlineMapEditor_defaultSetTileCount"));
    QQuickItem* const size = findVisibleItem(_rootItem, QStringLiteral("offlineMapEditor_defaultSetSize"));
    QVERIFY2(tileCount && size, "The editor does not tell of the tiles of the default set");
    QCOMPARE(tileCount->property("text").toString(), QGC::numberToString(defaultSet->savedTileCount()));
    QCOMPARE(size->property("text").toString(), QGC::bigSizeToString(defaultSet->savedTileSize()));

    stopUI();
}
