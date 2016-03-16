/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef ComplexMissionItemTest_H
#define ComplexMissionItemTest_H

#include "UnitTest.h"
#include "TCPLink.h"
#include "MultiSignalSpy.h"
#include "ComplexMissionItem.h"

#include <QGeoCoordinate>

/// Unit test for SimpleMissionItem
class ComplexMissionItemTest : public UnitTest
{
    Q_OBJECT
    
public:
    ComplexMissionItemTest(void);

protected:
    void init(void) final;
    void cleanup(void) final;
    
private slots:
    void _testDirty(void);
    void _testAddPolygonCoordinate(void);
    void _testClearPolygon(void);
    void _testCameraTrigger(void);

private:
    enum {
        polygonPathChangedIndex = 0,
        lastSequenceNumberChangedIndex,
        altitudeChangedIndex,
        gridAngleChangedIndex,
        gridPointsChangedIndex,
        cameraTriggerChangedIndex,
        altDifferenceChangedIndex,
        altPercentChangedIndex,
        azimuthChangedIndex,
        commandDescriptionChangedIndex,
        commandNameChangedIndex,
        abbreviationChangedIndex,
        coordinateChangedIndex,
        exitCoordinateChangedIndex,
        dirtyChangedIndex,
        distanceChangedIndex,
        isCurrentItemChangedIndex,
        sequenceNumberChangedIndex,
        isSimpleItemChangedIndex,
        specifiesCoordinateChangedIndex,
        isStandaloneCoordinateChangedIndex,
        coordinateHasRelativeAltitudeChangedIndex,
        exitCoordinateHasRelativeAltitudeChangedIndex,
        exitCoordinateSameAsEntryChangedIndex,
        maxSignalIndex
    };

    enum {
        polygonPathChangedMask =                        1 << polygonPathChangedIndex,
        lastSequenceNumberChangedMask =                 1 << lastSequenceNumberChangedIndex,
        altitudeChangedMask =                           1 << altitudeChangedIndex,
        gridAngleChangedMask =                          1 << gridAngleChangedIndex,
        gridPointsChangedMask =                         1 << gridPointsChangedIndex,
        cameraTriggerChangedMask =                      1 << cameraTriggerChangedIndex,
        altDifferenceChangedMask =                      1 << altDifferenceChangedIndex,
        altPercentChangedMask =                         1 << altPercentChangedIndex,
        azimuthChangedMask =                            1 << azimuthChangedIndex,
        commandDescriptionChangedMask =                 1 << commandDescriptionChangedIndex,
        commandNameChangedMask =                        1 << commandNameChangedIndex,
        coordinateChangedMask =                         1 << coordinateChangedIndex,
        exitCoordinateChangedMask =                     1 << exitCoordinateChangedIndex,
        dirtyChangedMask =                              1 << dirtyChangedIndex,
        distanceChangedMask =                           1 << distanceChangedIndex,
        isCurrentItemChangedMask =                      1 << isCurrentItemChangedIndex,
        sequenceNumberChangedMask =                     1 << sequenceNumberChangedIndex,
        isSimpleItemChangedMask =                       1 << isSimpleItemChangedIndex,
        specifiesCoordinateChangedMask =                1 << specifiesCoordinateChangedIndex,
        isStandaloneCoordinateChangedMask =             1 << isStandaloneCoordinateChangedIndex,
        coordinateHasRelativeAltitudeChangedMask =      1 << coordinateHasRelativeAltitudeChangedIndex,
        exitCoordinateHasRelativeAltitudeChangedMask =  1 << exitCoordinateHasRelativeAltitudeChangedIndex,
        exitCoordinateSameAsEntryChangedMask =          1 << exitCoordinateSameAsEntryChangedIndex,
    };

    static const size_t _cComplexMissionItemSignals = maxSignalIndex;
    const char*         _rgComplexMissionItemSignals[_cComplexMissionItemSignals];

    MultiSignalSpy*         _multiSpy;
    ComplexMissionItem*     _complexItem;
    QList<QGeoCoordinate>   _polyPoints;
};

#endif
