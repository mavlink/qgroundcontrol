/*=====================================================================

 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#include "ComplexMissionItemTest.h"

ComplexMissionItemTest::ComplexMissionItemTest(void)
{    
    _polyPoints << QGeoCoordinate(47.633550640000003, -122.08982199) << QGeoCoordinate(47.634129020000003, -122.08887249) <<
                  QGeoCoordinate(47.633619320000001, -122.08811074) << QGeoCoordinate(47.633189139999999, -122.08900124);
}

void ComplexMissionItemTest::init(void)
{
    _rgComplexMissionItemSignals[polygonPathChangedIndex] =         SIGNAL(polygonPathChanged());
    _rgComplexMissionItemSignals[lastSequenceNumberChangedIndex] =  SIGNAL(lastSequenceNumberChanged(int));
    _rgComplexMissionItemSignals[altitudeChangedIndex] =            SIGNAL(altitudeChanged(double));
    _rgComplexMissionItemSignals[gridAngleChangedIndex] =           SIGNAL(gridAngleChanged(double));
    _rgComplexMissionItemSignals[gridPointsChangedIndex] =          SIGNAL(gridPointsChanged());
    _rgComplexMissionItemSignals[cameraTriggerChangedIndex] =       SIGNAL(cameraTriggerChanged(bool));

    _rgComplexMissionItemSignals[altDifferenceChangedIndex] =           SIGNAL(altDifferenceChanged(double));
    _rgComplexMissionItemSignals[altPercentChangedIndex] =              SIGNAL(altPercentChanged(double));
    _rgComplexMissionItemSignals[azimuthChangedIndex] =                 SIGNAL(azimuthChanged(double));
    _rgComplexMissionItemSignals[commandDescriptionChangedIndex] =      SIGNAL(commandDescriptionChanged());
    _rgComplexMissionItemSignals[commandNameChangedIndex] =             SIGNAL(commandNameChanged());
    _rgComplexMissionItemSignals[abbreviationChangedIndex] =            SIGNAL(abbreviationChanged());
    _rgComplexMissionItemSignals[coordinateChangedIndex] =              SIGNAL(coordinateChanged(const QGeoCoordinate&));
    _rgComplexMissionItemSignals[exitCoordinateChangedIndex] =          SIGNAL(exitCoordinateChanged(const QGeoCoordinate&));
    _rgComplexMissionItemSignals[dirtyChangedIndex] =                   SIGNAL(dirtyChanged(bool));
    _rgComplexMissionItemSignals[distanceChangedIndex] =                SIGNAL(distanceChanged(double));
    _rgComplexMissionItemSignals[isCurrentItemChangedIndex] =           SIGNAL(isCurrentItemChanged(bool));
    _rgComplexMissionItemSignals[sequenceNumberChangedIndex] =          SIGNAL(sequenceNumberChanged(int));
    _rgComplexMissionItemSignals[isSimpleItemChangedIndex] =            SIGNAL(isSimpleItemChanged(bool));
    _rgComplexMissionItemSignals[specifiesCoordinateChangedIndex] =     SIGNAL(specifiesCoordinateChanged());
    _rgComplexMissionItemSignals[isStandaloneCoordinateChangedIndex] =  SIGNAL(isStandaloneCoordinateChanged());

    _rgComplexMissionItemSignals[coordinateHasRelativeAltitudeChangedIndex] =       SIGNAL(coordinateHasRelativeAltitudeChanged(bool));
    _rgComplexMissionItemSignals[exitCoordinateHasRelativeAltitudeChangedIndex] =   SIGNAL(exitCoordinateHasRelativeAltitudeChanged(bool));
    _rgComplexMissionItemSignals[exitCoordinateSameAsEntryChangedIndex] =           SIGNAL(exitCoordinateSameAsEntryChanged(bool));

    _complexItem = new ComplexMissionItem(NULL /* Vehicle */, this);

    // It's important to check that the right signals are emitted at the right time since that drives ui change.
    // It's also important to check that things are not being over-signalled when they should not be, since that can lead
    // to incorrect ui or perf impact of uneeded signals propogating ui change.

    _multiSpy = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpy);
    QCOMPARE(_multiSpy->init(_complexItem, _rgComplexMissionItemSignals, _cComplexMissionItemSignals), true);
}

