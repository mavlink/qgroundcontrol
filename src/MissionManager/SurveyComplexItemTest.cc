/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SurveyComplexItemTest.h"
#include "QGCApplication.h"

SurveyComplexItemTest::SurveyComplexItemTest(void)
    : _offlineVehicle(nullptr)
{
    _polyPoints << QGeoCoordinate(47.633550640000003, -122.08982199) << QGeoCoordinate(47.634129020000003, -122.08887249) <<
                   QGeoCoordinate(47.633619320000001, -122.08811074) << QGeoCoordinate(47.633189139999999, -122.08900124);
}

void SurveyComplexItemTest::init(void)
{
    UnitTest::init();

    _rgSurveySignals[surveyVisualTransectPointsChangedIndex] =    SIGNAL(visualTransectPointsChanged());
    _rgSurveySignals[surveyCameraShotsChangedIndex] =             SIGNAL(cameraShotsChanged());
    _rgSurveySignals[surveyCoveredAreaChangedIndex] =             SIGNAL(coveredAreaChanged());
    _rgSurveySignals[surveyTimeBetweenShotsChangedIndex] =        SIGNAL(timeBetweenShotsChanged());
    _rgSurveySignals[surveyRefly90DegreesChangedIndex] =          SIGNAL(refly90DegreesChanged(bool));
    _rgSurveySignals[surveyDirtyChangedIndex] =                   SIGNAL(dirtyChanged(bool));

    _offlineVehicle = new Vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, qgcApp()->toolbox()->firmwarePluginManager(), this);
    _surveyItem = new SurveyComplexItem(_offlineVehicle, false /* flyView */, QString() /* kmlFile */, this /* parent */);
    _surveyItem->turnAroundDistance()->setRawValue(0);  // Unit test written for no turnaround distance
    _surveyItem->setDirty(false);
    _mapPolygon = _surveyItem->surveyAreaPolygon();

    // It's important to check that the right signals are emitted at the right time since that drives ui change.
    // It's also important to check that things are not being over-signalled when they should not be, since that can lead
    // to incorrect ui or perf impact of uneeded signals propogating ui change.

    _multiSpy = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpy);
    QCOMPARE(_multiSpy->init(_surveyItem, _rgSurveySignals, _cSurveySignals), true);
}

void SurveyComplexItemTest::cleanup(void)
{
    delete _surveyItem;
    delete _offlineVehicle;
    delete _multiSpy;
}

void SurveyComplexItemTest::_testDirty(void)
{
    QVERIFY(!_surveyItem->dirty());
    _surveyItem->setDirty(false);
    QVERIFY(!_surveyItem->dirty());
    QVERIFY(_multiSpy->checkNoSignals());

    _surveyItem->setDirty(true);
    QVERIFY(_surveyItem->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(surveyDirtyChangedMask));
    QVERIFY(_multiSpy->pullBoolFromSignalIndex(surveyDirtyChangedIndex));
    _multiSpy->clearAllSignals();

    _surveyItem->setDirty(false);
    QVERIFY(!_surveyItem->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(surveyDirtyChangedMask));
    _multiSpy->clearAllSignals();

    // These facts should set dirty when changed
    QList<Fact*> rgFacts;
    rgFacts << _surveyItem->gridAngle() << _surveyItem->flyAlternateTransects();
    for(Fact* fact: rgFacts) {
        qDebug() << fact->name();
        QVERIFY(!_surveyItem->dirty());
        if (fact->typeIsBool()) {
            fact->setRawValue(!fact->rawValue().toBool());
        } else {
            fact->setRawValue(fact->rawValue().toDouble() + 1);
        }
        QVERIFY(_multiSpy->checkSignalByMask(surveyDirtyChangedMask));
        QVERIFY(_multiSpy->pullBoolFromSignalIndex(surveyDirtyChangedIndex));
        _surveyItem->setDirty(false);
        _multiSpy->clearAllSignals();
    }
    rgFacts.clear();
}

