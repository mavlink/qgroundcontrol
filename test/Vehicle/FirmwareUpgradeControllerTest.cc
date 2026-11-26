#include "FirmwareUpgradeControllerTest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtCore/QThreadPool>
#include <QtTest/QSignalSpy>

#include "FileFixtures.h"
#include "FirmwareImage.h"
#include "FirmwareUpgradeController.h"
#include "QGCFileDownload.h"

namespace {

/// An ArduPilot firmware manifest with one firmware, for the board @a boardId
QByteArray arduPilotManifest(int boardId)
{
    return QStringLiteral(R"({
  "firmware": [
    {
      "board_id": %1,
      "mav-type": "Copter",
      "format": "apj",
      "url": "https://example.com/fw.apj",
      "mav-firmware-version-type": "OFFICIAL",
      "USBID": ["0x1209/0x5740"],
      "mav-firmware-version": "4.5.0",
      "bootloader_str": ["BL42"],
      "platform": "fmuv3",
      "brand_name": "TestBrand"
    }
  ]
}
)")
        .arg(boardId)
        .toUtf8();
}

/// Stops the download of the manifest that the controller starts, so that only the test hands it one
void stopManifestDownload(FirmwareUpgradeController& controller)
{
    for (QGCFileDownload* const download : controller.findChildren<QGCFileDownload*>()) {
        (void) QObject::disconnect(download, nullptr, &controller, nullptr);
        delete download;
    }
}

}  // namespace

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

    stopManifestDownload(controller);
    controller._downloadingFirmwareList = true;
    expectLogMessage("Vehicle.VehicleSetup.FirmwareUpgradeController", QtWarningMsg, QRegularExpression("Json file read failed"));
    controller._ardupilotManifestDownloadComplete(true, missingFile, QString());
    QTRY_VERIFY_WITH_TIMEOUT(!controller._downloadingFirmwareList, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

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
    QVERIFY(TestFixtures::writeFile(manifestPath, arduPilotManifest(42)));

    stopManifestDownload(controller);
    controller._bootloaderFound = false;
    controller._rgManifestFirmwareInfo.clear();
    controller._downloadingFirmwareList = true;
    controller._ardupilotManifestDownloadComplete(true, manifestPath, QString());

    // The manifest is read on a worker, not on the thread that handles the download
    QVERIFY(controller._downloadingFirmwareList);
    QTRY_VERIFY_WITH_TIMEOUT(!controller._downloadingFirmwareList, TestTimeout::mediumMs());
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

void FirmwareUpgradeControllerTest::_manifestReadAgainReplacesEarlierResult()
{
    // A manifest that takes longer to read than the next one: its result, which comes last, is no longer wanted
    QTemporaryDir tempDir;
    const QString earlierPath = tempDir.filePath(QStringLiteral("earlier.json"));
    QByteArray earlier = arduPilotManifest(41);
    earlier.append(QByteArray(32 * 1024 * 1024, ' '));
    QVERIFY(TestFixtures::writeFile(earlierPath, earlier));
    const QString laterPath = tempDir.filePath(QStringLiteral("later.json"));
    QVERIFY(TestFixtures::writeFile(laterPath, arduPilotManifest(43)));

    FirmwareUpgradeController controller;
    stopManifestDownload(controller);
    controller._rgManifestFirmwareInfo.clear();
    controller._downloadingFirmwareList = true;
    controller._ardupilotManifestDownloadComplete(true, earlierPath, QString());
    controller._ardupilotManifestDownloadComplete(true, laterPath, QString());
    QTRY_VERIFY_WITH_TIMEOUT(!controller._downloadingFirmwareList, TestTimeout::mediumMs());

    QCOMPARE(controller._rgManifestFirmwareInfo.size(), 1);
    QCOMPARE(controller._rgManifestFirmwareInfo.first().boardId, static_cast<uint32_t>(43));

    // The earlier read ends later, and its result is not taken
    QVERIFY(QThreadPool::globalInstance()->waitForDone(TestTimeout::mediumMs()));
    QCoreApplication::processEvents();
    QCOMPARE(controller._rgManifestFirmwareInfo.first().boardId, static_cast<uint32_t>(43));
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

void FirmwareUpgradeControllerTest::_binImageSizeMustBeMultipleOf4()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const auto writeImage = [&tempDir](const QString& name, qsizetype size) {
        QFile file(tempDir.filePath(name));
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QByteArray(size, '\xff'));
        }
        return file.fileName();
    };

    FirmwareImage image;
    QSignalSpy statusSpy(&image, &FirmwareImage::statusMessage);
    QVERIFY(statusSpy.isValid());

    // The bootloader would reject the final 2-byte chunk after the board was erased
    QVERIFY(!image.load(writeImage(QStringLiteral("unaligned.bin"), 70), 0));
    QCOMPARE(statusSpy.size(), 1);
    QVERIFY(statusSpy.first().first().toString().contains(QStringLiteral("multiple of 4")));
    QCOMPARE(image.imageSize(), 0U);

    QVERIFY(image.load(writeImage(QStringLiteral("aligned.bin"), 72), 0));
    QCOMPARE(image.imageSize(), 72U);
    QCOMPARE(statusSpy.size(), 1);
}

UT_REGISTER_TEST(FirmwareUpgradeControllerTest, TestLabel::Unit, TestLabel::Vehicle)