void ComplexMissionItemTest::cleanup(void)
{
    delete _complexItem;
    delete _multiSpy;
}

void ComplexMissionItemTest::_testDirty(void)
{
    QVERIFY(!_complexItem->dirty());
    _complexItem->setDirty(false);
    QVERIFY(!_complexItem->dirty());
    QVERIFY(_multiSpy->checkNoSignals());
    _complexItem->setDirty(true);
    QVERIFY(_complexItem->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
    _multiSpy->clearAllSignals();
    _complexItem->setDirty(false);
    QVERIFY(!_complexItem->dirty());
    QVERIFY(_multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    QVERIFY(!_multiSpy->pullBoolFromSignalIndex(dirtyChangedIndex));
}

void ComplexMissionItemTest::_testAddPolygonCoordinate(void)
{
    QCOMPARE(_complexItem->polygonPath().count(), 0);

    // First call to addPolygonCoordinate should trigger:
    //      polygonPathChanged
    //      dirtyChanged

    _complexItem->addPolygonCoordinate(_polyPoints[0]);
    QVERIFY(_multiSpy->checkOnlySignalByMask(polygonPathChangedMask | dirtyChangedMask));

    // Validate object data
    QVariantList polyList = _complexItem->polygonPath();
    QCOMPARE(polyList.count(), 1);
    QCOMPARE(polyList[0].value<QGeoCoordinate>(), _polyPoints[0]);

    // Reset
    _complexItem->setDirty(false);
    _multiSpy->clearAllSignals();

    // Second call to addPolygonCoordinate should only trigger:
    //      polygonPathChanged
    //      dirtyChanged

    _complexItem->addPolygonCoordinate(_polyPoints[1]);
    QVERIFY(_multiSpy->checkOnlySignalByMask(polygonPathChangedMask | dirtyChangedMask));

    polyList = _complexItem->polygonPath();
    QCOMPARE(polyList.count(), 2);
    for (int i=0; i<polyList.count(); i++) {
        QCOMPARE(polyList[i].value<QGeoCoordinate>(), _polyPoints[i]);
    }

    _complexItem->setDirty(false);
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

    _complexItem->addPolygonCoordinate(_polyPoints[2]);
    QVERIFY(_multiSpy->checkOnlySignalByMask(polygonPathChangedMask | lastSequenceNumberChangedMask | gridPointsChangedMask | coordinateChangedMask |
                                            exitCoordinateChangedMask | specifiesCoordinateChangedMask | dirtyChangedMask));
    int seqNum = _multiSpy->pullIntFromSignalIndex(lastSequenceNumberChangedIndex);
    QVERIFY(seqNum > 0);

    polyList = _complexItem->polygonPath();
    QCOMPARE(polyList.count(), 3);
    for (int i=0; i<polyList.count(); i++) {
        QCOMPARE(polyList[i].value<QGeoCoordinate>(), _polyPoints[i]);
    }

    _complexItem->setDirty(false);
    _multiSpy->clearAllSignals();

    // Forth call to addPolygonCoordinate should trigger:
    //      polygonPathChanged
    //      dirtyChanged
    // Grid is generated again on polygon change which triggers:
    //      lastSequenceNumberChanged -  number of internal mission items changes
    //      gridPointsChanged - grid points show up for the first time
    //      exitCoordinateChanged - grid generates new exit coordinate
    // Note: Given the data set the entry coordinate stays the same

    _complexItem->addPolygonCoordinate(_polyPoints[3]);
    QVERIFY(_multiSpy->checkOnlySignalByMask(polygonPathChangedMask | lastSequenceNumberChangedMask | gridPointsChangedMask | exitCoordinateChangedMask |
                                             dirtyChangedMask));
    seqNum = _multiSpy->pullIntFromSignalIndex(lastSequenceNumberChangedIndex);
    QVERIFY(seqNum > 0);

    polyList = _complexItem->polygonPath();
    QCOMPARE(polyList.count(), 4);
    for (int i=0; i<polyList.count(); i++) {
        QCOMPARE(polyList[i].value<QGeoCoordinate>(), _polyPoints[i]);
    }
}

void ComplexMissionItemTest::_testClearPolygon(void)
{
    for (int i=0; i<3; i++) {
        _complexItem->addPolygonCoordinate(_polyPoints[i]);
    }
    _complexItem->setDirty(false);
    _multiSpy->clearAllSignals();

    // Call to clearPolygon should trigger:
    //      polygonPathChangedMask
    //      dirtyChanged
    //      lastSequenceNumberChangedMask
    //      gridPointsChangedMask
    //      dirtyChangedMask
    //      specifiesCoordinateChangedMask

    _complexItem->clearPolygon();
    QVERIFY(_multiSpy->checkOnlySignalByMask(polygonPathChangedMask | lastSequenceNumberChangedMask | gridPointsChangedMask | dirtyChangedMask |
                                             specifiesCoordinateChangedMask));
    QVERIFY(!_multiSpy->pullBoolFromSignalIndex(specifiesCoordinateChangedIndex));
    QCOMPARE(_multiSpy->pullIntFromSignalIndex(lastSequenceNumberChangedIndex), 0);

    QCOMPARE(_complexItem->polygonPath().count(), 0);
    QCOMPARE(_complexItem->gridPoints().count(), 0);

    _complexItem->setDirty(false);
    _multiSpy->clearAllSignals();
}

void ComplexMissionItemTest::_testCameraTrigger(void)
{
    QVERIFY(!_complexItem->property("cameraTrigger").toBool());

    // Turning on/off camera triggering while there is no grid should trigger:
    //      cameraTriggerChanged
    //      dirtyChanged
    // lastSequenceNumber should not change

    int lastSeq = _complexItem->lastSequenceNumber();
    _complexItem->setProperty("cameraTrigger", true);
    QVERIFY(_multiSpy->checkOnlySignalByMask(dirtyChangedMask | cameraTriggerChangedMask));
    QVERIFY(_multiSpy->pullBoolFromSignalIndex(cameraTriggerChangedIndex));
    QCOMPARE(_complexItem->lastSequenceNumber(), lastSeq);

    _complexItem->setDirty(false);
    _multiSpy->clearAllSignals();

    _complexItem->setProperty("cameraTrigger", false);
    QVERIFY(_multiSpy->checkOnlySignalByMask(dirtyChangedMask | cameraTriggerChangedMask));
    QVERIFY(!_multiSpy->pullBoolFromSignalIndex(cameraTriggerChangedIndex));
    QCOMPARE(_complexItem->lastSequenceNumber(), lastSeq);

    // Set up a grid

    for (int i=0; i<3; i++) {
        _complexItem->addPolygonCoordinate(_polyPoints[i]);
    }

    _complexItem->setDirty(false);
    _multiSpy->clearAllSignals();

    lastSeq = _complexItem->lastSequenceNumber();
    QVERIFY(lastSeq > 0);

    // Turning on camera triggering should add two more mission items, this should trigger:
    //      lastSequenceNumberChanged
    //      dirtyChanged

    _complexItem->setProperty("cameraTrigger", true);
    QVERIFY(_multiSpy->checkOnlySignalByMask(lastSequenceNumberChangedMask | dirtyChangedMask | cameraTriggerChangedMask));
    QCOMPARE(_multiSpy->pullIntFromSignalIndex(lastSequenceNumberChangedIndex), lastSeq + 2);

    _complexItem->setDirty(false);
    _multiSpy->clearAllSignals();

    // Turn off camera triggering and make sure things go back to previous count

    _complexItem->setProperty("cameraTrigger", false);
    QVERIFY(_multiSpy->checkOnlySignalByMask(lastSequenceNumberChangedMask | dirtyChangedMask | cameraTriggerChangedMask));
    QCOMPARE(_multiSpy->pullIntFromSignalIndex(lastSequenceNumberChangedIndex), lastSeq);
}
