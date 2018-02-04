/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CorridorScanComplexItemTest.h"
#include "QGCApplication.h"

CorridorScanComplexItemTest::CorridorScanComplexItemTest(void)
    : _offlineVehicle(NULL)
{
    _linePoints << QGeoCoordinate(47.633550640000003, -122.08982199)
                << QGeoCoordinate(47.634129020000003, -122.08887249)
                << QGeoCoordinate(47.633619320000001, -122.08811074);
}

void CorridorScanComplexItemTest::init(void)
{
    UnitTest::init();

    _offlineVehicle = new Vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, qgcApp()->toolbox()->firmwarePluginManager(), this);
    _corridorItem = new CorridorScanComplexItem(_offlineVehicle, this);
//    _corridorItem->setTurnaroundDist(0);  // Unit test written for no turnaround distance
    _corridorItem->setDirty(false);
    _mapPolyline = _corridorItem->corridorPolyline();

    _rgSignals[complexDistanceChangedIndex] =     SIGNAL(complexDistanceChanged());
    _rgSignals[greatestDistanceToChangedIndex] =  SIGNAL(greatestDistanceToChanged());
    _rgSignals[additionalTimeDelayChangedIndex] = SIGNAL(additionalTimeDelayChanged());
    _rgSignals[transectPointsChangedIndex] =      SIGNAL(transectPointsChanged());
    _rgSignals[cameraShotsChangedIndex] =         SIGNAL(cameraShotsChanged());
    _rgSignals[coveredAreaChangedIndex] =         SIGNAL(coveredAreaChanged());
    _rgSignals[timeBetweenShotsChangedIndex] =    SIGNAL(timeBetweenShotsChanged());
    _rgSignals[dirtyChangedIndex] =               SIGNAL(dirtyChanged(bool));

    _multiSpy = new MultiSignalSpy();
    QCOMPARE(_multiSpy->init(_corridorItem, _rgSignals, _cSignals), true);

    _rgCorridorPolygonSignals[corridorPolygonPathChangedIndex] = SIGNAL(pathChanged());

    _multiSpyCorridorPolygon = new MultiSignalSpy();
    QCOMPARE(_multiSpyCorridorPolygon->init(_corridorItem->surveyAreaPolygon(), _rgCorridorPolygonSignals, _cCorridorPolygonSignals), true);
}

void CorridorScanComplexItemTest::cleanup(void)
{
    delete _corridorItem;
    delete _offlineVehicle;
    delete _multiSpy;
}

void CorridorScanComplexItemTest::_testDirty(void)
{
    QVERIFY(!_corridorItem->dirty());
    _corridorItem->setDirty(false);
    QVERIFY(!_corridorItem->dirty());
    QVERIFY(_multiSpy->checkNoSignals());

    _corridorItem->setDirty(true);
    QVERIFY(_corridorItem->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
    _multiSpy->clearAllSignals();

    _corridorItem->setDirty(false);
    QVERIFY(!_corridorItem->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(!_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
    _multiSpy->clearAllSignals();

    // These facts should set dirty when changed
    QList<Fact*> rgFacts;
#if 0
    rgFacts << _corridorItem->gridAltitude() << _corridorItem->gridAngle() << _corridorItem->gridSpacing() << _corridorItem->turnaroundDist() << _corridorItem->cameraTriggerDistance() <<
               _corridorItem->gridAltitudeRelative() << _corridorItem->cameraTriggerInTurnaround() << _corridorItem->hoverAndCapture();
#endif
    rgFacts << _corridorItem->corridorWidth();
    foreach(Fact* fact, rgFacts) {
        qDebug() << fact->name();
        QVERIFY(!_corridorItem->dirty());
        if (fact->typeIsBool()) {
            fact->setRawValue(!fact->rawValue().toBool());
        } else {
            fact->setRawValue(fact->rawValue().toDouble() + 1);
        }
        QVERIFY(_multiSpy->checkSignalByMask(dirtyChangedMask));
        QVERIFY(_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
        _corridorItem->setDirty(false);
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();

    // These facts should not change dirty bit
#if 0
    rgFacts << _corridorItem->groundResolution() << _corridorItem->frontalOverlap() << _corridorItem->sideOverlap() << _corridorItem->cameraSensorWidth() << _corridorItem->cameraSensorHeight() <<
               _corridorItem->cameraResolutionWidth() << _corridorItem->cameraResolutionHeight() << _corridorItem->cameraFocalLength() << _corridorItem->cameraOrientationLandscape() <<
               _corridorItem->fixedValueIsAltitude() << _corridorItem->camera() << _corridorItem->manualGrid();
#endif
    foreach(Fact* fact, rgFacts) {
        qDebug() << fact->name();
        QVERIFY(!_corridorItem->dirty());
        if (fact->typeIsBool()) {
            fact->setRawValue(!fact->rawValue().toBool());
        } else {
            fact->setRawValue(fact->rawValue().toDouble() + 1);
        }
        QVERIFY(_multiSpy->checkNoSignalByMask(dirtyChangedMask));
        QVERIFY(!_corridorItem->dirty());
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();
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
        _mapPolyline->appendVertex(vertex);
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
        rgEntryLocation << SurveyMissionItem::EntryLocationTopLeft
                        << SurveyMissionItem::EntryLocationTopRight
                        << SurveyMissionItem::EntryLocationBottomLeft
                        << SurveyMissionItem::EntryLocationBottomRight;

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

void CorridorScanComplexItemTest::_testItemCount(void)
{
    QList<MissionItem*> items;

    _setPolyline();

//    _corridorItem->cameraTriggerInTurnaround()->setRawValue(false);
    _corridorItem->appendMissionItems(items, this);
    QCOMPARE(items.count(), _corridorItem->lastSequenceNumber());
    items.clear();
}

void CorridorScanComplexItemTest::_testPathChanges(void)
{
    _setPolyline();
    _corridorItem->setDirty(false);
     _multiSpy->clearAllSignals();
     _multiSpyCorridorPolygon->clearAllSignals();

     QGeoCoordinate vertex = _mapPolyline->vertexCoordinate(1);
     vertex.setLatitude(vertex.latitude() + 0.01);
     _mapPolyline->adjustVertex(1, vertex);

     QVERIFY(_corridorItem->dirty());
     QVERIFY(_multiSpy->checkOnlySignalsByMask(dirtyChangedMask | transectPointsChangedMask | cameraShotsChangedMask | coveredAreaChangedMask | complexDistanceChangedMask | greatestDistanceToChangedMask));
     QVERIFY(_multiSpyCorridorPolygon->checkSignalsByMask(corridorPolygonPathChangedMask));
     _multiSpy->clearAllSignals();
}
