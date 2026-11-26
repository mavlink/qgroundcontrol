#include "FirmwareUpgradeControllerTest.h"

#include <algorithm>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtCore/QThreadPool>
#include <QtTest/QSignalSpy>

#include "FileFixtures.h"
#include "FirmwareImage.h"
#include "FirmwarePluginManager.h"
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

/// A manifest of entries that QGC keeps and skips, written as the real one is, with what can trip a reader that
/// does not parse all of it: nested objects, brackets and quotes in strings, escapes, elements that are not
/// objects, and members besides the firmware list
QByteArray representativeManifest()
{
    return QByteArray(R"({
    "format-version": "1.0.0",
    "other": {"firmware": [{"mav-type": "Copter", "mav-firmware-version-type": "OFFICIAL", "format": "apj"}]},
    "firmware": [
        {
            "mav-autopilot": "ARDUPILOTMEGA", "vehicletype": "Copter", "platform": "CubeOrange",
            "git-sha": "d9c5097c", "url": "https://firmware.ardupilot.org/Copter/stable/CubeOrange/arducopter.apj",
            "mav-type": "Copter", "mav-firmware-version-type": "OFFICIAL", "mav-firmware-version-str": "V4.6.2",
            "latest": 1, "format": "apj", "mav-firmware-version": "4.6.2", "board_id": 140,
            "bootloader_str": ["CubeOrange-BL", "CubeOrange-periph-BL"], "USBID": ["0x2DAE/0x1016", "0x2DAE/0x1017"],
            "image_size": 1834512, "brand_name": "CubePilot Orange", "manufacturer": "CubePilot"
        },
        {
            "platform": "CubeOrange-heli", "url": "https://example.com/heli.apj", "mav-type": "HELICOPTER",
            "mav-firmware-version-type": "BETA", "format": "apj", "mav-firmware-version": "4.6.0-beta1",
            "board_id": 140, "bootloader_str": [], "USBID": []
        },
        {
            "platform": "fmuv2", "url": "https://example.com/fmuv2.px4", "mav-type": "Copter",
            "mav-firmware-version-type": "DEV", "format": "px4", "mav-firmware-version": "4.7.0-dev", "board_id": 9,
            "bootloader_str": ["PX4 BL FMU v2.x"], "USBID": ["0x26AC/0x0011"]
        },
        {
            "platform": "Pixhawk1-heli", "url": "https://example.com/not-heli.apj", "mav-type": "Copter",
            "mav-firmware-version-type": "OFFICIAL", "format": "apj", "mav-firmware-version": "4.6.2", "board_id": 9
        },
        {
            "platform": "Durandal", "url": "https://example.com/plane.apj", "mav-type": "FIXED_WING",
            "mav-firmware-version-type": "STABLE-4.5.1", "format": "apj", "mav-firmware-version": "4.5.1",
            "board_id": 139
        },
        {
            "platform": "Durandal", "url": "https://example.com/rover.hex", "mav-type": "GROUND_ROVER",
            "mav-firmware-version-type": "OFFICIAL", "format": "hex", "mav-firmware-version": "4.5.1"
        },
        {
            "platform": "BeastH7", "url": "https://example.com/tracker.apj", "mav-type": "ANTENNA_TRACKER",
            "mav-firmware-version-type": "OFFICIAL", "format": "apj", "mav-firmware-version": "4.7.3", "board_id": 1
        },
        {
            "platform": "Pixhawk6X", "url": "https://example.com/sub.apj", "mav-type": "SUBMARINE",
            "mav-firmware-version-type": "OFFICIAL", "format": "apj", "mav-firmware-version": "4.5.3", "board_id": 53
        },
        {
            "platform": "Pixhawk4", "url": "https://example.com/escaped.apj", "mav-type": "Copter",
            "mav-firmware-version-type": "OFFICIAL", "format": "\u0061pj", "mav-firmware-version": "4.6.2",
            "board_id": 50, "brand_name": "Holybro \"Pixhawk 4\" {v5}"
        },
        {
            "platform": "SITL", "git-sha": "}{][\",:\\", "url": "https://example.com/sitl.elf", "mav-type": "Copter",
            "mav-firmware-version-type": "OFFICIAL", "format": "elf",
            "extra": {"nested": [{"format": "px4"}, [1, 2.5e3, -3, true, false, null]]}
        },
        {
            "platform": "MatekH743", "url": "https://example.com/matek.apj", "mav-type": "FIXED_WING",
            "mav-firmware-version-type": "OFFICIAL", "format": "apj", "mav-firmware-version": "4.6.2",
            "board_id": 1013, "USBID": ["0x1209", "0x1209/0x5741"]
        },
        42,
        "not an entry",
        [{"mav-type": "Copter", "format": "apj"}]
    ],
    "after": {"z": [null]}
})");
}

