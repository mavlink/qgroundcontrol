#include "VideoManagerTest.h"

#include <optional>

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "QGCNetworkAvailabilityMonitor.h"
#include "SettingsManager.h"
#include "UVCCameraController.h"
#include "VideoDecoderOptions.h"
#include "VideoManager.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"
#include "VideoTestHelpers.h"

namespace {

using VideoTestHelpers::createVideoOutput;
using VideoTestHelpers::InlineQmlEngine;
using VideoTestHelpers::kStreamOutputQml;
using VideoTestHelpers::makeStreamInfo;
using VideoTestHelpers::showFrame;

/// The device's network as the test reports it, in place of the platform's.
class StandInNetworkMonitor : public QGCNetworkAvailabilityMonitor
{
public:
    explicit StandInNetworkMonitor(bool available)
        : QGCNetworkAvailabilityMonitor(available, nullptr)
    {}

    void setAvailable(bool available) { _setAvailable(available); }
};

}  // namespace

void VideoManagerTest::_testSettingReachesReceiver_data()
{
    QTest::addColumn<QString>("setting");
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("field");
    QTest::addColumn<QVariant>("expected");

    QTest::newRow("lowLatencyMode") << "lowLatencyMode" << QVariant(true) << "lowLatency" << QVariant(true);
    QTest::newRow("rtpJitterLatencyMs") << "rtpJitterLatencyMs" << QVariant(300) << "rtpJitterLatencyMs"
                                        << QVariant(300);
    QTest::newRow("softwareDecoderThreads")
        << "softwareDecoderThreads" << QVariant(2) << "softwareDecoderThreads" << QVariant(2);
    QTest::newRow("rtspAutoReconnect") << "rtspAutoReconnect" << QVariant(false) << "autoReconnect" << QVariant(false);
    QTest::newRow("udpUrl") << "udpUrl" << QVariant(QStringLiteral("0.0.0.0:5700")) << "uri"
                            << QVariant(QStringLiteral("udp://0.0.0.0:5700"));
}

void VideoManagerTest::_testSettingReachesReceiver()
{
    QFETCH(QString, setting);
    QFETCH(QVariant, value);
    QFETCH(QString, field);
    QFETCH(QVariant, expected);

    Fact* const fact = _videoSettings->property(setting.toLatin1().constData()).value<Fact*>();
    QVERIFY(fact);
    _facts->set(fact, value);
    _settle();

    QCOMPARE(_primary->configChanges, 1);
    QVERIFY(_primary->active());
    const VideoReceiver::Config& config = _primary->config();
    const QVariant actual = (field == QLatin1String("lowLatency"))           ? QVariant(config.lowLatency)
                            : (field == QLatin1String("rtpJitterLatencyMs")) ? QVariant(config.rtpJitterLatencyMs)
                            : (field == QLatin1String("softwareDecoderThreads"))
                                ? QVariant(config.softwareDecoderThreads)
                            : (field == QLatin1String("autoReconnect")) ? QVariant(config.autoReconnect)
                                                                        : QVariant(config.uri);
    QCOMPARE(actual, expected);
}

void VideoManagerTest::_testSettingChangesCoalesce()
{
    _facts->set(_videoSettings->videoSource(), QString(VideoSettings::videoSourceRTSP));
    _facts->set(_videoSettings->rtspUrl(), QStringLiteral("rtsp://cam.local/other"));
    _facts->set(_videoSettings->lowLatencyMode(), true);
    _facts->set(_videoSettings->rtspTimeout(), 15);
    _settle();

    QCOMPARE(_primary->configChanges, 1);
    QCOMPARE(_primary->activeChanges, 0);
    QCOMPARE(_primary->config().uri, QStringLiteral("rtsp://cam.local/other"));
    QVERIFY(_primary->config().lowLatency);
    QCOMPARE(_primary->config().timeout, std::chrono::seconds(15));
}

void VideoManagerTest::_testRestartingSettingsRestartStreams_data()
{
    QTest::addColumn<QString>("setting");
    QTest::addColumn<QVariant>("value");

    QTest::newRow("videoConversionElement")
        << "videoConversionElement" << QVariant(QStringLiteral("videoconvertscale"));
    QTest::newRow("forceCpuVideoPath") << "forceCpuVideoPath" << QVariant(true);
    QTest::newRow("forceVideoDecoder") << "forceVideoDecoder"
                                       << QVariant(static_cast<int>(VideoDecoderOption::Software));
}

void VideoManagerTest::_testRestartingSettingsRestartStreams()
{
    QFETCH(QString, setting);
    QFETCH(QVariant, value);

    Fact* const fact = _videoSettings->property(setting.toLatin1().constData()).value<Fact*>();
    QVERIFY(fact);
    _facts->set(fact, value);
    _settle();

    QCOMPARE(_primary->restarts, 1);
    QCOMPARE(_primary->configChanges, 0);
    // The thermal stream isn't active, and restart() doesn't start a stream.
    QCOMPARE(_thermal->restarts, 0);
    QVERIFY(!_thermal->active());
}

void VideoManagerTest::_testStopVideoCancelsEverything()
{
    QVERIFY(_primary->active());
    _primary->setState(VideoReceiver::State::Reconnecting);
    _facts->set(_videoSettings->lowLatencyMode(), true);

    _videoManager->stopVideo();
    QVERIFY(!_primary->active());

    // The settings change queued before stopVideo() must not start the stream again.
    _settle();
    QVERIFY(!_primary->active());
    QCOMPARE(_primary->activeChanges, 1);
    QVERIFY(_primary->config().lowLatency);

    _videoManager->startVideo();
    _settle();
    QVERIFY(_primary->active());
}

void VideoManagerTest::_testAutoStreamLeavesSettingsAlone()
{
    QList<QSharedPointer<QSignalSpy>> settingSpies;
    for (Fact* fact : {_videoSettings->videoSource(), _videoSettings->udpUrl(), _videoSettings->rtspUrl(),
                       _videoSettings->tcpUrl()}) {
        settingSpies.append(QSharedPointer<QSignalSpy>::create(fact, &Fact::rawValueChanged));
    }

    const auto primaryInfo = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5601");
    const auto thermalInfo =
        makeStreamInfo(VIDEO_STREAM_TYPE_RTSP, "rtsp://10.0.0.1/thermal", VIDEO_STREAM_ENCODING_H264, true);
    // The thermal view is on: its stream plays only while it is in use.
    QQuickItem thermalView;
    _videoManager->thermalStream()->registerVideoOutput(&thermalView);
    _videoManager->primaryStream()->setStreamInfo(primaryInfo.get());
    _videoManager->thermalStream()->setStreamInfo(thermalInfo.get());
    _settle();

    QCOMPARE(_primary->config().uri, QStringLiteral("udp://0.0.0.0:5601"));
    QVERIFY(_primary->active());
    QCOMPARE(_thermal->config().uri, QStringLiteral("rtsp://10.0.0.1/thermal"));
    QVERIFY(_thermal->active());
    QVERIFY(_videoManager->autoStreamConfigured());
    QVERIFY(_videoManager->isStreamSource());
    QVERIFY(_videoManager->hasThermal());
    for (const QSharedPointer<QSignalSpy>& spy : std::as_const(settingSpies)) {
        QCOMPARE(spy->count(), 0);
    }

    // The camera repeats its stream information: nothing restarts.
    _resetCounters();
    emit primaryInfo->infoChanged();
    emit thermalInfo->infoChanged();
    _settle();
    QCOMPARE(_primary->configChanges, 0);
    QCOMPARE(_thermal->configChanges, 0);
}

void VideoManagerTest::_testThermalOnlyChangeReconfiguresThermal()
{
    const auto first =
        makeStreamInfo(VIDEO_STREAM_TYPE_RTSP, "rtsp://10.0.0.1/thermal1", VIDEO_STREAM_ENCODING_H264, true);
    _videoManager->thermalStream()->setStreamInfo(first.get());
    _settle();
    QCOMPARE(_thermal->config().uri, QStringLiteral("rtsp://10.0.0.1/thermal1"));
    _resetCounters();

    const auto second =
        makeStreamInfo(VIDEO_STREAM_TYPE_RTSP, "rtsp://10.0.0.1/thermal2", VIDEO_STREAM_ENCODING_H264, true);
    _videoManager->thermalStream()->setStreamInfo(second.get());
    _settle();

    QCOMPARE(_thermal->configChanges, 1);
    QCOMPARE(_thermal->config().uri, QStringLiteral("rtsp://10.0.0.1/thermal2"));
    QCOMPARE(_primary->configChanges, 0);
    QCOMPARE(_videoSettings->rtspUrl()->rawValue().toString(), QStringLiteral("rtsp://cam.local/manual"));
}

