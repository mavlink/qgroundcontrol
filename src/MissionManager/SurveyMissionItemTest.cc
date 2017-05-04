/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SurveyMissionItemTest.h"
#include "QGCApplication.h"

SurveyMissionItemTest::SurveyMissionItemTest(void)
    : _offlineVehicle(NULL)
{
    _polyPoints << QGeoCoordinate(47.633550640000003, -122.08982199) << QGeoCoordinate(47.634129020000003, -122.08887249) <<
                   QGeoCoordinate(47.633619320000001, -122.08811074) << QGeoCoordinate(47.633189139999999, -122.08900124);
}

void SurveyMissionItemTest::init(void)
{
    UnitTest::init();

    _rgSurveySignals[gridPointsChangedIndex] =              SIGNAL(gridPointsChanged());
    _rgSurveySignals[cameraShotsChangedIndex] =             SIGNAL(cameraShotsChanged(int));
    _rgSurveySignals[coveredAreaChangedIndex] =             SIGNAL(coveredAreaChanged(double));
    _rgSurveySignals[cameraValueChangedIndex] =             SIGNAL(cameraValueChanged());
    _rgSurveySignals[gridTypeChangedIndex] =                SIGNAL(gridTypeChanged(QString));
    _rgSurveySignals[timeBetweenShotsChangedIndex] =        SIGNAL(timeBetweenShotsChanged());
    _rgSurveySignals[cameraOrientationFixedChangedIndex] =  SIGNAL(cameraOrientationFixedChanged(bool));
    _rgSurveySignals[refly90DegreesChangedIndex] =          SIGNAL(refly90DegreesChanged(bool));
    _rgSurveySignals[dirtyChangedIndex] =                   SIGNAL(dirtyChanged(bool));

    _offlineVehicle = new Vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, qgcApp()->toolbox()->firmwarePluginManager(), this);
    _surveyItem = new SurveyMissionItem(_offlineVehicle, this);
    _mapPolygon = _surveyItem->mapPolygon();

    // It's important to check that the right signals are emitted at the right time since that drives ui change.
    // It's also important to check that things are not being over-signalled when they should not be, since that can lead
    // to incorrect ui or perf impact of uneeded signals propogating ui change.

    _multiSpy = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpy);
    QCOMPARE(_multiSpy->init(_surveyItem, _rgSurveySignals, _cSurveySignals), true);
}

void SurveyMissionItemTest::cleanup(void)
{
    delete _surveyItem;
    delete _offlineVehicle;
    delete _multiSpy;
}