/// The downloads of the ArduPilot manifest that the controller runs
qsizetype manifestDownloads(const FirmwareUpgradeController& controller)
{
    const QList<QGCFileDownload*> downloads = controller.findChildren<QGCFileDownload*>();
    return std::count_if(downloads.cbegin(), downloads.cend(), [](const QGCFileDownload* download) {
        return download->url().toString().endsWith(QStringLiteral("/manifest.json.gz"));
    });
}

/// Stops the downloads the controller runs, so that only the test hands it a manifest
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

void FirmwareUpgradeControllerTest::_manifestIsDownloadedWhenAskedFor()
{
    if (!FirmwarePluginManager::instance()->firmwareClassSupported(QGCMAVLink::FirmwareClassArduPilot)) {
        QSKIP("The build does not offer ArduPilot");
    }

    // Opening the firmware page downloads no ArduPilot firmware list: PX4 users and those who only look need none
    FirmwareUpgradeController controller;
    QSignalSpy spy(&controller, &FirmwareUpgradeController::downloadingFirmwareListChanged);
    QVERIFY(spy.isValid());
    QCOMPARE(manifestDownloads(controller), 0);
    QVERIFY(!controller._downloadingFirmwareList);

    // The firmware selection asks for it when it shows ArduPilot firmware, and shows that it is being downloaded
    controller.loadArduPilotFirmwareList();
    QCOMPARE(manifestDownloads(controller), 1);
    QVERIFY(controller._downloadingFirmwareList);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.last().at(0).toBool(), true);
    stopManifestDownload(controller);
}

void FirmwareUpgradeControllerTest::_manifestIsDownloadedOnce()
{
    if (!FirmwarePluginManager::instance()->firmwareClassSupported(QGCMAVLink::FirmwareClassArduPilot)) {
        QSKIP("The build does not offer ArduPilot");
    }

    QTemporaryDir tempDir;
    const QString manifestPath = tempDir.filePath(QStringLiteral("manifest.json"));
    QVERIFY(TestFixtures::writeFile(manifestPath, arduPilotManifest(45)));

    FirmwareUpgradeController controller;
    controller.loadArduPilotFirmwareList();
    // Asked again while it is downloaded, as when the user picks ArduPilot twice
    controller.loadArduPilotFirmwareList();
    QCOMPARE(manifestDownloads(controller), 1);

    // Asked again while it is read, and after
    stopManifestDownload(controller);
    controller._ardupilotManifestDownloadComplete(true, manifestPath, QString());
    controller.loadArduPilotFirmwareList();
    QCOMPARE(manifestDownloads(controller), 0);
    QTRY_VERIFY_WITH_TIMEOUT(!controller._downloadingFirmwareList, TestTimeout::mediumMs());
    QCOMPARE(controller._rgManifestFirmwareInfo.size(), 1);
    controller.loadArduPilotFirmwareList();
    QCOMPARE(manifestDownloads(controller), 0);
    QVERIFY(!controller._downloadingFirmwareList);
}

void FirmwareUpgradeControllerTest::_manifestIsDownloadedAgainAfterFailure()
{
    if (!FirmwarePluginManager::instance()->firmwareClassSupported(QGCMAVLink::FirmwareClassArduPilot)) {
        QSKIP("The build does not offer ArduPilot");
    }

    FirmwareUpgradeController controller;
    controller.loadArduPilotFirmwareList();
    stopManifestDownload(controller);
    expectLogMessage("Vehicle.VehicleSetup.FirmwareUpgradeController", QtWarningMsg,
                     QRegularExpression("ArduPilot Manifest download failed"));
    controller._ardupilotManifestDownloadComplete(false, QString(), QStringLiteral("simulated error"));
    verifyExpectedLogMessage();
    QVERIFY(!controller._downloadingFirmwareList);

    // The list could not be shown: the next time it is to be shown, it is downloaded again
    controller.loadArduPilotFirmwareList();
    QCOMPARE(manifestDownloads(controller), 1);
    QVERIFY(controller._downloadingFirmwareList);
    stopManifestDownload(controller);
}

