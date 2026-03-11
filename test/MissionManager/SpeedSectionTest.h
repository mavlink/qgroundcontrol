#pragma once

#include "MultiSignalSpy.h"
#include "SectionTest.h"

#include <memory>

class SpeedSection;

/// Unit test for CameraSection
class SpeedSectionTest : public SectionTest
{
    Q_OBJECT

public:
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

    std::unique_ptr<MultiSignalSpy> _spySpeed;
    std::unique_ptr<MultiSignalSpy> _spySection;
    SpeedSection* _speedSection = nullptr;
};
