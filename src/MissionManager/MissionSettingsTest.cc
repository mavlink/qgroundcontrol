/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MissionSettingsTest.h"
#include "QGCApplication.h"
#include "QGroundControlQmlGlobal.h"
#include "SettingsManager.h"

MissionSettingsTest::MissionSettingsTest(void)
    : _settingsItem(nullptr)
{
    
}

void MissionSettingsTest::init(void)
{
    VisualMissionItemTest::init();

    _settingsItem = new MissionSettingsItem(_offlineVehicle, false /* flyView */, this);
}

void MissionSettingsTest::cleanup(void)
{
    delete _settingsItem;
    VisualMissionItemTest::cleanup();
}

void MissionSettingsTest::_testCameraSectionDirty(void)
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

void MissionSettingsTest::_testSpeedSectionDirty(void)
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
