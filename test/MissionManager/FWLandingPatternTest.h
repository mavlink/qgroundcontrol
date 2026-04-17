#pragma once

#include <memory>

#include "VisualMissionItemTest.h"

class MultiSignalSpy;
class FixedWingLandingComplexItem;
class SimpleMissionItem;

class FWLandingPatternTest : public VisualMissionItemTest
{
    Q_OBJECT

public:
    void init() override;
    void cleanup() override;

private slots:
    void _testDirty();
    void _testDefaults();
    void _testSaveLoad();

private:
    void _validateItem(FixedWingLandingComplexItem* newItem);

    FixedWingLandingComplexItem* _fwItem = nullptr;
    std::unique_ptr<MultiSignalSpy> _viSpy;
    SimpleMissionItem* _validStopVideoItem = nullptr;
    SimpleMissionItem* _validStopDistanceItem = nullptr;
    SimpleMissionItem* _validStopTimeItem = nullptr;
};
