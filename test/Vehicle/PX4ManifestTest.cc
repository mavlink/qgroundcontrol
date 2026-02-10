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
    QCOMPARE(stableRelease.builds.count(), 6);

    // Verify the _default build (second entry, after bootloader)
    const auto& build1 = stableRelease.builds[1];
    QCOMPARE(build1.boardId, static_cast<uint32_t>(50));
    QCOMPARE(build1.filename, QStringLiteral("px4_fmu-v5_default.px4"));
    QCOMPARE(build1.url, QStringLiteral("https://px4-travis.s3.amazonaws.com/Firmware/stable/px4_fmu-v5_default.px4"));
    QCOMPARE(build1.gitHash, QStringLiteral("abc123def456"));
    QCOMPARE(build1.sha256sum, QStringLiteral("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    QCOMPARE(build1.buildTime, static_cast<uint64_t>(1700000000));
    QCOMPARE(build1.imageSize, static_cast<uint64_t>(1048576));
    QCOMPARE(build1.mavAutopilot, 12);

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
             QStringLiteral("https://px4-travis.s3.amazonaws.com/Firmware/stable/px4_fmu-v5_bootloader.px4"));

    // Beta and dev only have _default builds for board 50
    QVERIFY(controller._rgFirmwareDynamic.contains(betaId));
    QCOMPARE(controller._rgFirmwareDynamic[betaId],
             QStringLiteral("https://px4-travis.s3.amazonaws.com/Firmware/beta/px4_fmu-v5_default.px4"));
    QVERIFY(controller._rgFirmwareDynamic.contains(devId));
    QCOMPARE(controller._rgFirmwareDynamic[devId],
             QStringLiteral("https://px4-travis.s3.amazonaws.com/Firmware/master/px4_fmu-v5_default.px4"));
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

UT_REGISTER_TEST(PX4ManifestTest, TestLabel::Unit, TestLabel::Vehicle)
