#pragma once

#include "BaseClasses/VehicleTest.h"

class PlanMasterController;

class PlanMasterControllerTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void init() final;
    void cleanup() final;

    void _testMissionPlannerFileLoad();
    void _testActiveVehicleChanged();
    void _testDirtyFlagsMatrix_data();
    void _testDirtyFlagsMatrix();

    // File name property tests
    void _testFileNamesSetOnLoad();
    void _testCurrentPlanFileNameWritable();
    void _testPlanFileRenamed();
    void _testSaveWithCurrentName();
    void _testSaveWithCurrentNameNoFile();
    void _testResolvedPlanFileExists();
    void _testFileNamesClearedOnRemoveAll();
    void _testFileNamesClearedOnRemoveAllFromVehicle();
    void _testSaveUpdatesOriginalFileName();

private:
    enum DirtyScenario {
        UploadPreservesSaveDirtyTrue,
        UploadPreservesSaveDirtyFalse,
        UploadFalseOnPlanClear,
        UploadTrueWhenSaveTrue,
        UploadTrueOnNewPlanLoad,
        SaveToFilePreservesUploadDirtyTrue,
        SaveToFilePreservesUploadDirtyFalse,
        SaveFalseOnSuccessfulLoad,
        ClearSaveDirtyPreservesUploadTrue,
        ClearSaveDirtyPreservesUploadFalse,
        DownloadWithItemsDirtyForSave,
        DownloadEmptyNotDirtyForSave,
    };

    enum DirtyState {
        DirtyStateFalse,
        DirtyStateTrue,
        DirtyStateUnchanged
    };

    PlanMasterController* _masterController = nullptr;
};
