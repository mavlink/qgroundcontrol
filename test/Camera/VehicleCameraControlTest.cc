#include "VehicleCameraControlTest.h"
#include <QtTest/QSignalSpy>

#include <cstring>

#include <QtCore/QDir>
#include <QtCore/QTemporaryDir>

#include "VehicleCameraControl.h"


#include "LinkManager.h"
#include "MavlinkCameraControlInterface.h"
#include "MockConfiguration.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "QGCCameraManager.h"
#include "Vehicle.h"

void VehicleCameraControlTest::_testNameFieldReadIsBounded()
{
    // CAMERA_INFORMATION.vendor_name and model_name are uint8_t[32] filled straight from
    // the wire. MAVLink does not guarantee a NUL, so reading them as C strings runs past
    // the array into whatever follows it in the struct.
    struct { uint8_t vendor[32]; uint8_t model[32]; } wire{};
    memset(wire.vendor, 'A', sizeof(wire.vendor));
    memcpy(wire.model, "SHOULD_NOT_APPEAR", 18);

    const QString bounded = VehicleCameraControl::boundedNameField(wire.vendor, sizeof(wire.vendor));

    QCOMPARE(bounded.length(), 32);
    QVERIFY(!bounded.contains(QLatin1String("SHOULD_NOT_APPEAR")));

    // A short, properly terminated field must still read normally.
    memset(&wire, 0, sizeof(wire));
    memcpy(wire.vendor, "Sony", 5);
    QCOMPARE(VehicleCameraControl::boundedNameField(wire.vendor, sizeof(wire.vendor)),
             QStringLiteral("Sony"));

    QVERIFY(VehicleCameraControl::boundedNameField(nullptr, 32).isEmpty());
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
        { QStringLiteral("../../../Desktop/QGC01_PWNED"), QStringLiteral("marker") },
        { QStringLiteral("..\\..\\..\\Desktop\\PWNED"),   QStringLiteral("marker") },
        { QStringLiteral("C:\\Windows\\Temp\\PWNED"),     QStringLiteral("marker") },
        { QStringLiteral("/etc/cron.d/PWNED"),            QStringLiteral("marker") },
        { QStringLiteral("T.txt:"),                       QStringLiteral("s") },
        { QStringLiteral("vendor"),                       QStringLiteral("../../evil") },
        { QStringLiteral(".."),                           QStringLiteral("..") },
        { QString(),                                      QStringLiteral("marker") },
    };

    for (const auto &testCase : hostile) {
        const QString token = QString::asprintf("%s_%s_%03d",
            VehicleCameraControl::pathSafeNameToken(testCase.first).toStdString().c_str(),
            VehicleCameraControl::pathSafeNameToken(testCase.second).toStdString().c_str(),
            1);
        const QString candidate = QDir(base).filePath(token + QStringLiteral(".xml"));

        QVERIFY2(VehicleCameraControl::pathIsInside(base, candidate), qPrintable(candidate));

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
    auto* mockConfig = new MockConfiguration(QStringLiteral("CameraCapFlagsTest"));
    mockConfig->setFirmwareType(MAV_AUTOPILOT_PX4);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setDynamic(true);
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

    // Connect MockLink
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

    // Wait for initial connect sequence to complete
    if (!_vehicle->isInitialConnectComplete()) {
        QSignalSpy spyConnect(_vehicle, &Vehicle::initialConnectComplete);
        QVERIFY(spyConnect.isValid());
        QVERIFY2(UnitTest::waitForSignal(spyConnect, TestTimeout::longMs(), QStringLiteral("initialConnectComplete")),
                 "Timeout waiting for initial connect");
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

    auto* mockConfig = new MockConfiguration(QStringLiteral("CameraZoomSettingsTest"));
    mockConfig->setFirmwareType(MAV_AUTOPILOT_PX4);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setDynamic(true);
    mockConfig->setEnableCamera(true);
    mockConfig->setCameraCaptureImage(true);
    mockConfig->setCameraHasBasicZoom(true);

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
