#pragma once

#include <memory>

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
    std::unique_ptr<MultiSignalSpy> _multiSpy;
    CameraCalc* _cameraCalc = nullptr;
};
