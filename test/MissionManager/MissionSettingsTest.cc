#include "MissionSettingsTest.h"

#include "MissionSettingsItem.h"

void MissionSettingsTest::init()
{
    VisualMissionItemTest::init();
    _settingsItem = new MissionSettingsItem(planController(), false /* flyView */);
}

void MissionSettingsTest::cleanup()
{
    delete _settingsItem;
    VisualMissionItemTest::cleanup();
}

void MissionSettingsTest::_testCameraSectionDirty()
{
    CameraSection* cameraSection = _settingsItem->cameraSection();
    QVERIFY(!cameraSection->dirty());
    QVERIFY(!_settingsItem->dirty());
    // Dirtying the camera section should also dirty the item
    cameraSection->setDirty(true);
    QVERIFY(_settingsItem->dirty());
    // Clearing the dirty bit from the item should also clear the dirty bit on the camera section
    _settingsItem->setDirty(false);
    QVERIFY(!cameraSection->dirty());
}

void MissionSettingsTest::_testSpeedSectionDirty()
{
    SpeedSection* speedSection = _settingsItem->speedSection();
    QVERIFY(!speedSection->dirty());
    QVERIFY(!_settingsItem->dirty());
    // Dirtying the speed section should also dirty the item
    speedSection->setDirty(true);
    QVERIFY(_settingsItem->dirty());
    // Clearing the dirty bit from the item should also clear the dirty bit on the camera section
    _settingsItem->setDirty(false);
    QVERIFY(!speedSection->dirty());
}

UT_REGISTER_TEST(MissionSettingsTest, TestLabel::Unit, TestLabel::MissionManager)
