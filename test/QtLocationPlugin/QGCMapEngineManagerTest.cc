#include "QGCMapEngineManagerTest.h"

#include <QtCore/QVariantMap>
#include <QtTest/QSignalSpy>
#include <limits>

#include "QGCCachedTileSet.h"
#include "QGCMapEngineManager.h"
#include "QmlObjectListModel.h"

void QGCMapEngineManagerTest::init()
{
    QGCMapEngineManager* manager = QGCMapEngineManager::instance();
    QVERIFY(manager != nullptr);
    QVERIFY(manager->tileSets() != nullptr);
    _initialTileSetCount = manager->tileSets()->count();
}

void QGCMapEngineManagerTest::cleanup()
{
    // Tests append throwaway sets to the singleton's model; trim them here so a failed
    // assertion mid-test cannot leak state into the next test.
    QmlObjectListModel* model = QGCMapEngineManager::instance()->tileSets();
    while (model->count() > _initialTileSetCount) {
        delete model->removeAt(model->count() - 1);
    }
}

void QGCMapEngineManagerTest::_testInstanceAndModelAvailable()
{
    QGCMapEngineManager* manager = QGCMapEngineManager::instance();
    QVERIFY(manager != nullptr);
    QVERIFY(manager->tileSets() != nullptr);
}

void QGCMapEngineManagerTest::_testProviderListsAndMapTypes()
{
    const QStringList mapList = QGCMapEngineManager::mapList();
    const QStringList providerList = QGCMapEngineManager::mapProviderList();
    const QStringList elevationList = QGCMapEngineManager::elevationProviderList();

    QVERIFY(!mapList.isEmpty());
    QVERIFY(!providerList.isEmpty());
    QVERIFY(!elevationList.isEmpty());

    const QString mapProvider = providerList.first();
    const QStringList mapTypes = QGCMapEngineManager::mapTypeList(mapProvider);
    QVERIFY2(!mapTypes.isEmpty(), qPrintable(QStringLiteral("No map types for provider: %1").arg(mapProvider)));
}

void QGCMapEngineManagerTest::_testUpdateForCurrentViewUpdatesTotals()
{
    QGCMapEngineManager* manager = QGCMapEngineManager::instance();
    QVERIFY(manager != nullptr);

    const QStringList mapList = QGCMapEngineManager::mapList();
    QVERIFY(!mapList.isEmpty());
    const QString mapName = mapList.first();

    QSignalSpy tileCountSpy(manager, &QGCMapEngineManager::tileCountChanged);
    QSignalSpy tileSizeSpy(manager, &QGCMapEngineManager::tileSizeChanged);

    manager->updateForCurrentView(-122.50, 37.80, -122.30, 37.70, 1, 3, mapName);

    QVERIFY(tileCountSpy.count() > 0);
    QVERIFY(tileSizeSpy.count() > 0);
    QVERIFY(manager->tileCount() > 0);
    QVERIFY(manager->tileSize() > 0);
}

void QGCMapEngineManagerTest::_testFindNameAndSelectionHelpers()
{
    QGCMapEngineManager* manager = QGCMapEngineManager::instance();
    QVERIFY(manager != nullptr);

    QmlObjectListModel* model = manager->tileSets();
    QVERIFY(model != nullptr);

    auto* setA = new QGCCachedTileSet(QStringLiteral("__unit_test_set_a__"));
    auto* setB = new QGCCachedTileSet(QStringLiteral("__unit_test_set_b__"));
    setA->setManager(manager);
    setB->setManager(manager);

    model->append(setA);
    model->append(setB);

    QVERIFY(manager->findName(QStringLiteral("__unit_test_set_a__")));
    QVERIFY(manager->findName(QStringLiteral("__unit_test_set_b__")));
    QVERIFY(!manager->findName(QStringLiteral("__unit_test_set_missing__")));

    manager->selectAll();
    QVERIFY(setA->selected());
    QVERIFY(setB->selected());

    manager->selectNone();
    QVERIFY(!setA->selected());
    QVERIFY(!setB->selected());
    QCOMPARE(manager->selectedCount(), 0);
}

void QGCMapEngineManagerTest::_testRenameOptimisticallyUpdatesName()
{
    QGCMapEngineManager* manager = QGCMapEngineManager::instance();
    QVERIFY(manager != nullptr);

    QmlObjectListModel* model = manager->tileSets();
    QVERIFY(model != nullptr);

    auto* set = new QGCCachedTileSet(QStringLiteral("__rename_source__"));
    set->setManager(manager);
    set->setId(std::numeric_limits<quint64>::max());
    model->append(set);

    manager->renameTileSet(set, QStringLiteral("__rename_target__"));
    QCOMPARE(set->name(), QStringLiteral("__rename_target__"));
}

void QGCMapEngineManagerTest::_testTaskErrorLabels()
{
    QGCMapEngineManager* manager = QGCMapEngineManager::instance();
    QVERIFY(manager != nullptr);

    // not on this branch: taskError() labels only a subset of task types; rename/prune/import
    // fall through to "Database Error". Assert against the types that ARE labeled here.
    manager->taskError(QGCMapTask::TaskType::taskCreateTileSet, QStringLiteral("create failed"));
    QVERIFY(manager->errorMessage().contains(QStringLiteral("Create Tile Set")));

    manager->taskError(QGCMapTask::TaskType::taskDeleteTileSet, QStringLiteral("delete failed"));
    QVERIFY(manager->errorMessage().contains(QStringLiteral("Delete Tile Set")));

    manager->taskError(QGCMapTask::TaskType::taskReset, QStringLiteral("reset failed"));
    QVERIFY(manager->errorMessage().contains(QStringLiteral("Reset Tile Sets")));
}

UT_REGISTER_TEST(QGCMapEngineManagerTest, TestLabel::Unit)
