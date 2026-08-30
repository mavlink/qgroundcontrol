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
    void _testTakeoffTextFileLoad_data();
    void _testTakeoffTextFileLoad();
    void _testActiveVehicleChanged();
    void _testDirtyFlagsMatrix_data();
    void _testDirtyFlagsMatrix();

    // File name property tests
    void _testFileAssociationSetOnLoad();
    void _testFileAssociationClearedOnRemoveAll();
    void _testFileAssociationClearedOnRemoveAllFromVehicle();
    void _testSaveUpdatesFileName();
    void _testFailedLoadClearsFileAssociation();
    void _testDownloadClearsFileAssociation();
    void _testBackgroundSyncPreservesFileAssociation();
    void _testUnrequestedSendCompletePreservesDirtyForUpload();
    void _testStaleInitialPlanLoadRallyCompletePreservesPlan();
    void _testShowPlanFromVehicleClearsFileAssociation();

    // showCreateFromTemplate tests — template selection mode
    void _testTemplateModeHidesTemplatesOnPlanCreatorSelection();
    void _testTemplateModeHidesTemplatesOnFileLoad();
    void _testTemplateModeRestoredOnRemoveAll();
    void _testTemplateModeRestoredOnIndividualItemRemoval();

    // showCreateFromTemplate tests — manual creation mode
    void _testManualCreationHidesTemplates();
    void _testManualCreationRestoredOnRemoveAll();
    void _testManualCreationRestoredOnIndividualItemRemoval();

    void _testPlanCreatorsFiltered();

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
        DownloadWithItemsNotDirtyForSave,
        DownloadEmptyNotDirtyForSave,
    };

    enum DirtyState {
        DirtyStateFalse,
        DirtyStateTrue,
        DirtyStateUnchanged
    };

    PlanMasterController* _masterController = nullptr;
};
