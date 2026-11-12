#include "OfflineMapEditorUITest.h"

#include <utility>

#include <QtCore/QPointer>
#include <QtCore/QRectF>
#include <QtCore/QScopeGuard>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoRectangle>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlExpression>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "ElevationMapProvider.h"
#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "FlightMapSettings.h"
#include "GeoMapCamera.h"
#include "OfflineMapsSettings.h"
#include "QGCCachedTileSet.h"
#include "QGCMapEngine.h"
#include "QGCMapEngineManager.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGroundControlQmlGlobal.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "TileReplyNetworkMock.h"
#include "UnitTestTileGenerator.h"
#include "UnitsSettings.h"

UT_REGISTER_TEST(OfflineMapEditorUITest, TestLabel::Integration)

namespace {

/// The sets of the map engine manager that the settings page lists the sets of
QList<QGCCachedTileSet*> offlineMapEditorSets()
{
    QList<QGCCachedTileSet*> sets;
    for (QObject* const object : std::as_const(*QGCMapEngineManager::instance()->tileSets()->objectList())) {
        if (QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(object)) {
            sets.append(set);
        }
    }
    return sets;
}

QGCCachedTileSet* offlineMapEditorSet(const QString& name)
{
    for (QGCCachedTileSet* const set : offlineMapEditorSets()) {
        if (set->name() == name) {
            return set;
        }
    }
    return nullptr;
}

/// While it lives, the sets that the settings page lists download from a network stand-in that holds its replies:
/// a set asks the network for its tiles without asking the cache, so the tile generator of the tests cannot answer
/// for it. In the end the set \a setName is deleted with its tiles, and no set is left downloading.
class OfflineMapEditorDownloads
{
public:
    explicit OfflineMapEditorDownloads(const QString& setName)
        : _setName(setName)
    {
        networkManager.body = tileReplyPngBody();
        networkManager.holdReplies = true;
        // A new set is listed before it asks for its tiles
        QGCMapEngineManager* const manager = QGCMapEngineManager::instance();
        _listedConnection =
            QObject::connect(manager, &QGCMapEngineManager::tileSetsChanged, manager, [this] { _useStandIn(); });
        _useStandIn();
    }

    ~OfflineMapEditorDownloads()
    {
        // A set that the cache worker is still saving is listed, and given the stand-in, once it is saved
        (void) UnitTestTileGenerator::waitForCacheTasks();
        (void) QObject::disconnect(_listedConnection);
        for (QGCCachedTileSet* const set : offlineMapEditorSets()) {
            // No set is left with a request that it would make without the stand-in
            if (set->downloading()) {
                set->cancelDownloadTask();
            }
            set->setNetworkManager(nullptr);
            if (set->name() == _setName) {
                QGCMapEngineManager::instance()->deleteTileSet(set);
            }
        }
        (void) QTest::qWaitFor([this] { return !offlineMapEditorSet(_setName); }, TestTimeout::mediumMs());
    }

    OfflineMapEditorDownloads(const OfflineMapEditorDownloads&) = delete;
    OfflineMapEditorDownloads& operator=(const OfflineMapEditorDownloads&) = delete;

    TileReplyMockNam networkManager;

private:
    void _useStandIn()
    {
        for (QGCCachedTileSet* const set : offlineMapEditorSets()) {
            set->setNetworkManager(&networkManager);
        }
    }

    const QString _setName;
    QMetaObject::Connection _listedConnection;
};

}  // namespace

void OfflineMapEditorUITest::initTestCase()
{
    QmlUITestBase::initTestCase();
    // A new set downloads the tiles that the tile cache of the process lacks, and its test counts them
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

void OfflineMapEditorUITest::_addEngineRows()
{
    QTest::addColumn<bool>("geoMapEngine");

    QTest::newRow("QtLocation") << false;
    QTest::newRow("GeoMap") << true;
}

void OfflineMapEditorUITest::_saveSet(const QString& name, const QString& shownType, const QGeoRectangle& area,
                                      quint64& setId)
{
    QGCMapEngine* const engine = getQGCMapEngine();
    engine->ensureInitialized();
    const QStringList maps = QGCMapEngineManager::mapList();
    QGCCachedTileSet* const savedSet = new QGCCachedTileSet(name);
    savedSet->setMapTypeStr(shownType);
    // The tiles of the set need a map of the list
    savedSet->setType(maps.contains(shownType) ? shownType : maps.constFirst());
    savedSet->setTopleftLat(area.topLeft().latitude());
    savedSet->setTopleftLon(area.topLeft().longitude());
    savedSet->setBottomRightLat(area.bottomRight().latitude());
    savedSet->setBottomRightLon(area.bottomRight().longitude());
    savedSet->setMinZoom(3);
    savedSet->setMaxZoom(3);
    savedSet->setTotalTileCount(1);
    // The task owns the set until it is saved
    QGCCreateTileSetTask* const createTask = new QGCCreateTileSetTask(savedSet);
    QSignalSpy savedSpy(createTask, &QGCCreateTileSetTask::tileSetSaved);
    QVERIFY(engine->addTask(createTask));
    QVERIFY_SIGNAL_COUNT_WAIT(savedSpy, 1, TestTimeout::mediumMs());
    setId = savedSet->id();
    delete savedSet;
}

void OfflineMapEditorUITest::_deleteSet(quint64 setId)
{
    if (setId == 0) {
        return;  // the set was not saved
    }
    QGCDeleteTileSetTask* const deleteTask = new QGCDeleteTileSetTask(setId);
    const QSignalSpy deletedSpy(deleteTask, &QGCDeleteTileSetTask::tileSetDeleted);
    if (getQGCMapEngine()->addTask(deleteTask)) {
        (void) QTest::qWaitFor([&deletedSpy] { return !deletedSpy.isEmpty(); }, TestTimeout::mediumMs());
    } else {
        delete deleteTask;
    }
}

void OfflineMapEditorUITest::_openMapsSettings(bool geoMapEngine)
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

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewSettings")));
    QQuickItem* const pageButton = findVisibleItem(_rootItem, QStringLiteral("settingsButton_Maps"));
    QVERIFY2(pageButton, "Maps settings page button not found");
    scrollIntoView(pageButton, QStringLiteral("settings_buttonList"));
    QVERIFY(clickButton(QStringLiteral("settingsButton_Maps")));
}

