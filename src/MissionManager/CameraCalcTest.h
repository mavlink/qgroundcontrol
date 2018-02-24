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
#include "MultiSignalSpy.h"
#include "CameraCalc.h"

#include <QGeoCoordinate>

class CameraCalcTest : public UnitTest
{
    Q_OBJECT
    
public:
    CameraCalcTest(void);

protected:
    void init(void) final;
    void cleanup(void) final;
    
private slots:
    void _testDirty(void);

private:
    enum {
        cameraNameChangedIndex = 0,
        dirtyChangedIndex,
        imageFootprintSideChangedIndex,
        imageFootprintFrontalChangedIndex,
        distanceToSurfaceRelativeChangedIndex,
        maxSignalIndex
    };

    enum {
        cameraNameChangedMask =                 1 << cameraNameChangedIndex,
        dirtyChangedMask =                      1 << dirtyChangedIndex,
        imageFootprintSideChangedMask =         1 << imageFootprintSideChangedIndex,
        imageFootprintFrontalChangedMask =      1 << imageFootprintFrontalChangedIndex,
        distanceToSurfaceRelativeChangedMask =  1 << distanceToSurfaceRelativeChangedIndex,
    };

    static const size_t _cSignals = maxSignalIndex;
    const char*         _rgSignals[_cSignals];

    Vehicle*        _offlineVehicle;
    MultiSignalSpy* _multiSpy;
    CameraCalc*     _cameraCalc;
};
