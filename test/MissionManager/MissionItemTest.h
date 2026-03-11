#pragma once

#include "BaseClasses/MissionTest.h"

class MissionItem;

/// Unit test for the MissionItem Object
class MissionItemTest : public OfflineMissionTest
{
    Q_OBJECT

private slots:
    void _testSetGet();
    void _testSignals();
    void _testFactSignals();
    void _testLoadFromStream();
    void _testSimpleLoadFromStream();
    void _testLoadFromJsonV1();
    void _testLoadFromJsonV2();
    void _testLoadFromJsonV3();
    void _testLoadFromJsonV3NaN();
    void _testSimpleLoadFromJson();
    void _testSaveToJson();

private:
    void _checkExpectedMissionItem(const MissionItem& missionItem, bool allNaNs = false) const;
    QJsonObject _createV1Json();
    QJsonObject _createV2Json();
    QJsonObject _createV3Json(bool allNaNs = false);

    int _seq = 10;
};