bool OfflineMapEditorUITest::_clickSetEditButton(const QString& setName)
{
    const QString editButton = QStringLiteral("offlineMaps_editButton_") + setName;
    // The page asks the cache for its sets as it opens. Their rows come with the answer, which waits for the tiles
    // that the map behind the page asked for before.
    return findVisibleItem(_rootItem, editButton, TestTimeout::mediumMs()) &&
           clickButtonScrolled(editButton, QStringLiteral("settingsPageFlickable"));
}

QQuickItem* OfflineMapEditorUITest::_editorMap(bool geoMapEngine)
{
    QQuickItem* const editor = findVisibleItem(_rootItem, QStringLiteral("offlineMapEditor"), TestTimeout::mediumMs());
    if (!editor) {
        QTest::qFail("offline map editor not visible", __FILE__, __LINE__);
        return nullptr;
    }
    const QString mapName =
        geoMapEngine ? QStringLiteral("offlineMapEditor_geoMap") : QStringLiteral("offlineMapEditor_flightMap");
    const QString otherMapName =
        geoMapEngine ? QStringLiteral("offlineMapEditor_flightMap") : QStringLiteral("offlineMapEditor_geoMap");
    QQuickItem* const map = findVisibleItem(editor, mapName, TestTimeout::mediumMs());
    if (!map) {
        QTest::qFail(qPrintable(mapName + QStringLiteral(" not visible")), __FILE__, __LINE__);
        return nullptr;
    }
    if (findItem(editor, otherMapName)) {
        QTest::qFail(qPrintable(otherMapName + QStringLiteral(" was created")), __FILE__, __LINE__);
        return nullptr;
    }
    if (!QTest::qWaitFor([map] { return (map->width() > 300) && (map->height() > 300); }, TestTimeout::mediumMs())) {
        QTest::qFail("the editor's map was not laid out", __FILE__, __LINE__);
        return nullptr;
    }
    return map;
}

bool OfflineMapEditorUITest::_typeIntoField(QQuickItem* field, const QString& text)
{
    if (!field) {
        QTest::qFail("no text field to type into", __FILE__, __LINE__);
        return false;
    }
    if (!_clickItemAt(field, 0.5, 0.5, field->objectName())) {
        return false;
    }
    if (!QTest::qWaitFor([field] { return field->hasActiveFocus(); }, TestTimeout::shortMs())) {
        QTest::qFail("the text field did not take the keyboard focus", __FILE__, __LINE__);
        return false;
    }
    // A field selects its text when it gets the focus, and not when it has the focus already
    (void) QMetaObject::invokeMethod(field, "selectAll");
    for (const QChar& character : text) {
        QTest::keyClick(_window, character.toLatin1());
    }
    QTest::keyClick(_window, Qt::Key_Return);
    return true;
}

bool OfflineMapEditorUITest::_chooseInCombo(const QString& objectName, const QString& text)
{
    QQuickItem* const labelledCombo = findVisibleItem(_rootItem, objectName);
    QQuickItem* const combo = labelledCombo ? labelledCombo->property("comboBox").value<QQuickItem*>() : nullptr;
    if (!combo) {
        QTest::qFail(qPrintable(objectName + QStringLiteral(" not visible")), __FILE__, __LINE__);
        return false;
    }
    int index = -1;
    if (!QMetaObject::invokeMethod(combo, "find", Q_RETURN_ARG(int, index), Q_ARG(QString, text)) || (index < 0)) {
        QTest::qFail(qPrintable(QStringLiteral("%1 does not offer %2").arg(objectName, text)), __FILE__, __LINE__);
        return false;
    }
    // What a click on an item of the list does: the item becomes the current one, and the combo box tells of it
    return combo->setProperty("currentIndex", index) &&
           QMetaObject::invokeMethod(combo, "activated", Q_ARG(int, index));
}

int OfflineMapEditorUITest::_visibleItemCount(const QQuickItem* root, const QString& objectName)
{
    int count = (root->isVisible() && (root->objectName() == objectName)) ? 1 : 0;
    for (const QQuickItem* child : root->childItems()) {
        count += _visibleItemCount(child, objectName);
    }
    return count;
}

void OfflineMapEditorUITest::_newSetAreaFollowsMap_data()
{
    _addEngineRows();
}