FirmwareUpgradeControllerTest::Split FirmwareUpgradeControllerTest::_splitManifest(const QByteArray& manifest,
                                                                                   qsizetype chunkSize, qsizetype cut)
{
    Split split;
    FirmwareUpgradeController::ManifestSplitter splitter([&split](QByteArrayView object) {
        split.objects.append(object.toByteArray());
        return true;
    });
    QList<QByteArrayView> chunks;
    if (cut > 0) {
        chunks = {QByteArrayView(manifest).first(cut), QByteArrayView(manifest).sliced(cut)};
    } else {
        for (qsizetype offset = 0; offset < manifest.size(); offset += chunkSize) {
            chunks.append(QByteArrayView(manifest).sliced(offset, std::min(chunkSize, manifest.size() - offset)));
        }
    }
    split.ok = std::all_of(chunks.cbegin(), chunks.cend(),
                           [&splitter](QByteArrayView chunk) { return splitter.feed(chunk); }) &&
               splitter.finish();
    split.error = splitter.errorString();
    return split;
}

FirmwareUpgradeControllerTest::FirmwareList FirmwareUpgradeControllerTest::_parsedWhole(const QByteArray& manifest)
{
    FirmwareUpgradeController controller;
    stopManifestDownload(controller);
    const QJsonArray entries = QJsonDocument::fromJson(manifest).object()[QStringLiteral("firmware")].toArray();
    FirmwareList firmware;
    for (const QJsonValue& entry : entries) {
        if (const auto info = FirmwareUpgradeController::_manifestFirmwareInfo(
                entry.toObject(), controller._manifestMavFirmwareVersionTypeToFirmwareBuildTypeMap,
                controller._manifestMavTypeToFirmwareVehicleTypeMap)) {
            firmware.append(*info);
        }
    }
    return firmware;
}

QString FirmwareUpgradeControllerTest::_difference(const FirmwareList& actual, const FirmwareList& expected)
{
    if (actual.size() != expected.size()) {
        return QStringLiteral("%1 entries instead of %2").arg(actual.size()).arg(expected.size());
    }
    for (qsizetype i = 0; i < actual.size(); ++i) {
        const auto& a = actual.at(i);
        const auto& e = expected.at(i);
        if ((a.boardId != e.boardId) || (a.firmwareBuildType != e.firmwareBuildType) ||
            (a.vehicleType != e.vehicleType) || (a.url != e.url) || (a.version != e.version) ||
            (a.rgBootloaderPortString != e.rgBootloaderPortString) || (a.rgVID != e.rgVID) || (a.rgPID != e.rgPID) ||
            (a.friendlyName != e.friendlyName) || (a.chibios != e.chibios) || (a.fmuv2 != e.fmuv2)) {
            return QStringLiteral("entry %1 (%2) differs").arg(i).arg(a.url);
        }
    }
    return {};
}

void FirmwareUpgradeControllerTest::_manifestFriendlyNameKeepsBrandName()
{
    // A brand name from the manifest may hold what looks like a placeholder
    QJsonObject entry = QJsonDocument::fromJson(arduPilotManifest(42))
                            .object()[QStringLiteral("firmware")]
                            .toArray()
                            .first()
                            .toObject();
    entry[QStringLiteral("brand_name")] = QStringLiteral("Brand %1%2");

    FirmwareUpgradeController controller;
    stopManifestDownload(controller);
    const auto info = FirmwareUpgradeController::_manifestFirmwareInfo(
        entry, controller._manifestMavFirmwareVersionTypeToFirmwareBuildTypeMap,
        controller._manifestMavTypeToFirmwareVehicleTypeMap);
    QVERIFY(info.has_value());
    QCOMPARE(info->friendlyName, QStringLiteral("Brand %1%2 - 4.5.0"));
}

