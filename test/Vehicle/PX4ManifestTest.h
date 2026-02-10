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
};
