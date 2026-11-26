#include "VideoManagerVehicleTest.h"

#include <memory>

#include <QtCore/QScopeGuard>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

#include "MavlinkCameraControlInterface.h"
#include "MockLink.h"
#include "QGCCameraManager.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleVideoStreams.h"
#include "VideoManager.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"
#include "VideoTestHelpers.h"

namespace {

// A VideoView with room for tiles. It follows the active vehicle's current camera.
constexpr const char* kVehicleVideoViewQml = R"QML(
import QtQuick
import QGroundControl.VideoManager

VideoView {
    width: 800
    height: 400
    showTiles: true
}
)QML";

}  // namespace

MavlinkCameraControlInterface* VideoManagerVehicleTest::_selectStreamingCamera()
{
    QGCCameraManager* const cameraManager = _vehicle ? _vehicle->cameraManager() : nullptr;
    if (!cameraManager || !waitForCondition([cameraManager] { return cameraManager->cameras()->count() >= 2; },
                                            TestTimeout::longMs(), u"two cameras discovered")) {
        return nullptr;
    }
    MavlinkCameraControlInterface* camera = nullptr;
    for (int i = 0; i < cameraManager->cameras()->count(); i++) {
        auto* const candidate = qobject_cast<MavlinkCameraControlInterface*>(cameraManager->cameras()->get(i));
        if (candidate && (candidate->compID() == MAV_COMP_ID_CAMERA)) {
            camera = candidate;
            cameraManager->setCurrentCamera(i);
        }
    }
    const bool streamsKnown = camera && waitForCondition([camera] { return camera->streamLabels().size() == 2; },
                                                         TestTimeout::longMs(), u"the camera's two streams announced");
    return streamsKnown ? camera : nullptr;
}

MavlinkCameraControlInterface* VideoManagerVehicleTest::_camera(int compId) const
{
    QGCCameraManager* const cameraManager = _vehicle ? _vehicle->cameraManager() : nullptr;
    for (int i = 0; cameraManager && (i < cameraManager->cameras()->count()); i++) {
        auto* const camera = qobject_cast<MavlinkCameraControlInterface*>(cameraManager->cameras()->get(i));
        if (camera && (camera->compID() == compId)) {
            return camera;
        }
    }
    return nullptr;
}

bool VideoManagerVehicleTest::_connectThermalCameras(MockConfiguration::CameraThermalStream thermalStream)
{
    QSignalSpy vehicleSpy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    auto* const mockConfig = new MockConfiguration(QStringLiteral("Thermal Camera MockLink"));
    mockConfig->setFirmwareType(MAV_AUTOPILOT_PX4);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setEnableCamera(true);
    mockConfig->setCameraThermalStream(thermalStream);
    _mockLink = MockLink::startMockLink(mockConfig);
    if (!_mockLink) {
        return false;
    }
    (void) connect(_mockLink, &QObject::destroyed, this, [this] { _mockLink = nullptr; });
    if (!waitForSignal(vehicleSpy, TestTimeout::longMs(), u"activeVehicleChanged")) {
        return false;
    }
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QGCCameraManager* const cameraManager = (_vehicle && waitForInitialConnect()) ? _vehicle->cameraManager() : nullptr;
    if (!cameraManager || !waitForCondition([cameraManager] { return cameraManager->cameras()->count() >= 2; },
                                            TestTimeout::longMs(), u"two cameras discovered")) {
        return false;
    }
    MavlinkCameraControlInterface* const camera = _camera(MAV_COMP_ID_CAMERA);
    if (!camera) {
        return false;
    }
    cameraManager->setCurrentCamera(cameraManager->cameras()->indexOf(camera));
    const auto hasThermalStream = [this](int compId) {
        MavlinkCameraControlInterface* const candidate = _camera(compId);
        return candidate && candidate->thermalStreamInstance();
    };
    return waitForCondition(
        [camera, hasThermalStream] {
            return (camera->streamLabels().size() == 1) &&
                   (hasThermalStream(MAV_COMP_ID_CAMERA) || hasThermalStream(MAV_COMP_ID_CAMERA2));
        },
        TestTimeout::longMs(), u"the cameras' streams announced");
}

