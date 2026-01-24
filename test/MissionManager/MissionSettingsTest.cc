#include "MissionSettingsTest.h"
#include "MissionSettingsItem.h"

#include <QtTest/QTest>

void MissionSettingsTest::init()
{
    VisualMissionItemTest::init();

    _settingsItem = new MissionSettingsItem(_masterController, false /* flyView */);
    VERIFY_NOT_NULL(_settingsItem);
}

void MissionSettingsTest::cleanup()
{
    delete _settingsItem;
    VisualMissionItemTest::cleanup();
}

void MissionSettingsTest::_testCameraSectionDirty()
{
    CameraSection* cameraSection = _settingsItem->cameraSection();
    VERIFY_NOT_NULL(cameraSection);

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
    VERIFY_NOT_NULL(speedSection);

    QVERIFY(!speedSection->dirty());
    QVERIFY(!_settingsItem->dirty());

    // Dirtying the speed section should also dirty the item
    speedSection->setDirty(true);
    QVERIFY(_settingsItem->dirty());

    // Clearing the dirty bit from the item should also clear the dirty bit on the camera section
    _settingsItem->setDirty(false);
    QVERIFY(!speedSection->dirty());
}
