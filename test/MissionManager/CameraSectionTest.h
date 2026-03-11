#pragma once

#include "MultiSignalSpy.h"
#include "SectionTest.h"

#include <memory>

class CameraSection;
class PlanMasterController;
class SimpleMissionItem;

/// Unit test for CameraSection
class CameraSectionTest : public SectionTest
{
    Q_OBJECT

public:
    void init() override;
    void cleanup() override;

    static SimpleMissionItem* createValidStopVideoItem(PlanMasterController* masterController);
    static SimpleMissionItem* createValidStopDistanceItem(PlanMasterController* masterController);
    static SimpleMissionItem* createValidStopTimeItem(PlanMasterController* masterController);
    static SimpleMissionItem* createInvalidStopVideoItem(PlanMasterController* masterController);
    static SimpleMissionItem* createInvalidStopDistanceItem(PlanMasterController* masterController);
    static SimpleMissionItem* createInvalidStopTimeItem(PlanMasterController* masterController);

private slots:
    void _testDirty();
    void _testSettingsAvailable();
    void _checkAvailable();
    void _testItemCount();
    void _testAppendSectionItems();
    void _testScanForGimbalSection();
    void _testScanForPhotoIntervalTimeSection();
    void _testScanForPhotoIntervalDistanceSection();
    void _testScanForStartVideoSection();
    void _testScanForStopVideoSection();
    void _testScanForStopPhotoSection();
    void _testScanForCameraModeSection();
    void _testScanForTakePhotoSection();
    void _testScanForMultipleItems();
    void _testSpecifiedGimbalValuesChanged();

private:
    void _createSpy(CameraSection* cameraSection, MultiSignalSpy** cameraSpy);
    void _validateItemScan(SimpleMissionItem* validItem);
    void _resetSection();

    std::unique_ptr<MultiSignalSpy> _spyCamera;
    std::unique_ptr<MultiSignalSpy> _spySection;
    CameraSection* _cameraSection = nullptr;
    SimpleMissionItem* _validGimbalItem = nullptr;
    SimpleMissionItem* _validDistanceItem = nullptr;
    SimpleMissionItem* _validTimeItem = nullptr;
    SimpleMissionItem* _validStartVideoItem = nullptr;
    SimpleMissionItem* _validStopVideoItem = nullptr;
    SimpleMissionItem* _validStopDistanceItem = nullptr;
    SimpleMissionItem* _validStopTimeItem = nullptr;
    SimpleMissionItem* _validCameraPhotoModeItem = nullptr;
    SimpleMissionItem* _validCameraVideoModeItem = nullptr;
    SimpleMissionItem* _validCameraSurveyPhotoModeItem = nullptr;
    SimpleMissionItem* _validTakePhotoItem = nullptr;
};
