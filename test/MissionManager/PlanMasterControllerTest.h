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
    };

    enum DirtyState {
        DirtyStateFalse,
        DirtyStateTrue,
        DirtyStateUnchanged
    };

    PlanMasterController* _masterController = nullptr;
};