void OfflineMapEditorUITest::_newSetAreaFollowsMap()
{
    QFETCH(bool, geoMapEngine);

    _openMapsSettings(geoMapEngine);
    if (QTest::currentTestFailed()) {
        return;
    }
    // Read here: the Fly View map, shown at startup, settles the shared view to what its window can show
    const QGeoCoordinate sharedPosition = QGroundControlQmlGlobal::flightMapPosition();
    const double sharedZoom = QGroundControlQmlGlobal::flightMapZoom();

    QVERIFY(clickButtonScrolled(QStringLiteral("offlineMaps_addButton"), QStringLiteral("settingsPageFlickable")));
    QQuickItem* const map = _editorMap(geoMapEngine);
    if (!map) {
        return;
    }
    QQuickItem* const editor = findVisibleItem(_rootItem, QStringLiteral("offlineMapEditor"));
    QVERIFY(editor);

    // A picture each of the smallest and the largest zoom level of the set
    QTRY_COMPARE_WITH_TIMEOUT(_visibleItemCount(editor, QStringLiteral("offlineMapEditor_zoomPreview")), 2,
                              TestTimeout::shortMs());

    // The area of the set is what the map shows: a closer view leaves fewer tiles to download
    QGCMapEngineManager* const manager = QGCMapEngineManager::instance();
    map->setProperty("center", QVariant::fromValue(QGeoCoordinate(47.3977, 8.5456)));
    map->setProperty("zoomLevel", 10.0);
    QTRY_COMPARE_GT_WITH_TIMEOUT(manager->tileCount(), quint64(0), TestTimeout::mediumMs());
    const quint64 tileCount = manager->tileCount();
    map->setProperty("zoomLevel", 12.0);
    QTRY_COMPARE_LT_WITH_TIMEOUT(manager->tileCount(), tileCount, TestTimeout::mediumMs());

    if (geoMapEngine) {
        auto* const camera = map->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
        QVERIFY2(camera, "GeoMapCamera not found");
        const QPoint mapCenter = map->mapToScene(QPointF(map->width() / 2, map->height() / 2)).toPoint();

        // The view must stay the upright rectangle a set is made of: the drag that orbits the Fly View map does
        // nothing here. The pans before and after it show that drags do reach the map.
        QGeoCoordinate center = camera->center();
        mouseDrag(Qt::LeftButton, mapCenter, QPoint(60, 40));
        QTRY_COMPARE_GT_WITH_TIMEOUT(camera->center().distanceTo(center), 1.0, TestTimeout::mediumMs());
        mouseDrag(Qt::RightButton, mapCenter, QPoint(qRound(map->width() / 4), 0));
        center = camera->center();
        mouseDrag(Qt::LeftButton, mapCenter, QPoint(60, 40));
        QTRY_COMPARE_GT_WITH_TIMEOUT(camera->center().distanceTo(center), 1.0, TestTimeout::mediumMs());
        QCOMPARE(camera->heading(), 0.0);
        QCOMPARE(camera->tilt(), 0.0);
    }

    // Looking around for a set's area leaves the map view of Fly View and Plan View where it was
    QCOMPARE(QGroundControlQmlGlobal::flightMapPosition(), sharedPosition);
    QCOMPARE(QGroundControlQmlGlobal::flightMapZoom(), sharedZoom);

    QVERIFY(clickButton(QStringLiteral("offlineMapEditor_cancelButton")));
    QTRY_VERIFY_WITH_TIMEOUT(!findItem(_rootItem, QStringLiteral("offlineMapEditor")), TestTimeout::shortMs());

    stopUI();
}

void OfflineMapEditorUITest::_newSetEstimateFollowsChosenType_data()
{
    _addEngineRows();
}

void OfflineMapEditorUITest::_newSetEstimateFollowsChosenType()
{
    QFETCH(bool, geoMapEngine);

    _openMapsSettings(geoMapEngine);
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY(clickButtonScrolled(QStringLiteral("offlineMaps_addButton"), QStringLiteral("settingsPageFlickable")));
    QQuickItem* const map = _editorMap(geoMapEngine);
    if (!map) {
        return;
    }
    QQuickItem* const editor = findVisibleItem(_rootItem, QStringLiteral("offlineMapEditor"));
    QVERIFY(editor);
    QQuickItem* const typeCombo = findVisibleItem(editor, QStringLiteral("offlineMapEditor_mapTypeCombo"));
    QVERIFY2(typeCombo, "map type list not visible");

    // The manager keeps the estimate of an earlier editor: wait for one of this editor, for the view set here
    QGCMapEngineManager* const manager = QGCMapEngineManager::instance();
    QSignalSpy estimateSpy(manager, &QGCMapEngineManager::tileCountChanged);
    map->setProperty("center", QVariant::fromValue(QGeoCoordinate(47.3977, 8.5456)));
    map->setProperty("zoomLevel", 10.0);
    QVERIFY_SIGNAL_COUNT_WAIT(estimateSpy, 1, TestTimeout::mediumMs());
    const quint64 shownTypeCount = manager->tileCount();

    // The list offers the elevation providers like every other provider, and neither engine's map can show one
    const QString elevationType = QString::fromLatin1(CopernicusElevationProvider::kProviderKey);
    const int elevationIndex = static_cast<int>(QGCMapEngineManager::mapList().indexOf(elevationType));
    QVERIFY2(elevationIndex >= 0, "elevation provider not in the map type list");
    QVERIFY(QMetaObject::invokeMethod(typeCombo, "activated", Q_ARG(int, elevationIndex)));
    QCOMPARE(editor->property("mapType").toString(), elevationType);
    const quint64 chosenTypeCount = manager->tileCount();

    // What the editor asks the manager for when its map moves: the estimate of its type for its view, which has
    // not changed since the type was chosen
    QVERIFY(QMetaObject::invokeMethod(editor, "handleChanges"));
    QCOMPARE(chosenTypeCount, manager->tileCount());
    QCOMPARE_NE(chosenTypeCount, shownTypeCount);

    QVERIFY(clickButton(QStringLiteral("offlineMapEditor_cancelButton")));
    QTRY_VERIFY_WITH_TIMEOUT(!findItem(_rootItem, QStringLiteral("offlineMapEditor")), TestTimeout::shortMs());

    stopUI();
}

