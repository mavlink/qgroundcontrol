#pragma once

#include "VisualMissionItemTest.h"

class Section;
class SimpleMissionItem;

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