void VideoManagerVehicleTest::_testTilesFollowCameraStreams()
{
    VideoSettings* const videoSettings = SettingsManager::instance()->videoSettings();
    VideoTestHelpers::FactRestorer facts;
    facts.set(videoSettings->videoSource(), QString(VideoSettings::videoDisabled));
    facts.set(videoSettings->maxVideoTiles(), 3);
    facts.set(videoSettings->uvcTileCamera(), QString());
    facts.set(videoSettings->disableWhenDisarmed(), false);

    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone, MockConfiguration::OptionEnableCamera);
    QVERIFY(_vehicle);
    QGCCameraManager* const cameraManager = _vehicle->cameraManager();
    QVERIFY(cameraManager);
    MavlinkCameraControlInterface* const camera = _selectStreamingCamera();
    QVERIFY(camera);

    const auto settle = [] { QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall); };
    const auto started = [this] {
        return _mockLink->receivedMavCommandCount(MAV_CMD_VIDEO_START_STREAMING, MAV_COMP_ID_CAMERA);
    };
    const auto stopped = [this] {
        return _mockLink->receivedMavCommandCount(MAV_CMD_VIDEO_STOP_STREAMING, MAV_COMP_ID_CAMERA);
    };
    // QGC requests a stream's status with REQUEST_MESSAGE or the deprecated command, in turn.
    const auto statusRequests = [this] {
        return _mockLink->receivedRequestMessageCount(MAV_COMP_ID_CAMERA, MAVLINK_MSG_ID_VIDEO_STREAM_STATUS) +
               _mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_VIDEO_STREAM_STATUS, MAV_COMP_ID_CAMERA);
    };

    VideoManager videoManager;
    videoManager._receiverFactory = [](QObject* parent) { return new FakeVideoReceiver(parent); };
    videoManager._createVideoReceivers();
    videoManager._vehicleStreams->setVehicle(_vehicle);
    settle();
    // The vehicle's current stream is asked for, as ever. The second stream shares its address: the camera sends
    // one of the two at a time, so it gets no tile.
    VideoStreamController* const firstSource = videoManager.primaryStream();
    auto* const firstReceiver = qobject_cast<FakeVideoReceiver*>(firstSource->receiver());
    QVERIFY(firstReceiver);
    QCOMPARE(firstReceiver->config().uri, QStringLiteral("udp://127.0.0.1:5600"));
    QCOMPARE(videoManager._sources.size(), 3);
    QVERIFY_TRUE_WAIT(started() == 1, TestTimeout::mediumMs());

    // The camera advertises a third stream, at an address of its own. No row of tiles is on screen, as with the
    // video in the picture-in-picture: the stream gets no source, and the camera isn't asked to transmit it.
    mavlink_video_stream_information_t zoom{};
    zoom.stream_id = 3;
    zoom.count = 3;
    zoom.type = VIDEO_STREAM_TYPE_RTPUDP;
    zoom.encoding = VIDEO_STREAM_ENCODING_H264;
    zoom.flags = VIDEO_STREAM_STATUS_FLAGS_RUNNING;
    (void) qstrncpy(zoom.name, "Zoom", sizeof(zoom.name));
    (void) qstrncpy(zoom.uri, "udp://127.0.0.1:5601", sizeof(zoom.uri));
    camera->handleVideoStreamInformation(zoom);
    emit cameraManager->streamChanged();
    settle();
    QCOMPARE(videoManager._sources.size(), 3);
    QCOMPARE(started(), 1);

    // A row of tiles is shown and tells its room: the stream plays in a tile, named after the camera, as the vehicle
    // has two, and the stream, and the camera is asked to transmit it.
    videoManager.setTileRoom(3);
    settle();
    QCOMPARE(videoManager._sources.size(), 4);
    VideoStreamController* const zoomSource = videoManager._sources.last();
    QVERIFY(zoomSource->tile());
    QCOMPARE(zoomSource->displayName(), camera->modelName() + QStringLiteral(": Zoom"));
    auto* const zoomReceiver = qobject_cast<FakeVideoReceiver*>(zoomSource->receiver());
    QVERIFY(zoomReceiver);
    QCOMPARE(zoomReceiver->config().uri, QStringLiteral("udp://127.0.0.1:5601"));
    QVERIFY_TRUE_WAIT(started() == 2, TestTimeout::mediumMs());
    QCOMPARE(stopped(), 0);

    // The tile is selected: its stream becomes the camera's current one and the main picture, and the first stream
    // takes the tile. Both play on, and the camera is asked to stop or start neither. The request for the new
    // current stream's status follows any such command.
    for (FakeVideoReceiver* const receiver : {firstReceiver, zoomReceiver}) {
        receiver->configChanges = 0;
        receiver->activeChanges = 0;
    }
    const int statusRequestsBefore = statusRequests();
    videoManager.selectSource(zoomSource);
    settle();
    QCOMPARE(camera->streamLabels().at(camera->currentStream()), QStringLiteral("Zoom"));
    QCOMPARE(videoManager.primaryStream(), zoomSource);
    QVERIFY(!zoomSource->tile());
    QCOMPARE(videoManager._sources.size(), 4);
    QCOMPARE(videoManager._sources.last(), firstSource);
    QVERIFY(firstSource->tile());
    for (const FakeVideoReceiver* const receiver : {firstReceiver, zoomReceiver}) {
        QVERIFY(receiver->active());
        QCOMPARE(receiver->configChanges, 0);
        QCOMPARE(receiver->activeChanges, 0);
    }
    QVERIFY_TRUE_WAIT(statusRequests() > statusRequestsBefore, TestTimeout::mediumMs());
    QCOMPARE(started(), 2);
    QCOMPARE(stopped(), 0);

    // Video that is stopped is not to be transmitted: the camera is asked to stop the tile's stream and then the
    // current one, and to start both again with the video.
    videoManager.stopVideo();
    QVERIFY_TRUE_WAIT(stopped() == 2, TestTimeout::mediumMs());
    QVERIFY(!_mockLink->cameraStreamTransmitted(MAV_COMP_ID_CAMERA, 1));
    QCOMPARE(videoManager._sources.size(), 4);
    videoManager.startVideo();
    QVERIFY_TRUE_WAIT(started() == 4, TestTimeout::mediumMs());
    QVERIFY(_mockLink->cameraStreamTransmitted(MAV_COMP_ID_CAMERA, 1));
    QCOMPARE(stopped(), 2);

    // The row has been out of view for longer than its grace period, as with the video back in the
    // picture-in-picture, and tells that it has no room: the first stream leaves the display, and the camera is asked
    // to stop it.
    videoManager.setTileRoom(0);
    settle();
    QCOMPARE(videoManager._sources.size(), 3);
    QVERIFY_TRUE_WAIT(!_mockLink->cameraStreamTransmitted(MAV_COMP_ID_CAMERA, 1), TestTimeout::mediumMs());
    QCOMPARE(stopped(), 3);

    // The vehicle is no longer the one on display: its current stream is stopped, as ever.
    videoManager._vehicleStreams->setVehicle(nullptr);
    QVERIFY_TRUE_WAIT(stopped() == 4, TestTimeout::mediumMs());
    QCOMPARE(started(), 4);
}

