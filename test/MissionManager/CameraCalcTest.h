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

class MultiSignalSpyV2;
class CameraCalc;
class PlanMasterController;

class CameraCalcTest : public UnitTest
{
    Q_OBJECT
    
public:
    CameraCalcTest(void);

protected:
    void init(void) final;
    void cleanup(void) final;
    
private slots:
    void _testDirty             (void);
    void _testAdjustedFootprint (void);
    void _testAltDensityRecalc  (void);

private:
    PlanMasterController*   _masterController   = nullptr;
    Vehicle*                _controllerVehicle  = nullptr;
    MultiSignalSpyV2*       _multiSpy           = nullptr;
    CameraCalc*             _cameraCalc         = nullptr;
};
