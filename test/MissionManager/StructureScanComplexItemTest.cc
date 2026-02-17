#include "StructureScanComplexItemTest.h"

#include <QtCore/QJsonArray>

#include "MultiSignalSpy.h"
#include "PlanMasterController.h"
#include "StructureScanComplexItem.h"

StructureScanComplexItemTest::StructureScanComplexItemTest()
{
    _polyPoints << QGeoCoordinate(47.633550640000003, -122.08982199)
                << QGeoCoordinate(47.634129020000003, -122.08887249)
                << QGeoCoordinate(47.633619320000001, -122.08811074)
                << QGeoCoordinate(47.633189139999999, -122.08900124);
}

void StructureScanComplexItemTest::init()
{
    OfflineMissionTest::init();
    _structureScanItem = new StructureScanComplexItem(planController(), false /* flyView */, QString() /* kmlFile */);
    _structureScanItem->setDirty(false);
    _multiSpy = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpy);
    QCOMPARE(_multiSpy->init(_structureScanItem), true);
}

void StructureScanComplexItemTest::cleanup()
{
    delete _multiSpy;
    _structureScanItem = nullptr;  // Deleted when planController is deleted
    _multiSpy = nullptr;
    OfflineMissionTest::cleanup();
}

void StructureScanComplexItemTest::_testDirty()
{
    QVERIFY(!_structureScanItem->dirty());
    _structureScanItem->setDirty(false);
    QVERIFY(!_structureScanItem->dirty());
    QVERIFY(_multiSpy->noneEmitted());
    _structureScanItem->setDirty(true);
    QVERIFY(_structureScanItem->dirty());
    QVERIFY(_multiSpy->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(_multiSpy->argument<bool>("dirtyChanged"));
    _multiSpy->clearAllSignals();
    _structureScanItem->setDirty(false);
    QVERIFY(!_structureScanItem->dirty());
    QVERIFY(_multiSpy->onlyEmittedOnce("dirtyChanged"));
    QVERIFY(!_multiSpy->argument<bool>("dirtyChanged"));
    _multiSpy->clearAllSignals();
    // These facts should set dirty when changed
    QList<Fact*> rgFacts;
    rgFacts << _structureScanItem->entranceAlt() << _structureScanItem->layers();
    for (Fact* fact : rgFacts) {
        qDebug() << fact->name();
        QVERIFY(!_structureScanItem->dirty());
        if (fact->typeIsBool()) {
            fact->setRawValue(!fact->rawValue().toBool());
        } else {
            fact->setRawValue(fact->rawValue().toDouble() + 1);
        }
        QVERIFY(_multiSpy->emittedOnce("dirtyChanged"));
        QVERIFY(_multiSpy->argument<bool>("dirtyChanged"));
        _structureScanItem->setDirty(false);
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();
}

void StructureScanComplexItemTest::_initItem()
{
    QGCMapPolygon* mapPolygon = _structureScanItem->structurePolygon();
    for (int i = 0; i < _polyPoints.count(); i++) {
        QGeoCoordinate& vertex = _polyPoints[i];
        mapPolygon->appendVertex(vertex);
    }
    _structureScanItem->cameraCalc()->setCameraBrand(CameraCalc::canonicalManualCameraName());
    _structureScanItem->layers()->setCookedValue(2);
    _structureScanItem->setDirty(false);
    _validateItem(_structureScanItem);
}

void StructureScanComplexItemTest::_validateItem(StructureScanComplexItem* item)
{
    QGCMapPolygon* mapPolygon = item->structurePolygon();
    for (int i = 0; i < _polyPoints.count(); i++) {
        QGeoCoordinate& expectedVertex = _polyPoints[i];
        QGeoCoordinate actualVertex = mapPolygon->vertexCoordinate(i);
        QCOMPARE(expectedVertex, actualVertex);
    }
    QVERIFY(_structureScanItem->cameraCalc()->isManualCamera());
    QCOMPARE(item->layers()->cookedValue().toInt(), 2);
}

void StructureScanComplexItemTest::_testSaveLoad()
{
    _initItem();
    QJsonArray items;
    _structureScanItem->save(items);
    QString errorString;
    StructureScanComplexItem* newItem =
        new StructureScanComplexItem(planController(), false /* flyView */, QString() /* kmlFile */);
    QVERIFY(newItem->load(items[0].toObject(), 10, errorString));
    QVERIFY(errorString.isEmpty());
    _validateItem(newItem);
    newItem->deleteLater();
}

void StructureScanComplexItemTest::_testItemCount()
{
    QList<MissionItem*> items;
    _initItem();
    _structureScanItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _structureScanItem->lastSequenceNumber());
}

UT_REGISTER_TEST(StructureScanComplexItemTest, TestLabel::Unit, TestLabel::MissionManager)