void OfflineMapEditorUITest::_newSetMapCentersOnSpecifiedPosition_data()
{
    _addEngineRows();
}

void OfflineMapEditorUITest::_newSetMapCentersOnSpecifiedPosition()
{
    QFETCH(bool, geoMapEngine);

    // The dialog asks for a position in the coordinate format of the units settings
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->unitsSettings()->coordinateFormat(),
                       UnitsSettings::CoordinateFormatDecimal);

    _openMapsSettings(geoMapEngine);
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY(clickButtonScrolled(QStringLiteral("offlineMaps_addButton"), QStringLiteral("settingsPageFlickable")));
    QQuickItem* const map = _editorMap(geoMapEngine);
    if (!map) {
        return;
    }

    // The button at the top left of the editor's map offers what to center the map on
    QVERIFY(clickButton(QStringLiteral("offlineMapEditor_centerMapButton")));
    const QString specifiedLocationButton = QStringLiteral("centerMap_specifiedLocationButton");
    QVERIFY2(findVisibleItem(_rootItem, specifiedLocationButton, TestTimeout::shortMs()),
             "the button to center the map does not offer a specified location");
    QVERIFY(clickButton(specifiedLocationButton));
    QVERIFY(waitForDialog(QStringLiteral("Specify Position")));

    // A place of its own, away from the map view the other views share
    const QGeoCoordinate position(-35.3632, 149.1652);
    QQuickItem* const latitude =
        findVisibleItem(_rootItem, QStringLiteral("editPosition_latitude"), TestTimeout::shortMs());
    QQuickItem* const longitude = findVisibleItem(_rootItem, QStringLiteral("editPosition_longitude"));
    QVERIFY2(latitude && longitude, "the dialog does not ask for a latitude and a longitude");
    QVERIFY(_typeIntoField(latitude->property("textField").value<QQuickItem*>(),
                           QString::number(position.latitude(), 'f', 7)));
    QVERIFY(_typeIntoField(longitude->property("textField").value<QQuickItem*>(),
                           QString::number(position.longitude(), 'f', 7)));
    QVERIFY(clickButton(QStringLiteral("editPosition_moveButton")));
    QTRY_VERIFY_WITH_TIMEOUT(!dialogVisible(QStringLiteral("Specify Position")), TestTimeout::shortMs());

    // A GeoMap is where its camera looks
    const GeoMapCamera* const camera =
        geoMapEngine ? map->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera")) : nullptr;
    QVERIFY2(camera || !geoMapEngine, "GeoMapCamera not found");
    const auto mapCenter = [map, camera] {
        return camera ? camera->center() : map->property("center").value<QGeoCoordinate>();
    };
    QTRY_COMPARE_LT_WITH_TIMEOUT(mapCenter().distanceTo(position), 1.0, TestTimeout::shortMs());

    QVERIFY(clickButton(QStringLiteral("offlineMapEditor_cancelButton")));
    QTRY_VERIFY_WITH_TIMEOUT(!findItem(_rootItem, QStringLiteral("offlineMapEditor")), TestTimeout::shortMs());

    stopUI();
}

void OfflineMapEditorUITest::_newSetIsListedAndDownloads_data()
{
    _addEngineRows();
}