void FirmwareUpgradeControllerTest::_manifestEntriesAreThoseOfTheWholeManifest()
{
    // The list is the one QGC built when it parsed the whole manifest, from the compressed file as it is downloaded
    const QByteArray manifest = representativeManifest();
    const FirmwareList expected = _parsedWhole(manifest);
    QCOMPARE(expected.size(), 6);

    QTemporaryDir tempDir;
    const QString manifestPath = tempDir.filePath(QStringLiteral("manifest.json.gz"));
    QVERIFY(TestFixtures::writeFile(manifestPath, TestFixtures::gzipMember(manifest)));
    FirmwareUpgradeController controller;
    stopManifestDownload(controller);
    const auto parsed = FirmwareUpgradeController::_parseArduPilotManifest(
        manifestPath, controller._manifestMavFirmwareVersionTypeToFirmwareBuildTypeMap,
        controller._manifestMavTypeToFirmwareVehicleTypeMap);
    QVERIFY2(parsed.errorString.isEmpty(), qPrintable(parsed.errorString));
    const QString difference = _difference(parsed.firmware, expected);
    QVERIFY2(difference.isEmpty(), qPrintable(difference));
}

void FirmwareUpgradeControllerTest::_manifestThatCannotBeReadKeepsTheList_data()
{
    QTest::addColumn<QByteArray>("file");
    QTest::addColumn<QString>("error");

    const QByteArray manifest = arduPilotManifest(44);
    const QByteArray compressed = TestFixtures::gzipMember(manifest);
    QByteArray changedChecksum = compressed;
    changedChecksum[changedChecksum.size() - 8] = static_cast<char>(changedChecksum.at(changedChecksum.size() - 8) ^ 1);

    QTest::newRow("cut download") << compressed.first(compressed.size() - 20) << QStringLiteral("truncated");
    QTest::newRow("checksum") << changedChecksum << QStringLiteral("does not match its checksum");
    QTest::newRow("cut text") << TestFixtures::gzipMember(manifest.first(manifest.size() - 8))
                              << QStringLiteral("before its end");
    QTest::newRow("no firmware list") << TestFixtures::gzipMember(R"({"format-version": "1.0.0"})")
                                      << QStringLiteral("no firmware list");
    QTest::newRow("firmware list that is not an array")
        << TestFixtures::gzipMember(R"({"firmware": {"board_id": 44}})") << QStringLiteral("not an array");
    QTest::newRow("entry that is not valid JSON")
        << TestFixtures::gzipMember(R"({"firmware": [{"format": "apj", "board_id": 4x4}]})")
        << QStringLiteral("not valid JSON");
}

