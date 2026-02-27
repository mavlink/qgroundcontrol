#pragma once

#include "VisualMissionItemTest.h"

class MissionSettingsItem;

/// Unit test for SimpleMissionItem
class MissionSettingsTest : public VisualMissionItemTest
{
    Q_OBJECT

public:
    void init() override;
    void cleanup() override;

private slots:
    void _testCameraSectionDirty();
    void _testSpeedSectionDirty();

private:
    MissionSettingsItem* _settingsItem = nullptr;
};