void OfflineMapEditorUITest::_newSetIsListedAndDownloads()
{
    QFETCH(bool, geoMapEngine);

    // A set of one zoom level and of a map that needs no token, without the sets of elevation tiles that a set of
    // imagery gets beside it
    constexpr int kZoom = 16;
    constexpr quint32 kMostTiles = 256;  // what the map of a window shows at the zoom of the set is far fewer
    constexpr int kRequestsAtOnce = 6;
    const QString chosenMap = QStringLiteral("Bing Hybrid");
    SettingsManager* const settings = SettingsManager::instance();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(settings->flightMapSettings()->mapProvider(), QStringLiteral("Bing"));
    saved.setFactValue(settings->flightMapSettings()->mapType(), QStringLiteral("Hybrid"));
    saved.setFactValue(settings->offlineMapsSettings()->minZoomLevelDownload(), kZoom);
    saved.setFactValue(settings->offlineMapsSettings()->maxZoomLevelDownload(), kZoom);
    QGCMapEngineManager* const manager = QGCMapEngineManager::instance();
    const auto restoreFetchElevation = qScopeGuard([manager, fetchElevation = manager->property("fetchElevation")] {
        (void) manager->setProperty("fetchElevation", fetchElevation);
    });
    QVERIFY(manager->setProperty("fetchElevation", false));

    _openMapsSettings(geoMapEngine);
    if (QTest::currentTestFailed()) {
        return;
    }
    // The page asks the cache for its sets as it opens. A set that arrives while the editor is open gives the new
    // set its default name again: they are all there once the cache worker has answered.
    QVERIFY2(
        findVisibleItem(_rootItem, QStringLiteral("offlineMaps_editButton_Default Tile Set"), TestTimeout::mediumMs()),
        "the page does not list the default set");
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());

    const QString setName = QStringLiteral("Download test %1 %2")
                                .arg(QCoreApplication::applicationPid())
                                .arg(QString::fromLatin1(QTest::currentDataTag()));
    OfflineMapEditorDownloads downloads(setName);

    QVERIFY(clickButtonScrolled(QStringLiteral("offlineMaps_addButton"), QStringLiteral("settingsPageFlickable")));
    QQuickItem* const map = _editorMap(geoMapEngine);
    if (!map) {
        return;
    }

    // A place of its own, shown at the zoom of the set: the set is of the few tiles that the map shows
    const QGeoCoordinate center(-35.3632, 149.1652);
    map->setProperty("center", QVariant::fromValue(center));
    map->setProperty("zoomLevel", double(kZoom));
    QTRY_COMPARE_LT_WITH_TIMEOUT(
        (mapPoint(map, center) - QPointF(map->width() / 2, map->height() / 2)).manhattanLength(), 3.0,
        TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT((manager->tileCount() > 0) && (manager->tileCount() <= kMostTiles),
                             TestTimeout::mediumMs());

    QQuickItem* const nameField = findVisibleItem(_rootItem, QStringLiteral("offlineMapEditor_newSetNameField"));
    QVERIFY2(nameField, "name field of the new set not visible");
    nameField->setProperty("text", setName);
    QVERIFY(verifyEnabled(QStringLiteral("offlineMapEditor_downloadButton"), true, QStringLiteral("named new set")));
    QVERIFY(clickButton(QStringLiteral("offlineMapEditor_downloadButton")));
    QTRY_VERIFY_WITH_TIMEOUT(!findItem(_rootItem, QStringLiteral("offlineMapEditor")), TestTimeout::shortMs());

    // The manager has the set, of the chosen map and of the tiles of the area that the map showed
    QTRY_VERIFY_WITH_TIMEOUT(offlineMapEditorSet(setName), TestTimeout::mediumMs());
    const QPointer<QGCCachedTileSet> set = offlineMapEditorSet(setName);
    QCOMPARE(set->type(), chosenMap);
    QCOMPARE(set->minZoom(), kZoom);
    QCOMPARE(set->maxZoom(), kZoom);
    const QGeoRectangle area(QGeoCoordinate(set->topleftLat(), set->topleftLon()),
                             QGeoCoordinate(set->bottomRightLat(), set->bottomRightLon()));
    QVERIFY2(area.contains(center), "the set is not of the area that the map showed");
    const quint32 tileCount = set->totalTileCount();
    QCOMPARE(quint64(tileCount), UrlFactory::getTileCount(kZoom, set->topleftLon(), set->topleftLat(),
                                                          set->bottomRightLon(), set->bottomRightLat(), chosenMap)
                                     .tileCount);
    QCOMPARE_GT(tileCount, 0U);
    QCOMPARE_LE(tileCount, kMostTiles);

    // The page lists the set
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("offlineMaps_editButton_") + setName, TestTimeout::mediumMs()),
             "the list does not show the new set");

    // The set downloads: it asks for as many of its tiles as a set asks for at a time
    QVERIFY(set->downloading());
    QTRY_COMPARE_WITH_TIMEOUT(downloads.networkManager.requestCount, qMin(int(tileCount), kRequestsAtOnce),
                              TestTimeout::mediumMs());

    // With every answer it asks for another tile, until it has them all
    downloads.networkManager.holdReplies = false;
    const QList<QPointer<TileReplyCannedReply>> heldReplies = downloads.networkManager.replies;
    for (const QPointer<TileReplyCannedReply>& reply : heldReplies) {
        if (reply) {
            reply->finishNow();
        }
    }
    QTRY_VERIFY_WITH_TIMEOUT(set && !set->downloading(), TestTimeout::mediumMs());
    QCOMPARE(set->savedTileCount(), tileCount);
    QCOMPARE(set->errorCount(), 0U);
    QVERIFY(set->complete());
    QCOMPARE(downloads.networkManager.requestCount, int(tileCount));

    stopUI();
}

void OfflineMapEditorUITest::_defaultSetInfoOpensAndCloses_data()
{
    _addEngineRows();
}

