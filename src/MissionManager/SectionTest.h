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
#include "SimpleMissionItemTest.h"

/// Unit test for Sections
class SectionTest : public VisualMissionItemTest
{
    Q_OBJECT
    
public:
    SectionTest(void);

    void init(void) override;
    void cleanup(void) override;

protected:
    void _createSpy(Section* section, MultiSignalSpy** sectionSpy);
    void _commonScanTest(Section* section);

    enum {
        availableChangedIndex = 0,
        settingsSpecifiedChangedIndex,
        dirtyChangedIndex,
        itemCountChangedIndex,
        maxSignalIndex,
    };

    enum {
        availableChangedMask =          1 << availableChangedIndex,
        settingsSpecifiedChangedMask =  1 << settingsSpecifiedChangedIndex,
        dirtyChangedMask =              1 << dirtyChangedIndex,
        itemCountChangedMask =          1 << itemCountChangedIndex
    };

    static const size_t cSectionSignals = maxSignalIndex;
    const char*         rgSectionSignals[cSectionSignals];

    SimpleMissionItem*  _simpleItem;
};