void VideoManagerTest::_testOtherStreamsPlayInTiles_data()
{
    QTest::addColumn<int>("maxTiles");
    QTest::addColumn<int>("tileRoom");  ///< What the row of tiles tells; -1 when none has
    QTest::addColumn<QStringList>("tileUris");

    const QString second = QStringLiteral("udp://0.0.0.0:5602");
    const QString wide = QStringLiteral("rtsp://10.0.0.2/wide");
    const QString fourth = QStringLiteral("udp://0.0.0.0:5604");
    QTest::newRow("no tiles") << 0 << 4 << QStringList();
    QTest::newRow("one tile") << 1 << 4 << QStringList({second});
    QTest::newRow("three tiles") << 3 << 4 << QStringList({second, wide, fourth});
    QTest::newRow("as many tiles as streams") << 4 << 4 << QStringList({second, wide, fourth});
    QTest::newRow("room for two of three tiles") << 3 << 2 << QStringList({second, wide});
    QTest::newRow("room for more tiles than allowed") << 1 << 4 << QStringList({second});
    QTest::newRow("no room for a tile") << 3 << 0 << QStringList();
    // No tile is on screen, as with the video in the picture-in-picture: nothing connects to the other streams.
    QTest::newRow("no row has told its room") << 3 << -1 << QStringList();
}

void VideoManagerTest::_testOtherStreamsPlayInTiles()
{
    QFETCH(int, maxTiles);
    QFETCH(int, tileRoom);
    QFETCH(QStringList, tileUris);

    _facts->set(_videoSettings->maxVideoTiles(), maxTiles);
    _videoManager->setTileRoom(tileRoom);
    const auto current = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5601");
    const auto second = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5602");
    // A camera sends the streams it advertises at one address one at a time: the stream on display has that port,
    // whatever the stream to listen for there.
    const auto sameAddress =
        makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "udp://127.0.0.1:5601", VIDEO_STREAM_ENCODING_H265);
    const auto wide = makeStreamInfo(VIDEO_STREAM_TYPE_RTSP, "rtsp://10.0.0.2/wide");
    const auto fourth = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5604");
    QSignalSpy sourcesSpy(_videoManager.get(), &VideoManager::sourcesChanged);
    _videoManager->_setAdvertisedStreams({current.get(), nullptr, QStringLiteral("Main")}, nullptr,
                                         {{second.get(), nullptr, QStringLiteral("Second")},
                                          {sameAddress.get(), nullptr, QStringLiteral("Same address")},
                                          {wide.get(), nullptr, QStringLiteral("Gimbal: Wide")},
                                          {fourth.get(), nullptr, QStringLiteral("Fourth")}});
    _settle();

    // The tiles' sources follow the three there always are, in the order the streams were given.
    const QList<VideoStreamController*> sources = _videoManager->_sources;
    QCOMPARE(sources.mid(0, 3),
             QList<VideoStreamController*>(
                 {_videoManager->primaryStream(), _videoManager->thermalStream(), _videoManager->_cameraSource}));
    QCOMPARE(sourcesSpy.count(), tileUris.isEmpty() ? 0 : 1);
    QStringList uris;
    QList<QPointer<VideoReceiver>> receivers;
    for (const VideoStreamController* tile : sources.mid(3)) {
        QCOMPARE(tile->role(), VideoStreamController::Role::Tile);
        QVERIFY(tile->tile());
        // Each plays its stream with a receiver of its own.
        const FakeVideoReceiver* const receiver = _receiverOf(tile);
        QVERIFY(receiver);
        QVERIFY(receiver->active());
        uris.append(receiver->config().uri);
        receivers.append(tile->receiver());
    }
    QCOMPARE(uris, tileUris);
    QCOMPARE(_videoManager->tileCount(), static_cast<int>(tileUris.size()));
    if (!tileUris.isEmpty()) {
        QCOMPARE(sources.at(3)->displayName(), QStringLiteral("Second"));
    }
    QCOMPARE(_primary->config().uri, QStringLiteral("udp://0.0.0.0:5601"));
    QVERIFY(!_videoManager->primaryStream()->tile());
    QCOMPARE(_videoManager->primaryStream()->displayName(), QStringLiteral("Main"));

    // The settings every stream is played with are a tile's too.
    _facts->set(_videoSettings->lowLatencyMode(), true);
    _settle();
    for (const VideoStreamController* tile : sources.mid(3)) {
        QVERIFY(_receiverOf(tile)->config().lowLatency);
    }

    // Once the streams are gone, so are the tiles, their sources and their receivers.
    sourcesSpy.clear();
    _videoManager->_setAdvertisedStreams({}, nullptr, {});
    QCOMPARE(_videoManager->_sources, sources.mid(0, 3));
    QCOMPARE(sourcesSpy.count(), tileUris.isEmpty() ? 0 : 1);
    for (const QPointer<VideoReceiver>& receiver : std::as_const(receivers)) {
        QVERIFY(receiver.isNull());
    }
}

void VideoManagerTest::_testStateChangeWhileTileGoesIsTakenUp()
{
    _facts->set(_videoSettings->disableWhenDisarmed(), true);
    const auto current = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5601");
    const auto other = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5602");
    const VideoManager::AdvertisedStream currentStream{current.get(), nullptr, QStringLiteral("Main")};
    _videoManager->_setAdvertisedStreams(currentStream, nullptr, {{other.get(), nullptr, QStringLiteral("Other")}});
    _settle();
    QCOMPARE(_videoManager->_sources.size(), 4);
    QVERIFY(_videoManager->hasVideo());

    // A receiver that is recording runs the event loop as it goes, here the tile's as its stream leaves. The vehicle
    // disarms in there, which turns the video off: that must hold once the tile is gone.
    (void) connect(_receiverOf(_videoManager->_sources.at(3)), &QObject::destroyed, this,
                   [this] { _videoManager->_onArmedChanged(false); });
    _videoManager->_setAdvertisedStreams(currentStream, nullptr, {});
    _settle();
    QCOMPARE(_videoManager->_sources.size(), 3);
    QVERIFY(!_videoManager->hasVideo());
    QVERIFY(!_primary->active());

    _videoManager->_setAdvertisedStreams({}, nullptr, {});
}

void VideoManagerTest::_testStreamInTileBecomesPrimaryAsItIs()
{
    const auto first = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5601");
    const auto second = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5602");
    const auto third = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5603");
    const VideoManager::AdvertisedStream firstStream{first.get(), nullptr, QStringLiteral("First")};
    const VideoManager::AdvertisedStream secondStream{second.get(), nullptr, QStringLiteral("Second")};
    const VideoManager::AdvertisedStream thirdStream{third.get(), nullptr, QStringLiteral("Third")};
    _videoManager->_setAdvertisedStreams(firstStream, nullptr, {secondStream, thirdStream});
    _settle();
    VideoStreamController* const firstSource = _videoManager->primaryStream();
    QCOMPARE(_videoManager->_sources.size(), 5);
    VideoStreamController* const secondSource = _videoManager->_sources.at(3);
    VideoStreamController* const thirdSource = _videoManager->_sources.at(4);
    const QList<FakeVideoReceiver*> receivers{_primary, _receiverOf(secondSource), _receiverOf(thirdSource)};
    _resetCounters();

    // The vehicle's current stream becomes the second one, as when its tile is selected: its source becomes the
    // primary stream and the first stream's takes its tile.
    QSignalSpy primarySpy(_videoManager.get(), &VideoManager::primaryStreamChanged);
    QSignalSpy mainSourceSpy(_videoManager.get(), &VideoManager::mainSourceChanged);
    _videoManager->_setAdvertisedStreams(secondStream, nullptr, {firstStream, thirdStream});
    _settle();
    QCOMPARE(primarySpy.count(), 1);
    QCOMPARE(_videoManager->primaryStream(), secondSource);
    QCOMPARE(mainSourceSpy.count(), 1);
    QCOMPARE(_videoManager->mainSource(), secondSource);
    QCOMPARE(secondSource->role(), VideoStreamController::Role::Primary);
    QVERIFY(!secondSource->tile());
    QCOMPARE(firstSource->role(), VideoStreamController::Role::Tile);
    QVERIFY(firstSource->tile());
    QCOMPARE(_videoManager->_sources,
             QList<VideoStreamController*>({secondSource, _videoManager->thermalStream(), _videoManager->_cameraSource,
                                            firstSource, thirdSource}));
    // Every stream plays on as it was.
    for (const FakeVideoReceiver* receiver : receivers) {
        QVERIFY(receiver->active());
        QCOMPARE(receiver->configChanges, 0);
        QCOMPARE(receiver->activeChanges, 0);
        QCOMPARE(receiver->restarts, 0);
    }
    QCOMPARE(_receiverOf(secondSource)->config().uri, QStringLiteral("udp://0.0.0.0:5602"));

    // The manager's video is the new primary stream's.
    QSignalSpy decodingSpy(_videoManager.get(), &VideoManager::decodingChanged);
    _primary->setDecoding(true);
    QVERIFY(!_videoManager->decoding());
    _receiverOf(secondSource)->setDecoding(true);
    QVERIFY(_videoManager->decoding());
    QCOMPARE(decodingSpy.count(), 1);

    // A stream that plays in no tile has no source to take over: the primary stream connects to it, as it does
    // without tiles.
    _facts->set(_videoSettings->maxVideoTiles(), 0);
    _settle();
    _resetCounters();
    _videoManager->_setAdvertisedStreams(thirdStream, nullptr, {firstStream, secondStream});
    _settle();
    QCOMPARE(primarySpy.count(), 1);
    QCOMPARE(_receiverOf(secondSource)->configChanges, 1);
    QCOMPARE(_receiverOf(secondSource)->config().uri, QStringLiteral("udp://0.0.0.0:5603"));
}