void OfflineMapEditorUITest::_defaultSetInfoOpensAndCloses()
{
    QFETCH(bool, geoMapEngine);

    _openMapsSettings(geoMapEngine);
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY(_clickSetEditButton(QStringLiteral("Default Tile Set")));
    if (!_editorMap(geoMapEngine)) {
        return;
    }
    // The info of a set has no zoom previews: they belong to a new set
    QCOMPARE(_visibleItemCount(_rootItem, QStringLiteral("offlineMapEditor_zoomPreview")), 0);

    // The editor covers the settings page: a click on its map must not press the button under it, which would
    // open another editor
    QQuickItem* const coveredButton = findItem(_rootItem, QStringLiteral("offlineMaps_editButton_Default Tile Set"));
    QVERIFY(coveredButton);
    QTest::mouseClick(
        _window, Qt::LeftButton, Qt::NoModifier,
        coveredButton->mapToScene(QPointF(coveredButton->width() / 2, coveredButton->height() / 2)).toPoint());
    QCoreApplication::processEvents();
    QCOMPARE(_visibleItemCount(_rootItem, QStringLiteral("offlineMapEditor")), 1);

    QVERIFY(clickButton(QStringLiteral("offlineMapEditor_closeButton")));
    QTRY_VERIFY_WITH_TIMEOUT(!findItem(_rootItem, QStringLiteral("offlineMapEditor")), TestTimeout::shortMs());

    stopUI();
}

void OfflineMapEditorUITest::_setInfoShowsSetAreaAndMap_data()
{
    QTest::addColumn<bool>("geoMapEngine");
    QTest::addColumn<bool>("typeInMapList");

    QTest::newRow("QtLocation") << false << true;
    QTest::newRow("GeoMap") << true << true;
    // A set of an earlier version, of a map that the list has no more
    QTest::newRow("QtLocation, type no longer a map") << false << false;
    QTest::newRow("GeoMap, type no longer a map") << true << false;
}

void OfflineMapEditorUITest::_setInfoShowsSetAreaAndMap()
{
    QFETCH(bool, geoMapEngine);
    QFETCH(bool, typeInMapList);

    // Another map than the chosen one, which the editor's map shows first
    const QString chosenMap = QGCMapEngineManager::instance()->chosenMap();
    const QStringList maps = QGCMapEngineManager::mapList();
    const QString otherMap = (maps.constFirst() != chosenMap) ? maps.constFirst() : maps.at(1);
    // The type that is no longer a map still has a provider of the list
    const QString setType =
        typeInMapList ? otherMap : otherMap.section(QLatin1Char(' '), 0, 0) + QStringLiteral(" Removed");
    QVERIFY(maps.contains(setType) == typeInMapList);
    // A place of its own, away from the map view the other views share
    const QGeoRectangle area(QGeoCoordinate(-35.35, 149.15), QGeoCoordinate(-35.37, 149.18));
    const QString setName = QStringLiteral("Info test %1").arg(QCoreApplication::applicationPid());
    quint64 setId = 0;
    const auto deleteSavedSet = qScopeGuard([&setId] { _deleteSet(setId); });
    _saveSet(setName, setType, area, setId);
    if (QTest::currentTestFailed()) {
        return;
    }

    _openMapsSettings(geoMapEngine);
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY(_clickSetEditButton(setName));
    QQuickItem* const map = _editorMap(geoMapEngine);
    if (!map) {
        return;
    }

    QTRY_VERIFY2_WITH_TIMEOUT(mapShowsCoordinates(map, {area.topLeft(), area.bottomRight()}),
                              "the map does not show the area of the set", TestTimeout::mediumMs());

    // The map shows the set's map. It keeps its own when the set's is none: a GeoMap would warn of a map type
    // that it does not know, which fails the test.
    QQmlExpression flightMapType(qmlContext(map), map, QStringLiteral("activeMapType.name"));
    const QString shownMap = geoMapEngine ? map->property("mapType").toString() : flightMapType.evaluate().toString();
    QCOMPARE(shownMap, typeInMapList ? setType : chosenMap);

    // The map credits the imagery that it shows, under its scale
    const QString mapCredit = QGCMapEngineManager::mapAttribution(shownMap);
    QVERIFY2(!mapCredit.isEmpty(), "the shown map has no credit");
    QQuickItem* const credit = findVisibleItem(map, QStringLiteral("mapAttribution"), TestTimeout::shortMs());
    QVERIFY2(credit, "the editor's map shows no credit");
    QCOMPARE(credit->property("text").toString(), mapCredit);
    QQuickItem* const scale = findVisibleItem(_rootItem, QStringLiteral("offlineMapEditor_mapScale"));
    QVERIFY2(scale, "map scale not visible");
    const auto sceneRect = [](const QQuickItem* item) {
        return item->mapRectToScene(QRectF(0, 0, item->width(), item->height()));
    };
    // The scale ends where the credit begins: half a pixel is for the rounding of the two positions
    QTRY_COMPARE_LE_WITH_TIMEOUT(sceneRect(scale).bottom(), sceneRect(credit).top() + 0.5, TestTimeout::shortMs());

    QVERIFY(clickButton(QStringLiteral("offlineMapEditor_closeButton")));
    QTRY_VERIFY_WITH_TIMEOUT(!findItem(_rootItem, QStringLiteral("offlineMapEditor")), TestTimeout::shortMs());

    stopUI();
}

