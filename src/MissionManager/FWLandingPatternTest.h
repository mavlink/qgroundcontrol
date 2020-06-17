/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "VisualMissionItemTest.h"
#include "FixedWingLandingComplexItem.h"
#include "MultiSignalSpy.h"
#include "PlanMasterController.h"

class FWLandingPatternTest : public VisualMissionItemTest
{
    Q_OBJECT
    
public:
    FWLandingPatternTest(void);

    void init(void) override;
    void cleanup(void) override;

private slots:
    void _testDirty                 (void);
    void _testItemCount             (void);
    void _testDefaults              (void);
    void _testAppendSectionItems    (void);
    void _testSaveLoad              (void);

private:
    void _validateItem(FixedWingLandingComplexItem* newItem);

    enum {
        dirtyChangedIndex = 0,
        maxSignalIndex,
    };

    enum {
        dirtyChangedMask = 1 << dirtyChangedIndex,
    };

    static const size_t cSignals = maxSignalIndex;
    const char*         rgSignals[cSignals];

    FixedWingLandingComplexItem*    _fwItem =                   nullptr;
    MultiSignalSpy*                 _multiSpy =                 nullptr;
    SimpleMissionItem*              _validStopVideoItem =       nullptr;
    SimpleMissionItem*              _validStopDistanceItem =    nullptr;
    SimpleMissionItem*              _validStopTimeItem =        nullptr;
};