void VideoManagerTest::_testTilesStopAndStartWithTheVideo()
{
    const auto current = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5601");
    const auto second = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5602");
    _videoManager->_setAdvertisedStreams({current.get(), nullptr, QStringLiteral("Main")}, nullptr,
                                         {{second.get(), nullptr, QStringLiteral("Second")}});
    _settle();
    QCOMPARE(_videoManager->_sources.size(), 4);
    const FakeVideoReceiver* tile = _receiverOf(_videoManager->_sources.last());
    QVERIFY(tile->active());

    // Stopped with the video, and started again with it.
    _videoManager->stopVideo();
    QVERIFY(!tile->active());
    _videoManager->startVideo();
    _settle();
    QVERIFY(tile->active());

    // A stream back on its link is asked to connect now, in a tile as well.
    _videoManager->_communicationLostChanged(false);
    QCOMPARE(tile->reconnects, 1);

    // Video that is off while the vehicle is disarmed has no view to show tiles in: their streams go, and come back
    // with the video.
    _facts->set(_videoSettings->disableWhenDisarmed(), true);
    _videoManager->_onArmedChanged(false);
    QVERIFY(!_videoManager->hasVideo());
    QCOMPARE(_videoManager->_sources.size(), 3);
    _videoManager->_onArmedChanged(true);
    _settle();
    QCOMPARE(_videoManager->_sources.size(), 4);
    tile = _receiverOf(_videoManager->_sources.last());
    QVERIFY(tile);
    QVERIFY(tile->active());
}

void VideoManagerTest::_testCameraSourcePlaysVideoSourceOrTileCamera_data()
{
    QTest::addColumn<QString>("videoSource");
    QTest::addColumn<QString>("tileCamera");
    QTest::addColumn<int>("maxTiles");
    QTest::addColumn<int>("tileRoom");    ///< What the row of tiles tells; -1 when none has
    QTest::addColumn<QString>("camera");  ///< The camera that plays; empty for none
    QTest::addColumn<bool>("tile");
    QTest::addColumn<bool>("hasCamera");  ///< A camera is there to play, with room for its tile or without

    const QString udp = VideoSettings::videoSourceUDPH264;
    const QString attached = QStringLiteral("QGC Test Camera");
    const QString other = QStringLiteral("QGC Other Test Camera");
    const QString missing = QStringLiteral("QGC Test Missing Camera");
    QTest::newRow("the video source") << attached << QString() << 3 << 4 << attached << false << true;
    QTest::newRow("the video source, not the tile's camera")
        << attached << other << 3 << 4 << attached << false << true;
    QTest::newRow("the video source without tiles") << attached << QString() << 0 << 4 << attached << false << true;
    QTest::newRow("the video source without a row of tiles")
        << attached << QString() << 3 << -1 << attached << false << true;
    QTest::newRow("in a tile beside a stream") << udp << attached << 3 << 4 << attached << true << true;
    QTest::newRow("no tile's camera") << udp << QString() << 3 << 4 << QString() << false << false;
    QTest::newRow("no tiles") << udp << attached << 0 << 4 << QString() << false << false;
    // The camera's settings stay in view while its tile has no room, where it doesn't play.
    QTest::newRow("no room for its tile") << udp << attached << 3 << 0 << QString() << false << true;
    QTest::newRow("no row has told its room") << udp << attached << 3 << -1 << QString() << false << true;
    QTest::newRow("no stream to play beside")
        << QString(VideoSettings::videoDisabled) << attached << 3 << 4 << QString() << false << false;
    QTest::newRow("the tile's camera isn't attached") << udp << missing << 3 << 4 << QString() << false << false;
}

void VideoManagerTest::_testCameraSourcePlaysVideoSourceOrTileCamera()
{
    QFETCH(QString, videoSource);
    QFETCH(QString, tileCamera);
    QFETCH(int, maxTiles);
    QFETCH(int, tileRoom);
    QFETCH(QString, camera);
    QFETCH(bool, tile);
    QFETCH(bool, hasCamera);

    _videoManager->_cameraAttached = [](const QString& name) {
        return name.startsWith(QLatin1String("QGC ")) && !name.contains(QLatin1String("Missing"));
    };
    _videoManager->setTileRoom(tileRoom);
    _facts->set(_videoSettings->maxVideoTiles(), maxTiles);
    _facts->set(_videoSettings->uvcTileCamera(), tileCamera);
    _facts->set(_videoSettings->videoSource(), videoSource);
    if (!camera.isEmpty()) {
        // The camera the test attached isn't one Qt knows.
        expectLogMessage("Video.UVCCameraController", QtWarningMsg,
                         QRegularExpression(QStringLiteral("No camera named")));
    }
    _settle();
    if (!camera.isEmpty()) {
        verifyExpectedLogMessage();
    }

    const UVCCameraController* const uvcCamera = _videoManager->uvcCamera();
    QCOMPARE(uvcCamera->active(), !camera.isEmpty());
    if (!camera.isEmpty()) {
        QCOMPARE(uvcCamera->device(), camera);
        QCOMPARE(_videoManager->_cameraSource->displayName(), camera);
    }
    QCOMPARE(_videoManager->_cameraSource->tile(), tile);
    QCOMPARE(_videoManager->hasUvcCamera(), hasCamera);
    // isUvc says that the camera is the main picture, which it isn't from a tile.
    const bool mainPicture = !camera.isEmpty() && !tile;
    QCOMPARE(_videoManager->isUvc(), mainPicture);
    QCOMPARE(_videoManager->mainSource(), mainPicture ? _videoManager->_cameraSource : _videoManager->primaryStream());
    QCOMPARE(_videoManager->_uvcVideoSourceID, mainPicture ? camera : QString());
    QCOMPARE(_videoManager->isStreamSource(), videoSource == QString(VideoSettings::videoSourceUDPH264));
}