void OfflineMapEditorUITest::_mapEngineErrorOpensOneDialog()
{
    _openMapsSettings(/*geoMapEngine*/ false);
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY(clickButtonScrolled(QStringLiteral("offlineMaps_addButton"), QStringLiteral("settingsPageFlickable")));
    if (!_editorMap(/*geoMapEngine*/ false)) {
        return;
    }

    // The settings page, which stays under the editor, reports the errors of the map engine
    QGCMapEngineManager* const manager = QGCMapEngineManager::instance();
    const auto restoreError =
        qScopeGuard([manager, savedError = manager->errorMessage()] { manager->setErrorMessage(savedError); });
    manager->setErrorMessage(QStringLiteral("Error of the test for one dialog"));
    QVERIFY(waitForDialog(QStringLiteral("Error Message")));
    QCOMPARE(_visibleItemCount(_rootItem, QStringLiteral("popupDialog_title")), 1);

    QVERIFY(rejectDialog());
    QTRY_VERIFY_WITH_TIMEOUT(!dialogVisible(QStringLiteral("Error Message")), TestTimeout::shortMs());
    QVERIFY(clickButton(QStringLiteral("offlineMapEditor_cancelButton")));
    QTRY_VERIFY_WITH_TIMEOUT(!findItem(_rootItem, QStringLiteral("offlineMapEditor")), TestTimeout::shortMs());

    stopUI();
}

void OfflineMapEditorUITest::_setCannotBeEditedDuringImport()
{
    _openMapsSettings(/*geoMapEngine*/ false);
    if (QTest::currentTestFailed()) {
        return;
    }

    const QString editButton = QStringLiteral("offlineMaps_editButton_Default Tile Set");
    QVERIFY2(findVisibleItem(_rootItem, editButton, TestTimeout::mediumMs()),
             "Edit button of the default set not visible");

    QGCMapEngineManager* const manager = QGCMapEngineManager::instance();
    const auto restoreImportAction =
        qScopeGuard([manager, savedAction = manager->importAction()] { manager->setImportAction(savedAction); });

    manager->setImportAction(QGCMapEngineManager::ImportAction::ActionImporting);
    QVERIFY(verifyEnabled(editButton, false, QStringLiteral("during an import")));
    manager->setImportAction(QGCMapEngineManager::ImportAction::ActionDone);
    QVERIFY(verifyEnabled(editButton, true, QStringLiteral("after the import")));

    stopUI();
}

void OfflineMapEditorUITest::_renamedSetIsListedByItsNewName()
{
    const QString oldName = QStringLiteral("Rename test %1").arg(QCoreApplication::applicationPid());
    const QString newName = oldName + QStringLiteral(" renamed");
    quint64 setId = 0;
    const auto deleteSavedSet = qScopeGuard([&setId] { _deleteSet(setId); });
    _saveSet(oldName, QGCMapEngineManager::mapList().constFirst(),
             QGeoRectangle(QGeoCoordinate(47.3906, 8.4991), QGeoCoordinate(47.3902, 8.4995)), setId);
    if (QTest::currentTestFailed()) {
        return;
    }

    _openMapsSettings(/*geoMapEngine*/ false);
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY(_clickSetEditButton(oldName));
    QQuickItem* const nameField =
        findVisibleItem(_rootItem, QStringLiteral("offlineMapEditor_setNameField"), TestTimeout::mediumMs());
    QVERIFY2(nameField, "name field of the set not visible");
    QCOMPARE(nameField->property("text").toString(), oldName);
    nameField->setProperty("text", newName);
    QVERIFY(clickButton(QStringLiteral("offlineMapEditor_okButton")));
    QTRY_VERIFY_WITH_TIMEOUT(!findItem(_rootItem, QStringLiteral("offlineMapEditor")), TestTimeout::shortMs());

    QVERIFY2(findVisibleItemScrolled(QStringLiteral("offlineMaps_editButton_") + newName,
                                     QStringLiteral("settingsPageFlickable")),
             "the list does not show the new name");
    QVERIFY(!findItem(_rootItem, QStringLiteral("offlineMaps_editButton_") + oldName));

    stopUI();
}

