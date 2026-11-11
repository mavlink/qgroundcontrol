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

void VideoManagerVehicleTest::_testTilesFollowCameraStreams()
{
    VideoSettings* const videoSettings = SettingsManager::instance()->videoSettings();
    VideoTestHelpers::FactRestorer facts;
    facts.set(videoSettings->videoSource(), QString(VideoSettings::videoDisabled));
    facts.set(videoSettings->maxVideoTiles(), 3);
    facts.set(videoSettings->uvcTileCamera(), QString());

    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone, MockConfiguration::OptionEnableCamera);
    QVERIFY(_vehicle);
    QGCCameraManager* const cameraManager = _vehicle->cameraManager();
    QVERIFY(cameraManager);
    // The mock vehicle's first camera advertises two streams at one address; its second camera has none.
    MavlinkCameraControlInterface* camera = nullptr;
    QVERIFY_TRUE_WAIT(cameraManager->cameras()->count() >= 2, TestTimeout::longMs());
    for (int i = 0; i < cameraManager->cameras()->count(); i++) {
        auto* const candidate = qobject_cast<MavlinkCameraControlInterface*>(cameraManager->cameras()->get(i));
        if (candidate && (candidate->compID() == MAV_COMP_ID_CAMERA)) {
            camera = candidate;
            cameraManager->setCurrentCamera(i);
        }
    }
    QVERIFY(camera);
    QVERIFY_TRUE_WAIT(camera->streamLabels().count() == 2, TestTimeout::longMs());

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

    // The camera advertises a third stream, at an address of its own: it plays in a tile, named after the camera,
    // as the vehicle has two, and the stream, and the camera is asked to transmit it.
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

    // Without tiles, the first stream leaves the display: the camera is asked to stop it.
    facts.set(videoSettings->maxVideoTiles(), 0);
    QCOMPARE(videoManager._sources.size(), 3);
    QVERIFY_TRUE_WAIT(!_mockLink->cameraStreamTransmitted(MAV_COMP_ID_CAMERA, 1), TestTimeout::mediumMs());
    QCOMPARE(stopped(), 1);

    // The vehicle is no longer the one on display: its current stream is stopped, as ever.
    videoManager._setActiveVehicle(nullptr);
    QVERIFY_TRUE_WAIT(stopped() == 2, TestTimeout::mediumMs());
    QCOMPARE(started(), 2);
}

UT_REGISTER_TEST(VideoManagerVehicleTest, TestLabel::Integration, TestLabel::Vehicle)
