#pragma once

#include "VisualMissionItemTest.h"

class MultiSignalSpy;
class FixedWingLandingComplexItem;
class SimpleMissionItem;

class FWLandingPatternTest : public VisualMissionItemTest
{
    Q_OBJECT

public:
    FWLandingPatternTest() = default;

    void init() override;
    void cleanup() override;

private slots:
    void _testDirty();
    void _testDefaults();
    void _testSaveLoad();

private:
    void _validateItem(FixedWingLandingComplexItem* newItem);

    FixedWingLandingComplexItem* _fwItem = nullptr;
    MultiSignalSpy* _viSpy = nullptr;
    SimpleMissionItem* _validStopVideoItem = nullptr;
    SimpleMissionItem* _validStopDistanceItem = nullptr;
    SimpleMissionItem* _validStopTimeItem = nullptr;
};
