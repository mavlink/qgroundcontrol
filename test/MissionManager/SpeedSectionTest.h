#pragma once

#include "SectionTest.h"

class SpeedSection;

/// Unit test for CameraSection
class SpeedSectionTest : public SectionTest
{
    Q_OBJECT

public:
    SpeedSectionTest() = default;

    void init() override;
    void cleanup() override;

private slots:
    void _testDirty();
    void _testSettingsAvailable();
    void _checkAvailable();
    void _testItemCount();
    void _testAppendSectionItems();
    void _testScanForSection();
    void _testSpecifiedFlightSpeedChanged();

private:
    void _createSpy(SpeedSection* speedSection, MultiSignalSpy** speedSpy);

    MultiSignalSpy* _spySpeed = nullptr;
    MultiSignalSpy* _spySection = nullptr;
    SpeedSection* _speedSection = nullptr;
};
