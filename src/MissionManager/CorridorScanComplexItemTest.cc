/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CorridorScanComplexItemTest.h"
#include "QGCApplication.h"

CorridorScanComplexItemTest::CorridorScanComplexItemTest(void)
    : _offlineVehicle(nullptr)
{
    _linePoints << QGeoCoordinate(47.633550640000003, -122.08982199)
                << QGeoCoordinate(47.634129020000003, -122.08887249)
                << QGeoCoordinate(47.633619320000001, -122.08811074);
}

void CorridorScanComplexItemTest::init(void)
{
    UnitTest::init();

    _offlineVehicle = new Vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, qgcApp()->toolbox()->firmwarePluginManager(), this);
    _corridorItem = new CorridorScanComplexItem(_offlineVehicle, false /* flyView */, QString() /* kmlFile */, this /* parent */);

    // vehicleSpeed need for terrain calcs
    MissionController::MissionFlightStatus_t missionFlightStatus;
    missionFlightStatus.vehicleSpeed = 5;
    _corridorItem->setMissionFlightStatus(missionFlightStatus);

    _setPolyline();
    _corridorItem->setDirty(false);

    _rgCorridorPolygonSignals[corridorPolygonPathChangedIndex] = SIGNAL(pathChanged());

    _multiSpyCorridorPolygon = new MultiSignalSpy();
    QCOMPARE(_multiSpyCorridorPolygon->init(_corridorItem->surveyAreaPolygon(), _rgCorridorPolygonSignals, _cCorridorPolygonSignals), true);
}

void CorridorScanComplexItemTest::cleanup(void)
{
    delete _corridorItem;
    delete _offlineVehicle;
}

void CorridorScanComplexItemTest::_testDirty(void)
{
    Fact* fact = _corridorItem->corridorWidth();
    fact->setRawValue(fact->rawValue().toDouble() + 1);
    QVERIFY(_corridorItem->dirty());
    _corridorItem->setDirty(false);

    changeFactValue(_corridorItem->cameraCalc()->distanceToSurface());
    QVERIFY(_corridorItem->dirty());
    _corridorItem->setDirty(false);

    QGeoCoordinate coord = _corridorItem->corridorPolyline()->vertexCoordinate(0);
    coord.setLatitude(coord.latitude() + 1);
    _corridorItem->corridorPolyline()->adjustVertex(1, coord);
    QVERIFY(_corridorItem->dirty());
    _corridorItem->setDirty(false);
}

void CorridorScanComplexItemTest::_testCameraTrigger(void)
{
#if 0
    QCOMPARE(_corridorItem->property("cameraTrigger").toBool(), true);

    // Set up a grid

    for (int i=0; i<3; i++) {
        _mapPolyline->appendVertex(_linePoints[i]);
    }

    _corridorItem->setDirty(false);
    _multiSpy->clearAllSignals();

    int lastSeq = _corridorItem->lastSequenceNumber();
    QVERIFY(lastSeq > 0);

    // Turning off camera triggering should remove two camera trigger mission items, this should trigger:
    //      lastSequenceNumberChanged
    //      dirtyChanged

    _corridorItem->setProperty("cameraTrigger", false);
    QVERIFY(_multiSpy->checkOnlySignalByMask(lastSequenceNumberChangedMask | dirtyChangedMask | cameraTriggerChangedMask));
    QCOMPARE(_multiSpy->pullIntFromSignalIndex(lastSequenceNumberChangedIndex), lastSeq - 2);

    _corridorItem->setDirty(false);
    _multiSpy->clearAllSignals();

    // Turn on camera triggering and make sure things go back to previous count

    _corridorItem->setProperty("cameraTrigger", true);
    QVERIFY(_multiSpy->checkOnlySignalByMask(lastSequenceNumberChangedMask | dirtyChangedMask | cameraTriggerChangedMask));
    QCOMPARE(_multiSpy->pullIntFromSignalIndex(lastSequenceNumberChangedIndex), lastSeq);
#endif
}

void CorridorScanComplexItemTest::_setPolyline(void)
{
    for (int i=0; i<_linePoints.count(); i++) {
        QGeoCoordinate& vertex = _linePoints[i];
        _corridorItem->corridorPolyline()->appendVertex(vertex);
    }
}