void VideoManagerTest::_testSelectedCameraTileTakesMainPicture()
{
    const QString camera = QStringLiteral("QGC Test Camera");
    _videoManager->_cameraAttached = [camera](const QString& name) { return name == camera; };
    const auto current = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5601");
    const auto thermalInfo =
        makeStreamInfo(VIDEO_STREAM_TYPE_RTSP, "rtsp://10.0.0.1/thermal", VIDEO_STREAM_ENCODING_H264, true);
    const auto second = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5602");
    const auto third = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5603");
    _facts->set(_videoSettings->maxVideoTiles(), 2);
    _facts->set(_videoSettings->uvcTileCamera(), camera);
    // The thermal view is on: its stream plays only while it is in use.
    QQuickItem thermalView;
    _videoManager->thermalStream()->registerVideoOutput(&thermalView);
    _videoManager->_setAdvertisedStreams(
        {current.get(), nullptr, QStringLiteral("Main")}, thermalInfo.get(),
        {{second.get(), nullptr, QStringLiteral("Second")}, {third.get(), nullptr, QStringLiteral("Third")}});
    // The camera the test attached isn't one Qt knows.
    expectLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("No camera named")));
    _settle();
    verifyExpectedLogMessage();
    VideoStreamController* const cameraSource = _videoManager->_cameraSource;
    VideoStreamController* const primaryStream = _videoManager->primaryStream();

    // The camera has the first of the two tiles, a stream the other.
    QVERIFY(cameraSource->tile());
    QCOMPARE(_videoManager->tileCount(), 2);
    QVERIFY(!_videoManager->isUvc());
    QCOMPARE(_videoManager->_sources.size(), 4);
    QVERIFY(_videoManager->hasThermal());
    QVERIFY(_thermal->active());
    _resetCounters();

    // Selected, the camera is the main picture: the primary stream plays on in the camera's tile, and its thermal
    // stream stops.
    QSignalSpy isUvcSpy(_videoManager.get(), &VideoManager::isUvcChanged);
    QSignalSpy mainSourceSpy(_videoManager.get(), &VideoManager::mainSourceChanged);
    _videoManager->selectSource(cameraSource);
    _settle();
    QCOMPARE(isUvcSpy.count(), 1);
    QVERIFY(_videoManager->isUvc());
    QCOMPARE(mainSourceSpy.count(), 1);
    QCOMPARE(_videoManager->mainSource(), cameraSource);
    QVERIFY(_videoSettings->uvcTileCameraAsMainPicture()->rawValue().toBool());
    QCOMPARE(_videoManager->_uvcVideoSourceID, camera);
    QVERIFY(!cameraSource->tile());
    QVERIFY(primaryStream->tile());
    QCOMPARE(_videoManager->primaryStream(), primaryStream);
    QVERIFY(_primary->active());
    QCOMPARE(_primary->activeChanges, 0);
    QCOMPARE(_primary->configChanges, 0);
    QCOMPARE(_videoManager->_sources.size(), 4);
    QVERIFY(!_videoManager->hasThermal());
    QVERIFY(!_thermal->active());
    QVERIFY(_videoManager->uvcCamera()->active());

    // The camera stays the main picture without a row of tiles on screen, as in the picture-in-picture, where the
    // other tiles' sources go: the stream it took the main picture from plays on, for its return.
    _videoManager->setTileRoom(0);
    _settle();
    QVERIFY(_videoManager->isUvc());
    QVERIFY(_videoManager->uvcCamera()->active());
    QVERIFY(primaryStream->tile());
    QVERIFY(_primary->active());
    QCOMPARE(_videoManager->_sources.size(), 3);
    _videoManager->setTileRoom(4);
    _settle();
    QCOMPARE(_videoManager->_sources.size(), 4);
    QCOMPARE(_primary->activeChanges, 0);

    // The stream is selected in its turn: it is the main picture again, and the camera is back in its tile.
    _videoManager->selectSource(primaryStream);
    _settle();
    QVERIFY(!_videoManager->isUvc());
    QCOMPARE(mainSourceSpy.count(), 2);
    QCOMPARE(_videoManager->mainSource(), primaryStream);
    QVERIFY(!_videoSettings->uvcTileCameraAsMainPicture()->rawValue().toBool());
    QVERIFY(cameraSource->tile());
    QVERIFY(!primaryStream->tile());
    QCOMPARE(_primary->activeChanges, 0);
    QVERIFY(_videoManager->hasThermal());
    QVERIFY(_thermal->active());

    // The camera leaves the main picture while it is unplugged, too.
    _videoManager->selectSource(cameraSource);
    QVERIFY(_videoManager->isUvc());
    _videoManager->_cameraAttached = [](const QString&) { return false; };
    emit _videoManager->uvcCamera()->camerasChanged();
    _settle();
    QVERIFY(!_videoManager->isUvc());
    QVERIFY(!primaryStream->tile());
    QVERIFY(!_videoManager->uvcCamera()->active());
    // With no camera to show, both tiles are streams'.
    QCOMPARE(_videoManager->_sources.size(), 5);

    // The choice is kept, as it is from one run to the next: plugged in again, the camera is the main picture again.
    _videoManager->_cameraAttached = [camera](const QString& name) { return name == camera; };
    expectLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("No camera named")));
    emit _videoManager->uvcCamera()->camerasChanged();
    _settle();
    verifyExpectedLogMessage();
    QVERIFY(_videoManager->isUvc());
    QVERIFY(primaryStream->tile());
    QCOMPARE(_videoManager->_sources.size(), 4);

    // It was of that camera: another one chosen for the tile plays in the tile.
    _videoManager->_cameraAttached = [](const QString&) { return true; };
    expectLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("No camera named")));
    _facts->set(_videoSettings->uvcTileCamera(), QStringLiteral("QGC Other Test Camera"));
    _settle();
    verifyExpectedLogMessage();
    QVERIFY(!_videoManager->isUvc());
    QVERIFY(cameraSource->tile());
    QVERIFY(!primaryStream->tile());
}

void VideoManagerTest::_testStateChangesBeforeItsSignals()
{
    const QString camera = QStringLiteral("QGC Test Camera");
    _videoManager->_cameraAttached = [camera](const QString& name) { return name == camera; };
    QVERIFY(_videoManager->isStreamSource());
    QVERIFY(!_videoManager->isUvc());
    QVERIFY(!_videoManager->hasUvcCamera());

    // What a slot, or a QML binding, finds on the first signal of the main picture going from the stream to the camera.
    struct Seen
    {
        bool isStreamSource;
        bool isUvc;
        bool hasUvcCamera;
        const VideoStreamController* mainSource;
        bool othersSignalled;
    };

    std::optional<Seen> seen;
    QSignalSpy isUvcSpy(_videoManager.get(), &VideoManager::isUvcChanged);
    QSignalSpy mainSourceSpy(_videoManager.get(), &VideoManager::mainSourceChanged);
    QSignalSpy hasUvcCameraSpy(_videoManager.get(), &VideoManager::hasUvcCameraChanged);
    const QObject slotOwner;
    (void) connect(_videoManager.get(), &VideoManager::isStreamSourceChanged, &slotOwner, [&] {
        seen = Seen{.isStreamSource = _videoManager->isStreamSource(),
                    .isUvc = _videoManager->isUvc(),
                    .hasUvcCamera = _videoManager->hasUvcCamera(),
                    .mainSource = _videoManager->mainSource(),
                    .othersSignalled = !isUvcSpy.isEmpty() || !mainSourceSpy.isEmpty() || !hasUvcCameraSpy.isEmpty()};
    });
    _facts->set(_videoSettings->videoSource(), camera);
    // The camera the test attached isn't one Qt knows.
    expectLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("No camera named")));
    _settle();
    verifyExpectedLogMessage();

    // The whole state, though the other properties' signals were yet to come.
    QVERIFY(seen);
    QVERIFY(!seen->othersSignalled);
    QVERIFY(!seen->isStreamSource);
    QVERIFY(seen->isUvc);
    QVERIFY(seen->hasUvcCamera);
    QCOMPARE(seen->mainSource, _videoManager->_cameraSource);
    QCOMPARE(isUvcSpy.count(), 1);
    QCOMPARE(mainSourceSpy.count(), 1);
    QCOMPARE(hasUvcCameraSpy.count(), 1);
}

void VideoManagerTest::_testDisableWhenDisarmed_data()
{
    QTest::addColumn<int>("lift");

    QTest::newRow("re-armed") << 0;
    QTest::newRow("setting turned off") << 1;
    QTest::newRow("video restarted") << 2;
}

void VideoManagerTest::_testDisableWhenDisarmed()
{
    QFETCH(int, lift);

    // Without the setting, disarming changes nothing.
    _videoManager->_onArmedChanged(false);
    _settle();
    QVERIFY(_primary->active());

    // The setting goes by the vehicle's state, not by its next change: turned on for a disarmed vehicle, it turns the
    // video off.
    _facts->set(_videoSettings->disableWhenDisarmed(), true);
    QVERIFY(!_videoManager->hasVideo());
    QCOMPARE(_videoManager->noVideoReason(), VideoManager::NoVideoReason::DisabledWhileDisarmed);
    _settle();
    QVERIFY(!_primary->active());
    _videoManager->_onArmedChanged(true);
    QVERIFY(_videoManager->hasVideo());
    _settle();
    QVERIFY(_primary->active());

    // Full screen needs a connected vehicle, which this test has none of.
    _videoManager->_fullScreen = true;
    _videoManager->_onArmedChanged(false);
    QVERIFY(!_videoManager->hasVideo());
    // The Fly View can't leave full screen once the video is gone.
    QVERIFY(!_videoManager->fullScreen());
    _settle();
    QVERIFY(!_primary->active());

    switch (lift) {
        case 0:
            _videoManager->_onArmedChanged(true);
            break;
        case 1:
            _videoSettings->disableWhenDisarmed()->setRawValue(false);
            break;
        default:
            _videoManager->startVideo();
            break;
    }
    QVERIFY(_videoManager->hasVideo());
    QCOMPARE(_videoManager->noVideoReason(), VideoManager::NoVideoReason::Waiting);
    _settle();
    QVERIFY(_primary->active());

    if (lift == 2) {
        // The restart holds until the vehicle's armed state changes: other changes leave the video on, and the
        // vehicle's next disarming turns it off again.
        _facts->set(_videoSettings->lowLatencyMode(), true);
        QVERIFY(_videoManager->hasVideo());
        _videoManager->_onArmedChanged(true);
        QVERIFY(_videoManager->hasVideo());
        _videoManager->_onArmedChanged(false);
        QVERIFY(!_videoManager->hasVideo());
    }
}

