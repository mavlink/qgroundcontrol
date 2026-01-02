#pragma once

#include "VisualMissionItemTest.h"

class MultiSignalSpy;
class FixedWingLandingComplexItem;
class SimpleMissionItem;

class FWLandingPatternTest : public VisualMissionItemTest
{
    Q_OBJECT

public:
    FWLandingPatternTest(void);

    void init(void) override;
    void cleanup(void) override;

private slots:
    void _testDirty     (void);
    void _testDefaults  (void);
    void _testSaveLoad  (void);

private:
    void _validateItem(FixedWingLandingComplexItem* newItem);

    FixedWingLandingComplexItem*    _fwItem                 = nullptr;
    MultiSignalSpy*                 _viSpy                  = nullptr;
    SimpleMissionItem*              _validStopVideoItem     = nullptr;
    SimpleMissionItem*              _validStopDistanceItem  = nullptr;
    SimpleMissionItem*              _validStopTimeItem      = nullptr;
};