void VideoManagerVehicleTest::_testVideoOffStopsCameraStream()
{
    VideoSettings* const videoSettings = SettingsManager::instance()->videoSettings();
    VideoTestHelpers::FactRestorer facts;
    facts.set(videoSettings->videoSource(), QString(VideoSettings::videoDisabled));
    facts.set(videoSettings->uvcTileCamera(), QString());
    facts.set(videoSettings->disableWhenDisarmed(), true);

    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone, MockConfiguration::OptionEnableCamera);
    QVERIFY(_vehicle);
    QVERIFY(!_vehicle->armed());

    const auto settle = [] { QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall); };
    const auto transmitted = [this] { return _mockLink->cameraStreamTransmitted(MAV_COMP_ID_CAMERA, 1); };
    const auto started = [this] {
        return _mockLink->receivedMavCommandCount(MAV_CMD_VIDEO_START_STREAMING, MAV_COMP_ID_CAMERA);
    };
    const auto stopped = [this] {
        return _mockLink->receivedMavCommandCount(MAV_CMD_VIDEO_STOP_STREAMING, MAV_COMP_ID_CAMERA);
    };
    const auto setArmed = [this](bool armed) {
        _vehicle->setArmed(armed, false /* showError */);
        return waitForCondition([this, armed] { return _vehicle->armed() == armed; }, TestTimeout::longMs(),
                                u"the vehicle's armed state");
    };

    // The vehicle is on display before its cameras are found, as one that has just connected.
    VideoManager videoManager;
    videoManager._receiverFactory = [](QObject* parent) { return new FakeVideoReceiver(parent); };
    videoManager._createVideoReceivers();
    videoManager._vehicleStreams->setVehicle(_vehicle);
    QVERIFY(_selectStreamingCamera());
    settle();
    auto* const receiver = qobject_cast<FakeVideoReceiver*>(videoManager.primaryStream()->receiver());
    QVERIFY(receiver);
    QCOMPARE(receiver->config().uri, QStringLiteral("udp://127.0.0.1:5600"));

    // It is disarmed: its video is off without it having changed its armed state, and its camera is told to stop the
    // stream it transmits without being asked.
    QVERIFY(!videoManager.hasVideo());
    QCOMPARE(videoManager.noVideoReason(), VideoManager::NoVideoReason::DisabledWhileDisarmed);
    QVERIFY(!receiver->active());
    QVERIFY_TRUE_WAIT(!transmitted(), TestTimeout::mediumMs());
    QCOMPARE(stopped(), 1);
    QCOMPARE(started(), 0);

    // Armed, it gets its video, and the camera transmits again.
    QVERIFY(setArmed(true));
    QVERIFY(videoManager.hasVideo());
    settle();
    QVERIFY(receiver->active());
    QVERIFY_TRUE_WAIT(transmitted(), TestTimeout::mediumMs());
    QCOMPARE(started(), 1);

    QVERIFY(setArmed(false));
    QVERIFY(!videoManager.hasVideo());
    QVERIFY_TRUE_WAIT(!transmitted(), TestTimeout::mediumMs());
    QCOMPARE(stopped(), 2);

    // The setting turned off and on again takes the vehicle as it is: disarmed.
    facts.set(videoSettings->disableWhenDisarmed(), false);
    QVERIFY(videoManager.hasVideo());
    QVERIFY_TRUE_WAIT(transmitted(), TestTimeout::mediumMs());
    facts.set(videoSettings->disableWhenDisarmed(), true);
    QVERIFY(!videoManager.hasVideo());
    QVERIFY_TRUE_WAIT(!transmitted(), TestTimeout::mediumMs());
    QCOMPARE(started(), 2);
    QCOMPARE(stopped(), 3);

    // Video started again plays for the disarmed vehicle, until its armed state changes.
    videoManager.startVideo();
    QVERIFY(videoManager.hasVideo());
    settle();
    QVERIFY(receiver->active());
    QVERIFY_TRUE_WAIT(transmitted(), TestTimeout::mediumMs());
    QCOMPARE(started(), 3);

    // Video that is stopped is not transmitted either.
    videoManager.stopVideo();
    QVERIFY(!receiver->active());
    QVERIFY_TRUE_WAIT(!transmitted(), TestTimeout::mediumMs());
    QCOMPARE(stopped(), 4);
    videoManager.startVideo();
    QVERIFY_TRUE_WAIT(transmitted(), TestTimeout::mediumMs());
    QCOMPARE(started(), 4);

    QVERIFY(setArmed(true));
    QVERIFY(videoManager.hasVideo());
    QVERIFY(setArmed(false));
    QVERIFY(!videoManager.hasVideo());
    QVERIFY_TRUE_WAIT(!transmitted(), TestTimeout::mediumMs());
    QCOMPARE(stopped(), 5);

    // The camera was told to stop already when the vehicle leaves the display.
    videoManager._vehicleStreams->setVehicle(nullptr);
    QVERIFY(setArmed(true));
    QCOMPARE(stopped(), 5);
    QCOMPARE(started(), 4);
}