void VideoManagerTest::_testVideoOutputRegistration()
{
    VideoStreamController* const stream = _videoManager->primaryStream();
    QQuickItem first;
    QQuickItem second;

    // Registered before a receiver exists: attached once one does.
    stream->setReceiver(nullptr);
    stream->registerVideoOutput(&first);
    _videoManager->_createVideoReceivers();
    auto* const receiver = qobject_cast<FakeVideoReceiver*>(stream->receiver());
    QVERIFY(receiver);
    QCOMPARE(receiver->videoOutput(), &first);

    // Registering again (after a window change) re-attaches it.
    receiver->videoOutputChanges = 0;
    stream->registerVideoOutput(&first);
    QCOMPARE(receiver->videoOutputChanges, 1);
    QCOMPARE(receiver->videoOutput(), &first);

    stream->registerVideoOutput(&second);
    QCOMPARE(receiver->videoOutput(), &second);
    stream->unregisterVideoOutput(&first);
    QCOMPARE(receiver->videoOutput(), &second);

    // An output that leaves as another one registers hands the stream over: the receiver, which decodes only into an
    // output, is never without one.
    receiver->videoOutputChanges = 0;
    stream->unregisterVideoOutput(&second);
    QCOMPARE(stream->videoOutput(), nullptr);
    stream->registerVideoOutput(&first);
    _settle();
    QCOMPARE(receiver->videoOutput(), &first);
    QCOMPARE(receiver->videoOutputChanges, 1);

    // With no other output by the next pass, the receiver loses its own.
    stream->unregisterVideoOutput(&first);
    QCOMPARE(receiver->videoOutput(), &first);
    _settle();
    QCOMPARE(receiver->videoOutput(), nullptr);

    {
        QQuickItem destroyed;
        stream->registerVideoOutput(&destroyed);
        QCOMPARE(receiver->videoOutput(), &destroyed);
    }
    QCOMPARE(receiver->videoOutput(), nullptr);
    QCOMPARE(stream->videoOutput(), nullptr);
}

void VideoManagerTest::_testBackgroundSuspendsDisplay()
{
    VideoStreamController* const stream = _videoManager->primaryStream();
    QQuickItem output;
    stream->registerVideoOutput(&output);
    QCOMPARE(_primary->videoOutput(), &output);

    // Without the focus the window is still in view.
    _videoManager->_onApplicationStateChanged(Qt::ApplicationInactive);
    QCOMPARE(_primary->videoOutput(), &output);

    // In the background nothing is displayed: the receiver, which decodes only into an output, has none.
    _videoManager->_onApplicationStateChanged(Qt::ApplicationHidden);
    QCOMPARE(_primary->videoOutput(), nullptr);
    _videoManager->_onApplicationStateChanged(Qt::ApplicationSuspended);
    QCOMPARE(_primary->videoOutput(), nullptr);
    QCOMPARE(stream->videoOutput(), &output);

    // An output that registers meanwhile waits like the others.
    QQuickItem other;
    stream->registerVideoOutput(&other);
    QCOMPARE(_primary->videoOutput(), nullptr);

    _videoManager->_onApplicationStateChanged(Qt::ApplicationActive);
    QCOMPARE(_primary->videoOutput(), &other);
    stream->unregisterVideoOutput(&other);
    _settle();
}

void VideoManagerTest::_testScreenshotsComeFromStreamsOnDisplay()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QQmlEngine engine;
    const std::unique_ptr<QQuickItem> primaryOutput = createVideoOutput(engine);
    const std::unique_ptr<QQuickItem> thermalOutput = createVideoOutput(engine);
    QVERIFY(primaryOutput && thermalOutput);
    _videoManager->primaryStream()->registerVideoOutput(primaryOutput.get());
    _videoManager->thermalStream()->registerVideoOutput(thermalOutput.get());

    QImage frame(QSize(64, 48), QImage::Format_RGBA8888);
    frame.fill(Qt::red);
    showFrame(primaryOutput.get(), frame);
    showFrame(thermalOutput.get(), frame);
    _primary->setState(VideoReceiver::State::Streaming);

    // Qt Multimedia converts on the CPU when the platform (offscreen here) has no RHI, and says so.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));

    // The thermal output holds a frame but isn't streaming: only the primary stream is saved. Files are written off
    // the GUI thread.
    const QString shot = tempDir.filePath(QStringLiteral("shot.png"));
    _videoManager->grabImage(shot);
    QTRY_COMPARE_WITH_TIMEOUT(QImage(shot).size(), frame.size(), TestTimeout::mediumMs());
    QVERIFY(!QFile::exists(tempDir.filePath(QStringLiteral("shot.thermalVideo.png"))));

    // A stream in a tile is on display too, but a photo is of the main picture and its thermal stream.
    const auto current = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5601");
    const auto other = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5602");
    _videoManager->_setAdvertisedStreams({current.get(), nullptr, QString()}, nullptr,
                                         {{other.get(), nullptr, QString()}});
    QCOMPARE(_videoManager->_sources.size(), 4);
    VideoStreamController* const tile = _videoManager->_sources.last();
    const std::unique_ptr<QQuickItem> tileOutput = createVideoOutput(engine);
    tile->registerVideoOutput(tileOutput.get());
    showFrame(tileOutput.get(), frame);
    _receiverOf(tile)->setState(VideoReceiver::State::Streaming);
    QVERIFY(tile->canTakeScreenshot());

    _thermal->setState(VideoReceiver::State::Streaming);
    const QString second = tempDir.filePath(QStringLiteral("second.png"));
    _videoManager->grabImage(second);
    QTRY_COMPARE_WITH_TIMEOUT(QImage(second).size(), frame.size(), TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(QImage(tempDir.filePath(QStringLiteral("second.thermalVideo.png"))).size(), frame.size(),
                              TestTimeout::mediumMs());
    QCOMPARE(_primary->screenshots, 2);
    QCOMPARE(_thermal->screenshots, 1);
    QCOMPARE(_receiverOf(tile)->screenshots, 0);
}

void VideoManagerTest::_testScreenshotFailuresReachUser_data()
{
    // The file is relative to the test's directory; none asks for the default, in the photo save path.
    QTest::addColumn<bool>("onDisplay");
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("message");

    QTest::newRow("no video on display") << false << "shot.png" << "no video is on display";
    QTest::newRow("file can't be written") << true << "missing/shot.png" << "Unable to save the photo shot\\.png";
    QTest::newRow("no save path") << true << QString() << "save path must be specified";
}

void VideoManagerTest::_testScreenshotFailuresReachUser()
{
    QFETCH(bool, onDisplay);
    QFETCH(QString, file);
    QFETCH(QString, message);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    _facts->set(SettingsManager::instance()->appSettings()->savePath(), QString());

    QQmlEngine engine;
    const std::unique_ptr<QQuickItem> output = createVideoOutput(engine);
    QVERIFY(output);
    if (onDisplay) {
        _videoManager->primaryStream()->registerVideoOutput(output.get());
        QImage frame(QSize(64, 48), QImage::Format_RGBA8888);
        frame.fill(Qt::red);
        showFrame(output.get(), frame);
        _primary->setState(VideoReceiver::State::Streaming);
    }
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    QSignalSpy errorSpy(_videoManager->primaryStream(), &VideoStreamController::errorOccurred);

    const bool written = onDisplay && !file.isEmpty();
    if (written) {
        // A debug build of Qt, which a Windows Debug build links, warns of the failed write as well.
        ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("^QImage::save: failed to write")));
        expectLogMessage("Video.VideoScreenshot", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Failed to save screenshot")));
    }
    if (written || !onDisplay) {
        expectLogMessage("Video.VideoManager", QtWarningMsg,
                         QRegularExpression(written ? message : QStringLiteral("No video on display")));
    }
    expectAppMessage(QRegularExpression(message));
    _videoManager->grabImage(file.isEmpty() ? QString() : tempDir.filePath(file));
    if (written) {
        // The file is written, or not, off the GUI thread.
        QVERIFY_SIGNAL_COUNT_WAIT(errorSpy, 1, TestTimeout::mediumMs());
        verifyExpectedLogMessage();
    }
    if (written || !onDisplay) {
        verifyExpectedLogMessage();
    }
    verifyExpectedLogMessage();
}

