#pragma once

#include "UnitTest.h"

class MissionItem;
class PlanMasterController;

/// Unit test for the MissionItem Object
class MissionItemTest : public UnitTest
{
    Q_OBJECT

public:
    MissionItemTest(void);

    void init(void) override;
    void cleanup(void) override;

private slots:
    void _testSetGet(void);
    void _testSignals(void);
    void _testFactSignals(void);
    void _testLoadFromStream(void);
    void _testSimpleLoadFromStream(void);
    void _testLoadFromJsonV1(void);
    void _testLoadFromJsonV2(void);
    void _testLoadFromJsonV3(void);
    void _testLoadFromJsonV3NaN(void);
    void _testSimpleLoadFromJson(void);
    void _testSaveToJson(void);

private:
    void _checkExpectedMissionItem(const MissionItem& missionItem, bool allNaNs = false) const;
    QJsonObject _createV1Json(void);
    QJsonObject _createV2Json(void);
    QJsonObject _createV3Json(bool allNaNs = false);

    int                     _seq = 10;
    PlanMasterController*   _masterController = nullptr;
};