void SurveyMissionItemTest::_testDirty(void)
{
    QVERIFY(!_surveyItem->dirty());
    _surveyItem->setDirty(false);
    QVERIFY(!_surveyItem->dirty());
    QVERIFY(_multiSpy->checkNoSignals());

    _surveyItem->setDirty(true);
    QVERIFY(_surveyItem->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
    _multiSpy->clearAllSignals();

    _surveyItem->setDirty(false);
    QVERIFY(!_surveyItem->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(!_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
    _multiSpy->clearAllSignals();

    // These facts should set dirty when changed
    QList<Fact*> rgFacts;
    rgFacts << _surveyItem->gridAltitude() << _surveyItem->gridAngle() << _surveyItem->gridSpacing() << _surveyItem->turnaroundDist() << _surveyItem->cameraTriggerDistance() <<
               _surveyItem->gridAltitudeRelative() << _surveyItem->cameraTriggerInTurnaround() << _surveyItem->hoverAndCapture();
    foreach(Fact* fact, rgFacts) {
        qDebug() << fact->name();
        QVERIFY(!_surveyItem->dirty());
        if (fact->typeIsBool()) {
            fact->setRawValue(!fact->rawValue().toBool());
        } else {
            fact->setRawValue(fact->rawValue().toDouble() + 1);
        }
        QVERIFY(_multiSpy->checkSignalByMask(dirtyChangedMask));
        QVERIFY(_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
        _surveyItem->setDirty(false);
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();

    // These facts should not change dirty bit
    rgFacts << _surveyItem->groundResolution() << _surveyItem->frontalOverlap() << _surveyItem->sideOverlap() << _surveyItem->cameraSensorWidth() << _surveyItem->cameraSensorHeight() <<
               _surveyItem->cameraResolutionWidth() << _surveyItem->cameraResolutionHeight() << _surveyItem->cameraFocalLength() << _surveyItem->cameraOrientationLandscape() <<
                _surveyItem->fixedValueIsAltitude() << _surveyItem->camera() << _surveyItem->manualGrid();
    foreach(Fact* fact, rgFacts) {
        qDebug() << fact->name();
        QVERIFY(!_surveyItem->dirty());
        if (fact->typeIsBool()) {
            fact->setRawValue(!fact->rawValue().toBool());
        } else {
            fact->setRawValue(fact->rawValue().toDouble() + 1);
        }
        QVERIFY(_multiSpy->checkNoSignalByMask(dirtyChangedMask));
        QVERIFY(!_surveyItem->dirty());
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();
}

void SurveyMissionItemTest::_testCameraValueChanged(void)
{
    // These facts should trigger cameraValueChanged when changed
    QList<Fact*> rgFacts;
    rgFacts << _surveyItem->groundResolution() << _surveyItem->frontalOverlap() << _surveyItem->sideOverlap() << _surveyItem->cameraSensorWidth() << _surveyItem->cameraSensorHeight() <<
               _surveyItem->cameraResolutionWidth() << _surveyItem->cameraResolutionHeight() << _surveyItem->cameraFocalLength() << _surveyItem->cameraOrientationLandscape();
    foreach(Fact* fact, rgFacts) {
        qDebug() << fact->name();
        if (fact->typeIsBool()) {
            fact->setRawValue(!fact->rawValue().toBool());
        } else {
            fact->setRawValue(fact->rawValue().toDouble() + 1);
        }
        QVERIFY(_multiSpy->checkSignalByMask(cameraValueChangedMask));
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();

    // These facts should not trigger cameraValueChanged
    rgFacts << _surveyItem->gridAltitude() << _surveyItem->gridAngle() << _surveyItem->gridSpacing() << _surveyItem->turnaroundDist() << _surveyItem->cameraTriggerDistance() <<
               _surveyItem->gridAltitudeRelative() << _surveyItem->cameraTriggerInTurnaround() << _surveyItem->hoverAndCapture() <<
               _surveyItem->fixedValueIsAltitude() << _surveyItem->camera() << _surveyItem->manualGrid();
    foreach(Fact* fact, rgFacts) {
        qDebug() << fact->name();
        if (fact->typeIsBool()) {
            fact->setRawValue(!fact->rawValue().toBool());
        } else {
            fact->setRawValue(fact->rawValue().toDouble() + 1);
        }
        QVERIFY(_multiSpy->checkNoSignalByMask(cameraValueChangedMask));
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();
}

#if 0
void SurveyMissionItemTest::_testAddPolygonCoordinate(void)
{
    QCOMPARE(_mapPolygon->count(), 0);

    // First call to addPolygonCoordinate should trigger:
    //      polygonPathChanged
    //      dirtyChanged

    _mapPolygon->appendVertex(_polyPoints[0]);
    QVERIFY(_multiSpy->checkOnlySignalByMask(polygonPathChangedMask | dirtyChangedMask));

    // Validate object data
    QVariantList polyList = _mapPolygon->path();
    QCOMPARE(polyList.count(), 1);
    QCOMPARE(polyList[0].value<QGeoCoordinate>(), _polyPoints[0]);

    // Reset
    _surveyItem->setDirty(false);
    _multiSpy->clearAllSignals();

    // Second call to addPolygonCoordinate should only trigger:
    //      polygonPathChanged
    //      dirtyChanged

    _mapPolygon->appendVertex(_polyPoints[1]);
    QVERIFY(_multiSpy->checkOnlySignalByMask(polygonPathChangedMask | dirtyChangedMask));

    polyList = _mapPolygon->path();
    QCOMPARE(polyList.count(), 2);
    for (int i=0; i<polyList.count(); i++) {
        QCOMPARE(polyList[i].value<QGeoCoordinate>(), _polyPoints[i]);
    }

    _surveyItem->setDirty(false);
    _multiSpy->clearAllSignals();

    // Third call to addPolygonCoordinate should trigger:
    //      polygonPathChanged
    //      dirtyChanged
    // Grid is generated for the first time on closing of polygon which triggers:
    //      coordinateChanged - grid generates new entry coordinate
    //      exitCoordinateChanged - grid generates new exit coordinate
    //      specifiesCoordinateChanged - once grid entry/exit shows up specifiesCoordinate gets set to true
    // Grid generation triggers the following signals
    //      lastSequenceNumberChanged -  number of internal mission items changes
    //      gridPointsChanged - grid points show up for the first time

    _mapPolygon->appendVertex(_polyPoints[2]);
    QVERIFY(_multiSpy->checkOnlySignalByMask(polygonPathChangedMask | lastSequenceNumberChangedMask | gridPointsChangedMask | coordinateChangedMask |
                                             exitCoordinateChangedMask | specifiesCoordinateChangedMask | dirtyChangedMask));
    int seqNum = _multiSpy->pullIntFromSignalIndex(lastSequenceNumberChangedIndex);
    QVERIFY(seqNum > 0);

    polyList = _mapPolygon->path();
    QCOMPARE(polyList.count(), 3);
    for (int i=0; i<polyList.count(); i++) {
        QCOMPARE(polyList[i].value<QGeoCoordinate>(), _polyPoints[i]);
    }

    // Test that number of waypoints is doubled when using turnaround waypoints
    _surveyItem->setTurnaroundDist(60.0);
    QVariantList gridPoints = _surveyItem->gridPoints();
    _surveyItem->setTurnaroundDist(0.0);
    QVariantList gridPointsNoT = _surveyItem->gridPoints();
    QCOMPARE(gridPoints.count(), 2 * gridPointsNoT.count());

}

void SurveyMissionItemTest::_testClearPolygon(void)
{
    for (int i=0; i<3; i++) {
        _mapPolygon->appendVertex(_polyPoints[i]);
    }
    _surveyItem->setDirty(false);
    _multiSpy->clearAllSignals();

    // Call to clearPolygon should trigger:
    //      polygonPathChangedMask
    //      dirtyChanged
    //      lastSequenceNumberChangedMask
    //      gridPointsChangedMask
    //      dirtyChangedMask
    //      specifiesCoordinateChangedMask

    _mapPolygon->clear();
    QVERIFY(_multiSpy->checkOnlySignalByMask(polygonPathChangedMask | lastSequenceNumberChangedMask | gridPointsChangedMask | dirtyChangedMask |
                                             specifiesCoordinateChangedMask));
    QVERIFY(!_multiSpy->pullBoolFromSignalIndex(specifiesCoordinateChangedIndex));
    QCOMPARE(_multiSpy->pullIntFromSignalIndex(lastSequenceNumberChangedIndex), 0);

    QCOMPARE(_mapPolygon->path().count(), 0);
    QCOMPARE(_surveyItem->gridPoints().count(), 0);

    _surveyItem->setDirty(false);
    _multiSpy->clearAllSignals();
}

void SurveyMissionItemTest::_testCameraTrigger(void)
{
    QCOMPARE(_surveyItem->property("cameraTrigger").toBool(), true);

    // Set up a grid

    for (int i=0; i<3; i++) {
        _mapPolygon->appendVertex(_polyPoints[i]);
    }

    _surveyItem->setDirty(false);
    _multiSpy->clearAllSignals();

    int lastSeq = _surveyItem->lastSequenceNumber();
    QVERIFY(lastSeq > 0);

    // Turning off camera triggering should remove two camera trigger mission items, this should trigger:
    //      lastSequenceNumberChanged
    //      dirtyChanged

    _surveyItem->setProperty("cameraTrigger", false);
    QVERIFY(_multiSpy->checkOnlySignalByMask(lastSequenceNumberChangedMask | dirtyChangedMask | cameraTriggerChangedMask));
    QCOMPARE(_multiSpy->pullIntFromSignalIndex(lastSequenceNumberChangedIndex), lastSeq - 2);

    _surveyItem->setDirty(false);
    _multiSpy->clearAllSignals();

    // Turn on camera triggering and make sure things go back to previous count

    _surveyItem->setProperty("cameraTrigger", true);
    QVERIFY(_multiSpy->checkOnlySignalByMask(lastSequenceNumberChangedMask | dirtyChangedMask | cameraTriggerChangedMask));
    QCOMPARE(_multiSpy->pullIntFromSignalIndex(lastSequenceNumberChangedIndex), lastSeq);
}
#endif
