#pragma once

#include "UnitTest.h"

class PX4ManifestTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testParsePX4ManifestV2();
    void _testParsePX4ManifestInvalidVersion();
    void _testParsePX4ManifestMissingFields();
    void _testBuildFirmwareHashFromManifest();
    void _testBoardNotInManifest();
    void _testSha256MapPopulation();
    void _testBuildPX4FirmwareNames();
    void _testBootloaderFilteredOut();
    void _testDefaultBuildPreSelected();
    void _testFirmwareCategoryFilter();
    void _testDevModeShowsAllCategories();
    void _testLabelPrettyDisplayName();
    void _testMissingManifestObjectFallback();
    void _testHardwareInfoParsed();
    void _testBuildPX4AdvancedVersions();
    void _testBuildPX4AdvancedBuildNames();
    void _testPX4AdvancedSelectedChannel();
    void _testSetSelectedPX4AdvancedVersionByIndex();
};
