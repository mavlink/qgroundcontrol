#pragma once

#include "SectionTest.h"

class SpeedSection;

/// Unit test for CameraSection
class SpeedSectionTest : public SectionTest
{
    Q_OBJECT

public:
    SpeedSectionTest(void);

    void init(void) override;
    void cleanup(void) override;

private slots:
    void _testDirty                         (void);
    void _testSettingsAvailable             (void);
    void _checkAvailable                    (void);
    void _testItemCount                     (void);
    void _testAppendSectionItems            (void);
    void _testScanForSection                (void);
    void _testSpecifiedFlightSpeedChanged   (void);

private:
    void _createSpy(SpeedSection* speedSection, MultiSignalSpy** speedSpy);

    enum {
        specifyFlightSpeedChangedIndex = 0,
        maxSignalIndex,
    };

    enum {
        specifyFlightSpeedChangedMask = 1 << specifyFlightSpeedChangedIndex
    };

    static const size_t cSpeedSignals = maxSignalIndex;
    const char*         rgSpeedSignals[cSpeedSignals];

    MultiSignalSpy* _spySpeed;
    MultiSignalSpy* _spySection;
    SpeedSection*   _speedSection;
};
