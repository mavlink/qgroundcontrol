/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "PlanMasterController.h"

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
    void _waitForReadyForSave(void);

    enum {
        corridorPolygonPathChangedIndex = 0,
        maxCorridorPolygonSignalIndex
    };

    enum {
        corridorPolygonPathChangedMask = 1 << corridorPolygonPathChangedIndex,
    };

    static const size_t _cCorridorPolygonSignals = maxCorridorPolygonSignalIndex;
    const char*         _rgCorridorPolygonSignals[_cCorridorPolygonSignals];

    PlanMasterController*       _masterController =         nullptr;
    Vehicle*                    _controllerVehicle =        nullptr;
    MultiSignalSpy*             _multiSpyCorridorPolygon =  nullptr;
    CorridorScanComplexItem*    _corridorItem =             nullptr;
    QList<QGeoCoordinate>       _linePoints;
};
