/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "TransectStyleComplexItemTestBase.h"
#include "TCPLink.h"
#include "MultiSignalSpy.h"
#include "CorridorScanComplexItem.h"
#include "PlanMasterController.h"

#include <QGeoCoordinate>

class CorridorScanComplexItemTest : public TransectStyleComplexItemTestBase
{
    Q_OBJECT
    
public:
    CorridorScanComplexItemTest(void);

protected:
    void init   (void) final;
    void cleanup(void) final;
    
#if 1
private slots:
    void _testDirty         (void);
    void _testCameraTrigger (void);
    void _testPathChanges   (void);
    void _testItemGeneration(void);
    void _testItemCount     (void);
#else
    // Used to debug a single test
private slots:
    void _testItemGeneration(void);
private:
    void _testDirty         (void);
    void _testCameraTrigger (void);
    void _testPathChanges   (void);
    void _testItemCount     (void);
#endif

private:
    void            _waitForReadyForSave(void);
    QList<MAV_CMD>  _createExpectedCommands(bool hasTurnaround, bool useConditionGate);
    void            _testItemGenerationWorker(bool imagesInTurnaround, bool hasTurnaround, bool useConditionGate, const QList<MAV_CMD>& expectedCommands);

    enum {
        corridorPolygonPathChangedIndex = 0,
        maxCorridorPolygonSignalIndex
    };

    enum {
        corridorPolygonPathChangedMask = 1 << corridorPolygonPathChangedIndex,
    };

    static const size_t _cCorridorPolygonSignals = maxCorridorPolygonSignalIndex;
    const char*         _rgCorridorPolygonSignals[_cCorridorPolygonSignals];

    MultiSignalSpy*             _multiSpyCorridorPolygon =  nullptr;
    CorridorScanComplexItem*    _corridorItem =             nullptr;
    QList<QGeoCoordinate>       _polyLineVertices;

    static constexpr int    _expectedTransectCount =        2;
    static constexpr double _corridorLineSegmentDistance =  200.0;
    static constexpr double _corridorWidth =                50.0;
};
