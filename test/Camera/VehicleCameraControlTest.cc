#include "VehicleCameraControlTest.h"

#include <cstring>
#include <memory>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "LinkManager.h"
#include "MavlinkCameraControlInterface.h"
#include "MockConfiguration.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "QGCCameraManager.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleCameraControl.h"

namespace {

class CameraControlProbe : public VehicleCameraControl
{
public:
    using VehicleCameraControl::VehicleCameraControl;

    QString cacheFile() const { return _cacheFile; }

    bool paramComplete() const { return _paramComplete; }
};

MockConfiguration* newMockConfiguration(const QString& name)
{
    auto* mockConfig = new MockConfiguration(name);
    mockConfig->setFirmwareType(MAV_AUTOPILOT_PX4);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setDynamic(true);
    return mockConfig;
}

/// Points the app save path at rootPath until the returned guard goes out of scope.
[[nodiscard]] auto overrideSavePath(const QString& rootPath)
{
    Fact* savePathFact = SettingsManager::instance()->appSettings()->savePath();
    const QVariant originalSavePath = savePathFact->rawValue();
    savePathFact->setRawValue(rootPath);
    return qScopeGuard([savePathFact, originalSavePath]() { savePathFact->setRawValue(originalSavePath); });
}

}  // namespace

void VehicleCameraControlTest::_testNameFieldReadIsBounded()
{
    // CAMERA_INFORMATION.vendor_name and model_name are uint8_t[32] filled straight from
    // the wire. MAVLink does not guarantee a NUL, so reading them as C strings runs past
    // the array into whatever follows it in the struct.
    struct
    {
        uint8_t vendor[32];
        uint8_t model[32];
    } wire{};

    memset(wire.vendor, 'A', sizeof(wire.vendor));
    memcpy(wire.model, "SHOULD_NOT_APPEAR", 18);

    const QString bounded = QGCMAVLink::boundedStringField(wire.vendor);

    QCOMPARE(bounded.length(), 32);
    QVERIFY(!bounded.contains(QLatin1String("SHOULD_NOT_APPEAR")));

    // A short, properly terminated field must still read normally.
    memset(&wire, 0, sizeof(wire));
    memcpy(wire.vendor, "Sony", 5);
    QCOMPARE(QGCMAVLink::boundedStringField(wire.vendor), QStringLiteral("Sony"));

    // QGC has always decoded these names as UTF-8, so a non-ASCII name must not turn into Latin-1 mojibake.
    static constexpr char utf8Name[] = "C\xC3\xA1mara";
    memset(&wire, 0, sizeof(wire));
    memcpy(wire.vendor, utf8Name, sizeof(utf8Name));
    QCOMPARE(QGCMAVLink::boundedStringField(wire.vendor), QStringLiteral("C\u00e1mara"));
}

void VehicleCameraControlTest::_testCameraNamesCannotSteerCachePath()
{
    // The vendor/model strings are formatted into the camera-definition cache file name.
    // Whatever they contain, the result has to stay a single component inside the cache
    // directory.
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString base = QDir::cleanPath(QDir(tempDir.path()).absolutePath());

    const QList<QPair<QString, QString>> hostile = {
        {QStringLiteral("../../../Desktop/QGC01_PWNED"), QStringLiteral("marker")},
        {QStringLiteral("..\\..\\..\\Desktop\\PWNED"), QStringLiteral("marker")},
        {QStringLiteral("C:\\Windows\\Temp\\PWNED"), QStringLiteral("marker")},
        {QStringLiteral("/etc/cron.d/PWNED"), QStringLiteral("marker")},
        {QStringLiteral("T.txt:"), QStringLiteral("s")},
        {QStringLiteral("vendor"), QStringLiteral("../../evil")},
        {QStringLiteral(".."), QStringLiteral("..")},
        {QString(), QStringLiteral("marker")},
    };

    for (const auto& testCase : hostile) {
        const QString token = VehicleCameraControl::cacheFileToken(testCase.first, testCase.second, 1);
        const QString candidateFilePath = QDir(base).filePath(token + QStringLiteral(".xml"));

        QVERIFY2(VehicleCameraControl::pathIsInside(base, candidateFilePath), qPrintable(candidateFilePath));

        // It must be one component: no separator, and no drive or NTFS stream qualifier.
        QVERIFY2(!token.contains(QLatin1Char('/')), qPrintable(token));
        QVERIFY2(!token.contains(QLatin1Char('\\')), qPrintable(token));
        QVERIFY2(!token.contains(QLatin1Char(':')), qPrintable(token));
    }

    // An unset cache directory must be refused rather than becoming a write at "/".
    QVERIFY(!VehicleCameraControl::pathIsInside(QString(), QStringLiteral("/v_m_001.xml")));
}

