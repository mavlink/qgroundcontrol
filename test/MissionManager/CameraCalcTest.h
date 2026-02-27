#pragma once

#include "BaseClasses/MissionTest.h"

class MultiSignalSpy;
class CameraCalc;

class CameraCalcTest : public OfflineMissionTest
{
    Q_OBJECT

protected:
    void init() final;
    void cleanup() final;

private slots:
    void _testDirty();
    void _testAdjustedFootprint();
    void _testAltDensityRecalc();

private:
    MultiSignalSpy* _multiSpy = nullptr;
    CameraCalc* _cameraCalc = nullptr;
};