void FirmwareUpgradeControllerTest::_manifestThatCannotBeReadKeepsTheList()
{
    QFETCH(QByteArray, file);
    QFETCH(QString, error);

    QTemporaryDir tempDir;
    const QString manifestPath = tempDir.filePath(QStringLiteral("manifest.json.gz"));
    QVERIFY(TestFixtures::writeFile(manifestPath, file));

    FirmwareUpgradeController controller;
    stopManifestDownload(controller);
    FirmwareUpgradeController::ManifestFirmwareInfo_t kept{};
    kept.boardId = 7;
    controller._rgManifestFirmwareInfo.append(kept);
    controller._downloadingFirmwareList = true;
    expectLogMessage("Vehicle.VehicleSetup.FirmwareUpgradeController", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Json file read failed.*") + QRegularExpression::escape(error)));
    controller._ardupilotManifestDownloadComplete(true, manifestPath, QString());
    QTRY_VERIFY_WITH_TIMEOUT(!controller._downloadingFirmwareList, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    // Not the entries that were read before the damage was found
    QCOMPARE(controller._rgManifestFirmwareInfo.size(), 1);
    QCOMPARE(controller._rgManifestFirmwareInfo.first().boardId, static_cast<uint32_t>(7));
}

void FirmwareUpgradeControllerTest::_manifestSplitterEntries_data()
{
    QTest::addColumn<QByteArray>("manifest");
    QTest::addColumn<QList<QByteArray>>("objects");
    // What the error says, or empty if the manifest is sound
    QTest::addColumn<QString>("error");

    const auto sound = [](const char* tag, const QByteArray& manifest, const QList<QByteArray>& objects) {
        QTest::newRow(tag) << manifest << objects << QString();
    };
    const auto malformed = [](const char* tag, const QByteArray& manifest, const char* error) {
        QTest::newRow(tag) << manifest << QList<QByteArray>() << QString::fromLatin1(error);
    };

    sound("nested objects", R"({"firmware":[{"a":{"b":{"c":1}},"d":[{"e":2}]},{"x":1}]})",
          {R"({"a":{"b":{"c":1}},"d":[{"e":2}]})", R"({"x":1})"});
    sound("brackets and quotes in strings", R"({"firmware":[{"s":"}{][\",:"},{"t":"]"}]})",
          {R"({"s":"}{][\",:"})", R"({"t":"]"})"});
    sound("escapes", R"({"firmware":[{"s":"\\\"}\\","u":"\u007D\uD83D\uDE00","c":"\/\b\f\n\r\t"}]})",
          {R"({"s":"\\\"}\\","u":"\u007D\uD83D\uDE00","c":"\/\b\f\n\r\t"})"});
    sound("members before and after, and a firmware member inside another",
          R"({"format-version":"1.0.0","other":{"firmware":[{"no":1}]},"firmware":[{"yes":1}],"after":[1,{"z":null}]})",
          {R"({"yes":1})"});
    sound("elements that are not objects", R"({"firmware":[1,"s",[{"inner":1}],null,true,{"o":-2.5e-3}]})",
          {R"({"o":-2.5e-3})"});
    sound("empty", R"({"firmware":[]})", {});
    sound("escaped name", R"({"firm\u0077are":[{"k":1}]})", {R"({"k":1})"});
    sound("white space", "\n{\t\"firmware\" :\r\n [ {\"k\" : [ ] } , { } ]\n}\n", {"{\"k\" : [ ] }", "{ }"});

    malformed("no firmware list", R"({"format-version":"1.0.0"})", "no firmware list");
    malformed("firmware list that is not an array", R"({"firmware":{"a":1}})", "not an array");
    malformed("two firmware lists", R"({"firmware":[],"firmware":[]})", "a second firmware list");
    malformed("array", R"([{"firmware":[]}])", "not an object");
    malformed("nothing", "", "before its end");
    malformed("data after the object", R"({"firmware":[]}x)", "where it cannot be");
    malformed("second object", R"({"firmware":[]}{})", "where it cannot be");
    malformed("bracket that closes nothing open", R"({"firmware":[}]})", "closes nothing open");
    malformed("escape that JSON does not have", R"({"firmware":[{"s":"\q"}]})", "escape");
    malformed("short unicode escape", R"({"firmware":[{"s":"\u12"}]})", "four hex digits");
    malformed("control character", QByteArray("{\"firmware\":[{\"s\":\"a\x01\"}]}"), "control character");
    malformed("literal", R"({"firmware":[],"a":tru})", "not one");
    malformed("missing comma", R"({"firmware":[{"a":1} {"b":2}]})", "where it cannot be");
    malformed("missing colon", R"({"firmware" []})", "where it cannot be");
    malformed("comma before the end of an array", R"({"firmware":[{"a":1},]})", "closes nothing open");
    malformed("comma before the end of an object", R"({"firmware":[{"a":1,}]})", "closes nothing open");
    malformed("name that is not a string", R"({"firmware":[{a:1}]})", "where it cannot be");
    malformed("nested too deeply",
              QByteArray(R"({"firmware":[)") + QByteArray(70, '[') + QByteArray(70, ']') + QByteArray("]}"),
              "nested too deeply");
}

void FirmwareUpgradeControllerTest::_manifestSplitterEntries()
{
    QFETCH(QByteArray, manifest);
    QFETCH(QList<QByteArray>, objects);
    QFETCH(QString, error);

    // Whatever the chunks: all at once, a byte at a time, and cut in two at every byte
    QList<Split> splits{_splitManifest(manifest, manifest.size() + 1), _splitManifest(manifest, 1)};
    for (qsizetype cut = 1; cut < manifest.size(); ++cut) {
        splits.append(_splitManifest(manifest, 0, cut));
    }
    for (const Split& split : std::as_const(splits)) {
        if (error.isEmpty()) {
            QVERIFY2(split.ok, qPrintable(split.error));
            QCOMPARE(split.objects, objects);
        } else {
            QVERIFY(!split.ok);
            QVERIFY2(split.error.contains(error), qPrintable(split.error));
        }
    }
}

void FirmwareUpgradeControllerTest::_manifestSplitterRefusesEveryPrefix()
{
    const QByteArray manifest = representativeManifest();
    QVERIFY(_splitManifest(manifest, manifest.size()).ok);
    for (qsizetype size = 0; size < manifest.size(); ++size) {
        const Split split = _splitManifest(manifest.first(size), 4096);
        QVERIFY2(!split.ok, qPrintable(QStringLiteral("A manifest of %1 bytes was taken").arg(size)));
    }
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