void OfflineMapEditorUITest::_deletedSetLeavesTheList()
{
    // A set with one of its tiles stored: the editor offers to delete a set that has stored something
    const QString setName = QStringLiteral("Delete test %1").arg(QCoreApplication::applicationPid());
    const QString mapType = QGCMapEngineManager::mapList().constFirst();
    const QGeoRectangle area(QGeoCoordinate(47.3906, 8.4991), QGeoCoordinate(47.3902, 8.4995));
    constexpr int kSetZoom = 3;  // the zoom of the sets that _saveSet() saves
    quint64 setId = 0;
    const auto deleteSavedSet = qScopeGuard([&setId] { _deleteSet(setId); });
    _saveSet(setName, mapType, area, setId);
    if (QTest::currentTestFailed()) {
        return;
    }
    const QString tileHash =
        UrlFactory::getTileHash(mapType, UrlFactory::long2tileX(mapType, area.topLeft().longitude(), kSetZoom),
                                UrlFactory::lat2tileY(mapType, area.topLeft().latitude(), kSetZoom), kSetZoom);
    getQGCMapEngine()->cacheTile(mapType, tileHash, tileReplyPngBody(), QStringLiteral("png"), setId);
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());

    _openMapsSettings(/*geoMapEngine*/ false);
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY(_clickSetEditButton(setName));
    if (!_editorMap(/*geoMapEngine*/ false)) {
        return;
    }
    const QString deleteButton = QStringLiteral("offlineMapEditor_deleteButton");
    QVERIFY(verifyEnabled(deleteButton, true, QStringLiteral("set with a stored tile")));
    QVERIFY(clickButton(deleteButton));
    QVERIFY(waitForDialog(QStringLiteral("Confirm Delete")));
    QVERIFY(acceptDialog());

    // The editor closes, and the set leaves the manager and the list of the page
    QTRY_VERIFY_WITH_TIMEOUT(!findItem(_rootItem, QStringLiteral("offlineMapEditor")), TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(!offlineMapEditorSet(setName), TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(!findItem(_rootItem, QStringLiteral("offlineMaps_editButton_") + setName),
                             TestTimeout::shortMs());

    stopUI();
}

void OfflineMapEditorUITest::_providerAndTypeCombosChooseTheMap_data()
{
    _addEngineRows();
}

void OfflineMapEditorUITest::_providerAndTypeCombosChooseTheMap()
{
    QFETCH(bool, geoMapEngine);

    const QString bing = QStringLiteral("Bing");
    const QString google = QStringLiteral("Google");
    const QString openAip = QStringLiteral("OpenAIP");
    const QStringList bingTypes = QGCMapEngineManager::mapTypeList(bing);
    const QStringList googleTypes = QGCMapEngineManager::mapTypeList(google);
    QVERIFY(bingTypes.contains(QStringLiteral("Hybrid")));
    QCOMPARE_GE(googleTypes.size(), 2);
    QVERIFY(QGCMapEngineManager::mapTypeList(openAip).isEmpty());

    Fact* const providerFact = SettingsManager::instance()->flightMapSettings()->mapProvider();
    Fact* const typeFact = SettingsManager::instance()->flightMapSettings()->mapType();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(providerFact, bing);
    saved.setFactValue(typeFact, QStringLiteral("Hybrid"));

    _openMapsSettings(geoMapEngine);
    if (QTest::currentTestFailed()) {
        return;
    }

    // The Fly View stays loaded under the settings: its map shows the map that is chosen here
    QQuickItem* const flyViewMap =
        findItem(_rootItem, geoMapEngine ? QStringLiteral("flyViewGeoMapAdapter") : QStringLiteral("flyViewMap"));
    QVERIFY2(flyViewMap, "Fly View map not found");
    QQmlExpression shownMapName(qmlContext(flyViewMap), flyViewMap,
                                geoMapEngine ? QStringLiteral("geoMap.mapType") : QStringLiteral("activeMapType.name"));
    const auto shownMap = [&shownMapName] { return shownMapName.evaluate().toString(); };
    const QGCMapEngineManager* const manager = QGCMapEngineManager::instance();
    const QString providerCombo = QStringLiteral("mapSettings_providerCombo");
    const QString typeCombo = QStringLiteral("mapSettings_typeCombo");

    // The combos show the map of the settings
    QVERIFY(verifyProperty(providerCombo, "currentText", bing, QStringLiteral("page opened")));
    QVERIFY(verifyProperty(typeCombo, "currentText", QStringLiteral("Hybrid"), QStringLiteral("page opened")));
    QTRY_COMPARE_WITH_TIMEOUT(shownMap(), QStringLiteral("Bing Hybrid"), TestTimeout::mediumMs());

    // A provider comes with the first of its types
    const QString firstGoogleMap = QGCMapEngineManager::mapName(google, googleTypes.constFirst());
    QVERIFY(_chooseInCombo(providerCombo, google));
    QCOMPARE(providerFact->rawValue().toString(), google);
    QCOMPARE(typeFact->rawValue().toString(), googleTypes.constFirst());
    QCOMPARE(manager->chosenMap(), firstGoogleMap);
    QVERIFY(verifyProperty(typeCombo, "currentText", googleTypes.constFirst(), QStringLiteral("provider chosen")));
    QTRY_COMPARE_WITH_TIMEOUT(shownMap(), firstGoogleMap, TestTimeout::shortMs());

    // Another type of the provider
    const QString lastGoogleMap = QGCMapEngineManager::mapName(google, googleTypes.constLast());
    QVERIFY(_chooseInCombo(typeCombo, googleTypes.constLast()));
    QCOMPARE(providerFact->rawValue().toString(), google);
    QCOMPARE(typeFact->rawValue().toString(), googleTypes.constLast());
    QCOMPARE(manager->chosenMap(), lastGoogleMap);
    QTRY_COMPARE_WITH_TIMEOUT(shownMap(), lastGoogleMap, TestTimeout::shortMs());

    // The map of OpenAIP has no type: there is none to choose
    QVERIFY(_chooseInCombo(providerCombo, openAip));
    QCOMPARE(manager->chosenMap(), openAip);
    QVERIFY(verifyVisibility(typeCombo, false, QStringLiteral("provider whose map has no type")));
    QTRY_COMPARE_WITH_TIMEOUT(shownMap(), openAip, TestTimeout::shortMs());

    // A provider with types brings the choice back
    const QString firstBingMap = QGCMapEngineManager::mapName(bing, bingTypes.constFirst());
    QVERIFY(_chooseInCombo(providerCombo, bing));
    QCOMPARE(manager->chosenMap(), firstBingMap);
    QVERIFY(verifyProperty(typeCombo, "currentText", bingTypes.constFirst(), QStringLiteral("provider with types")));
    QTRY_COMPARE_WITH_TIMEOUT(shownMap(), firstBingMap, TestTimeout::shortMs());

    stopUI();
}