void VideoManagerTest::_testForcedAspectRatioAppliesToConfiguredStream()
{
    VideoStreamController* const stream = _videoManager->primaryStream();
    QSignalSpy forcedSpy(stream, &VideoStreamController::forcedAspectRatioChanged);
    QCOMPARE(stream->forcedAspectRatio(), 0.0);

    // A stretched picture takes a sink that leaves non-square pixels as they are: the stream restarts.
    _facts->set(_videoSettings->forcedAspectRatio(), static_cast<int>(VideoSettings::AspectRatio4x3));
    QCOMPARE(stream->forcedAspectRatio(), 4.0 / 3.0);
    QCOMPARE(forcedSpy.count(), 1);
    _settle();
    QVERIFY(_primary->displayConfig().disablePixelAspectRatio);
    QCOMPARE(_primary->restarts, 1);
    QCOMPARE(_primary->configChanges, 0);

    // Another ratio takes no other sink.
    _facts->set(_videoSettings->forcedAspectRatio(), static_cast<int>(VideoSettings::AspectRatio16x9));
    QCOMPARE(stream->forcedAspectRatio(), 16.0 / 9.0);
    _settle();
    QCOMPARE(_primary->restarts, 1);
    QCOMPARE(_videoManager->thermalStream()->forcedAspectRatio(), 0.0);
    QVERIFY(!_thermal->displayConfig().disablePixelAspectRatio);

    // A camera says what its stream is: its picture isn't stretched, and its sink scales to square pixels again.
    const auto info = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5601");
    stream->setStreamInfo(info.get());
    QCOMPARE(stream->forcedAspectRatio(), 0.0);
    _settle();
    QVERIFY(!_primary->displayConfig().disablePixelAspectRatio);
}

void VideoManagerTest::_testScreenshotFollowsForcedAspectRatio()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    _facts->set(_videoSettings->forcedAspectRatio(), static_cast<int>(VideoSettings::AspectRatio16x9));
    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> root = engine.create<QQuickItem>(
        kStreamOutputQml, {{QStringLiteral("stream"), QVariant::fromValue(_videoManager->primaryStream())}});
    QVERIFY2(root, qPrintable(engine.error()));
    auto* const output = root->findChild<QQuickItem*>(QStringLiteral("output"));
    QVERIFY(output);

    // A frame of 64 × 48 is saved as wide as the output shows it, with every row it has.
    showFrame(output, VideoTestHelpers::redTopLeftQuadrant());
    _primary->setState(VideoReceiver::State::Streaming);
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    const QString shot = tempDir.filePath(QStringLiteral("shot.png"));
    _videoManager->grabImage(shot);
    QTRY_COMPARE_WITH_TIMEOUT(QImage(shot).size(), QSize(85, 48), TestTimeout::mediumMs());
    QCOMPARE(VideoTestHelpers::redCorner(QImage(shot)), std::optional<Qt::Corner>(Qt::TopLeftCorner));
}

void VideoManagerTest::_testAdvertisedRotationTurnsVideo_data()
{
    // What a camera advertises for a stream of 64 × 48 frames whose top-left quadrant is red.
    QTest::addColumn<int>("advertised");
    QTest::addColumn<int>("rotation");
    QTest::addColumn<QSize>("videoSize");
    QTest::addColumn<int>("redCorner");

    QTest::newRow("upright") << 0 << 0 << QSize(64, 48) << int(Qt::TopLeftCorner);
    QTest::newRow("90") << 90 << 90 << QSize(48, 64) << int(Qt::TopRightCorner);
    QTest::newRow("180") << 180 << 180 << QSize(64, 48) << int(Qt::BottomRightCorner);
    QTest::newRow("270") << 270 << 270 << QSize(48, 64) << int(Qt::BottomLeftCorner);
    QTest::newRow("more than a turn") << 450 << 90 << QSize(48, 64) << int(Qt::TopRightCorner);
    // Only right angles can be shown.
    QTest::newRow("45") << 45 << 0 << QSize(64, 48) << int(Qt::TopLeftCorner);
}

void VideoManagerTest::_testAdvertisedRotationTurnsVideo()
{
    QFETCH(int, advertised);
    QFETCH(int, rotation);
    QFETCH(QSize, videoSize);
    QFETCH(int, redCorner);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    VideoStreamController* const stream = _videoManager->primaryStream();
    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> root =
        engine.create<QQuickItem>(kStreamOutputQml, {{QStringLiteral("stream"), QVariant::fromValue(stream)}});
    QVERIFY2(root, qPrintable(engine.error()));
    auto* const output = root->findChild<QQuickItem*>(QStringLiteral("output"));
    QVERIFY(output);

    const auto info = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5601", VIDEO_STREAM_ENCODING_H264, false,
                                     static_cast<quint16>(advertised));
    QSignalSpy rotationSpy(stream, &VideoStreamController::rotationChanged);
    stream->setStreamInfo(info.get());
    QCOMPARE(stream->rotation(), rotation);
    QCOMPARE(rotationSpy.count(), (rotation == 0) ? 0 : 1);
    // The output turns the frames, on top of any rotation of their own.
    QCOMPARE(output->property("orientation").toInt(), rotation);

    // Until frames arrive, the advertised resolution (1280 × 720) gives the ratio of the video as shown.
    const bool sideways = (rotation % 180) != 0;
    QCOMPARE(stream->aspectRatio(), sideways ? (720.0 / 1280.0) : (1280.0 / 720.0));
    _primary->setVideoSize(QSize(64, 48));
    QCOMPARE(stream->videoSize(), videoSize);
    QCOMPARE(stream->aspectRatio(), static_cast<double>(videoSize.width()) / videoSize.height());

    // The output tells the ratio of the picture as it shows it, turned, and a screenshot shows the same.
    showFrame(output, VideoTestHelpers::redTopLeftQuadrant());
    QTRY_COMPARE_WITH_TIMEOUT(output->property("aspectRatio").toReal(),
                              static_cast<qreal>(videoSize.width()) / videoSize.height(), TestTimeout::shortMs());
    _primary->setState(VideoReceiver::State::Streaming);
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    const QString shot = tempDir.filePath(QStringLiteral("shot.png"));
    _videoManager->grabImage(shot);
    QTRY_COMPARE_WITH_TIMEOUT(QImage(shot).size(), videoSize, TestTimeout::mediumMs());
    QCOMPARE(VideoTestHelpers::redCorner(QImage(shot)), std::optional<Qt::Corner>(static_cast<Qt::Corner>(redCorner)));

    // Without the camera's stream the video is upright again.
    stream->setStreamInfo(nullptr);
    QCOMPARE(stream->rotation(), 0);
    QCOMPARE(output->property("orientation").toInt(), 0);
    QCOMPARE(stream->videoSize(), QSize(64, 48));
}

void VideoManagerTest::_testCameraTransformTurnsItsPicture()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString shot = tempDir.filePath(QStringLiteral("uvc.png"));
    VideoStreamController* const cameraSource = _videoManager->_cameraSource;
    UVCCameraController* const camera = _videoManager->uvcCamera();
    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> root =
        engine.create<QQuickItem>(kStreamOutputQml, {{QStringLiteral("stream"), QVariant::fromValue(cameraSource)}});
    QVERIFY2(root, qPrintable(engine.error()));
    auto* const output = root->findChild<QQuickItem*>(QStringLiteral("output"));
    QVERIFY(output);
    // The camera's output registers with the camera's source, which hands it to the camera.
    QCOMPARE(cameraSource->videoOutput(), output);
    QCOMPARE(camera->videoOutput(), output);
    QCOMPARE(output->property("orientation").toInt(), 0);
    QVERIFY(!output->property("mirrored").toBool());

    // The output turns and mirrors the picture. The camera's frames, which its recorder gets too, stay as they are.
    _facts->set(_videoSettings->uvcCameraRotation(), 90);
    _facts->set(_videoSettings->uvcCameraMirror(), true);
    QCOMPARE(cameraSource->rotation(), 90);
    QVERIFY(cameraSource->mirrored());
    QCOMPARE(output->property("orientation").toInt(), 90);
    QVERIFY(output->property("mirrored").toBool());
    showFrame(output, VideoTestHelpers::redTopLeftQuadrant());
    auto* const sink = output->property("videoSink").value<QVideoSink*>();
    QVERIFY(sink);
    QCOMPARE(sink->videoFrame().rotation(), QtVideo::Rotation::None);
    QVERIFY(!sink->videoFrame().mirrored());
    // The size it is shown at is the turned one.
    camera->setVideoSize(QSize(64, 48));
    QCOMPARE(cameraSource->videoSize(), QSize(48, 64));

    // Screenshots save it as displayed: turned clockwise, which puts the red quadrant top right, and mirrored, which
    // brings it back to the top left.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    QVERIFY(camera->takeScreenshot(shot));
    QTRY_COMPARE_WITH_TIMEOUT(QImage(shot).size(), QSize(48, 64), TestTimeout::mediumMs());
    QCOMPARE(VideoTestHelpers::redCorner(QImage(shot)), std::optional<Qt::Corner>(Qt::TopLeftCorner));
}