// Clamp expected grid angle from 0<->180. We don't care about opposite angles like 90/270
double SurveyComplexItemTest::_clampGridAngle180(double gridAngle)
{
    if (gridAngle >= 0.0) {
        if (gridAngle == 360.0) {
            gridAngle = 0.0;
        } else if (gridAngle >= 180.0) {
            gridAngle -= 180.0;
        }
    } else {
        if (gridAngle < -180.0) {
            gridAngle += 360.0;
        } else {
            gridAngle += 180.0;
        }
    }
    return gridAngle;
}

void SurveyComplexItemTest::_setPolygon(void)
{
    for (int i=0; i<_polyPoints.count(); i++) {
        QGeoCoordinate& vertex = _polyPoints[i];
        _mapPolygon->appendVertex(vertex);
    }
}

void SurveyComplexItemTest::_testGridAngle(void)
{
    _setPolygon();

    for (double gridAngle=-360.0; gridAngle<=360.0; gridAngle++) {
        _surveyItem->gridAngle()->setRawValue(gridAngle);

        QVariantList gridPoints = _surveyItem->visualTransectPoints();
        QGeoCoordinate firstTransectEntry = gridPoints[0].value<QGeoCoordinate>();
        QGeoCoordinate firstTransectExit = gridPoints[1].value<QGeoCoordinate>();
        double azimuth = firstTransectEntry.azimuthTo(firstTransectExit);
        //qDebug() << gridAngle << azimuth << _clampGridAngle180(gridAngle) << _clampGridAngle180(azimuth);
        QCOMPARE((int)_clampGridAngle180(gridAngle), (int)_clampGridAngle180(azimuth));
    }
}

void SurveyComplexItemTest::_testEntryLocation(void)
{
    _setPolygon();

    for (double gridAngle=-360.0; gridAngle<=360.0; gridAngle++) {
        _surveyItem->gridAngle()->setRawValue(gridAngle);

        // Validate that each entry location is unique
        QList<QGeoCoordinate> rgSeenEntryCoords;
        for (int rotateCount=0; rotateCount<3; rotateCount++) {
            _surveyItem->rotateEntryPoint();
            QVERIFY(!rgSeenEntryCoords.contains(_surveyItem->coordinate()));
            rgSeenEntryCoords << _surveyItem->coordinate();
        }

        _surveyItem->rotateEntryPoint();    // Rotate back for first entry point
        rgSeenEntryCoords.clear();
    }
}


void SurveyComplexItemTest::_testItemCount(void)
{
    QList<MissionItem*> items;

    _setPolygon();

    _surveyItem->hoverAndCapture()->setRawValue(false);
    _surveyItem->cameraTriggerInTurnAround()->setRawValue(false);
    _surveyItem->refly90Degrees()->setRawValue(false);
    _surveyItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _surveyItem->lastSequenceNumber());
    items.clear();

    _surveyItem->hoverAndCapture()->setRawValue(false);
    _surveyItem->cameraTriggerInTurnAround()->setRawValue(true);
    _surveyItem->refly90Degrees()->setRawValue(false);
    _surveyItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _surveyItem->lastSequenceNumber());
    items.clear();

    _surveyItem->hoverAndCapture()->setRawValue(true);
    _surveyItem->cameraTriggerInTurnAround()->setRawValue(false);
    _surveyItem->refly90Degrees()->setRawValue(false);
    _surveyItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _surveyItem->lastSequenceNumber());
    items.clear();

    _surveyItem->hoverAndCapture()->setRawValue(true);
    _surveyItem->cameraTriggerInTurnAround()->setRawValue(false);
    _surveyItem->refly90Degrees()->setRawValue(true);
    _surveyItem->appendMissionItems(items, this);
    QCOMPARE(items.count() - 1, _surveyItem->lastSequenceNumber());
    items.clear();
}
