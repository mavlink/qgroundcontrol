#pragma once

#include "TempDirectoryTest.h"

class FirmwareUpgradeControllerTest : public TempDirectoryTest
{
    Q_OBJECT

private slots:
    void _manifestCompleteErrorClearsDownloadingState();
    void _manifestCompleteBadJsonClearsDownloadingState();
    void _manifestCompleteValidJsonPopulatesManifestInfo();
    void _px4ReleasesCompleteParsesStableAndBeta();
    void _px4ReleasesCompleteBadJsonKeepsPreviousVersions();
    void _px4ReleasesCompleteNonArrayJsonKeepsPreviousVersions();
    void _px4ReleasesCompleteOnlyStableKeepsBetaEmpty();
};