void VideoManagerTest::_testSourceErrorsReachUser()
{
    // The USB camera's among them, as when its photo can't be saved.
    for (VideoReceiver* const receiver :
         {static_cast<VideoReceiver*>(_thermal), _videoManager->_cameraSource->receiver()}) {
        const QRegularExpression message(QStringLiteral("source failed"));
        expectLogMessage("Video.VideoManager", QtWarningMsg, message);
        expectAppMessage(message);
        emit receiver->errorOccurred(QStringLiteral("source failed"));
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
    }
}

void VideoManagerTest::_testReceiverSetWhileOldOneGoes()
{
    VideoStreamController* const stream = _videoManager->primaryStream();
    const QPointer<FakeVideoReceiver> requested = new FakeVideoReceiver;
    auto* const later = new FakeVideoReceiver;
    // A receiver that finishes a recording runs the event loop in its destructor, where a change of the video backend
    // sets the stream another receiver.
    (void) connect(_primary, &QObject::destroyed, this, [stream, later] { stream->setReceiver(later); });
    _primary = nullptr;

    stream->setReceiver(requested);
    QCOMPARE(stream->receiver(), later);
    QVERIFY(!requested);

    QSignalSpy videoSizeSpy(stream, &VideoStreamController::videoSizeChanged);
    later->setVideoSize(QSize(1280, 720));
    QCOMPARE(videoSizeSpy.count(), 1);
}

void VideoManagerTest::_testReconnectingFollowsLostStream()
{
    using State = VideoReceiver::State;
    VideoStreamController* const stream = _videoManager->primaryStream();
    QSignalSpy reconnectingSpy(stream, &VideoStreamController::reconnectingChanged);
    QList<bool> reconnectingAtDecodingChange;
    (void) connect(stream, &VideoStreamController::decodingChanged, this,
                   [&] { reconnectingAtDecodingChange.append(stream->reconnecting()); });

    // Before the first decoded frame there is nothing to recover.
    _primary->setState(State::Connecting);
    _primary->setState(State::Streaming);
    QVERIFY(!stream->reconnecting());

    // Lost after it was decoded: reconnecting through the backoff and the next attempt, until it decodes again.
    _primary->setDecoding(true);
    QVERIFY(!stream->reconnecting());
    _primary->setDecoding(false);
    QVERIFY(stream->reconnecting());
    // Already reconnecting when decodingChanged arrives, so QML never sees the stream neither.
    QCOMPARE(reconnectingAtDecodingChange, QList<bool>({false, true}));
    _primary->setState(State::Reconnecting);
    _primary->setState(State::Connecting);
    QVERIFY(stream->reconnecting());
    _primary->setDecoding(true);
    QVERIFY(!stream->reconnecting());
    QCOMPARE(reconnectingSpy.count(), 2);

    // Given up.
    _primary->setDecoding(false);
    QVERIFY(stream->reconnecting());
    _primary->setState(State::Stopped);
    QVERIFY(!stream->reconnecting());

    // A new URI is a new stream, not a reconnect of the old one.
    _primary->setState(State::Streaming);
    _primary->setDecoding(true);
    _primary->setDecoding(false);
    QVERIFY(stream->reconnecting());
    _facts->set(_videoSettings->udpUrl(), QStringLiteral("0.0.0.0:5601"));
    _settle();
    QVERIFY(!stream->reconnecting());
    _primary->setState(State::Connecting);
    QVERIFY(!stream->reconnecting());

    // So is the stream after the video was stopped.
    _primary->setDecoding(true);
    _primary->setDecoding(false);
    QVERIFY(stream->reconnecting());
    _videoManager->stopVideo();
    _settle();
    QVERIFY(!stream->reconnecting());

    // A stream that isn't decoded because its output left, as a hidden tile's does, wasn't lost.
    QQuickItem output;
    stream->registerVideoOutput(&output);
    _primary->setState(State::Streaming);
    _primary->setDecoding(true);
    stream->unregisterVideoOutput(&output);
    _settle();
    _primary->setDecoding(false);
    QVERIFY(!stream->reconnecting());
}

void VideoManagerTest::_testLinkRegainedReconnectsStreams()
{
    // Losing the link is not the streams' business: each notices its own loss.
    _videoManager->_communicationLostChanged(true);
    QCOMPARE(_primary->reconnects, 0);

    // With the link, the camera is likely back: a stream backing off is asked to connect now.
    _videoManager->_communicationLostChanged(false);
    QCOMPARE(_primary->reconnects, 1);
    QCOMPARE(_primary->restarts, 0);
}

void VideoManagerTest::_testNetworkRegainedReconnectsStreams()
{
    StandInNetworkMonitor network(/*available=*/true);
    _videoManager->_followNetwork(&network);
    QVERIFY(_primary->active());
    QVERIFY(!_thermal->active());

    // Losing the network is not the streams' business either: each notices its own loss.
    network.setAvailable(false);
    QCOMPARE(_primary->reconnects, 0);

    // With the device's network a stream's source is likely back, though no vehicle's link tells of it: a stream
    // backing off is asked to connect now. One that doesn't play is not asked.
    network.setAvailable(true);
    QCOMPARE(_primary->reconnects, 1);
    QCOMPARE(_primary->restarts, 0);
    QCOMPARE(_thermal->reconnects, 0);
}

void VideoManagerTest::_testUvcCameraFormatReachesCamera()
{
    UVCCameraController* const camera = _videoManager->uvcCamera();
    QCOMPARE(camera->format(), _videoSettings->uvcCameraFormat()->rawValue().toString());

    _facts->set(_videoSettings->uvcCameraFormat(), QStringLiteral("640x480 30 fps YUYV"));
    QCOMPARE(camera->format(), QStringLiteral("640x480 30 fps YUYV"));
    _facts->set(_videoSettings->uvcCameraFormat(), QString());
    QVERIFY(camera->format().isEmpty());

    // The formats the camera offers are the setting's choices, after Auto.
    camera->_formats = {QStringLiteral("640x480 30 fps YUYV")};
    emit camera->formatsChanged();
    QCOMPARE(_videoSettings->uvcCameraFormat()->enumValues(),
             QVariantList({QString(), QStringLiteral("640x480 30 fps YUYV")}));
}

void VideoManagerTest::_testUvcControlsReachCamera()
{
    UVCCameraController* const camera = _videoManager->uvcCamera();
    QCOMPARE(camera->controls(), UVCCameraController::Controls{});

    _facts->set(_videoSettings->uvcWhiteBalance(), int(QCamera::WhiteBalanceCloudy));
    _facts->set(_videoSettings->uvcZoom(), 1.5);
    _facts->set(_videoSettings->uvcExposureCompensation(), -0.5);
    _facts->set(_videoSettings->uvcFocusMode(), int(QCamera::FocusModeInfinity));
    const UVCCameraController::Controls expected{.whiteBalance = QCamera::WhiteBalanceCloudy,
                                                 .zoom = 1.5f,
                                                 .exposureCompensation = -0.5f,
                                                 .focusMode = QCamera::FocusModeInfinity};
    QCOMPARE(camera->controls(), expected);
}

void VideoManagerTest::_testSourceFallingShortDegradesVideo_data()
{
    QTest::addColumn<bool>("usbCamera");  ///< The source is the USB camera, else the primary stream
    QTest::addColumn<bool>("measured");
    QTest::addColumn<double>("lossPercent");
    QTest::addColumn<double>("droppedFps");
    QTest::addColumn<double>("fps");
    QTest::addColumn<double>("cameraFps");  ///< What the stream's camera advertises, or the USB camera's own; 0: none
    QTest::addColumn<bool>("decoding");
    QTest::addColumn<bool>("degraded");

    constexpr double kNoRtp = -1.0;
    QTest::newRow("steady") << false << true << kNoRtp << 0.0 << 30.0 << 30.0 << true << false;
    QTest::newRow("losing packets") << false << true << 1.5 << 0.0 << 30.0 << 30.0 << true << true;
    QTest::newRow("dropping frames") << false << true << kNoRtp << 2.0 << 30.0 << 30.0 << true << true;
    QTest::newRow("under three quarters of the advertised frame rate")
        << false << true << kNoRtp << 0.0 << 22.0 << 30.0 << true << true;
    QTest::newRow("three quarters of the advertised frame rate")
        << false << true << kNoRtp << 0.0 << 22.5 << 30.0 << true << false;
    QTest::newRow("no advertised frame rate") << false << true << kNoRtp << 0.0 << 5.0 << 0.0 << true << false;
    QTest::newRow("not decoded, as the thermal stream with its view off")
        << false << true << kNoRtp << 0.0 << 0.0 << 30.0 << false << false;
    QTest::newRow("not measured") << false << false << 1.5 << 2.0 << 0.0 << 30.0 << true << false;
    QTest::newRow("USB camera under three quarters of its frame rate")
        << true << true << kNoRtp << 0.0 << 20.0 << 30.0 << true << true;
    QTest::newRow("USB camera at its frame rate") << true << true << kNoRtp << 0.0 << 30.0 << 30.0 << true << false;
}

