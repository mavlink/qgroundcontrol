#include "FirmwareUpgradeControllerTest.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtTest/QSignalSpy>

#include "FirmwareUpgradeController.h"
#include <QtCore/QTemporaryDir>

void FirmwareUpgradeControllerTest::_manifestCompleteErrorClearsDownloadingState()
{
    FirmwareUpgradeController controller;
    QSignalSpy spy(&controller, &FirmwareUpgradeController::downloadingFirmwareListChanged);
    QVERIFY(spy.isValid());

    controller._downloadingFirmwareList = true;
    expectLogMessage("Vehicle.VehicleSetup.FirmwareUpgradeController", QtWarningMsg, QRegularExpression("ArduPilot Manifest download failed"));
    controller._ardupilotManifestDownloadComplete(false, QString(), QStringLiteral("simulated error"));
    verifyExpectedLogMessage();

    QVERIFY(!controller._downloadingFirmwareList);
    QVERIFY(!spy.isEmpty());
    QCOMPARE(spy.last().at(0).toBool(), false);
}

void FirmwareUpgradeControllerTest::_manifestCompleteBadJsonClearsDownloadingState()
{
    QTemporaryDir tempDir;
    FirmwareUpgradeController controller;
    QSignalSpy spy(&controller, &FirmwareUpgradeController::downloadingFirmwareListChanged);
    QVERIFY(spy.isValid());

    const QString missingFile = tempDir.filePath(QStringLiteral("missing_manifest.json"));

    controller._downloadingFirmwareList = true;
    expectLogMessage("Vehicle.VehicleSetup.FirmwareUpgradeController", QtWarningMsg, QRegularExpression("Json file read failed"));
    controller._ardupilotManifestDownloadComplete(true, missingFile, QString());
    verifyExpectedLogMessage();

    QVERIFY(!controller._downloadingFirmwareList);
    QVERIFY(!spy.isEmpty());
    QCOMPARE(spy.last().at(0).toBool(), false);
}

