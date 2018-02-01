/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"
#include "TCPLink.h"
#include "MultiSignalSpy.h"
#include "CorridorScanComplexItem.h"

#include <QGeoCoordinate>

class CorridorScanComplexItemTest : public UnitTest
{
    Q_OBJECT
    
public:
    CorridorScanComplexItemTest(void);

protected:
    void init(void) final;
    void cleanup(void) final;
    
private slots:
    void _testDirty(void);
    void _testCameraTrigger(void);
//    void _testEntryLocation(void);
    void _testItemCount(void);
    void _testPathChanges(void);

private:
    void _setPolyline(void);

    enum {
        complexDistanceChangedIndex = 0,
        greatestDistanceToChangedIndex,
        additionalTimeDelayChangedIndex,
        transectPointsChangedIndex,
        cameraShotsChangedIndex,
        coveredAreaChangedIndex,
        timeBetweenShotsChangedIndex,
        dirtyChangedIndex,
        maxSignalIndex
    };

    enum {
        complexDistanceChangedMask =        1 << complexDistanceChangedIndex,
        greatestDistanceToChangedMask =     1 << greatestDistanceToChangedIndex,
        additionalTimeDelayChangedMask =    1 << additionalTimeDelayChangedIndex,
        transectPointsChangedMask =         1 << transectPointsChangedIndex,
        cameraShotsChangedMask =            1 << cameraShotsChangedIndex,
        coveredAreaChangedMask =            1 << coveredAreaChangedIndex,
        timeBetweenShotsChangedMask =       1 << timeBetweenShotsChangedIndex,
        dirtyChangedMask =                  1 << dirtyChangedIndex
    };

    static const size_t _cSignals = maxSignalIndex;
    const char*         _rgSignals[_cSignals];

    enum {
        corridorPolygonPathChangedIndex = 0,
        maxCorridorPolygonSignalIndex
    };

    enum {
        corridorPolygonPathChangedMask = 1 << corridorPolygonPathChangedIndex,
    };

    static const size_t _cCorridorPolygonSignals = maxCorridorPolygonSignalIndex;
    const char*         _rgCorridorPolygonSignals[_cCorridorPolygonSignals];

    Vehicle*                    _offlineVehicle;
    MultiSignalSpy*             _multiSpy;
    MultiSignalSpy*             _multiSpyCorridorPolygon;
    CorridorScanComplexItem*    _corridorItem;
    QGCMapPolyline*             _mapPolyline;
    QList<QGeoCoordinate>       _linePoints;
};