void VideoManagerTest::_testSourceFallingShortDegradesVideo()
{
    QFETCH(bool, usbCamera);
    QFETCH(bool, measured);
    QFETCH(double, lossPercent);
    QFETCH(double, droppedFps);
    QFETCH(double, fps);
    QFETCH(double, cameraFps);
    QFETCH(bool, decoding);
    QFETCH(bool, degraded);

    VideoStreamStats stats;
    stats.valid = measured;
    stats.lossPercent = lossPercent;
    stats.droppedFps = droppedFps;
    stats.fps = fps;
    mavlink_video_stream_information_t info{};
    info.type = VIDEO_STREAM_TYPE_RTPUDP;
    info.encoding = VIDEO_STREAM_ENCODING_H264;
    info.framerate = static_cast<float>(cameraFps);
    (void) qstrncpy(info.uri, "5601", sizeof(info.uri));
    QGCVideoStreamInfo cameraStream(info);
    UVCCameraController* const camera = _videoManager->uvcCamera();
    QSignalSpy degradedSpy(_videoManager.get(), &VideoManager::degradedChanged);

    if (usbCamera) {
        stats.cameraFps = cameraFps;
        camera->setStats(stats);
    } else {
        if (cameraFps > 0) {
            _videoManager->primaryStream()->setStreamInfo(&cameraStream);
        }
        _primary->setDecoding(decoding);
        _primary->setStats(stats);
    }
    QCOMPARE(_videoManager->degraded(), degraded);
    QCOMPARE(degradedSpy.count(), degraded ? 1 : 0);

    // A source that is no longer measured doesn't fall short.
    camera->setStats({});
    _primary->setStats({});
    QVERIFY(!_videoManager->degraded());
    _videoManager->primaryStream()->setStreamInfo(nullptr);
    _settle();
}

void VideoManagerTest::_testThermalPlaysOnlyWhileInUse()
{
    VideoStreamController* const thermalStream = _videoManager->thermalStream();
    const auto thermalInfo =
        makeStreamInfo(VIDEO_STREAM_TYPE_RTSP, "rtsp://10.0.0.1/thermal", VIDEO_STREAM_ENCODING_H264, true);
    thermalStream->setStreamInfo(thermalInfo.get());
    _settle();
    // What the manager goes by when it asks the stream's camera to transmit it.
    QSignalSpy idleSpy(thermalStream, &VideoStreamController::idleChanged);

    // Advertised with its view off, the stream is there to show, and nothing connects to it.
    QVERIFY(_videoManager->hasThermal());
    QCOMPARE(_thermal->config().uri, QStringLiteral("rtsp://10.0.0.1/thermal"));
    QVERIFY(!_thermal->active());
    QVERIFY(!thermalStream->wantsStream());

    // It connects for its view.
    QQuickItem view;
    thermalStream->registerVideoOutput(&view);
    QVERIFY(thermalStream->wantsStream());
    QCOMPARE(idleSpy.count(), 1);
    _settle();
    QVERIFY(_thermal->active());

    // The view goes: the stream plays on for the grace period, so a view that is back by then finds it playing.
    _resetCounters();
    thermalStream->unregisterVideoOutput(&view);
    _settle();
    QVERIFY(thermalStream->_idleTimer.isActive());
    thermalStream->registerVideoOutput(&view);
    _settle();
    QVERIFY(!thermalStream->_idleTimer.isActive());
    QCOMPARE(_thermal->activeChanges, 0);
    QCOMPARE(idleSpy.count(), 1);

    // Out of use for longer, it disconnects.
    thermalStream->_idleTimer.setInterval(1);
    thermalStream->unregisterVideoOutput(&view);
    QTRY_VERIFY_WITH_TIMEOUT(!_thermal->active(), TestTimeout::mediumMs());
    QVERIFY(!thermalStream->wantsStream());
    QCOMPARE(idleSpy.count(), 2);

    // A recording connects it at once, with its view off, and keeps it from when it is asked for until it has ended.
    const QString file = QStringLiteral("flight.thermalVideo.mkv");
    QVERIFY(thermalStream->canRecord());
    thermalStream->startRecording(file, VideoReceiver::FileFormat::Mkv);
    QVERIFY(_thermal->active());
    QCOMPARE(_thermal->recordings.size(), 1);
    QCOMPARE(_thermal->recordings.first().file, file);
    QVERIFY(!thermalStream->_idleTimer.isActive());
    _thermal->setRecording(true);
    thermalStream->stopRecording();
    QCOMPARE(_thermal->recordingStops, 1);
    QVERIFY(!thermalStream->_idleTimer.isActive());
    _thermal->setRecording(false);
    QTRY_VERIFY_WITH_TIMEOUT(!_thermal->active(), TestTimeout::mediumMs());

    // So does a recording that fails before it began.
    thermalStream->startRecording(file, VideoReceiver::FileFormat::Mkv);
    QVERIFY(_thermal->active());
    expectLogMessage("Video.VideoRecordingService", QtWarningMsg, QRegularExpression(QStringLiteral("no encoder")));
    expectAppMessage(QRegularExpression(QStringLiteral("no encoder")));
    emit _thermal->recordingFinished(file, QStringLiteral("no encoder"));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QTRY_VERIFY_WITH_TIMEOUT(!_thermal->active(), TestTimeout::mediumMs());
}

void VideoManagerTest::_testStreamUriAndTransport_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("uri");
    QTest::addColumn<QString>("transport");

    QTest::newRow("udp h.264") << QString(VideoSettings::videoSourceUDPH264) << "0.0.0.0:5600"
                               << "udp://0.0.0.0:5600" << "RTP/UDP (H.264)";
    QTest::newRow("udp h.265") << QString(VideoSettings::videoSourceUDPH265) << "0.0.0.0:5600"
                               << "udp265://0.0.0.0:5600" << "RTP/UDP (H.265)";
    QTest::newRow("mpeg-ts") << QString(VideoSettings::videoSourceMPEGTS) << "0.0.0.0:5600"
                             << "mpegts://0.0.0.0:5600" << "MPEG-TS/UDP";
    QTest::newRow("tcp") << QString(VideoSettings::videoSourceTCP) << "10.0.0.2:5600" << "tcp://10.0.0.2:5600"
                         << "MPEG-TS/TCP";
    // The URI is shown in the Fly View, so it loses credentials and query values.
    QTest::newRow("rtsp") << QString(VideoSettings::videoSourceRTSP) << "rtsp://pilot:secret@cam.local/live?token=abc"
                          << "rtsp://cam.local/live?token=REDACTED" << "RTSP";
    QTest::newRow("rtsp over tcp") << QString(VideoSettings::videoSourceRTSP) << "rtspt://cam.local/live"
                                   << "rtspt://cam.local/live" << "RTSP";
}

void VideoManagerTest::_testStreamUriAndTransport()
{
    QFETCH(QString, source);
    QFETCH(QString, url);
    QFETCH(QString, uri);
    QFETCH(QString, transport);

    VideoStreamController* const stream = _videoManager->primaryStream();
    QSignalSpy uriSpy(stream, &VideoStreamController::uriChanged);
    _facts->set(_videoSettings->videoSource(), source);
    Fact* const urlFact = (source == QString(VideoSettings::videoSourceRTSP))  ? _videoSettings->rtspUrl()
                          : (source == QString(VideoSettings::videoSourceTCP)) ? _videoSettings->tcpUrl()
                                                                               : _videoSettings->udpUrl();
    _facts->set(urlFact, url);
    _settle();

    QCOMPARE(stream->uri(), uri);
    QCOMPARE(stream->transport(), transport);
    QCOMPARE(uriSpy.count(), (uri == QStringLiteral("udp://0.0.0.0:5600")) ? 0 : 1);

    // Without a stream to play there is nothing to show.
    _facts->set(_videoSettings->videoSource(), QString(VideoSettings::videoDisabled));
    _settle();
    QVERIFY(stream->uri().isEmpty());
    QVERIFY(stream->transport().isEmpty());
}

UT_REGISTER_TEST(VideoManagerTest, TestLabel::Unit)
