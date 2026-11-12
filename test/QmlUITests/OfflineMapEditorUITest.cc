#include "OfflineMapEditorUITest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtPositioning/QGeoCoordinate>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "ElevationMapProvider.h"
#include "Fact.h"
#include "FlyViewSettings.h"
#include "GeoMapCamera.h"
#include "QGCCachedTileSet.h"
#include "QGCMapEngine.h"
#include "QGCMapEngineManager.h"
#include "QGCMapTasks.h"
#include "QGroundControlQmlGlobal.h"
#include "SettingsManager.h"

UT_REGISTER_TEST(OfflineMapEditorUITest, TestLabel::Integration)

void OfflineMapEditorUITest::_addEngineRows()
{
    QTest::addColumn<bool>("geoMapEngine");

    QTest::newRow("QtLocation") << false;
    QTest::newRow("GeoMap") << true;
}

void OfflineMapEditorUITest::_openMapsSettings(bool geoMapEngine)
{
    Fact* const geoEngineFact = SettingsManager::instance()->flyViewSettings()->useGeoMapEngine();
    _savedEngine = geoEngineFact->rawValue();
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    geoEngineFact->setRawValue(geoMapEngine);

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

void OfflineMapEditorUITest::_mouseDrag(Qt::MouseButton button, const QPoint& from, const QPoint& delta)
{
    QTest::mousePress(_window, button, Qt::NoModifier, from);
    constexpr int kSteps = 10;
    for (int i = 1; i <= kSteps; i++) {
        QTest::mouseMove(_window, from + ((delta * i) / kSteps));
    }
    QTest::mouseRelease(_window, button, Qt::NoModifier, from + delta);
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
    const auto restoreEngine = qScopeGuard(
        [this] { SettingsManager::instance()->flyViewSettings()->useGeoMapEngine()->setRawValue(_savedEngine); });
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
        _mouseDrag(Qt::LeftButton, mapCenter, QPoint(60, 40));
        QTRY_COMPARE_GT_WITH_TIMEOUT(camera->center().distanceTo(center), 1.0, TestTimeout::mediumMs());
        _mouseDrag(Qt::RightButton, mapCenter, QPoint(qRound(map->width() / 4), 0));
        center = camera->center();
        _mouseDrag(Qt::LeftButton, mapCenter, QPoint(60, 40));
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
    const auto restoreEngine = qScopeGuard(
        [this] { SettingsManager::instance()->flyViewSettings()->useGeoMapEngine()->setRawValue(_savedEngine); });
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

void OfflineMapEditorUITest::_defaultSetInfoOpensAndCloses_data()
{
    _addEngineRows();
}

void OfflineMapEditorUITest::_defaultSetInfoOpensAndCloses()
{
    QFETCH(bool, geoMapEngine);

    _openMapsSettings(geoMapEngine);
    const auto restoreEngine = qScopeGuard(
        [this] { SettingsManager::instance()->flyViewSettings()->useGeoMapEngine()->setRawValue(_savedEngine); });
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY(clickButtonScrolled(QStringLiteral("offlineMaps_editButton_Default Tile Set"),
                                QStringLiteral("settingsPageFlickable")));
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

void OfflineMapEditorUITest::_renamedSetIsListedByItsNewName()
{
    // A set that is only in the database, like one of an earlier run: nothing downloads its tiles
    QGCMapEngine* const engine = getQGCMapEngine();
    engine->ensureInitialized();
    const QString mapType = QGCMapEngineManager::mapList().constFirst();
    const QString oldName = QStringLiteral("Rename test %1").arg(QCoreApplication::applicationPid());
    const QString newName = oldName + QStringLiteral(" renamed");
    QGCCachedTileSet* const savedSet = new QGCCachedTileSet(oldName);
    savedSet->setMapTypeStr(mapType);
    savedSet->setType(mapType);
    savedSet->setTopleftLat(47.3906);
    savedSet->setTopleftLon(8.4991);
    savedSet->setBottomRightLat(47.3902);
    savedSet->setBottomRightLon(8.4995);
    savedSet->setMinZoom(3);
    savedSet->setMaxZoom(3);
    savedSet->setTotalTileCount(1);
    // The task owns the set until it is saved
    QGCCreateTileSetTask* const createTask = new QGCCreateTileSetTask(savedSet);
    QSignalSpy savedSpy(createTask, &QGCCreateTileSetTask::tileSetSaved);
    QVERIFY(engine->addTask(createTask));
    QVERIFY_SIGNAL_COUNT_WAIT(savedSpy, 1, TestTimeout::mediumMs());
    const quint64 setId = savedSet->id();
    delete savedSet;
    const auto deleteSavedSet = qScopeGuard([engine, setId] {
        QGCDeleteTileSetTask* const deleteTask = new QGCDeleteTileSetTask(setId);
        const QSignalSpy deletedSpy(deleteTask, &QGCDeleteTileSetTask::tileSetDeleted);
        if (engine->addTask(deleteTask)) {
            (void) QTest::qWaitFor([&deletedSpy] { return !deletedSpy.isEmpty(); }, TestTimeout::mediumMs());
        } else {
            delete deleteTask;
        }
    });

    _openMapsSettings(/*geoMapEngine*/ false);
    const auto restoreEngine = qScopeGuard(
        [this] { SettingsManager::instance()->flyViewSettings()->useGeoMapEngine()->setRawValue(_savedEngine); });
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY(clickButtonScrolled(QStringLiteral("offlineMaps_editButton_") + oldName,
                                QStringLiteral("settingsPageFlickable")));
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
