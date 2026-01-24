#pragma once

#include "TestFixtures.h"

class MissionItem;
class PlanMasterController;

/// Unit test for the MissionItem Object.
/// Uses OfflineTest since it works with offline PlanMasterController.
class MissionItemTest : public OfflineTest
{
    Q_OBJECT

public:
    MissionItemTest() = default;

    void init() override;
    void cleanup() override;

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
    PlanMasterController* _masterController = nullptr;
};
