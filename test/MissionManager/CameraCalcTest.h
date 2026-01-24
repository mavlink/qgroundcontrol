#pragma once

#include "TestFixtures.h"

class MultiSignalSpy;
class CameraCalc;
class PlanMasterController;

/// Unit test for CameraCalc.
/// Uses OfflineTest since it works with offline PlanMasterController.
class CameraCalcTest : public OfflineTest
{
    Q_OBJECT

public:
    CameraCalcTest() = default;

protected:
    void init() final;
    void cleanup() final;

private slots:
    void _testDirty();
    void _testAdjustedFootprint();
    void _testAltDensityRecalc();

private:
    PlanMasterController* _masterController = nullptr;
    Vehicle* _controllerVehicle = nullptr;
    MultiSignalSpy* _multiSpy = nullptr;
    CameraCalc* _cameraCalc = nullptr;
};
