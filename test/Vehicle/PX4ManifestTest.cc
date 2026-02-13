#include "PX4ManifestTest.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "FirmwareUpgradeController.h"

void PX4ManifestTest::_testParsePX4ManifestV2()
{
    FirmwareUpgradeController controller;

    // Load test fixture
    QFile file(QStringLiteral(":/Vehicle/px4_manifest_v2.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(!doc.isNull());

    // Parse
    QVERIFY(controller._parsePX4Manifest(doc));

    // Verify latest version pointers
    QCOMPARE(controller._px4ManifestLatestStable, QStringLiteral("v1.15.2"));
    QCOMPARE(controller._px4ManifestLatestBeta, QStringLiteral("v1.16.0-beta1"));
    QCOMPARE(controller._px4ManifestLatestDev, QStringLiteral("v1.16.0-dev"));

    // Verify version display strings
    QCOMPARE(controller._px4StableVersion, QStringLiteral("v1.15.2"));
    QCOMPARE(controller._px4BetaVersion, QStringLiteral("v1.16.0-beta1"));
    QCOMPARE(controller._px4DevVersion, QStringLiteral("v1.16.0-dev"));

    // Verify releases parsed
    QCOMPARE(controller._px4ManifestReleases.count(), 3);
    QVERIFY(controller._px4ManifestReleases.contains(QStringLiteral("v1.15.2")));
    QVERIFY(controller._px4ManifestReleases.contains(QStringLiteral("v1.16.0-beta1")));
    QVERIFY(controller._px4ManifestReleases.contains(QStringLiteral("v1.16.0-dev")));

    // Verify stable release builds (now includes bootloader + variant builds)
    const auto& stableRelease = controller._px4ManifestReleases[QStringLiteral("v1.15.2")];
    QCOMPARE(stableRelease.gitTag, QStringLiteral("v1.15.2"));
    QCOMPARE(stableRelease.releaseDate, QStringLiteral("2024-11-20"));
    QCOMPARE(stableRelease.channel, QStringLiteral("stable"));
    QCOMPARE(stableRelease.builds.count(), 7);

    // Verify the Default build (second entry, after bootloader)
    const auto& build1 = stableRelease.builds[1];
    QCOMPARE(build1.boardId, static_cast<uint32_t>(50));
    QCOMPARE(build1.filename, QStringLiteral("px4_fmu-v5_default.px4"));
    QCOMPARE(build1.url, QStringLiteral("https://artifacts.px4.io/Firmware/stable/px4_fmu-v5_allinone.px4"));
    QCOMPARE(build1.gitHash, QStringLiteral("abc123def456"));
    QCOMPARE(build1.sha256sum, QStringLiteral("aa11bb22cc33dd44ee55ff66aa11bb22cc33dd44ee55ff66aa11bb22cc33dd44"));
    QCOMPARE(build1.buildTime, static_cast<uint64_t>(1700000000));
    QCOMPARE(build1.imageSize, static_cast<uint64_t>(1200000));
    QCOMPARE(build1.mavAutopilot, 12);

    // Verify nested manifest object fields
    QCOMPARE(build1.manifestName, QStringLiteral("PX4_FMU_V5"));
    QCOMPARE(build1.manifestTarget, QStringLiteral("px4_fmu-v5_default"));
    QCOMPARE(build1.labelPretty, QStringLiteral("Default"));
    QCOMPARE(build1.firmwareCategory, QStringLiteral("dev"));
    QCOMPARE(build1.manufacturer, QStringLiteral("Holybro"));

    // Verify hardware sub-object
    QCOMPARE(build1.hardware.architecture, QStringLiteral("arm"));
    QCOMPARE(build1.hardware.vendorId, QStringLiteral("0x26ac"));
    QCOMPARE(build1.hardware.productId, QStringLiteral("0x0032"));
    QCOMPARE(build1.hardware.chip, QStringLiteral("stm32f7"));
    QCOMPARE(build1.hardware.productStr, QStringLiteral("PX4 BL FMU v5.x"));

    // Verify build WITHOUT manifest object has empty fields (board 9, index 6)
    const auto& buildNoManifest = stableRelease.builds[6];
    QCOMPARE(buildNoManifest.boardId, static_cast<uint32_t>(9));
    QVERIFY(buildNoManifest.manifestName.isEmpty());
    QVERIFY(buildNoManifest.labelPretty.isEmpty());
    QVERIFY(buildNoManifest.firmwareCategory.isEmpty());
    QVERIFY(buildNoManifest.hardware.vendorId.isEmpty());

    // Verify available versions list
    QCOMPARE(controller._px4AvailableVersions.count(), 3);
    // Should be sorted in reverse order
    QCOMPARE(controller._px4AvailableVersions[0], QStringLiteral("v1.16.0-dev"));
}

void PX4ManifestTest::_testParsePX4ManifestInvalidVersion()
{
    FirmwareUpgradeController controller;

    // Create manifest with wrong format_version
    QJsonObject json;
    json["format_version"] = 1;
    json["latest_stable"] = "v1.0.0";
    json["releases"] = QJsonObject();
    QJsonDocument doc(json);

    QVERIFY(!controller._parsePX4Manifest(doc));
    QVERIFY(controller._px4ManifestReleases.isEmpty());
}

void PX4ManifestTest::_testParsePX4ManifestMissingFields()
{
    FirmwareUpgradeController controller;

    // Create manifest with format_version but missing other fields
    QJsonObject json;
    json["format_version"] = 2;
    // No latest_stable, latest_beta, latest_dev, or releases
    QJsonDocument doc(json);

    // Should still parse without crashing, just with empty data
    QVERIFY(controller._parsePX4Manifest(doc));
    QVERIFY(controller._px4ManifestLatestStable.isEmpty());
    QVERIFY(controller._px4ManifestLatestBeta.isEmpty());
    QVERIFY(controller._px4ManifestLatestDev.isEmpty());
    QVERIFY(controller._px4ManifestReleases.isEmpty());
}

void PX4ManifestTest::_testBuildFirmwareHashFromManifest()
{
    FirmwareUpgradeController controller;

    // Load and parse test fixture
    QFile file(QStringLiteral(":/Vehicle/px4_manifest_v2.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(controller._parsePX4Manifest(doc));

    // Build firmware hash for board ID 50 (fmu-v5)
    // Note: _buildPX4FirmwareHashFromManifest picks the FIRST matching build per board,
    // which is the bootloader in the updated fixture
    controller._buildPX4FirmwareHashFromManifest(50);

    FirmwareUpgradeController::FirmwareIdentifier stableId(FirmwareUpgradeController::AutoPilotStackPX4,
                                                           FirmwareUpgradeController::StableFirmware,
                                                           FirmwareUpgradeController::DefaultVehicleFirmware);

    FirmwareUpgradeController::FirmwareIdentifier betaId(FirmwareUpgradeController::AutoPilotStackPX4,
                                                         FirmwareUpgradeController::BetaFirmware,
                                                         FirmwareUpgradeController::DefaultVehicleFirmware);

    FirmwareUpgradeController::FirmwareIdentifier devId(FirmwareUpgradeController::AutoPilotStackPX4,
                                                        FirmwareUpgradeController::DeveloperFirmware,
                                                        FirmwareUpgradeController::DefaultVehicleFirmware);

    // Stable picks first match for board 50 (bootloader in this fixture)
    QVERIFY(controller._rgFirmwareDynamic.contains(stableId));
    QCOMPARE(controller._rgFirmwareDynamic[stableId],
             QStringLiteral("https://artifacts.px4.io/Firmware/stable/px4_fmu-v5_bootloader.px4"));

    // Beta and dev only have _default builds for board 50
    QVERIFY(controller._rgFirmwareDynamic.contains(betaId));
    QCOMPARE(controller._rgFirmwareDynamic[betaId],
             QStringLiteral("https://artifacts.px4.io/Firmware/beta/px4_fmu-v5_default.px4"));
    QVERIFY(controller._rgFirmwareDynamic.contains(devId));
    QCOMPARE(controller._rgFirmwareDynamic[devId],
             QStringLiteral("https://artifacts.px4.io/Firmware/master/px4_fmu-v5_default.px4"));
}

void PX4ManifestTest::_testBoardNotInManifest()
{
    FirmwareUpgradeController controller;

    // Load and parse test fixture
    QFile file(QStringLiteral(":/Vehicle/px4_manifest_v2.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(controller._parsePX4Manifest(doc));

    // Build firmware hash for a board ID that doesn't exist in the manifest
    controller._rgFirmwareDynamic.clear();
    controller._buildPX4FirmwareHashFromManifest(99999);

    // No entries should be inserted
    FirmwareUpgradeController::FirmwareIdentifier stableId(FirmwareUpgradeController::AutoPilotStackPX4,
                                                           FirmwareUpgradeController::StableFirmware,
                                                           FirmwareUpgradeController::DefaultVehicleFirmware);

    QVERIFY(!controller._rgFirmwareDynamic.contains(stableId));
}

void PX4ManifestTest::_testSha256MapPopulation()
{
    FirmwareUpgradeController controller;

    // Load and parse test fixture
    QFile file(QStringLiteral(":/Vehicle/px4_manifest_v2.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(controller._parsePX4Manifest(doc));

    // Use _buildPX4FirmwareNames for board 50 to populate the sha256 map
    controller._bootloaderFound = true;
    controller._bootloaderBoardID = 50;
    controller._px4ManifestLoaded = true;
    controller._selectedFirmwareBuildType = FirmwareUpgradeController::StableFirmware;
    controller._buildPX4FirmwareNames();

    // Verify SHA-256 map is populated for URLs with non-empty sha256sum
    // Default build (dev category, allowed by label_pretty)
    QVERIFY(controller._px4FirmwareSha256Map.contains(
        QStringLiteral("https://artifacts.px4.io/Firmware/stable/px4_fmu-v5_allinone.px4")));
    QCOMPARE(
        controller
            ._px4FirmwareSha256Map[QStringLiteral("https://artifacts.px4.io/Firmware/stable/px4_fmu-v5_allinone.px4")],
        QStringLiteral("aa11bb22cc33dd44ee55ff66aa11bb22cc33dd44ee55ff66aa11bb22cc33dd44"));

    // Multicopter build
    QVERIFY(controller._px4FirmwareSha256Map.contains(
        QStringLiteral("https://artifacts.px4.io/Firmware/stable/px4_fmu-v5_default.px4")));
    QCOMPARE(
        controller
            ._px4FirmwareSha256Map[QStringLiteral("https://artifacts.px4.io/Firmware/stable/px4_fmu-v5_default.px4")],
        QStringLiteral("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));

    // Rover build also has sha256sum
    QVERIFY(controller._px4FirmwareSha256Map.contains(
        QStringLiteral("https://artifacts.px4.io/Firmware/stable/px4_fmu-v5_rover.px4")));

    // Bootloader builds are filtered out by firmware_category, so their URLs should NOT be in the map
    QVERIFY(!controller._px4FirmwareSha256Map.contains(
        QStringLiteral("https://artifacts.px4.io/Firmware/stable/px4_fmu-v5_bootloader.px4")));

    // Board 9 stable has empty sha256sum - should not be in the map
    controller._bootloaderBoardID = 9;
    controller._buildPX4FirmwareNames();
    QVERIFY(!controller._px4FirmwareSha256Map.contains(
        QStringLiteral("https://artifacts.px4.io/Firmware/stable/px4_fmu-v2_default.px4")));
}

void PX4ManifestTest::_testBuildPX4FirmwareNames()
{
    FirmwareUpgradeController controller;

    // Load and parse test fixture
    QFile file(QStringLiteral(":/Vehicle/px4_manifest_v2.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(controller._parsePX4Manifest(doc));

    // Simulate bootloader found for board 50
    controller._bootloaderFound = true;
    controller._bootloaderBoardID = 50;
    controller._px4ManifestLoaded = true;
    controller._selectedFirmwareBuildType = FirmwareUpgradeController::StableFirmware;

    controller._buildPX4FirmwareNames();

    // Board 50 stable has 5 builds: bootloader(bootloader), default(dev/Default), multicopter(vehicle),
    // rover(vehicle), canbootloader(canbootloader). After filtering: Default passes via label_pretty,
    // vehicle builds pass, bootloader/canbootloader filtered out — should have 3: Default, Multicopter, Rover
    QCOMPARE(controller._px4FirmwareNames.count(), 3);
    QCOMPARE(controller._px4FirmwareUrls.count(), 3);

    // Verify friendly names use label_pretty
    QCOMPARE(controller._px4FirmwareNames[0], QStringLiteral("Default (v1.15.2)"));
    QCOMPARE(controller._px4FirmwareNames[1], QStringLiteral("Multicopter (v1.15.2)"));
    QCOMPARE(controller._px4FirmwareNames[2], QStringLiteral("Rover (v1.15.2)"));

    // Verify URLs
    QCOMPARE(controller._px4FirmwareUrls[0],
             QStringLiteral("https://artifacts.px4.io/Firmware/stable/px4_fmu-v5_allinone.px4"));
    QCOMPARE(controller._px4FirmwareUrls[1],
             QStringLiteral("https://artifacts.px4.io/Firmware/stable/px4_fmu-v5_default.px4"));
    QCOMPARE(controller._px4FirmwareUrls[2],
             QStringLiteral("https://artifacts.px4.io/Firmware/stable/px4_fmu-v5_rover.px4"));

    // Switch to beta and verify the list updates
    controller._selectedFirmwareBuildType = FirmwareUpgradeController::BetaFirmware;
    controller._buildPX4FirmwareNames();

    QCOMPARE(controller._px4FirmwareNames.count(), 1);
    QCOMPARE(controller._px4FirmwareNames[0], QStringLiteral("Multicopter (v1.16.0-beta1)"));
    QCOMPARE(controller._px4FirmwareUrls[0],
             QStringLiteral("https://artifacts.px4.io/Firmware/beta/px4_fmu-v5_default.px4"));

    // Board with no builds should produce empty lists
    controller._bootloaderBoardID = 99999;
    controller._selectedFirmwareBuildType = FirmwareUpgradeController::StableFirmware;
    controller._buildPX4FirmwareNames();
    QCOMPARE(controller._px4FirmwareNames.count(), 0);
    QCOMPARE(controller._px4FirmwareUrls.count(), 0);
}

void PX4ManifestTest::_testBootloaderFilteredOut()
{
    FirmwareUpgradeController controller;

    // Load and parse test fixture
    QFile file(QStringLiteral(":/Vehicle/px4_manifest_v2.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(controller._parsePX4Manifest(doc));

    // Board 50 stable has: bootloader(bootloader), default(dev/Default), multicopter(vehicle), rover(vehicle),
    // canbootloader(canbootloader)
    controller._bootloaderFound = true;
    controller._bootloaderBoardID = 50;
    controller._px4ManifestLoaded = true;
    controller._selectedFirmwareBuildType = FirmwareUpgradeController::StableFirmware;

    controller._buildPX4FirmwareNames();

    // Verify no bootloader entries in the names list
    for (const QString& name : controller._px4FirmwareNames) {
        QVERIFY2(!name.contains(QStringLiteral("bootloader"), Qt::CaseInsensitive),
                 qPrintable(QStringLiteral("Bootloader build should be filtered: %1").arg(name)));
    }

    // Verify no bootloader entries in the urls list
    for (const QString& url : controller._px4FirmwareUrls) {
        QVERIFY2(!url.contains(QStringLiteral("bootloader"), Qt::CaseInsensitive),
                 qPrintable(QStringLiteral("Bootloader URL should be filtered: %1").arg(url)));
    }
}

void PX4ManifestTest::_testDefaultBuildPreSelected()
{
    FirmwareUpgradeController controller;

    // Load and parse test fixture
    QFile file(QStringLiteral(":/Vehicle/px4_manifest_v2.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(controller._parsePX4Manifest(doc));

    // Board 50 stable: after filtering, builds are [Default, Multicopter, Rover]
    controller._bootloaderFound = true;
    controller._bootloaderBoardID = 50;
    controller._px4ManifestLoaded = true;
    controller._selectedFirmwareBuildType = FirmwareUpgradeController::StableFirmware;

    controller._buildPX4FirmwareNames();

    // "Default" label should be pre-selected (index 0 in filtered list)
    QCOMPARE(controller._px4FirmwareNamesBestIndex, 0);
    QVERIFY(controller._px4FirmwareNames[controller._px4FirmwareNamesBestIndex].contains(QStringLiteral("Default")));

    // Board 9 stable: no manifest object, falls back to filename-based name
    controller._bootloaderBoardID = 9;
    controller._buildPX4FirmwareNames();

    QCOMPARE(controller._px4FirmwareNamesBestIndex, 0);
    QCOMPARE(controller._px4FirmwareNames[0], QStringLiteral("px4_fmu-v2_default (v1.15.2)"));
}

void PX4ManifestTest::_testFirmwareCategoryFilter()
{
    FirmwareUpgradeController controller;

    QFile file(QStringLiteral(":/Vehicle/px4_manifest_v2.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(controller._parsePX4Manifest(doc));

    controller._bootloaderFound = true;
    controller._bootloaderBoardID = 50;
    controller._px4ManifestLoaded = true;
    controller._selectedFirmwareBuildType = FirmwareUpgradeController::StableFirmware;
    controller._buildPX4FirmwareNames();

    // Board 50 stable: 5 builds total, 2 vehicle + 1 Default (dev category, allowed by label_pretty) pass
    QCOMPARE(controller._px4FirmwareNames.count(), 3);

    // Verify all displayed URLs are vehicle firmware (no bootloader/canbootloader)
    for (const QString& url : controller._px4FirmwareUrls) {
        QVERIFY2(!url.contains(QStringLiteral("bootloader")),
                 qPrintable(QStringLiteral("Non-vehicle build should be filtered: %1").arg(url)));
    }
}

void PX4ManifestTest::_testDevModeShowsAllCategories()
{
    FirmwareUpgradeController controller;

    QFile file(QStringLiteral(":/Vehicle/px4_manifest_v2.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(controller._parsePX4Manifest(doc));

    controller._bootloaderFound = true;
    controller._bootloaderBoardID = 50;
    controller._px4ManifestLoaded = true;

    // Developer mode picks v1.16.0-dev which has 2 board-50 builds: default(vehicle) + bootloader(bootloader)
    controller._selectedFirmwareBuildType = FirmwareUpgradeController::DeveloperFirmware;
    controller._buildPX4FirmwareNames();

    // Dev mode should show ALL categories — both vehicle and bootloader
    QCOMPARE(controller._px4FirmwareNames.count(), 2);

    // Verify bootloader is included in dev mode
    bool foundBootloader = false;
    for (const QString& url : controller._px4FirmwareUrls) {
        if (url.contains(QStringLiteral("bootloader"))) {
            foundBootloader = true;
            break;
        }
    }
    QVERIFY2(foundBootloader, "Dev mode should include bootloader builds");
}

void PX4ManifestTest::_testLabelPrettyDisplayName()
{
    FirmwareUpgradeController controller;

    QFile file(QStringLiteral(":/Vehicle/px4_manifest_v2.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(controller._parsePX4Manifest(doc));

    controller._bootloaderFound = true;
    controller._bootloaderBoardID = 50;
    controller._px4ManifestLoaded = true;
    controller._selectedFirmwareBuildType = FirmwareUpgradeController::StableFirmware;
    controller._buildPX4FirmwareNames();

    // Builds with label_pretty should use it for display
    QCOMPARE(controller._px4FirmwareNames[0], QStringLiteral("Default (v1.15.2)"));
    QCOMPARE(controller._px4FirmwareNames[1], QStringLiteral("Multicopter (v1.15.2)"));
    QCOMPARE(controller._px4FirmwareNames[2], QStringLiteral("Rover (v1.15.2)"));

    // Board 9 has no manifest object — should fall back to filename-based name
    controller._bootloaderBoardID = 9;
    controller._buildPX4FirmwareNames();

    QCOMPARE(controller._px4FirmwareNames.count(), 1);
    QCOMPARE(controller._px4FirmwareNames[0], QStringLiteral("px4_fmu-v2_default (v1.15.2)"));
}

void PX4ManifestTest::_testMissingManifestObjectFallback()
{
    FirmwareUpgradeController controller;

    QFile file(QStringLiteral(":/Vehicle/px4_manifest_v2.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(controller._parsePX4Manifest(doc));

    // Board 9 has NO manifest object — should use filename-based filtering and display
    controller._bootloaderFound = true;
    controller._bootloaderBoardID = 9;
    controller._px4ManifestLoaded = true;
    controller._selectedFirmwareBuildType = FirmwareUpgradeController::StableFirmware;
    controller._buildPX4FirmwareNames();

    QCOMPARE(controller._px4FirmwareNames.count(), 1);
    // Falls back to filename-based name
    QCOMPARE(controller._px4FirmwareNames[0], QStringLiteral("px4_fmu-v2_default (v1.15.2)"));
    QCOMPARE(controller._px4FirmwareUrls[0],
             QStringLiteral("https://artifacts.px4.io/Firmware/stable/px4_fmu-v2_default.px4"));
}

void PX4ManifestTest::_testHardwareInfoParsed()
{
    FirmwareUpgradeController controller;

    QFile file(QStringLiteral(":/Vehicle/px4_manifest_v2.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(controller._parsePX4Manifest(doc));

    const auto& stableRelease = controller._px4ManifestReleases[QStringLiteral("v1.15.2")];

    // The Default build (index 1) has a manifest with hardware info
    const auto& build = stableRelease.builds[1];
    QCOMPARE(build.hardware.architecture, QStringLiteral("arm"));
    QCOMPARE(build.hardware.vendorId, QStringLiteral("0x26ac"));
    QCOMPARE(build.hardware.productId, QStringLiteral("0x0032"));
    QCOMPARE(build.hardware.chip, QStringLiteral("stm32f7"));
    QCOMPARE(build.hardware.productStr, QStringLiteral("PX4 BL FMU v5.x"));

    // Board 53 build (index 5) has different hardware
    const auto& build53 = stableRelease.builds[5];
    QCOMPARE(build53.hardware.vendorId, QStringLiteral("0x3185"));
    QCOMPARE(build53.hardware.productId, QStringLiteral("0x0035"));
    QCOMPARE(build53.hardware.chip, QStringLiteral("stm32h7"));

    // Board 9 build (index 6, no manifest object) should have empty hardware fields
    const auto& buildNoHw = stableRelease.builds[6];
    QVERIFY(buildNoHw.hardware.architecture.isEmpty());
    QVERIFY(buildNoHw.hardware.vendorId.isEmpty());
    QVERIFY(buildNoHw.hardware.productId.isEmpty());
    QVERIFY(buildNoHw.hardware.chip.isEmpty());
    QVERIFY(buildNoHw.hardware.productStr.isEmpty());
}

void PX4ManifestTest::_testBuildPX4AdvancedVersions()
{
    FirmwareUpgradeController controller;

    QFile file(QStringLiteral(":/Vehicle/px4_manifest_v2.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(controller._parsePX4Manifest(doc));

    // Board 50: all 3 versions have board 50 builds
    controller._bootloaderFound = true;
    controller._bootloaderBoardID = 50;
    controller._px4ManifestLoaded = true;
    controller._buildPX4AdvancedVersions();

    QCOMPARE(controller._px4AdvancedVersions.count(), 3);
    QCOMPARE(controller._px4AdvancedVersionTags.count(), 3);

    // Versions should be sorted descending (matching _px4AvailableVersions order)
    QCOMPARE(controller._px4AdvancedVersionTags[0], QStringLiteral("v1.16.0-dev"));
    QCOMPARE(controller._px4AdvancedVersionTags[1], QStringLiteral("v1.16.0-beta1"));
    QCOMPARE(controller._px4AdvancedVersionTags[2], QStringLiteral("v1.15.2"));

    // Display names should include channel annotation
    QCOMPARE(controller._px4AdvancedVersions[0], QStringLiteral("v1.16.0-dev (dev)"));
    QCOMPARE(controller._px4AdvancedVersions[1], QStringLiteral("v1.16.0-beta1 (beta)"));
    QCOMPARE(controller._px4AdvancedVersions[2], QStringLiteral("v1.15.2 (stable)"));

    // Latest stable should be pre-selected (index 2)
    QCOMPARE(controller._px4AdvancedVersionsBestIndex, 2);
    QCOMPARE(controller._selectedPX4AdvancedVersion, QStringLiteral("v1.15.2"));

    // Board 53: stable + beta have board 53 builds, dev does not
    controller._bootloaderBoardID = 53;
    controller._buildPX4AdvancedVersions();

    QCOMPARE(controller._px4AdvancedVersions.count(), 2);
    QCOMPARE(controller._px4AdvancedVersionTags[0], QStringLiteral("v1.16.0-beta1"));
    QCOMPARE(controller._px4AdvancedVersionTags[1], QStringLiteral("v1.15.2"));
    QCOMPARE(controller._px4AdvancedVersionsBestIndex, 1);  // stable at index 1

    // Board 9: only stable has board 9 builds
    controller._bootloaderBoardID = 9;
    controller._buildPX4AdvancedVersions();

    QCOMPARE(controller._px4AdvancedVersions.count(), 1);
    QCOMPARE(controller._px4AdvancedVersionTags[0], QStringLiteral("v1.15.2"));
    QCOMPARE(controller._px4AdvancedVersionsBestIndex, 0);

    // Board 99999: no versions
    controller._bootloaderBoardID = 99999;
    controller._buildPX4AdvancedVersions();

    QCOMPARE(controller._px4AdvancedVersions.count(), 0);
    QCOMPARE(controller._px4AdvancedVersionTags.count(), 0);
}

void PX4ManifestTest::_testBuildPX4AdvancedBuildNames()
{
    FirmwareUpgradeController controller;

    QFile file(QStringLiteral(":/Vehicle/px4_manifest_v2.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(controller._parsePX4Manifest(doc));

    controller._bootloaderFound = true;
    controller._bootloaderBoardID = 50;
    controller._px4ManifestLoaded = true;

    // Board 50 + stable: Default, Multicopter, Rover (no version in name)
    controller._selectedPX4AdvancedVersion = QStringLiteral("v1.15.2");
    controller._buildPX4AdvancedBuildNames();

    QCOMPARE(controller._px4AdvancedBuildNames.count(), 3);
    QCOMPARE(controller._px4AdvancedBuildNames[0], QStringLiteral("Default"));
    QCOMPARE(controller._px4AdvancedBuildNames[1], QStringLiteral("Multicopter"));
    QCOMPARE(controller._px4AdvancedBuildNames[2], QStringLiteral("Rover"));

    // Names should NOT include version tag
    for (const QString& name : controller._px4AdvancedBuildNames) {
        QVERIFY2(!name.contains(QStringLiteral("v1.15.2")),
                 qPrintable(QStringLiteral("Build name should not contain version: %1").arg(name)));
    }

    // Default should be pre-selected
    QCOMPARE(controller._px4AdvancedBuildNamesBestIndex, 0);

    // URLs should match
    QCOMPARE(controller._px4AdvancedBuildUrls[0],
             QStringLiteral("https://artifacts.px4.io/Firmware/stable/px4_fmu-v5_allinone.px4"));
    QCOMPARE(controller._px4AdvancedBuildUrls[1],
             QStringLiteral("https://artifacts.px4.io/Firmware/stable/px4_fmu-v5_default.px4"));

    // Board 50 + dev: shows all categories including bootloader
    controller._selectedPX4AdvancedVersion = QStringLiteral("v1.16.0-dev");
    controller._buildPX4AdvancedBuildNames();

    QCOMPARE(controller._px4AdvancedBuildNames.count(), 2);
    // Should include both Multicopter and Bootloader in dev mode
    bool foundMulticopter = false;
    bool foundBootloader = false;
    for (const QString& name : controller._px4AdvancedBuildNames) {
        if (name == QStringLiteral("Multicopter"))
            foundMulticopter = true;
        if (name == QStringLiteral("Bootloader"))
            foundBootloader = true;
    }
    QVERIFY(foundMulticopter);
    QVERIFY(foundBootloader);
}

void PX4ManifestTest::_testPX4AdvancedSelectedChannel()
{
    FirmwareUpgradeController controller;

    QFile file(QStringLiteral(":/Vehicle/px4_manifest_v2.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(controller._parsePX4Manifest(doc));

    // No version selected — empty channel
    controller._selectedPX4AdvancedVersion.clear();
    QVERIFY(controller.px4AdvancedSelectedChannel().isEmpty());

    // Stable version
    controller._selectedPX4AdvancedVersion = QStringLiteral("v1.15.2");
    QCOMPARE(controller.px4AdvancedSelectedChannel(), QStringLiteral("stable"));

    // Beta version
    controller._selectedPX4AdvancedVersion = QStringLiteral("v1.16.0-beta1");
    QCOMPARE(controller.px4AdvancedSelectedChannel(), QStringLiteral("beta"));

    // Dev version
    controller._selectedPX4AdvancedVersion = QStringLiteral("v1.16.0-dev");
    QCOMPARE(controller.px4AdvancedSelectedChannel(), QStringLiteral("dev"));

    // Non-existent version
    controller._selectedPX4AdvancedVersion = QStringLiteral("v99.99.99");
    QVERIFY(controller.px4AdvancedSelectedChannel().isEmpty());
}

void PX4ManifestTest::_testSetSelectedPX4AdvancedVersionByIndex()
{
    FirmwareUpgradeController controller;

    QFile file(QStringLiteral(":/Vehicle/px4_manifest_v2.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(controller._parsePX4Manifest(doc));

    controller._bootloaderFound = true;
    controller._bootloaderBoardID = 50;
    controller._px4ManifestLoaded = true;
    controller._buildPX4AdvancedVersions();

    // Pre-selected is stable (index 2)
    QCOMPARE(controller._selectedPX4AdvancedVersion, QStringLiteral("v1.15.2"));
    QCOMPARE(controller._px4AdvancedBuildNames.count(), 3);  // Default, Multicopter, Rover

    // Switch to dev (index 0)
    controller.setSelectedPX4AdvancedVersionByIndex(0);
    QCOMPARE(controller._selectedPX4AdvancedVersion, QStringLiteral("v1.16.0-dev"));
    QCOMPARE(controller._px4AdvancedBuildNames.count(), 2);  // Multicopter, Bootloader

    // Switch to beta (index 1)
    controller.setSelectedPX4AdvancedVersionByIndex(1);
    QCOMPARE(controller._selectedPX4AdvancedVersion, QStringLiteral("v1.16.0-beta1"));
    QCOMPARE(controller._px4AdvancedBuildNames.count(), 1);  // Multicopter

    // Invalid index: negative — no-op
    controller.setSelectedPX4AdvancedVersionByIndex(-1);
    QCOMPARE(controller._selectedPX4AdvancedVersion, QStringLiteral("v1.16.0-beta1"));  // unchanged

    // Invalid index: out of range — no-op
    controller.setSelectedPX4AdvancedVersionByIndex(99);
    QCOMPARE(controller._selectedPX4AdvancedVersion, QStringLiteral("v1.16.0-beta1"));  // unchanged
}

UT_REGISTER_TEST(PX4ManifestTest, TestLabel::Unit, TestLabel::Vehicle)
