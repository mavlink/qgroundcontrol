/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "StructureScanComplexItemTest.h"
#include "QGCApplication.h"
#include "PlanMasterController.h"

StructureScanComplexItemTest::StructureScanComplexItemTest(void)
{
    _polyPoints << QGeoCoordinate(47.633550640000003, -122.08982199) << QGeoCoordinate(47.634129020000003, -122.08887249) <<
                   QGeoCoordinate(47.633619320000001, -122.08811074) << QGeoCoordinate(47.633189139999999, -122.08900124);
}

void StructureScanComplexItemTest::init(void)
{
    UnitTest::init();

    _rgSignals[dirtyChangedIndex] = SIGNAL(dirtyChanged(bool));

    _masterController = new PlanMasterController(this);
    _structureScanItem = new StructureScanComplexItem(_masterController, false /* flyView */, QString() /* kmlFile */);
    _structureScanItem->setDirty(false);

    _multiSpy = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpy);
    QCOMPARE(_multiSpy->init(_structureScanItem, _rgSignals, _cSignals), true);
}

void StructureScanComplexItemTest::cleanup(void)
{
    delete _masterController;
    delete _multiSpy;

    _masterController   = nullptr;
    _structureScanItem  = nullptr;  // Deleted when _masterController is deleted
    _multiSpy           = nullptr;

    UnitTest::cleanup();
}

void StructureScanComplexItemTest::_testDirty(void)
{
    QVERIFY(!_structureScanItem->dirty());
    _structureScanItem->setDirty(false);
    QVERIFY(!_structureScanItem->dirty());
    QVERIFY(_multiSpy->checkNoSignals());

    _structureScanItem->setDirty(true);
    QVERIFY(_structureScanItem->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
    _multiSpy->clearAllSignals();

    _structureScanItem->setDirty(false);
    QVERIFY(!_structureScanItem->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(!_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
    _multiSpy->clearAllSignals();

    // These facts should set dirty when changed
    QList<Fact*> rgFacts;
    rgFacts << _structureScanItem->entranceAlt() << _structureScanItem->layers();
    for(Fact* fact: rgFacts) {
        qDebug() << fact->name();
        QVERIFY(!_structureScanItem->dirty());
        if (fact->typeIsBool()) {
            fact->setRawValue(!fact->rawValue().toBool());
        } else {
            fact->setRawValue(fact->rawValue().toDouble() + 1);
        }
        QVERIFY(_multiSpy->checkSignalByMask(dirtyChangedMask));
        QVERIFY(_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
        _structureScanItem->setDirty(false);
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();
}

void StructureScanComplexItemTest::_initItem(void)
{
    QGCMapPolygon* mapPolygon = _structureScanItem->structurePolygon();

    for (int i=0; i<_polyPoints.count(); i++) {
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

    for (int i=0; i<_polyPoints.count(); i++) {
        QGeoCoordinate& expectedVertex = _polyPoints[i];
        QGeoCoordinate actualVertex = mapPolygon->vertexCoordinate(i);
        QCOMPARE(expectedVertex, actualVertex);
    }

    QVERIFY(_structureScanItem->cameraCalc()->isManualCamera());
    QCOMPARE(item->layers()->cookedValue().toInt(), 2);
}

void StructureScanComplexItemTest::_testSaveLoad(void)
{
    _initItem();

    QJsonArray  items;
    _structureScanItem->save(items);

    QString errorString;
    StructureScanComplexItem* newItem = new StructureScanComplexItem(_masterController, false /* flyView */, QString() /* kmlFile */);
    QVERIFY(newItem->load(items[0].toObject(), 10, errorString));
    QVERIFY(errorString.isEmpty());
    _validateItem(newItem);
    newItem->deleteLater();
}

void StructureScanComplexItemTest::_testItemCount(void)
{
    QList<MissionItem*> items;

    _initItem();
    _structureScanItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _structureScanItem->lastSequenceNumber());

}
