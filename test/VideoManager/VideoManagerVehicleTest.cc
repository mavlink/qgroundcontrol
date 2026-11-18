#include "VideoManagerVehicleTest.h"

#include "MavlinkCameraControlInterface.h"
#include "MockLink.h"
#include "QGCCameraManager.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VideoManager.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"
#include "VideoTestHelpers.h"

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
    videoManager._setActiveVehicle(_vehicle);
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
    videoManager._setActiveVehicle(nullptr);
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
    videoManager._setActiveVehicle(_vehicle);
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
    videoManager._setActiveVehicle(nullptr);
    QVERIFY(setArmed(true));
    QCOMPARE(stopped(), 5);
    QCOMPARE(started(), 4);
}

UT_REGISTER_TEST(VideoManagerVehicleTest, TestLabel::Integration, TestLabel::Vehicle)
