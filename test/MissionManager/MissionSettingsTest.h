#pragma once

#include "VisualMissionItemTest.h"

class MissionSettingsItem;

/// Unit test for SimpleMissionItem
class MissionSettingsTest : public VisualMissionItemTest
{
    Q_OBJECT

public:
    MissionSettingsTest(void);

    void init(void) override;
    void cleanup(void) override;

private slots:
    void _testCameraSectionDirty(void);
    void _testSpeedSectionDirty(void);

private:
    MissionSettingsItem* _settingsItem;
};