void VehicleCameraControlTest::_testOrdinaryCameraNamesUnchanged()
{
    // Containment must not mangle a real camera's cache file name beyond replacing
    // characters that cannot appear in one.
    QCOMPARE(VehicleCameraControl::pathSafeNameToken(QStringLiteral("Sony")), QStringLiteral("Sony"));
    QCOMPARE(VehicleCameraControl::pathSafeNameToken(QStringLiteral("ILCE-7RM4")), QStringLiteral("ILCE-7RM4"));
    QCOMPARE(VehicleCameraControl::pathSafeNameToken(QStringLiteral("Skynode.v2")), QStringLiteral("Skynode.v2"));
    QCOMPARE(VehicleCameraControl::pathSafeNameToken(QStringLiteral("FLIR Boson")), QStringLiteral("FLIR_Boson"));
    QCOMPARE(VehicleCameraControl::pathSafeNameToken(QString()), QStringLiteral("unknown"));

    const QString token = VehicleCameraControl::cacheFileToken(QStringLiteral("Sony"), QStringLiteral("ILCE-7RM4"), 7);
    QVERIFY2(token.startsWith(QStringLiteral("Sony_ILCE-7RM4_")), qPrintable(token));
    QVERIFY2(token.endsWith(QStringLiteral("_007")), qPrintable(token));
    QCOMPARE(token, VehicleCameraControl::cacheFileToken(QStringLiteral("Sony"), QStringLiteral("ILCE-7RM4"), 7));
}

void VehicleCameraControlTest::_testDistinctCameraNamesDoNotShareCacheFile()
{
    // pathSafeNameToken() is lossy; distinct cameras must still get distinct cache files.
    const QList<QPair<QPair<QString, QString>, QPair<QString, QString>>> collidingAfterSanitising = {
        {{QStringLiteral("FLIR"), QStringLiteral("Boson 640")}, {QStringLiteral("FLIR"), QStringLiteral("Boson_640")}},
        {{QStringLiteral("FLIR"), QStringLiteral("Boson/640")}, {QStringLiteral("FLIR"), QStringLiteral("Boson:640")}},
        {{QStringLiteral("\u5927\u7586"), QStringLiteral("Cam")},
         {QStringLiteral("\u6d77\u5eb7"), QStringLiteral("Cam")}},
        {{QStringLiteral(".Cam"), QStringLiteral("X")}, {QStringLiteral("Cam"), QStringLiteral("X")}},
        {{QStringLiteral("A_B"), QStringLiteral("C")}, {QStringLiteral("A"), QStringLiteral("B_C")}},
        {{QStringLiteral("Sony"), QStringLiteral("X")}, {QStringLiteral("sony"), QStringLiteral("X")}},
    };

    for (const auto& pair : collidingAfterSanitising) {
        const QString first = VehicleCameraControl::cacheFileToken(pair.first.first, pair.first.second, 1);
        const QString second = VehicleCameraControl::cacheFileToken(pair.second.first, pair.second.second, 1);
        QVERIFY2(first.compare(second, Qt::CaseInsensitive) != 0, qPrintable(first + QStringLiteral(" == ") + second));
    }
}

