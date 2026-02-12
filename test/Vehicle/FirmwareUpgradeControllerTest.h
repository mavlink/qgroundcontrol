#pragma once

#include "UnitTest.h"

class FirmwareUpgradeControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _manifestCompleteErrorClearsDownloadingState();
    void _manifestCompleteBadJsonClearsDownloadingState();
    void _manifestCompleteValidJsonPopulatesManifestInfo();
    void _px4ReleasesCompleteParsesStableAndBeta();
};
