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
#include "MissionSettingsItem.h"

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
