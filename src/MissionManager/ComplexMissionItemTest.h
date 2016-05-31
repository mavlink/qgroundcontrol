/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