void VehicleCameraControlTest::_connectVehicle(MockConfiguration* mockConfig)
{
    QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY(spyVehicle.isValid());

    SharedLinkConfigurationPtr linkConfig = LinkManager::instance()->addConfiguration(mockConfig);
    QVERIFY(LinkManager::instance()->createConnectedLink(linkConfig));
    QVERIFY2(UnitTest::waitForSignal(spyVehicle, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged")),
             "Timeout waiting for vehicle connection");

    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(_vehicle);
    _mockLink = qobject_cast<MockLink*>(linkConfig->link());
    QVERIFY(_mockLink);

    if (!_vehicle->isInitialConnectComplete()) {
        QSignalSpy spyConnect(_vehicle, &Vehicle::initialConnectComplete);
        QVERIFY(spyConnect.isValid());
        QVERIFY2(UnitTest::waitForSignal(spyConnect, TestTimeout::longMs(), QStringLiteral("initialConnectComplete")),
                 "Timeout waiting for initial connect");
    }
}

void VehicleCameraControlTest::_testConstructorBuildsCacheFileFromBoundedNames()
{
    _connectVehicle(newMockConfiguration(QStringLiteral("CameraCacheFileTest")));
    if (QTest::currentTestFailed()) {
        return;
    }

    const QString cacheDir = SettingsManager::instance()->appSettings()->parameterSavePath();
    QVERIFY(!cacheDir.isEmpty());

    // model_name is completely full and lens_id, which follows it, is non-zero, so an
    // unbounded read would pick up the extra byte.
    mavlink_camera_information_t info{};
    memcpy(info.vendor_name, "../../evil", 11);
    memset(info.model_name, 'M', sizeof(info.model_name));
    info.lens_id = 'X';
    info.cam_definition_version = 3;

    auto camera = std::make_unique<CameraControlProbe>(&info, _vehicle, MAV_COMP_ID_CAMERA6);

    const QString expectedModel(static_cast<qsizetype>(sizeof(info.model_name)), QLatin1Char('M'));
    QCOMPARE(camera->vendor(), QStringLiteral("../../evil"));
    QCOMPARE(camera->modelName(), expectedModel);

    const QString expectedToken = VehicleCameraControl::cacheFileToken(QStringLiteral("../../evil"), expectedModel, 3);
    QCOMPARE(camera->cacheFile(), QDir::cleanPath(QDir(cacheDir).filePath(expectedToken + QStringLiteral(".xml"))));
    QVERIFY(VehicleCameraControl::pathIsInside(cacheDir, camera->cacheFile()));

    // No definition URI, so the camera initialises as a basic camera straight away.
    QVERIFY(camera->paramComplete());
}

void VehicleCameraControlTest::_testFtpDefinitionDownloadRefusedFallsBack()
{
    _connectVehicle(newMockConfiguration(QStringLiteral("CameraFtpFallbackTest")));
    if (QTest::currentTestFailed()) {
        return;
    }

    // parameterSavePath() is empty when the save root does not exist. Point the save path
    // at a temporary root and remove it, rather than clearing the setting, which would
    // make AppSettings create its subdirectories in the working directory.
    QTemporaryDir missingRoot;
    QVERIFY(missingRoot.isValid());
    const auto restoreSavePath = overrideSavePath(missingRoot.path());
    QVERIFY(missingRoot.remove());
    QVERIFY(SettingsManager::instance()->appSettings()->parameterSavePath().isEmpty());

    // cam_definition_uri is completely full and the bytes after it are non-zero, so an
    // unbounded read would run past the array into them.
    struct
    {
        mavlink_camera_information_t info;
        char tail[4];
    } wire{};

    memcpy(wire.info.vendor_name, "Acme", 5);
    memcpy(wire.info.model_name, "Cam", 4);
    memset(wire.info.cam_definition_uri, 'A', sizeof(wire.info.cam_definition_uri));
    memcpy(wire.info.cam_definition_uri, "mftp://", 7);
    wire.info.gimbal_device_id = 'B';
    wire.info.camera_device_id = 'C';
    memcpy(wire.tail, "ZZZ", 4);

    const qsizetype uriPayloadLength = static_cast<qsizetype>(sizeof(wire.info.cam_definition_uri)) - 7;
    expectLogMessage("Camera.VehicleCameraControl", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Camera definition cache directory is not set")));
    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Destination directory is not set")));
    expectLogMessage(
        "Camera.VehicleCameraControl", QtWarningMsg,
        QRegularExpression(
            QStringLiteral("Could not start camera definition ftp download: \"mftp://A{%1}\"$").arg(uriPayloadLength)));

    auto camera = std::make_unique<CameraControlProbe>(&wire.info, _vehicle, MAV_COMP_ID_CAMERA6);

    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(camera->cacheFile().isEmpty());

    // The refused download must not leave the camera waiting forever for a definition.
    QVERIFY(camera->paramComplete());
}

void VehicleCameraControlTest::_testFtpDefinitionDownloadIsSavedAsCacheFile_data()
{
    QTest::addColumn<QString>("vehiclePath");
    QTest::addColumn<QString>("compressedSuffix");

    QTest::newRow("plain") << QStringLiteral("/general.json") << QString();
    QTest::newRow("xz") << QStringLiteral("/general.json.xz") << QStringLiteral(".xz");
}

void VehicleCameraControlTest::_testFtpDefinitionDownloadIsSavedAsCacheFile()
{
    // The downloaded file is the cache: _ftpDownloadComplete() marks the definition as cached,
    // so nothing else writes it. Its name, minus any compression suffix, therefore has to be
    // _cacheFile, or the next connect never finds it and downloads it again.
    QFETCH(QString, vehiclePath);
    QFETCH(QString, compressedSuffix);

    _connectVehicle(newMockConfiguration(QStringLiteral("CameraFtpCacheNameTest")));
    if (QTest::currentTestFailed()) {
        return;
    }

    QTemporaryDir saveRoot;
    QVERIFY(saveRoot.isValid());
    const auto restoreSavePath = overrideSavePath(saveRoot.path());
    const QString cacheDir = SettingsManager::instance()->appSettings()->parameterSavePath();
    QVERIFY(QDir(cacheDir).exists());

    // MockLinkFTP serves its JSON metadata at these paths; the content only has to survive
    // the trip, it does not need to be a valid camera definition.
    const QString uri = QStringLiteral("mftp://[;comp=%1]%2").arg(MAV_COMP_ID_AUTOPILOT1).arg(vehiclePath);
    const QByteArray uriBytes = uri.toUtf8();
    mavlink_camera_information_t info{};
    memcpy(info.vendor_name, "Acme", 5);
    memcpy(info.model_name, "Cam", 4);
    QVERIFY(uriBytes.size() < static_cast<qsizetype>(sizeof(info.cam_definition_uri)));
    memcpy(info.cam_definition_uri, uriBytes.constData(), static_cast<size_t>(uriBytes.size()));

    expectLogMessage("default", QtCriticalMsg, QRegularExpression(QStringLiteral("Unable to parse camera definition")));
    // The second message is Qt's own XML parser error text, which can change between Qt
    // versions, so only its presence is checked.
    expectLogMessage("default", QtCriticalMsg, QRegularExpression(QStringLiteral(".+")));

    auto camera = std::make_unique<CameraControlProbe>(&info, _vehicle, MAV_COMP_ID_CAMERA6);
    QTRY_VERIFY_WITH_TIMEOUT(camera->paramComplete(), TestTimeout::longMs());

    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    const QString expectedCacheFile = QDir::cleanPath(
        QDir(cacheDir).filePath(VehicleCameraControl::cacheFileToken(QStringLiteral("Acme"), QStringLiteral("Cam"), 0) +
                                QStringLiteral(".xml")));
    QCOMPARE(camera->cacheFile(), expectedCacheFile);

    QFile cached(expectedCacheFile);
    QVERIFY2(cached.open(QIODevice::ReadOnly), qPrintable(expectedCacheFile));
    QFile served(QStringLiteral(":MockLink/General.MetaData.json"));
    QVERIFY(served.open(QIODevice::ReadOnly));
    QCOMPARE(cached.readAll(), served.readAll());

    if (!compressedSuffix.isEmpty()) {
        QVERIFY2(QFile::exists(expectedCacheFile + compressedSuffix), qPrintable(expectedCacheFile + compressedSuffix));
    }
}

void VehicleCameraControlTest::initTestCase()
{
    UnitTest::initTestCase();
    MultiVehicleManager::instance()->init();
}

void VehicleCameraControlTest::init()
{
    UnitTest::init();
    _mockLink = nullptr;
    _vehicle = nullptr;
}

void VehicleCameraControlTest::cleanup()
{
    if (_mockLink) {
        QSignalSpy spyDisconnect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
        _mockLink->disconnect();
        _mockLink = nullptr;

        if (_vehicle) {
            UnitTest::waitForSignal(spyDisconnect, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged"));
        }
        _vehicle = nullptr;

        UnitTest::settleEventLoopForCleanup();
    }

    dumpFailureContextIfTestFailed(QStringLiteral("cleanup"));
    UnitTest::cleanup();
}

void VehicleCameraControlTest::_testCameraCapFlags_data()
{
    // MockConfiguration camera flags
    QTest::addColumn<bool>("captureVideo");
    QTest::addColumn<bool>("captureImage");
    QTest::addColumn<bool>("hasModes");
    QTest::addColumn<bool>("hasVideoStream");
    QTest::addColumn<bool>("canCaptureImageInVideoMode");
    QTest::addColumn<bool>("canCaptureVideoInImageMode");
    QTest::addColumn<bool>("hasBasicZoom");
    QTest::addColumn<bool>("hasTrackingPoint");
    QTest::addColumn<bool>("hasTrackingRectangle");

    // Expected VehicleCameraControl properties
    QTest::addColumn<bool>("expectedCapturesVideo");
    QTest::addColumn<bool>("expectedCapturesPhotos");
    QTest::addColumn<bool>("expectedHasModes");
    QTest::addColumn<bool>("expectedHasZoom");
    QTest::addColumn<bool>("expectedHasVideoStream");
    QTest::addColumn<bool>("expectedPhotosInVideoMode");
    QTest::addColumn<bool>("expectedVideoInPhotoMode");
    QTest::addColumn<bool>("expectedHasTracking");

    // Note: xCapVid and xCapImg (capturesVideo/capturesPhotos) are "virtual" capabilities that account for
    // local gstreamer support. When a camera has a video stream (HAS_VIDEO_STREAM), QGC can locally record
    // video via gstreamer and screen-grab photos from the stream, even if the camera itself doesn't support
    // native video recording or photo capture. See "video-stream-only" and "stream-no-native-capture" rows.

    //                                       capVid   capImg   modes    stream   imgInVid vidInImg zoom     trkPt    trkRect   xCapVid  xCapImg  xModes   xZoom    xStream  xImgVid  xVidImg  xTrack
    QTest::newRow("all-caps")                << true  << true  << true  << true  << true  << true  << true  << true  << true   << true  << true  << true  << true  << true  << true  << true  << true;
    QTest::newRow("no-caps")                 << false << false << false << false << false << false << false << false << false  << false << false << false << false << false << false << false << false;
    QTest::newRow("photo-only")              << false << true  << false << false << false << false << false << false << false  << false << true  << false << false << false << false << false << false;
    QTest::newRow("video-only")              << true  << false << false << false << false << false << false << false << false  << true  << false << false << false << false << false << false << false;
    QTest::newRow("video-stream-only")       << false << false << false << true  << false << false << false << false << false  << true  << true  << false << false << true  << false << false << false;
    QTest::newRow("modes-zoom")              << false << true  << true  << false << false << false << true  << false << false  << false << true  << true  << true  << false << false << false << false;
    QTest::newRow("tracking-point")          << false << true  << false << false << false << false << false << true  << false  << false << true  << false << false << false << false << false << true;
    QTest::newRow("tracking-rect")           << false << true  << false << false << false << false << false << false << true   << false << true  << false << false << false << false << false << true;
    QTest::newRow("tracking-both")           << false << true  << false << false << false << false << false << true  << true   << false << true  << false << false << false << false << false << true;
    QTest::newRow("image-in-video")          << true  << true  << true  << false << true  << false << false << false << false  << true  << true  << true  << false << false << true  << false << false;
    QTest::newRow("video-in-image")          << true  << true  << true  << false << false << true  << false << false << false  << true  << true  << true  << false << false << false << true  << false;
    QTest::newRow("stream-no-native-capture")<< false << false << false << true  << false << false << false << false << false  << true  << true  << false << false << true  << false << false << false;
    QTest::newRow("stream-plus-photo")       << false << true  << false << true  << false << false << false << false << false  << true  << true  << false << false << true  << false << false << false;
}

void VehicleCameraControlTest::_testCameraCapFlags()
{
    // Fetch test data
    QFETCH(bool, captureVideo);
    QFETCH(bool, captureImage);
    QFETCH(bool, hasModes);
    QFETCH(bool, hasVideoStream);
    QFETCH(bool, canCaptureImageInVideoMode);
    QFETCH(bool, canCaptureVideoInImageMode);
    QFETCH(bool, hasBasicZoom);
    QFETCH(bool, hasTrackingPoint);
    QFETCH(bool, hasTrackingRectangle);

    QFETCH(bool, expectedCapturesVideo);
    QFETCH(bool, expectedCapturesPhotos);
    QFETCH(bool, expectedHasModes);
    QFETCH(bool, expectedHasZoom);
    QFETCH(bool, expectedHasVideoStream);
    QFETCH(bool, expectedPhotosInVideoMode);
    QFETCH(bool, expectedVideoInPhotoMode);
    QFETCH(bool, expectedHasTracking);

    // Create MockConfiguration with camera enabled and custom flags
    auto* mockConfig = newMockConfiguration(QStringLiteral("CameraCapFlagsTest"));
    mockConfig->setEnableCamera(true);
    mockConfig->setCameraCaptureVideo(captureVideo);
    mockConfig->setCameraCaptureImage(captureImage);
    mockConfig->setCameraHasModes(hasModes);
    mockConfig->setCameraHasVideoStream(hasVideoStream);
    mockConfig->setCameraCanCaptureImageInVideoMode(canCaptureImageInVideoMode);
    mockConfig->setCameraCanCaptureVideoInImageMode(canCaptureVideoInImageMode);
    mockConfig->setCameraHasBasicZoom(hasBasicZoom);
    mockConfig->setCameraHasTrackingPoint(hasTrackingPoint);
    mockConfig->setCameraHasTrackingRectangle(hasTrackingRectangle);

    _connectVehicle(mockConfig);
    if (QTest::currentTestFailed()) {
        return;
    }

    // Wait for camera manager to discover cameras
    QGCCameraManager* cameraManager = _vehicle->cameraManager();
    QVERIFY(cameraManager);

    // MockLinkCamera creates two cameras; wait for both to be discovered
    QVERIFY_TRUE_WAIT(cameraManager->cameras()->count() >= 2, TestTimeout::longMs());

    // Find Camera 1 (MAV_COMP_ID_CAMERA) which has our configured flags.
    // Camera 2 (MAV_COMP_ID_CAMERA2) is always photo-only and not what we're testing.
    MavlinkCameraControlInterface* camera = nullptr;
    for (int i = 0; i < cameraManager->cameras()->count(); i++) {
        auto* cam = qobject_cast<MavlinkCameraControlInterface*>(cameraManager->cameras()->get(i));
        if (cam && cam->compID() == MAV_COMP_ID_CAMERA) {
            camera = cam;
            break;
        }
    }
    QVERIFY2(camera, "Camera 1 (MAV_COMP_ID_CAMERA) not found in camera list");

    // Verify capability properties match expected values
    QCOMPARE(camera->capturesVideo(),     expectedCapturesVideo);
    QCOMPARE(camera->capturesPhotos(),    expectedCapturesPhotos);
    QCOMPARE(camera->hasModes(),          expectedHasModes);
    QCOMPARE(camera->hasZoom(),           expectedHasZoom);
    QCOMPARE(camera->hasVideoStream(),    expectedHasVideoStream);
    QCOMPARE(camera->photosInVideoMode(), expectedPhotosInVideoMode);
    QCOMPARE(camera->videoInPhotoMode(),  expectedVideoInPhotoMode);
    QCOMPARE(camera->hasTracking(),       expectedHasTracking);

    // hasFocus is always false since MockLinkCamera doesn't support CAMERA_CAP_FLAGS_HAS_BASIC_FOCUS
    QCOMPARE(camera->hasFocus(), false);
}

void VehicleCameraControlTest::_testZoomTriggersCameraSettingsRequest()
{
    // A spec-minimal camera does not broadcast CAMERA_SETTINGS after a zoom change,
    // so QGC must re-request it itself once a zoom command is accepted. Otherwise
    // the zoom level (and FOV) shown by QGC goes stale after the first zoom.

    auto* mockConfig = newMockConfiguration(QStringLiteral("CameraZoomSettingsTest"));
    mockConfig->setEnableCamera(true);
    mockConfig->setCameraCaptureImage(true);
    mockConfig->setCameraHasBasicZoom(true);

    _connectVehicle(mockConfig);
    if (QTest::currentTestFailed()) {
        return;
    }

    QGCCameraManager* cameraManager = _vehicle->cameraManager();
    QVERIFY(cameraManager);
    QVERIFY_TRUE_WAIT(cameraManager->cameras()->count() >= 2, TestTimeout::longMs());

    MavlinkCameraControlInterface* camera = nullptr;
    for (int i = 0; i < cameraManager->cameras()->count(); i++) {
        auto* cam = qobject_cast<MavlinkCameraControlInterface*>(cameraManager->cameras()->get(i));
        if (cam && cam->compID() == MAV_COMP_ID_CAMERA) {
            camera = cam;
            break;
        }
    }
    QVERIFY2(camera, "Camera 1 (MAV_COMP_ID_CAMERA) not found in camera list");
    QVERIFY(camera->hasZoom());

    // Wait for the initial CAMERA_SETTINGS exchange to settle: the mock reports zoom
    // level 1.0, so once QGC shows it the initial request/retry cycle is complete.
    QVERIFY_TRUE_WAIT(qFuzzyCompare(camera->zoomLevel(), 1.0), TestTimeout::longMs());

    // QGC alternates between REQUEST_MESSAGE and the deprecated REQUEST_CAMERA_SETTINGS
    // on retries (dual-transport migration), so accept either as a settings request.
    auto settingsRequestCount = [this]() {
        return _mockLink->receivedRequestMessageCount(MAV_COMP_ID_CAMERA, MAVLINK_MSG_ID_CAMERA_SETTINGS)
             + _mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_CAMERA_SETTINGS, MAV_COMP_ID_CAMERA);
    };

    // An accepted zoom command must cause QGC to re-request CAMERA_SETTINGS on its own
    const int baselineAfterConnect = settingsRequestCount();
    camera->setZoomLevel(50.0);
    QTRY_VERIFY2_WITH_TIMEOUT(settingsRequestCount() > baselineAfterConnect,
                              "QGC did not re-request CAMERA_SETTINGS after setZoomLevel was accepted",
                              TestTimeout::longMs());
    // And the fresh settings update QGC's stale zoom level
    QVERIFY_TRUE_WAIT(qFuzzyCompare(camera->zoomLevel(), 50.0), TestTimeout::longMs());

    // Continuous zoom (start/stop) must trigger a refresh as well
    const int baselineAfterSetZoom = settingsRequestCount();
    camera->startZoom(1);
    camera->stopZoom();
    QTRY_VERIFY2_WITH_TIMEOUT(settingsRequestCount() > baselineAfterSetZoom,
                              "QGC did not re-request CAMERA_SETTINGS after startZoom/stopZoom were accepted",
                              TestTimeout::longMs());
}

UT_REGISTER_TEST(VehicleCameraControlTest, TestLabel::Integration, TestLabel::Vehicle)