void VideoManagerVehicleTest::_testThermalStreamOfAnyCameraIsShown_data()
{
    QTest::addColumn<MockConfiguration::CameraThermalStream>("thermalStream");
    QTest::addColumn<int>("thermalCompId");
    QTest::addColumn<int>("thermalStreamId");

    QTest::newRow("the current camera's second stream")
        << MockConfiguration::CameraThermalStreamSecondStream << static_cast<int>(MAV_COMP_ID_CAMERA) << 2;
    // As mavlink-camera-manager advertises it.
    QTest::newRow("another camera's only stream")
        << MockConfiguration::CameraThermalStreamSecondCamera << static_cast<int>(MAV_COMP_ID_CAMERA2) << 1;
}

void VideoManagerVehicleTest::_testThermalStreamOfAnyCameraIsShown()
{
    QFETCH(MockConfiguration::CameraThermalStream, thermalStream);
    QFETCH(int, thermalCompId);
    QFETCH(int, thermalStreamId);

    VideoSettings* const videoSettings = SettingsManager::instance()->videoSettings();
    VideoTestHelpers::FactRestorer facts;
    facts.set(videoSettings->videoSource(), QString(VideoSettings::videoDisabled));
    facts.set(videoSettings->maxVideoTiles(), 3);
    facts.set(videoSettings->uvcTileCamera(), QString());
    facts.set(videoSettings->disableWhenDisarmed(), false);

    QVERIFY(_connectThermalCameras(thermalStream));
    MavlinkCameraControlInterface* const camera = _camera(MAV_COMP_ID_CAMERA);
    MavlinkCameraControlInterface* const thermalCamera = _camera(thermalCompId);
    QVERIFY(thermalCamera);
    QGCVideoStreamInfo* const thermalInfo = thermalCamera->thermalStreamInstance();
    QVERIFY(thermalInfo);
    QCOMPARE(thermalInfo->streamID(), thermalStreamId);
    // The view's mode is a setting the cameras share, which the camera saves.
    const auto restoreThermalMode =
        qScopeGuard([camera, mode = camera->thermalMode()] { camera->setThermalMode(mode); });
    camera->setThermalMode(MavlinkCameraControlInterface::THERMAL_BLEND);

    const auto settle = [] { QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall); };
    const auto transmitted = [this, thermalCompId, thermalStreamId] {
        return _mockLink->cameraStreamTransmitted(static_cast<uint8_t>(thermalCompId),
                                                  static_cast<uint8_t>(thermalStreamId));
    };

    VideoManager videoManager;
    videoManager._receiverFactory = [](QObject* parent) { return new FakeVideoReceiver(parent); };
    videoManager._createVideoReceivers();
    videoManager._vehicleStreams->setVehicle(_vehicle);
    videoManager.setTileRoom(3);
    settle();

    // The visible-light stream is the main picture, and the thermal stream is the vehicle's, whichever camera has
    // it: it goes with the main picture, and gets no tile.
    VideoStreamController* const thermalSource = videoManager.thermalStream();
    auto* const primaryReceiver = qobject_cast<FakeVideoReceiver*>(videoManager.primaryStream()->receiver());
    auto* const thermalReceiver = qobject_cast<FakeVideoReceiver*>(thermalSource->receiver());
    QVERIFY(primaryReceiver);
    QVERIFY(thermalReceiver);
    QCOMPARE(primaryReceiver->config().uri, QStringLiteral("udp://127.0.0.1:5600"));
    QCOMPARE(thermalSource->streamInfo(), thermalInfo);
    QVERIFY(videoManager.hasThermal());
    QCOMPARE(videoManager._sources.size(), 3);

    // Nobody looks at it yet: it isn't connected to, and once the camera has been asked for its current stream, as
    // ever, it has not been asked for the thermal one.
    QVERIFY_TRUE_WAIT(_mockLink->receivedMavCommandCount(MAV_CMD_VIDEO_START_STREAMING, MAV_COMP_ID_CAMERA) == 1,
                      TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(!_vehicle->isMavCommandPending(MAV_COMP_ID_CAMERA, MAV_CMD_VIDEO_START_STREAMING) &&
                          !_vehicle->isMavCommandPending(thermalCompId, MAV_CMD_VIDEO_START_STREAMING),
                      TestTimeout::mediumMs());
    QVERIFY(!thermalReceiver->active());
    QVERIFY(!transmitted());

    // The video view shows it over the main picture, as the current camera's thermal mode asks: the stream
    // connects, and the camera that has it is asked to transmit it.
    VideoTestHelpers::InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> view = engine.create<QQuickItem>(
        kVehicleVideoViewQml, {{QStringLiteral("videoManager"), QVariant::fromValue(&videoManager)}});
    QVERIFY2(view, qPrintable(engine.error()));
    const QQuickItem* const overlay = view->findChild<QQuickItem*>(QStringLiteral("videoViewThermal"));
    QVERIFY(overlay);
    settle();
    QVERIFY(thermalReceiver->videoOutput());
    QVERIFY(thermalReceiver->active());
    QCOMPARE(thermalReceiver->config().uri, QStringLiteral("udp://127.0.0.1:5601"));
    QVERIFY_TRUE_WAIT(transmitted(), TestTimeout::mediumMs());
    primaryReceiver->setDecoding(true);
    QVERIFY(overlay->isVisible());

    // The thermal view is turned off. Once the stream has been out of use for its grace period it disconnects, and
    // its camera is asked to stop it.
    thermalSource->_idleTimer.setInterval(1);
    camera->setThermalMode(MavlinkCameraControlInterface::THERMAL_OFF);
    QVERIFY_TRUE_WAIT(!transmitted(), TestTimeout::mediumMs());
    QVERIFY(!thermalReceiver->active());
    QVERIFY(!overlay->isVisible());

    // Turned on again, it is asked for again, and no longer while the video is stopped.
    camera->setThermalMode(MavlinkCameraControlInterface::THERMAL_PIP);
    QVERIFY_TRUE_WAIT(transmitted(), TestTimeout::mediumMs());
    videoManager.stopVideo();
    QVERIFY_TRUE_WAIT(!transmitted(), TestTimeout::mediumMs());
    videoManager.startVideo();
    QVERIFY_TRUE_WAIT(transmitted(), TestTimeout::mediumMs());
}