#if 0
void CorridorScanComplexItemTest::_testEntryLocation(void)
{
    _setPolygon();

    for (double gridAngle=-360.0; gridAngle<=360.0; gridAngle++) {
        _corridorItem->gridAngle()->setRawValue(gridAngle);

        QList<QGeoCoordinate> rgSeenEntryCoords;
        QList<int> rgEntryLocation;
        rgEntryLocation << SurveyComplexItem::EntryLocationTopLeft
                        << SurveyComplexItem::EntryLocationTopRight
                        << SurveyComplexItem::EntryLocationBottomLeft
                        << SurveyComplexItem::EntryLocationBottomRight;

        // Validate that each entry location is unique
        for (int i=0; i<rgEntryLocation.count(); i++) {
            int entryLocation = rgEntryLocation[i];

            _corridorItem->gridEntryLocation()->setRawValue(entryLocation);
            QVERIFY(!rgSeenEntryCoords.contains(_corridorItem->coordinate()));
            rgSeenEntryCoords << _corridorItem->coordinate();
        }
        rgSeenEntryCoords.clear();
    }
}
#endif

void CorridorScanComplexItemTest::_waitForReadyForSave(void)
{
    int loops = 0;
    while (loops++ < 8) {
        if (_corridorItem->readyForSaveState() == CorridorScanComplexItem::ReadyForSave) {
            return;
        }
        QTest::qWait(500);
    }
    QVERIFY(false);
}

void CorridorScanComplexItemTest::_testItemCount(void)
{
    QList<MissionItem*> items;

    _corridorItem->turnAroundDistance()->setRawValue(0);
    _corridorItem->cameraTriggerInTurnAround()->setRawValue(true);
    _corridorItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _corridorItem->lastSequenceNumber());
    items.clear();

    _corridorItem->turnAroundDistance()->setRawValue(0);
    _corridorItem->cameraTriggerInTurnAround()->setRawValue(false);
    _corridorItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _corridorItem->lastSequenceNumber());
    items.clear();

    _corridorItem->turnAroundDistance()->setRawValue(20);
    _corridorItem->cameraTriggerInTurnAround()->setRawValue(true);
    _corridorItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _corridorItem->lastSequenceNumber());
    items.clear();

    _corridorItem->turnAroundDistance()->setRawValue(20);
    _corridorItem->cameraTriggerInTurnAround()->setRawValue(false);
    _corridorItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _corridorItem->lastSequenceNumber());
    items.clear();

#if 0
    // Terrain queries seem to take random amount of time so these don't work 100%
    _corridorItem->setFollowTerrain(true);

    _corridorItem->turnAroundDistance()->setRawValue(0);
    _corridorItem->cameraTriggerInTurnAround()->setRawValue(true);
    _waitForReadyForSave();
    _corridorItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _corridorItem->lastSequenceNumber());
    items.clear();

    _corridorItem->turnAroundDistance()->setRawValue(0);
    _corridorItem->cameraTriggerInTurnAround()->setRawValue(false);
    _waitForReadyForSave();
    _corridorItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _corridorItem->lastSequenceNumber());
    items.clear();

    _corridorItem->turnAroundDistance()->setRawValue(20);
    _corridorItem->cameraTriggerInTurnAround()->setRawValue(true);
    _waitForReadyForSave();
    _corridorItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _corridorItem->lastSequenceNumber());
    items.clear();

    _corridorItem->turnAroundDistance()->setRawValue(20);
    _corridorItem->cameraTriggerInTurnAround()->setRawValue(false);
    _waitForReadyForSave();
    _corridorItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _corridorItem->lastSequenceNumber());
    items.clear();
#endif
}

void CorridorScanComplexItemTest::_testPathChanges(void)
{
     QGeoCoordinate vertex = _corridorItem->corridorPolyline()->vertexCoordinate(1);
     vertex.setLatitude(vertex.latitude() + 0.01);
     _corridorItem->corridorPolyline()->adjustVertex(1, vertex);

     QVERIFY(_multiSpyCorridorPolygon->checkSignalsByMask(corridorPolygonPathChangedMask));
}