void FirmwareUpgradeControllerTest::_manifestCompleteValidJsonPopulatesManifestInfo()
{
    QTemporaryDir tempDir;
    FirmwareUpgradeController controller;
    QSignalSpy spy(&controller, &FirmwareUpgradeController::downloadingFirmwareListChanged);
    QVERIFY(spy.isValid());

    const QString manifestPath = tempDir.filePath(QStringLiteral("manifest.json"));

    QFile manifestFile(manifestPath);
    QVERIFY(manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    QTextStream out(&manifestFile);
    out << "{\n";
    out << "  \"firmware\": [\n";
    out << "    {\n";
    out << "      \"board_id\": 42,\n";
    out << "      \"mav-type\": \"Copter\",\n";
    out << "      \"format\": \"apj\",\n";
    out << "      \"url\": \"https://example.com/fw.apj\",\n";
    out << "      \"mav-firmware-version-type\": \"OFFICIAL\",\n";
    out << "      \"USBID\": [\"0x1209/0x5740\"],\n";
    out << "      \"mav-firmware-version\": \"4.5.0\",\n";
    out << "      \"bootloader_str\": [\"BL42\"],\n";
    out << "      \"platform\": \"fmuv3\",\n";
    out << "      \"brand_name\": \"TestBrand\"\n";
    out << "    }\n";
    out << "  ]\n";
    out << "}\n";
    manifestFile.close();

    controller._bootloaderFound = false;
    controller._rgManifestFirmwareInfo.clear();
    controller._downloadingFirmwareList = true;
    controller._ardupilotManifestDownloadComplete(true, manifestPath, QString());

    QVERIFY(!controller._downloadingFirmwareList);
    QVERIFY(!spy.isEmpty());
    QCOMPARE(spy.last().at(0).toBool(), false);

    QCOMPARE(controller._rgManifestFirmwareInfo.size(), 1);
    const auto& info = controller._rgManifestFirmwareInfo.first();
    QCOMPARE(info.boardId, static_cast<uint32_t>(42));
    QCOMPARE(info.firmwareBuildType, FirmwareUpgradeController::StableFirmware);
    QCOMPARE(info.vehicleType, FirmwareUpgradeController::CopterFirmware);
    QCOMPARE(info.url, QStringLiteral("https://example.com/fw.apj"));
    QCOMPARE(info.version, QStringLiteral("4.5.0"));
    QVERIFY(info.chibios);
    QVERIFY(!info.fmuv2);
    QCOMPARE(info.rgBootloaderPortString.size(), 1);
    QCOMPARE(info.rgBootloaderPortString.first(), QStringLiteral("BL42"));
    QCOMPARE(info.rgVID.size(), 1);
    QCOMPARE(info.rgPID.size(), 1);
    QCOMPARE(info.rgVID.first(), 0x1209);
    QCOMPARE(info.rgPID.first(), 0x5740);
    QVERIFY(info.friendlyName.contains(QStringLiteral("TestBrand")));
}

void FirmwareUpgradeControllerTest::_px4ReleasesCompleteParsesStableAndBeta()
{
    QTemporaryDir tempDir;
    FirmwareUpgradeController controller;

    const QString releasesPath = tempDir.filePath(QStringLiteral("releases.json"));

    QFile releasesFile(releasesPath);
    QVERIFY(releasesFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    QTextStream out(&releasesFile);
    out << "[\n";
    out << "  {\"name\": \"v1.15.2\", \"prerelease\": false},\n";
    out << "  {\"name\": \"v1.16.0-beta1\", \"prerelease\": true}\n";
    out << "]\n";
    releasesFile.close();

    controller._px4StableVersion.clear();
    controller._px4BetaVersion.clear();
    controller._px4ReleasesGithubDownloadComplete(true, releasesPath, QString());

    QCOMPARE(controller._px4StableVersion, QStringLiteral("v1.15.2"));
    QCOMPARE(controller._px4BetaVersion, QStringLiteral("v1.16.0-beta1"));
}

void FirmwareUpgradeControllerTest::_px4ReleasesCompleteBadJsonKeepsPreviousVersions()
{
    QTemporaryDir tempDir;
    FirmwareUpgradeController controller;
    controller._px4StableVersion = QStringLiteral("old_stable");
    controller._px4BetaVersion = QStringLiteral("old_beta");

    const QString releasesPath = tempDir.filePath(QStringLiteral("releases_bad.json"));
    QFile releasesFile(releasesPath);
    QVERIFY(releasesFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    QTextStream out(&releasesFile);
    out << "{ invalid json\n";
    releasesFile.close();

    expectLogMessage("Vehicle.VehicleSetup.FirmwareUpgradeController", QtWarningMsg, QRegularExpression("Unable to open px4 releases json document"));
    controller._px4ReleasesGithubDownloadComplete(true, releasesPath, QString());
    verifyExpectedLogMessage();

    QCOMPARE(controller._px4StableVersion, QStringLiteral("old_stable"));
    QCOMPARE(controller._px4BetaVersion, QStringLiteral("old_beta"));
}

void FirmwareUpgradeControllerTest::_px4ReleasesCompleteNonArrayJsonKeepsPreviousVersions()
{
    QTemporaryDir tempDir;
    FirmwareUpgradeController controller;
    controller._px4StableVersion = QStringLiteral("old_stable");
    controller._px4BetaVersion = QStringLiteral("old_beta");

    const QString releasesPath = tempDir.filePath(QStringLiteral("releases_object.json"));
    QFile releasesFile(releasesPath);
    QVERIFY(releasesFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    QTextStream out(&releasesFile);
    out << "{ \"name\": \"v1.15.2\", \"prerelease\": false }\n";
    releasesFile.close();

    expectLogMessage("Vehicle.VehicleSetup.FirmwareUpgradeController", QtWarningMsg, QRegularExpression("px4 releases json document is not an array"));
    controller._px4ReleasesGithubDownloadComplete(true, releasesPath, QString());
    verifyExpectedLogMessage();

    QCOMPARE(controller._px4StableVersion, QStringLiteral("old_stable"));
    QCOMPARE(controller._px4BetaVersion, QStringLiteral("old_beta"));
}

void FirmwareUpgradeControllerTest::_px4ReleasesCompleteOnlyStableKeepsBetaEmpty()
{
    QTemporaryDir tempDir;
    FirmwareUpgradeController controller;

    const QString releasesPath = tempDir.filePath(QStringLiteral("releases_stable_only.json"));
    QFile releasesFile(releasesPath);
    QVERIFY(releasesFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    QTextStream out(&releasesFile);
    out << "[\n";
    out << "  {\"name\": \"v1.15.2\", \"prerelease\": false},\n";
    out << "  {\"name\": \"v1.14.0\", \"prerelease\": false}\n";
    out << "]\n";
    releasesFile.close();

    controller._px4StableVersion.clear();
    controller._px4BetaVersion.clear();
    controller._px4ReleasesGithubDownloadComplete(true, releasesPath, QString());

    QCOMPARE(controller._px4StableVersion, QStringLiteral("v1.15.2"));
    QVERIFY(controller._px4BetaVersion.isEmpty());
}

UT_REGISTER_TEST(FirmwareUpgradeControllerTest, TestLabel::Unit, TestLabel::Vehicle)