void VideoManagerVehicleTest::_testThermalCameraIsMainPictureWhileCurrent()
{
    VideoSettings* const videoSettings = SettingsManager::instance()->videoSettings();
    VideoTestHelpers::FactRestorer facts;
    // A video source to fall back to, which a camera without a stream to show would get.
    facts.set(videoSettings->videoSource(), QString(VideoSettings::videoSourceUDPH264));
    facts.set(videoSettings->udpUrl(), QStringLiteral("0.0.0.0:5700"));
    facts.set(videoSettings->maxVideoTiles(), 3);
    facts.set(videoSettings->uvcTileCamera(), QString());
    facts.set(videoSettings->disableWhenDisarmed(), false);

    // As mavlink-camera-manager: a camera for the visible-light stream and one for the thermal stream.
    QVERIFY(_connectThermalCameras(MockConfiguration::CameraThermalStreamSecondCamera));
    QGCCameraManager* const cameraManager = _vehicle->cameraManager();
    MavlinkCameraControlInterface* const camera = _camera(MAV_COMP_ID_CAMERA);
    MavlinkCameraControlInterface* const thermalCamera = _camera(MAV_COMP_ID_CAMERA2);
    QVERIFY(thermalCamera);
    QGCVideoStreamInfo* const thermalInfo = thermalCamera->thermalStreamInstance();
    QVERIFY(thermalInfo);
    const auto restoreThermalMode =
        qScopeGuard([camera, mode = camera->thermalMode()] { camera->setThermalMode(mode); });
    camera->setThermalMode(MavlinkCameraControlInterface::THERMAL_BLEND);

    const QString visibleUri = QStringLiteral("udp://127.0.0.1:5600");
    const QString thermalUri = QStringLiteral("udp://127.0.0.1:5601");
    // Also what a deleted output still has to tell its source.
    const auto settle = [] {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    };
    const auto transmitted = [this] { return _mockLink->cameraStreamTransmitted(MAV_COMP_ID_CAMERA2, 1); };
    const auto started = [this] {
        return _mockLink->receivedMavCommandCount(MAV_CMD_VIDEO_START_STREAMING, MAV_COMP_ID_CAMERA2);
    };
    const auto stopped = [this] {
        return _mockLink->receivedMavCommandCount(MAV_CMD_VIDEO_STOP_STREAMING, MAV_COMP_ID_CAMERA2);
    };

    VideoManager videoManager;
    videoManager._receiverFactory = [](QObject* parent) { return new FakeVideoReceiver(parent); };
    videoManager._createVideoReceivers();
    videoManager._vehicleStreams->setVehicle(_vehicle);
    VideoTestHelpers::InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> view = engine.create<QQuickItem>(
        kVehicleVideoViewQml, {{QStringLiteral("videoManager"), QVariant::fromValue(&videoManager)}});
    QVERIFY2(view, qPrintable(engine.error()));
    settle();

    VideoStreamController* const thermalSource = videoManager.thermalStream();
    auto* const thermalReceiver = qobject_cast<FakeVideoReceiver*>(thermalSource->receiver());
    QVERIFY(thermalReceiver);
    const auto primaryReceiver = [&videoManager] {
        return qobject_cast<FakeVideoReceiver*>(videoManager.primaryStream()->receiver());
    };
    // While the visible-light camera is the current one: its stream, with the other camera's thermal stream over it.
    const auto verifyThermalOverVisible = [&] {
        QCOMPARE(cameraManager->currentCameraInstance(), camera);
        QVERIFY(primaryReceiver());
        QCOMPARE(primaryReceiver()->config().uri, visibleUri);
        QVERIFY(primaryReceiver()->active());
        QCOMPARE(thermalSource->streamInfo(), thermalInfo);
        QVERIFY(videoManager.hasThermal());
        QVERIFY(thermalReceiver->videoOutput());
        QCOMPARE(thermalReceiver->config().uri, thermalUri);
        QVERIFY(thermalReceiver->active());
        QCOMPARE(videoManager._sources.size(), 3);
    };
    // While the thermal camera is the current one: its only stream is the main picture, in place of the video source
    // of the settings, with nothing over it. The visible-light stream has a tile.
    const auto verifyThermalAsMainPicture = [&] {
        QCOMPARE(cameraManager->currentCameraInstance(), thermalCamera);
        QVERIFY(!thermalCamera->currentStreamInstance());
        QCOMPARE(videoManager.primaryStream()->streamInfo(), thermalInfo);
        QCOMPARE(videoManager.primaryStream()->displayName(),
                 QStringLiteral("%1: %2").arg(thermalCamera->modelName(), thermalInfo->name()));
        QVERIFY(primaryReceiver());
        QCOMPARE(primaryReceiver()->config().uri, thermalUri);
        QVERIFY(primaryReceiver()->active());
        QVERIFY(primaryReceiver()->videoOutput());
        QVERIFY(videoManager.autoStreamConfigured());
        QVERIFY(!thermalSource->streamInfo());
        QVERIFY(!videoManager.hasThermal());
        QVERIFY(!thermalReceiver->videoOutput());
        QVERIFY(!thermalReceiver->active());
        QCOMPARE(videoManager._sources.size(), 4);
        const VideoStreamController* const tile = videoManager._sources.last();
        QVERIFY(tile->tile());
        const auto* const tileReceiver = qobject_cast<FakeVideoReceiver*>(tile->receiver());
        QVERIFY(tileReceiver);
        QCOMPARE(tileReceiver->config().uri, visibleUri);
    };

    verifyThermalOverVisible();
    if (QTest::currentTestFailed()) {
        return;
    }
    QVERIFY_TRUE_WAIT(transmitted(), TestTimeout::mediumMs());
    QCOMPARE(started(), 1);

    // The thermal camera is made the current one.
    cameraManager->setCurrentCamera(cameraManager->cameras()->indexOf(thermalCamera));
    settle();
    verifyThermalAsMainPicture();
    if (QTest::currentTestFailed()) {
        return;
    }

    // The visible-light stream is selected in its tile: its camera is the current one again, and the thermal stream
    // is shown over it again.
    videoManager.selectSource(videoManager._sources.last());
    settle();
    verifyThermalOverVisible();
    if (QTest::currentTestFailed()) {
        return;
    }

    // The same as the camera panel changes the current camera, there and back.
    cameraManager->setCurrentCamera(cameraManager->cameras()->indexOf(thermalCamera));
    settle();
    verifyThermalAsMainPicture();
    if (QTest::currentTestFailed()) {
        return;
    }
    cameraManager->setCurrentCamera(cameraManager->cameras()->indexOf(camera));
    settle();
    verifyThermalOverVisible();
    if (QTest::currentTestFailed()) {
        return;
    }
    // The thermal camera lists no stream, so the stream is never its current one: it was asked to transmit it for
    // as long as a source played it, which the stream's grace period made one request.
    QVERIFY_TRUE_WAIT(!_vehicle->isMavCommandPending(MAV_COMP_ID_CAMERA2, MAV_CMD_VIDEO_START_STREAMING),
                      TestTimeout::mediumMs());
    QVERIFY(transmitted());
    QCOMPARE(started(), 1);
    QCOMPARE(stopped(), 0);

    // The thermal camera is lost, as the camera manager drops a camera whose heartbeats stopped: nothing of its
    // stream is left on display.
    cameraManager->cameras()->removeOne(thermalCamera)->deleteLater();
    cameraManager->setCurrentCamera(0);
    emit cameraManager->streamChanged();
    settle();
    QVERIFY(!thermalSource->streamInfo());
    QVERIFY(!videoManager.hasThermal());
    QVERIFY(!thermalReceiver->videoOutput());
    QVERIFY(!thermalReceiver->active());
    QCOMPARE(primaryReceiver()->config().uri, visibleUri);
    QCOMPARE(videoManager._sources.size(), 3);
}

UT_REGISTER_TEST(VideoManagerVehicleTest, TestLabel::Integration, TestLabel::Vehicle)
