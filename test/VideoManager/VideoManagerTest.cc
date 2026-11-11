#include "VideoManagerTest.h"

#include <array>
#include <functional>

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "MavlinkCameraControlInterface.h"
#include "SettingsManager.h"
#include "UVCCameraController.h"
#include "UVCStreamMonitor.h"
#include "VideoBackend.h"
#include "VideoDecoderOptions.h"
#include "VideoManager.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"
#include "VideoTelemetryOverlay.h"
#include "VideoTestHelpers.h"

using VideoTestHelpers::createVideoOutput;
using VideoTestHelpers::makeStreamInfo;
using VideoTestHelpers::showFrame;
using VideoTestHelpers::showsText;

namespace {

// The Video indicator's decoding settings.
constexpr const char* kDecodingSettingsQml = R"QML(
import QtQuick
import QGroundControl.VideoManager

VideoDecodingSettingsGroup {
    width: 400
}
)QML";

// The Video indicator's network stream settings.
constexpr const char* kStreamSettingsQml = R"QML(
import QtQuick
import QGroundControl.VideoManager

VideoStreamSettingsGroup {
    width: 400
}
)QML";

// VideoStreamOutput inside a container that tests move between windows.
constexpr const char* kStreamOutputQml = R"QML(
import QtQuick
import QGroundControl.VideoManager

Item {
    id: root

    property var stream

    Item {
        objectName: "holder"

        VideoStreamOutput {
            objectName: "output"
            stream: root.stream
        }
    }
}
)QML";

constexpr const char* kStatsGroupQml = R"QML(
import QtQuick
import QGroundControl.VideoManager

VideoStreamStatsGroup {
}
)QML";

// The Video indicator in a stand-in toolbar, which gives it its height.
constexpr const char* kIndicatorQml = R"QML(
import QtQuick
import QGroundControl.Toolbar

Item {
    id: root

    property var videoManager

    width: 400
    height: 40

    VideoStreamIndicator {
        videoManager: root.videoManager
    }
}
)QML";

// The statistics in the Video indicator's drawer.
constexpr const char* kSourceStatsQml = R"QML(
import QtQuick
import QGroundControl.VideoManager

VideoSourceStats {
    width: 400
}
)QML";

// VideoStreamFormat as other modules reach it: a JavaScript resource of the module, not a relative import.
constexpr const char* kStreamFormatQml = R"QML(
import QtQml
import QGroundControl.VideoManager

QtObject {
    readonly property string kbit: VideoStreamFormat.bitrateText(850)
    readonly property string mbit: VideoStreamFormat.bitrateText(8000)
    readonly property string minutes: VideoStreamFormat.durationText(65000)
    readonly property string hours: VideoStreamFormat.durationText(3725000)
}
)QML";

constexpr const char* kPlaceholderQml = R"QML(
import QtQuick
import QGroundControl.VideoManager

NoVideoPlaceholder {
    width: 400
    height: 300
}
)QML";

constexpr const char* kVideoViewQml = R"QML(
import QtQuick
import QGroundControl.VideoManager

VideoView {
    width: 400
    height: 400
}
)QML";

// CameraPinchZoom on a stand-in zoom camera, which records the zoom steps it is sent.
constexpr const char* kCameraPinchZoomQml = R"QML(
import QtQuick
import QGroundControl.VideoManager

CameraPinchZoom {
    camera: QtObject {
        property bool hasZoom: true
        property var steps: []

        function stepZoom(direction) {
            steps.push(direction)
        }
    }
}
)QML";

// A view holding a ThermalVideoOverlay that follows a stand-in camera, whose thermal mode the test changes.
constexpr const char* kThermalOverlayQml = R"QML(
import QtQuick
import QGroundControl
import QGroundControl.VideoManager

Item {
    id: root

    property alias videoManager: overlay.videoManager
    property QtObject camera: QtObject {
        property int thermalMode: MavlinkCameraControlInterface.THERMAL_PIP
        property real thermalOpacity: 100
    }

    width: 400
    height: 400

    ThermalVideoOverlay {
        id: overlay
        camera: root.camera
    }
}
)QML";

// A VideoView following a stand-in camera, whose thermal mode the test changes.
constexpr const char* kThermalVideoViewQml = R"QML(
import QtQuick
import QGroundControl
import QGroundControl.Controls
import QGroundControl.VideoManager

VideoView {
    width: 400
    height: 400
    thermalPipTopInset: 30
    camera: QtObject {
        property int thermalMode: MavlinkCameraControlInterface.THERMAL_PIP
        property real thermalOpacity: 100
        property bool hasZoom: false
    }

    readonly property real pipX: ScreenTools.defaultFontPixelWidth * 12
    readonly property real pipY: thermalPipTopInset + (ScreenTools.defaultFontPixelHeight * 0.5)
}
)QML";

// A VideoView with room for tiles, following a stand-in camera, whose thermal mode the test changes.
constexpr const char* kTilesVideoViewQml = R"QML(
import QtQuick
import QGroundControl
import QGroundControl.Controls
import QGroundControl.VideoManager

VideoView {
    width: 800
    height: 400
    showTiles: true
    thermalPipTopInset: 30
    camera: QtObject {
        property int thermalMode: MavlinkCameraControlInterface.THERMAL_OFF
        property real thermalOpacity: 100
        property bool hasZoom: false
    }

    readonly property real pipX: ScreenTools.defaultFontPixelWidth * 12
}
)QML";

// The Fly View's video as its main view, which a stand-in picture-in-picture view makes it.
constexpr const char* kFlyViewVideoQml = R"QML(
import QtQuick
import QGroundControl.FlyView

Item {
    id: root

    property alias videoManager: video.videoManager
    // The video's obstacle overlay takes the vehicle from the view it is in.
    property var _activeVehicle: null

    width: 800
    height: 400

    Item {
        id: pipView

        property Item _pipContentItem: pipContent
        property Item _windowContentItem: pipContent

        Item {
            id: pipContent
        }
    }

    FlyViewVideo {
        id: video
        pipView: pipView

        Component.onCompleted: pipState.state = pipState.fullState
    }
}
)QML";

/// The tiles @p tiles, a VideoTiles, shows: each the item of a loader.
QList<QQuickItem*> shownTiles(const QQuickItem* tiles)
{
    QList<QQuickItem*> shown;
    for (const QQuickItem* const child : tiles->childItems()) {
        auto* const tile = child->property("item").value<QQuickItem*>();
        if (child->isVisible() && tile) {
            shown.append(tile);
        }
    }
    return shown;
}

}  // namespace

void VideoManagerTest::init()
{
    UnitTest::init();

    _videoSettings = SettingsManager::instance()->videoSettings();
    _facts = std::make_unique<VideoTestHelpers::FactRestorer>();
    _facts->set(_videoSettings->videoSource(), QString(VideoSettings::videoSourceUDPH264));
    _facts->set(_videoSettings->udpUrl(), QStringLiteral("0.0.0.0:5600"));
    _facts->set(_videoSettings->rtspUrl(), QStringLiteral("rtsp://cam.local/manual"));
    _facts->set(_videoSettings->disableWhenDisarmed(), false);
    _facts->set(_videoSettings->lowLatencyMode(), false);
    _facts->set(_videoSettings->rtpJitterLatencyMs(), 80);
    _facts->set(_videoSettings->rtspAutoReconnect(), true);
    _facts->set(_videoSettings->rtspTimeout(), 8);
    _facts->set(_videoSettings->uvcTileCameraAsMainPicture(), false);

    _videoManager = std::make_unique<VideoManager>();
    _videoManager->_receiverFactory = [](QObject* parent) { return new FakeVideoReceiver(parent); };
    _videoManager->_createVideoReceivers();
    _primary = qobject_cast<FakeVideoReceiver*>(_videoManager->primaryStream()->receiver());
    _thermal = qobject_cast<FakeVideoReceiver*>(_videoManager->thermalStream()->receiver());
    QVERIFY(_primary);
    QVERIFY(_thermal);
    _settle();
    _resetCounters();
}

void VideoManagerTest::cleanup()
{
    _primary = nullptr;
    _thermal = nullptr;
    _videoManager.reset();
    _facts.reset();
    _settle();

    UnitTest::cleanup();
}

void VideoManagerTest::_settle()
{
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
}

void VideoManagerTest::_resetCounters()
{
    for (const VideoStreamController* source : std::as_const(_videoManager->_sources)) {
        if (FakeVideoReceiver* const receiver = _receiverOf(source)) {
            receiver->configChanges = 0;
            receiver->activeChanges = 0;
            receiver->restarts = 0;
            receiver->videoOutputChanges = 0;
        }
    }
}

FakeVideoReceiver* VideoManagerTest::_receiverOf(const VideoStreamController* source)
{
    return qobject_cast<FakeVideoReceiver*>(source->receiver());
}

void VideoManagerTest::_testSettingReachesReceiver_data()
{
    QTest::addColumn<QString>("setting");
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("field");
    QTest::addColumn<QVariant>("expected");

    QTest::newRow("lowLatencyMode") << "lowLatencyMode" << QVariant(true) << "lowLatency" << QVariant(true);
    QTest::newRow("rtpJitterLatencyMs") << "rtpJitterLatencyMs" << QVariant(300) << "rtpJitterLatencyMs"
                                        << QVariant(300);
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
                            : (field == QLatin1String("autoReconnect"))      ? QVariant(config.autoReconnect)
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
    QTest::newRow("disablePixelAspectRatio") << "disablePixelAspectRatio" << QVariant(true);
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

void VideoManagerTest::_testDecodingSettingsGroupRestartsStreams()
{
    Fact* const decoderFact = _videoSettings->forceVideoDecoder();
    Fact* const cpuPathFact = _videoSettings->forceCpuVideoPath();
    _facts->set(decoderFact, static_cast<int>(VideoDecoderOption::Default));
    _facts->set(cpuPathFact, false);
    _settle();
    _resetCounters();

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kDecodingSettingsQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/DecodingSettings.qml")));
    const std::unique_ptr<QObject> group(component.create());
    QVERIFY2(group, qPrintable(component.errorString()));

    QObject* const decoderControl = group->findChild<QObject*>(QStringLiteral("videoDecoderPriority"));
    QObject* const cpuPathControl = group->findChild<QObject*>(QStringLiteral("videoForceCpuPath"));
    QVERIFY(decoderControl);
    QVERIFY(cpuPathControl);
    QCOMPARE(decoderControl->property("visible"), decoderFact->property("userVisible"));
    QCOMPARE(cpuPathControl->property("visible"), cpuPathFact->property("userVisible"));

    // As a click on the switch does.
    QVERIFY(cpuPathControl->setProperty("checked", true));
    QVERIFY(QMetaObject::invokeMethod(cpuPathControl, "clicked"));
    _settle();
    QVERIFY(cpuPathFact->rawValue().toBool());
    QCOMPARE(_primary->restarts, 1);

    // As picking the software decoder in the combo box does.
    QObject* const comboBox = decoderControl->property("comboBox").value<QObject*>();
    QVERIFY(comboBox);
    const int softwareIndex =
        decoderFact->enumValues().indexOf(QVariant(static_cast<int>(VideoDecoderOption::Software)));
    QVERIFY(softwareIndex >= 0);
    QVERIFY(QMetaObject::invokeMethod(comboBox, "activated", Q_ARG(int, softwareIndex)));
    _settle();
    QCOMPARE(decoderFact->rawValue().toInt(), static_cast<int>(VideoDecoderOption::Software));
    QCOMPARE(_primary->restarts, 2);
    QCOMPARE(_primary->configChanges, 0);
}

void VideoManagerTest::_testStreamSettingsGroupFollowsLowLatency()
{
    if (!VideoBackend::instance().capabilities().pipelineSettings) {
        QSKIP("The RTP jitter-buffer latency is a GStreamer setting");
    }
    _facts->set(_videoSettings->lowLatencyMode(), false);

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kStreamSettingsQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/StreamSettings.qml")));
    const std::unique_ptr<QObject> group(component.create());
    QVERIFY2(group, qPrintable(component.errorString()));

    QObject* const lowLatencyControl = group->findChild<QObject*>(QStringLiteral("videoLowLatencyMode"));
    QObject* const jitterControl = group->findChild<QObject*>(QStringLiteral("videoRtpJitterLatency"));
    QVERIFY(lowLatencyControl);
    QVERIFY(jitterControl);
    QCOMPARE(lowLatencyControl->property("fact").value<Fact*>(), _videoSettings->lowLatencyMode());
    QCOMPARE(jitterControl->property("fact").value<Fact*>(), _videoSettings->rtpJitterLatencyMs());
    QVERIFY(jitterControl->property("visible").toBool());

    // Low Latency Mode has no jitter buffer, so its latency is hidden.
    _facts->set(_videoSettings->lowLatencyMode(), true);
    QVERIFY(!jitterControl->property("visible").toBool());
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
    QTest::addColumn<int>("tileRoom");  ///< What the row of tiles tells; -1 when none does
    QTest::addColumn<QStringList>("tileUris");

    const QString second = QStringLiteral("udp://0.0.0.0:5602");
    const QString wide = QStringLiteral("rtsp://10.0.0.2/wide");
    const QString fourth = QStringLiteral("udp://0.0.0.0:5604");
    QTest::newRow("no tiles") << 0 << -1 << QStringList();
    QTest::newRow("one tile") << 1 << -1 << QStringList({second});
    QTest::newRow("three tiles") << 3 << -1 << QStringList({second, wide, fourth});
    QTest::newRow("as many tiles as streams") << 4 << -1 << QStringList({second, wide, fourth});
    QTest::newRow("room for two of three tiles") << 3 << 2 << QStringList({second, wide});
    QTest::newRow("room for more tiles than allowed") << 1 << 4 << QStringList({second});
    QTest::newRow("no room for a tile") << 3 << 0 << QStringList();
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
    _videoManager->_setAdvertisedStreams(secondStream, nullptr, {firstStream, thirdStream});
    _settle();
    QCOMPARE(primarySpy.count(), 1);
    QCOMPARE(_videoManager->primaryStream(), secondSource);
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
    QTest::addColumn<QString>("camera");  ///< The camera that plays; empty for none
    QTest::addColumn<bool>("tile");

    const QString udp = VideoSettings::videoSourceUDPH264;
    const QString attached = QStringLiteral("QGC Test Camera");
    const QString other = QStringLiteral("QGC Other Test Camera");
    const QString missing = QStringLiteral("QGC Test Missing Camera");
    QTest::newRow("the video source") << attached << QString() << 3 << attached << false;
    QTest::newRow("the video source, not the tile's camera") << attached << other << 3 << attached << false;
    QTest::newRow("the video source without tiles") << attached << QString() << 0 << attached << false;
    QTest::newRow("in a tile beside a stream") << udp << attached << 3 << attached << true;
    QTest::newRow("no tile's camera") << udp << QString() << 3 << QString() << false;
    QTest::newRow("no tiles") << udp << attached << 0 << QString() << false;
    QTest::newRow("no stream to play beside")
        << QString(VideoSettings::videoDisabled) << attached << 3 << QString() << false;
    QTest::newRow("the tile's camera isn't attached") << udp << missing << 3 << QString() << false;
}

void VideoManagerTest::_testCameraSourcePlaysVideoSourceOrTileCamera()
{
    QFETCH(QString, videoSource);
    QFETCH(QString, tileCamera);
    QFETCH(int, maxTiles);
    QFETCH(QString, camera);
    QFETCH(bool, tile);

    _videoManager->_cameraAttached = [](const QString& name) {
        return name.startsWith(QLatin1String("QGC ")) && !name.contains(QLatin1String("Missing"));
    };
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
    QCOMPARE(_videoManager->hasUvcCamera(), !camera.isEmpty());
    // isUvc says that the camera is the main picture, which it isn't from a tile.
    const bool mainPicture = !camera.isEmpty() && !tile;
    QCOMPARE(_videoManager->isUvc(), mainPicture);
    QCOMPARE(_videoManager->uvcVideoSourceID(), mainPicture ? camera : QString());
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
    QVERIFY(!_videoManager->isUvc());
    QCOMPARE(_videoManager->_sources.size(), 4);
    QVERIFY(_videoManager->hasThermal());
    QVERIFY(_thermal->active());
    _resetCounters();

    // Selected, the camera is the main picture: the primary stream plays on in the camera's tile, and its thermal
    // stream stops.
    QSignalSpy isUvcSpy(_videoManager.get(), &VideoManager::isUvcChanged);
    _videoManager->selectSource(cameraSource);
    _settle();
    QCOMPARE(isUvcSpy.count(), 1);
    QVERIFY(_videoManager->isUvc());
    QVERIFY(_videoSettings->uvcTileCameraAsMainPicture()->rawValue().toBool());
    QCOMPARE(_videoManager->uvcVideoSourceID(), camera);
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

    // The stream is selected in its turn: it is the main picture again, and the camera is back in its tile.
    _videoManager->selectSource(primaryStream);
    _settle();
    QVERIFY(!_videoManager->isUvc());
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

    _facts->set(_videoSettings->disableWhenDisarmed(), true);
    _videoManager->_onArmedChanged(true);
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
    _settle();
    QVERIFY(_primary->active());
}

void VideoManagerTest::_testPlaceholderTellsWhyThereIsNoVideo_data()
{
    QTest::addColumn<int>("reason");
    QTest::addColumn<QString>("text");

    QTest::newRow("awaited") << int(VideoManager::NoVideoReason::Waiting) << "WAITING FOR VIDEO";
    QTest::newRow("stopped") << int(VideoManager::NoVideoReason::Stopped) << "VIDEO STOPPED";
    QTest::newRow("disarmed") << int(VideoManager::NoVideoReason::DisabledWhileDisarmed)
                              << "VIDEO DISABLED WHILE DISARMED";
    QTest::newRow("stopped and disarmed") << int(VideoManager::NoVideoReason::Stopped) << "VIDEO STOPPED";
}

void VideoManagerTest::_testPlaceholderTellsWhyThereIsNoVideo()
{
    QFETCH(int, reason);
    QFETCH(QString, text);

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kPlaceholderQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/Placeholder.qml")));
    const std::unique_ptr<QQuickItem> placeholder(qobject_cast<QQuickItem*>(component.createWithInitialProperties(
        {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}})));
    QVERIFY2(placeholder, qPrintable(component.errorString()));
    // Items outside a scene are never visible.
    QQuickWindow window;
    placeholder->setParentItem(window.contentItem());
    QVERIFY(showsText(placeholder.get(), QStringLiteral("WAITING FOR VIDEO")));

    const QString row = QString::fromLatin1(QTest::currentDataTag());
    if (row.contains(QLatin1String("disarmed"))) {
        _facts->set(_videoSettings->disableWhenDisarmed(), true);
        _videoManager->_onArmedChanged(false);
    }
    if (row.contains(QLatin1String("stopped"))) {
        _videoManager->stopVideo();
    }
    QCOMPARE(int(_videoManager->noVideoReason()), reason);
    QVERIFY(showsText(placeholder.get(), text));

    _videoManager->startVideo();
    QCOMPARE(_videoManager->noVideoReason(), VideoManager::NoVideoReason::Waiting);
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

void VideoManagerTest::_testQmlVideoOutputRegistersItself()
{
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kStreamOutputQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/StreamOutput.qml")));
    std::unique_ptr<QQuickItem> root(qobject_cast<QQuickItem*>(component.createWithInitialProperties(
        {{QStringLiteral("stream"), QVariant::fromValue(_videoManager->primaryStream())}})));
    QVERIFY2(root, qPrintable(component.errorString()));
    auto* const holder = root->findChild<QQuickItem*>(QStringLiteral("holder"));
    auto* const output = root->findChild<QQuickItem*>(QStringLiteral("output"));
    QVERIFY(holder && output);

    QCOMPARE(_videoManager->primaryStream()->videoOutput(), output);
    QCOMPARE(_primary->videoOutput(), output);
    QCOMPARE(_primary->videoOutputChanges, 1);
    QCOMPARE(_thermal->videoOutput(), nullptr);

    // Each move to another window (the PiP pop-out) re-attaches the output.
    QQuickWindow mainWindow;
    QQuickWindow popOutWindow;
    root->setParentItem(mainWindow.contentItem());
    QCOMPARE(_primary->videoOutputChanges, 2);
    holder->setParentItem(popOutWindow.contentItem());
    QCOMPARE(_primary->videoOutputChanges, 3);
    QCOMPARE(_primary->videoOutput(), output);

    // The output follows a change of the stream to show, as the main picture's when a stream in a tile takes its
    // place. The stream it leaves is without an output on the next pass, no other output having taken it over.
    QVERIFY(root->setProperty("stream", QVariant::fromValue(_videoManager->thermalStream())));
    QCOMPARE(_videoManager->thermalStream()->videoOutput(), output);
    QCOMPARE(_thermal->videoOutput(), output);
    QCOMPARE(_videoManager->primaryStream()->videoOutput(), nullptr);
    _settle();
    QCOMPARE(_primary->videoOutput(), nullptr);

    holder->setParentItem(root.get());
    root.reset();
    QCOMPARE(_thermal->videoOutput(), nullptr);
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
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kStreamOutputQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/StreamOutput.qml")));
    const std::unique_ptr<QQuickItem> root(qobject_cast<QQuickItem*>(
        component.createWithInitialProperties({{QStringLiteral("stream"), QVariant::fromValue(stream)}})));
    QVERIFY2(root, qPrintable(component.errorString()));
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
    QCOMPARE(_videoManager->aspectRatio(), static_cast<double>(videoSize.width()) / videoSize.height());

    // A screenshot shows the video as the output does.
    showFrame(output, VideoTestHelpers::redTopLeftQuadrant());
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

void VideoManagerTest::_testUvcCameraFormatReachesCamera()
{
    UVCCameraController* const camera = _videoManager->uvcCamera();
    QCOMPARE(camera->format(), _videoSettings->uvcCameraFormat()->rawValue().toString());

    _facts->set(_videoSettings->uvcCameraFormat(), QStringLiteral("640x480 30 fps YUYV"));
    QCOMPARE(camera->format(), QStringLiteral("640x480 30 fps YUYV"));
    _facts->set(_videoSettings->uvcCameraFormat(), QString());
    QVERIFY(camera->format().isEmpty());
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

void VideoManagerTest::_testRecordingEncodingReachesEverySource()
{
    const auto receivers = [this] {
        QList<VideoReceiver*> receivers;
        for (const VideoStreamController* source : std::as_const(_videoManager->_sources)) {
            receivers.append(source->receiver());
        }
        return receivers;
    };
    // Set first, so that choosing the other backend below is a change.
    _facts->set(_videoSettings->videoBackend(), int(VideoSettings::VideoBackendGStreamer));

    _facts->set(_videoSettings->uvcRecordingCodec(), QStringLiteral("MPEG2"));
    _facts->set(_videoSettings->uvcRecordingQuality(), int(QMediaRecorder::LowQuality));
    const VideoReceiver::RecordingEncoding expected{.codec = QMediaFormat::VideoCodec::MPEG2,
                                                    .quality = QMediaRecorder::LowQuality};
    // A tile's source, made after the settings were, among them: it records once it is the primary stream.
    const auto current = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5601");
    const auto other = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5602");
    _videoManager->_setAdvertisedStreams({current.get(), nullptr, QStringLiteral("Main")}, nullptr,
                                         {{other.get(), nullptr, QStringLiteral("Other")}});
    QCOMPARE(receivers().size(), 4);
    for (const VideoReceiver* receiver : receivers()) {
        QVERIFY(receiver);
        QCOMPARE(receiver->recordingEncoding(), expected);
    }

    // The receivers another video backend gives the streams get them too.
    _primary = nullptr;
    _thermal = nullptr;
    _facts->set(_videoSettings->videoBackend(), int(VideoSettings::VideoBackendQtMultimedia));
    QVERIFY(!_videoManager->primaryStream()->receiver());
    _videoManager->_createVideoReceivers();
    for (const VideoReceiver* receiver : receivers()) {
        QVERIFY(receiver);
        QCOMPARE(receiver->recordingEncoding(), expected);
    }
    _videoManager->_setAdvertisedStreams({}, nullptr, {});
}

void VideoManagerTest::_testStreamStatsReachQml()
{
    VideoStreamController* const stream = _videoManager->primaryStream();
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kStatsGroupQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/StatsGroup.qml")));
    const std::unique_ptr<QQuickItem> group(qobject_cast<QQuickItem*>(
        component.createWithInitialProperties({{QStringLiteral("stream"), QVariant::fromValue(stream)}})));
    QVERIFY2(group, qPrintable(component.errorString()));
    // Items outside a scene are never visible.
    QQuickWindow window;
    group->setParentItem(window.contentItem());
    QSignalSpy statsSpy(stream, &VideoStreamController::statsChanged);
    QVERIFY(!group->isVisible());

    VideoStreamStats stats;
    stats.valid = true;
    stats.fps = 29.5;
    stats.decoder = QStringLiteral("avdec_h264");
    _primary->setStats(stats);
    QCOMPARE(statsSpy.count(), 1);
    QVERIFY(stream->stats() == stats);
    QVERIFY(group->isVisible());
    // A stream that isn't decoded, as the thermal stream while its view is off, shows no frames to count.
    QVERIFY(!showsText(group.get(), QStringLiteral("29.5 fps")));
    _primary->setDecoding(true);
    QVERIFY(showsText(group.get(), QStringLiteral("29.5 fps")));
    QVERIFY(showsText(group.get(), QStringLiteral("avdec_h264 (software)")));
    // A stream without RTP counters hides the rows that need them.
    QVERIFY(!showsText(group.get(), QStringLiteral("Packet loss")));

    QVERIFY(showsText(group.get(), QStringLiteral("RTP/UDP (H.264)")));
    QVERIFY(showsText(group.get(), QStringLiteral("0:00")));
    // Counts that are still zero, and what wasn't measured, stay hidden.
    QVERIFY(!showsText(group.get(), QStringLiteral("Decoder restarts")));
    QVERIFY(!showsText(group.get(), QStringLiteral("Keyframe interval")));
    QVERIFY(!showsText(group.get(), QStringLiteral("Recording")));

    // A stream without a jitter buffer counts its RTP packets itself: what it hasn't measured, and what only a jitter
    // buffer tells, stays hidden.
    stats.packetsLost = 3;
    stats.lossPercent = 1.5;
    _primary->setStats(stats);
    QCOMPARE(statsSpy.count(), 2);
    QVERIFY(showsText(group.get(), QStringLiteral("1.5% (3 packet(s))")));
    QVERIFY(!showsText(group.get(), QStringLiteral("Jitter")));
    QVERIFY(!showsText(group.get(), QStringLiteral("Late / duplicate packets")));
    QVERIFY(!showsText(group.get(), QStringLiteral("Retransmissions")));

    stats.jitterMs = 2.0;
    stats.packetsLate = 1;
    stats.packetsDuplicate = 0;
    stats.rtxRequested = 4;
    stats.rtxRecovered = 3;
    stats.keyframeIntervalMs = 1000.0;
    stats.decoderRestarts = 2;
    stats.uptimeMs = 3'725'000;
    stats.recordingMs = 65'000;
    stats.recordingBytes = 2 * 1024 * 1024;
    stats.codec = QStringLiteral("H.264");
    _primary->setStats(stats);
    QCOMPARE(statsSpy.count(), 3);
    QVERIFY(showsText(group.get(), QStringLiteral("Packet loss")));
    QVERIFY(showsText(group.get(), QStringLiteral("1.5% (3 packet(s))")));
    QVERIFY(showsText(group.get(), QStringLiteral("2.0 ms")));
    QVERIFY(showsText(group.get(), QStringLiteral("1 / 0")));
    QVERIFY(showsText(group.get(), QStringLiteral("4 requested, 3 recovered")));
    QVERIFY(showsText(group.get(), QStringLiteral("1.0 s")));
    QVERIFY(showsText(group.get(), QStringLiteral("Decoder restarts")));
    QVERIFY(showsText(group.get(), QStringLiteral("1:02:05")));
    QVERIFY(showsText(group.get(), QStringLiteral("1:05, 2.0MB")));

    // The same measurement again is no change, and the thermal stream's are its own.
    _primary->setStats(stats);
    _thermal->setStats(stats);
    QCOMPARE(statsSpy.count(), 3);

    // A camera's advertised stream follows the measured values.
    mavlink_video_stream_information_t info{};
    info.type = VIDEO_STREAM_TYPE_RTPUDP;
    info.encoding = VIDEO_STREAM_ENCODING_H264;
    info.framerate = 30.0f;
    info.bitrate = 8'000'000;
    info.resolution_h = 1280;
    info.resolution_v = 720;
    (void) qstrncpy(info.uri, "5601", sizeof(info.uri));
    (void) qstrncpy(info.name, "Main camera", sizeof(info.name));
    QGCVideoStreamInfo cameraStream(info);
    stream->setStreamInfo(&cameraStream);
    _settle();
    QVERIFY(showsText(group.get(), QStringLiteral("Main camera")));
    QVERIFY(showsText(group.get(), QStringLiteral("29.5 fps (camera 30 fps)")));
    QVERIFY(showsText(group.get(), QStringLiteral("0 kbit/s (camera 8.0 Mbit/s)")));
    QVERIFY(showsText(group.get(), QStringLiteral("H.264 (camera 1280 × 720)")));
    stream->setStreamInfo(nullptr);
    _settle();

    // A receiver going away takes its measurements with it.
    stream->setReceiver(nullptr);
    _primary = nullptr;
    QCOMPARE(statsSpy.count(), 4);
    QVERIFY(!group->isVisible());
}

void VideoManagerTest::_testIndicatorReportsMeasuredSource()
{
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kIndicatorQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/Indicator.qml")));
    const std::unique_ptr<QQuickItem> toolbar(qobject_cast<QQuickItem*>(component.createWithInitialProperties(
        {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}})));
    QVERIFY2(toolbar, qPrintable(component.errorString()));
    // Items outside a scene are never visible.
    QQuickWindow window;
    toolbar->setParentItem(window.contentItem());
    const QQuickItem* const indicator = toolbar->findChild<QQuickItem*>(QStringLiteral("toolbar_videoStreamIndicator"));
    QVERIFY(indicator);
    QVERIFY(!indicator->property("showIndicator").toBool());

    // Only the thermal stream is measured: the figures are its own.
    VideoStreamStats thermalStats;
    thermalStats.valid = true;
    thermalStats.fps = 9.0;
    thermalStats.bitrateKbps = 850.0;
    _thermal->setStats(thermalStats);
    QVERIFY(indicator->property("showIndicator").toBool());
    QVERIFY(showsText(indicator, QStringLiteral("9 fps")));
    QVERIFY(showsText(indicator, QStringLiteral("850 kbit/s")));

    // The primary stream's come before them.
    VideoStreamStats primaryStats;
    primaryStats.valid = true;
    primaryStats.fps = 30.0;
    primaryStats.bitrateKbps = 8000.0;
    _primary->setStats(primaryStats);
    QVERIFY(showsText(indicator, QStringLiteral("30 fps")));
    QVERIFY(showsText(indicator, QStringLiteral("8.0 Mbit/s")));

    // The USB camera, which runs in the primary stream's place, has no bitrate to show: its resolution instead.
    _primary->setStats({});
    _thermal->setStats({});
    QVERIFY(!indicator->property("showIndicator").toBool());
    UVCCameraController* const camera = _videoManager->uvcCamera();
    VideoStreamStats cameraStats;
    cameraStats.valid = true;
    cameraStats.fps = 25.0;
    cameraStats.bitrateKbps = -1.0;
    camera->setVideoSize(QSize(1280, 720));
    camera->setStats(cameraStats);
    QVERIFY(indicator->property("showIndicator").toBool());
    QVERIFY(showsText(indicator, QStringLiteral("25 fps")));
    QVERIFY(showsText(indicator, QStringLiteral("720p")));

    // A source that drops frames colours the icon, whichever source the figures are of.
    const QQuickItem* const icon = indicator->findChild<QQuickItem*>(QStringLiteral("videoStreamIcon"));
    QVERIFY(icon);
    const QVariant steadyColor = icon->property("color");
    cameraStats.droppedFps = 2.0;
    camera->setStats(cameraStats);
    QVERIFY(icon->property("color") != steadyColor);
    _primary->setStats(primaryStats);
    QVERIFY(showsText(indicator, QStringLiteral("30 fps")));
    QVERIFY(icon->property("color") != steadyColor);
    // With the primary stream in a tile, the figures are those of the camera that took the main picture from it.
    _videoManager->primaryStream()->setTile(true);
    QVERIFY(showsText(indicator, QStringLiteral("25 fps")));
    _videoManager->primaryStream()->setTile(false);
    camera->setStats({});
    QCOMPARE(icon->property("color"), steadyColor);
}

void VideoManagerTest::_testSourceStatsShowMeasuredSources()
{
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kSourceStatsQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/SourceStats.qml")));
    const std::unique_ptr<QQuickItem> sourceStats(qobject_cast<QQuickItem*>(component.createWithInitialProperties(
        {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}})));
    QVERIFY2(sourceStats, qPrintable(component.errorString()));
    // Items outside a scene are never visible.
    QQuickWindow window;
    sourceStats->setParentItem(window.contentItem());
    QVERIFY(showsText(sourceStats.get(), QStringLiteral("No video stream")));

    // Each source has a group of its kind, in a loader.
    QList<QQuickItem*> loaders;
    for (QQuickItem* const child : sourceStats->childItems()) {
        if (child->property("sourceComponent").isValid()) {
            loaders.append(child);
        }
    }
    QCOMPARE(loaders.size(), 3);
    const auto group = [&loaders](int index) { return loaders.at(index)->property("item").value<QObject*>(); };
    QCOMPARE(group(0)->property("stream").value<VideoStreamController*>(), _videoManager->primaryStream());
    QCOMPARE(group(0)->property("heading").toString(), QStringLiteral("Video"));
    QCOMPARE(group(1)->property("stream").value<VideoStreamController*>(), _videoManager->thermalStream());
    QCOMPARE(group(1)->property("heading").toString(), QStringLiteral("Thermal"));
    UVCCameraController* const camera = _videoManager->uvcCamera();
    QCOMPARE(group(2)->property("monitor").value<UVCStreamMonitor*>(), camera->monitor());

    // A group shows while its source is measured.
    const std::array<std::function<void(const VideoStreamStats&)>, 3> setStats{
        [this](const VideoStreamStats& stats) { _primary->setStats(stats); },
        [this](const VideoStreamStats& stats) { _thermal->setStats(stats); },
        [camera](const VideoStreamStats& stats) { camera->setStats(stats); },
    };
    VideoStreamStats measured;
    measured.valid = true;
    for (size_t shown = 0; shown < setStats.size(); ++shown) {
        setStats.at(shown)(measured);
        QVERIFY(!showsText(sourceStats.get(), QStringLiteral("No video stream")));
        for (size_t index = 0; index < setStats.size(); ++index) {
            QCOMPARE(loaders.at(index)->isVisible(), index == shown);
        }
        setStats.at(shown)({});
    }
    QVERIFY(showsText(sourceStats.get(), QStringLiteral("No video stream")));

    // A stream in a tile gets a group too, under its name.
    const auto current = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5601");
    const auto other = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5602");
    _videoManager->_setAdvertisedStreams({current.get(), nullptr, QStringLiteral("Main")}, nullptr,
                                         {{other.get(), nullptr, QStringLiteral("Gimbal: Wide")}});
    loaders.clear();
    for (QQuickItem* const child : sourceStats->childItems()) {
        if (child->property("sourceComponent").isValid()) {
            loaders.append(child);
        }
    }
    QCOMPARE(loaders.size(), 4);
    QCOMPARE(group(3)->property("stream").value<VideoStreamController*>(), _videoManager->_sources.last());
    QCOMPARE(group(3)->property("heading").toString(), QStringLiteral("Gimbal: Wide"));
    QCOMPARE(group(0)->property("heading").toString(), QStringLiteral("Video"));
}

void VideoManagerTest::_testStreamFormatText()
{
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kStreamFormatQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/StreamFormat.qml")));
    const std::unique_ptr<QObject> format(component.create());
    QVERIFY2(format, qPrintable(component.errorString()));
    QCOMPARE(format->property("kbit").toString(), QStringLiteral("850 kbit/s"));
    QCOMPARE(format->property("mbit").toString(), QStringLiteral("8.0 Mbit/s"));
    QCOMPARE(format->property("minutes").toString(), QStringLiteral("1:05"));
    QCOMPARE(format->property("hours").toString(), QStringLiteral("1:02:05"));
}

void VideoManagerTest::_testVideoViewFitsVideo_data()
{
    // Whether the fitted video takes its width from the view's height, and its height from the view's width, for a
    // video wider than the square view, and whether the outputs crop it to the view.
    QTest::addColumn<int>("fitMode");
    QTest::addColumn<bool>("widthFromHeight");
    QTest::addColumn<bool>("heightFromWidth");
    QTest::addColumn<bool>("crops");

    QTest::newRow("fit width") << int(VideoSettings::VideoFitWidth) << false << true << false;
    QTest::newRow("fit height") << int(VideoSettings::VideoFitHeight) << true << false << true;
    QTest::newRow("fill") << int(VideoSettings::VideoFitFill) << true << false << true;
    QTest::newRow("no crop") << int(VideoSettings::VideoFitNoCrop) << false << true << false;
}

void VideoManagerTest::_testVideoViewFitsVideo()
{
    QFETCH(int, fitMode);
    QFETCH(bool, widthFromHeight);
    QFETCH(bool, heightFromWidth);
    QFETCH(bool, crops);

    _facts->set(_videoSettings->videoFit(), fitMode);
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kVideoViewQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/VideoView.qml")));
    const std::unique_ptr<QObject> view(component.create());
    QVERIFY2(view, qPrintable(component.errorString()));

    const qreal aspectRatio = view->property("_ar").toReal();
    QVERIFY(aspectRatio > 1.0);
    QCOMPARE(view->property("videoWidth").toReal(), widthFromHeight ? (400 * aspectRatio) : 400);
    QCOMPARE(view->property("videoHeight").toReal(), heightFromWidth ? (400 / aspectRatio) : 400);
    QCOMPARE(view->property("_cropsVideo").toBool(), crops);
}

void VideoManagerTest::_testTelemetryOverlayFollowsTelemetry()
{
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kVideoViewQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/VideoView.qml")));
    const std::unique_ptr<QQuickItem> view(qobject_cast<QQuickItem*>(component.createWithInitialProperties(
        {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}})));
    QVERIFY2(view, qPrintable(component.errorString()));
    // Items outside a scene are never visible.
    QQuickWindow window;
    view->setParentItem(window.contentItem());
    auto* const overlay = view->findChild<QQuickItem*>(QStringLiteral("videoTelemetryOverlay"));
    QVERIFY(overlay);
    const auto setTelemetry = [this](const QString& text) {
        _videoManager->_telemetryOverlay->_text = text;
        emit _videoManager->_telemetryOverlay->textChanged();
    };

    _primary->setState(VideoReceiver::State::Streaming);
    _primary->setDecoding(true);
    QVERIFY(!overlay->isVisible());

    // The overlay is an item over the video: it shows the telemetry without waiting for a frame.
    setTelemetry(QStringLiteral("Alt: 10 m"));
    QVERIFY(overlay->isVisible());
    QVERIFY(showsText(view.get(), QStringLiteral("Alt: 10 m")));

    // The stream is lost: its last frame stays up, and the telemetry over it goes on.
    _primary->setDecoding(false);
    QVERIFY(_videoManager->primaryStream()->reconnecting());
    setTelemetry(QStringLiteral("Alt: 12 m"));
    QVERIFY(showsText(view.get(), QStringLiteral("Alt: 12 m")));
    QVERIFY(showsText(view.get(), QStringLiteral("VIDEO LOST - RECONNECTING")));

    setTelemetry(QString());
    QVERIFY(!overlay->isVisible());
}

void VideoManagerTest::_testCameraPinchZoomSteps()
{
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kCameraPinchZoomQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/CameraPinchZoom.qml")));
    const std::unique_ptr<QObject> zoom(component.create());
    QVERIFY2(zoom, qPrintable(component.errorString()));
    auto* const camera = zoom->property("camera").value<QObject*>();
    QVERIFY(camera);

    // Feeds one pinch's scales through it, as its pinch updates do, and returns the zoom steps the camera got.
    const auto pinch = [&zoom, camera](std::initializer_list<qreal> scales) {
        (void) camera->setProperty("steps", QVariantList());
        QVariant sent = 0;
        for (const qreal scale : scales) {
            QVariant next;
            if (!QMetaObject::invokeMethod(zoom.get(), "_stepToward", Q_RETURN_ARG(QVariant, next),
                                           Q_ARG(QVariant, scale), Q_ARG(QVariant, sent))) {
                return QVariantList({QStringLiteral("invoke failed")});
            }
            sent = next;
        }
        return camera->property("steps").toList();
    };

    // Two fingers that don't move send nothing; each 25% of spread or pinch sends one step, one per update.
    QCOMPARE(pinch({1.0, 1.0, 1.1}), QVariantList());
    QCOMPARE(pinch({1.0, 1.3, 1.3, 1.6}), QVariantList({1, 1}));
    QCOMPARE(pinch({1.0, 2.0, 2.0, 2.0}), QVariantList({1, 1, 1}));
    QCOMPARE(pinch({1.0, 0.75, 0.6}), QVariantList({-1, -1}));
    QCOMPARE(pinch({1.0, 1.3, 1.0}), QVariantList({1, -1}));
}

void VideoManagerTest::_testVideoViewPlacesThermalPip()
{
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kThermalVideoViewQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/ThermalVideoView.qml")));
    const std::unique_ptr<QObject> view(component.create());
    QVERIFY2(view, qPrintable(component.errorString()));
    auto* const thermal = view->findChild<QQuickItem*>(QStringLiteral("videoViewThermal"));
    QVERIFY(thermal);
    auto* const camera = view->property("camera").value<QObject*>();
    QVERIFY(camera);

    QCOMPARE(thermal->x(), view->property("pipX").toReal());
    QCOMPARE(thermal->y(), view->property("pipY").toReal());

    (void) view->setProperty("thermalPipTopInset", 60);
    QCOMPARE(thermal->y(), view->property("pipY").toReal());

    (void) camera->setProperty("thermalMode", MavlinkCameraControlInterface::THERMAL_FULL);
    QCOMPARE(thermal->height(), 400.0);
    QCOMPARE(thermal->x() + (thermal->width() / 2), 200.0);
    QCOMPARE(thermal->y(), 0.0);

    // A view narrower than the thermal video bounds its width.
    const qreal aspectRatio = thermal->width() / thermal->height();
    (void) view->setProperty("width", 200);
    QCOMPARE(thermal->width(), 200.0);
    QCOMPARE(thermal->height(), 200.0 / aspectRatio);
    QCOMPARE(thermal->y() + (thermal->height() / 2), 200.0);
    (void) view->setProperty("width", 400);

    (void) camera->setProperty("thermalMode", MavlinkCameraControlInterface::THERMAL_PIP);
    QCOMPARE(thermal->x(), view->property("pipX").toReal());
    QCOMPARE(thermal->y(), view->property("pipY").toReal());
}

void VideoManagerTest::_testVideoViewShowsOtherSourcesInTiles()
{
    const auto current = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5601");
    const auto thermalInfo =
        makeStreamInfo(VIDEO_STREAM_TYPE_RTSP, "rtsp://10.0.0.1/thermal", VIDEO_STREAM_ENCODING_H264, true);
    const auto second = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5602");
    const auto third = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5603");
    _videoManager->_setAdvertisedStreams(
        {current.get(), nullptr, QStringLiteral("Main")}, thermalInfo.get(),
        {{second.get(), nullptr, QStringLiteral("Second")}, {third.get(), nullptr, QStringLiteral("Third")}});
    _settle();
    VideoStreamController* const secondSource = _videoManager->_sources.at(3);
    FakeVideoReceiver* const secondReceiver = _receiverOf(secondSource);

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kTilesVideoViewQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/TilesVideoView.qml")));
    const std::unique_ptr<QQuickItem> view(qobject_cast<QQuickItem*>(component.createWithInitialProperties(
        {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}})));
    QVERIFY2(view, qPrintable(component.errorString()));
    // Items outside a scene are never visible.
    QQuickWindow window;
    view->setParentItem(window.contentItem());
    auto* const tiles = view->findChild<QQuickItem*>(QStringLiteral("videoViewTiles"));
    QVERIFY(tiles);

    // Each other stream has a tile, under its name, below what covers the top of the view and clear of the tool
    // strip at its left.
    QVERIFY(tiles->isVisible());
    QCOMPARE(shownTiles(tiles).size(), 2);
    // The row places its tiles when it is polished, as a window does before each frame.
    tiles->ensurePolished();
    const QQuickItem* const secondTile = shownTiles(tiles).first();
    QCOMPARE(secondTile->property("source").value<VideoStreamController*>(), secondSource);
    QVERIFY(showsText(secondTile, QStringLiteral("Second")));
    QVERIFY(showsText(shownTiles(tiles).last(), QStringLiteral("Third")));
    QCOMPARE(tiles->x(), view->property("pipX").toReal());
    QVERIFY(tiles->y() > 30);
    QVERIFY(tiles->width() > (2 * secondTile->width()));
    QVERIFY((tiles->x() + tiles->width()) < 800);
    QVERIFY(shownTiles(tiles).last()->mapToItem(tiles, QPointF()).x() > secondTile->width());
    // A press on a tile is the tile's, which the view tells whoever takes the presses over it.
    const auto tileAt = [&view](qreal x, qreal y) {
        QVariant onTile;
        return QMetaObject::invokeMethod(view.get(), "tileAt", Q_RETURN_ARG(QVariant, onTile), Q_ARG(QVariant, x),
                                         Q_ARG(QVariant, y)) &&
               onTile.toBool();
    };
    QVERIFY(tileAt(tiles->x() + tiles->width() - 1, tiles->y() + 1));
    QVERIFY(!tileAt(tiles->x() + tiles->width() + 1, tiles->y() + 1));

    // The stream plays into its tile, which says what a main picture does while it has none to show.
    QVERIFY(secondReceiver->videoOutput());
    QVERIFY(showsText(secondTile, QStringLiteral("WAITING FOR VIDEO")));
    secondReceiver->setState(VideoReceiver::State::Streaming);
    secondReceiver->setDecoding(true);
    QVERIFY(!showsText(secondTile, QStringLiteral("WAITING FOR VIDEO")));
    secondReceiver->setDecoding(false);
    QVERIFY(showsText(secondTile, QStringLiteral("VIDEO LOST - RECONNECTING")));

    // The tiles start right of the thermal picture-in-picture.
    auto* const thermalView = view->findChild<QQuickItem*>(QStringLiteral("videoViewThermal"));
    auto* const camera = view->property("camera").value<QObject*>();
    QVERIFY(thermalView && camera);
    _primary->setState(VideoReceiver::State::Streaming);
    _primary->setDecoding(true);
    (void) camera->setProperty("thermalMode", MavlinkCameraControlInterface::THERMAL_PIP);
    QVERIFY(thermalView->isVisible());
    QVERIFY(tiles->x() > (thermalView->x() + thermalView->width()));
    (void) camera->setProperty("thermalMode", MavlinkCameraControlInterface::THERMAL_OFF);
    QCOMPARE(tiles->x(), view->property("pipX").toReal());

    // With less room, as beside the Fly View's widgets, the tiles are smaller, down to a tile's least size. With room
    // for one tile of that size, the first stream alone has a tile: the other one's source is gone, so nothing
    // connects to its stream.
    const qreal roomyWidth = tiles->width();
    const qreal minimumTileWidth = tiles->property("minimumTileWidth").toReal();
    QVERIFY(minimumTileWidth > 0);
    (void) view->setProperty("tilesRightInset", tiles->property("maximumWidth").toReal() - (1.5 * minimumTileWidth));
    _settle();
    QCOMPARE(_videoManager->tileRoom(), 1);
    QCOMPARE(_videoManager->_sources.size(), 4);
    QCOMPARE(shownTiles(tiles).size(), 1);
    tiles->ensurePolished();
    QCOMPARE(shownTiles(tiles).first()->property("source").value<VideoStreamController*>(), secondSource);
    QVERIFY(tiles->width() < roomyWidth);
    QVERIFY(tiles->width() >= minimumTileWidth);

    // A view without room for tiles, as the picture-in-picture, shows none: their streams aren't decoded. It leaves
    // the manager with the room there was, and so the tiles' sources as they are.
    (void) view->setProperty("showTiles", false);
    QVERIFY(!tiles->isVisible());
    (void) view->setProperty("tilesRightInset", 0);
    _settle();
    QCOMPARE(secondReceiver->videoOutput(), nullptr);
    QCOMPARE(_videoManager->tileRoom(), 1);
    (void) view->setProperty("showTiles", true);
    QVERIFY(secondReceiver->videoOutput());
    _settle();
    QCOMPARE(shownTiles(tiles).size(), 2);

    // Out of sight behind another view, as the Fly View is behind the Plan view, the tiles play on, as the main picture
    // does: their streams would otherwise wait for a keyframe once the view is back.
    view->setVisible(false);
    QVERIFY(!tiles->isVisible());
    _settle();
    QVERIFY(secondReceiver->videoOutput());
}

void VideoManagerTest::_testTappedTileBecomesMainPicture()
{
    const QString camera = QStringLiteral("QGC Test Camera");
    _videoManager->_cameraAttached = [camera](const QString& name) { return name == camera; };
    _facts->set(_videoSettings->uvcTileCamera(), camera);
    // The camera the test attached isn't one Qt knows.
    expectLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("No camera named")));
    _settle();
    verifyExpectedLogMessage();
    const UVCCameraController* const uvcCamera = _videoManager->uvcCamera();

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kTilesVideoViewQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/TilesVideoView.qml")));
    const std::unique_ptr<QQuickItem> view(qobject_cast<QQuickItem*>(component.createWithInitialProperties(
        {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}})));
    QVERIFY2(view, qPrintable(component.errorString()));
    QQuickWindow window;
    window.resize(800, 400);
    view->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* const tiles = view->findChild<QQuickItem*>(QStringLiteral("videoViewTiles"));
    QVERIFY(tiles);
    const auto tapTile = [&window, tiles] {
        const QQuickItem* const tile = shownTiles(tiles).value(0);
        if (tile) {
            QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier,
                              tile->mapToScene(QPointF(tile->width() / 2, tile->height() / 2)).toPoint());
        }
        return tile != nullptr;
    };

    // The camera plays in the one tile, the stream in the main picture.
    QCOMPARE(shownTiles(tiles).size(), 1);
    QCOMPARE(shownTiles(tiles).first()->property("source").value<VideoStreamController*>(),
             _videoManager->_cameraSource);
    const QPointer<QQuickItem> streamOutput = _primary->videoOutput();
    const QPointer<QQuickItem> cameraOutput = uvcCamera->videoOutput();
    QVERIFY(streamOutput);
    QVERIFY(cameraOutput);
    _resetCounters();

    // A tap on its tile makes the camera the main picture. The stream takes the tile, and each plays on into the
    // output that took it over: the stream's receiver is never without one.
    QVERIFY(tapTile());
    QVERIFY(_videoManager->isUvc());
    _settle();
    QCOMPARE(shownTiles(tiles).size(), 1);
    QCOMPARE(shownTiles(tiles).first()->property("source").value<VideoStreamController*>(),
             _videoManager->primaryStream());
    QVERIFY(_primary->videoOutput());
    QVERIFY(_primary->videoOutput() != streamOutput);
    QCOMPARE(_primary->videoOutputChanges, 1);
    QCOMPARE(_primary->activeChanges, 0);
    QVERIFY(uvcCamera->videoOutput());
    QVERIFY(uvcCamera->videoOutput() != cameraOutput);

    // A tap on the stream's tile gives it the main picture back.
    QVERIFY(tapTile());
    QVERIFY(!_videoManager->isUvc());
    _settle();
    QCOMPARE(shownTiles(tiles).first()->property("source").value<VideoStreamController*>(),
             _videoManager->_cameraSource);
    QCOMPARE(_primary->videoOutputChanges, 2);
    QCOMPARE(_primary->activeChanges, 0);
}

void VideoManagerTest::_testTilesOutlastViewOnItsWayToFullSize()
{
    const auto current = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5601");
    const auto second = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5602");
    _videoManager->_setAdvertisedStreams({current.get(), nullptr, QStringLiteral("Main")}, nullptr,
                                         {{second.get(), nullptr, QStringLiteral("Second")}});
    _settle();
    const QPointer<VideoReceiver> tileReceiver = _videoManager->_sources.at(3)->receiver();
    QVERIFY(tileReceiver);

    // The Fly View's video becomes the main view before it has that view's size: it shows its tiles while it still has
    // the room of the picture-in-picture, none.
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kFlyViewVideoQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/FlyViewVideo.qml")));
    const std::unique_ptr<QQuickItem> flyView(qobject_cast<QQuickItem*>(component.createWithInitialProperties(
        {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}})));
    QVERIFY2(flyView, qPrintable(component.errorString()));
    _settle();

    // The stream in the tile played on through that: it didn't have to connect anew.
    QVERIFY(_videoManager->tileRoom() > 0);
    QVERIFY(tileReceiver);
}

void VideoManagerTest::_testFlyViewLeavesTapsOnTilesToThem()
{
    const QString camera = QStringLiteral("QGC Test Camera");
    _videoManager->_cameraAttached = [camera](const QString& name) { return name == camera; };
    _facts->set(_videoSettings->uvcTileCamera(), camera);
    // The camera the test attached isn't one Qt knows.
    expectLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("No camera named")));
    _settle();
    verifyExpectedLogMessage();

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kFlyViewVideoQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/FlyViewVideo.qml")));
    const std::unique_ptr<QQuickItem> flyView(qobject_cast<QQuickItem*>(component.createWithInitialProperties(
        {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}})));
    QVERIFY2(flyView, qPrintable(component.errorString()));
    QQuickWindow window;
    window.resize(800, 400);
    flyView->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* const tiles = flyView->findChild<QQuickItem*>(QStringLiteral("videoViewTiles"));
    QVERIFY(tiles);
    QVERIFY(tiles->isVisible());
    QCOMPARE(shownTiles(tiles).size(), 1);

    // The Fly View takes the presses on its video, for the gimbal and for full screen. One on a tile it leaves to the
    // tile, which lies below it.
    const QQuickItem* const tile = shownTiles(tiles).first();
    const QPoint onTile = tile->mapToScene(QPointF(tile->width() / 2, tile->height() / 2)).toPoint();
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, onTile);
    QVERIFY(_videoManager->isUvc());

    // A double tap selects once: the tile that takes the place of the selected one isn't selected with it.
    QCOMPARE(shownTiles(tiles).size(), 1);
    QTest::mouseDClick(&window, Qt::LeftButton, Qt::NoModifier, onTile);
    QVERIFY(!_videoManager->isUvc());
}

void VideoManagerTest::_testThermalDecodesOnlyWhileShown()
{
    const auto thermalInfo =
        makeStreamInfo(VIDEO_STREAM_TYPE_RTSP, "rtsp://10.0.0.1/thermal", VIDEO_STREAM_ENCODING_H264, true);
    _videoManager->thermalStream()->setStreamInfo(thermalInfo.get());
    QVERIFY(_videoManager->hasThermal());

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kThermalOverlayQml, QUrl(QStringLiteral("qrc:/VideoManagerTest/ThermalOverlay.qml")));
    const std::unique_ptr<QQuickItem> view(qobject_cast<QQuickItem*>(component.createWithInitialProperties(
        {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}})));
    QVERIFY2(view, qPrintable(component.errorString()));
    auto* const camera = view->property("camera").value<QObject*>();
    QVERIFY(camera);

    // The receiver decodes while it has an output: the overlay gives it one only while the thermal mode shows it. An
    // output that goes is the receiver's until the next pass.
    QVERIFY(_thermal->videoOutput());
    (void) camera->setProperty("thermalMode", MavlinkCameraControlInterface::THERMAL_OFF);
    _settle();
    QCOMPARE(_thermal->videoOutput(), nullptr);
    (void) camera->setProperty("thermalMode", MavlinkCameraControlInterface::THERMAL_BLEND);
    QVERIFY(_thermal->videoOutput());

    // Hidden with the view it is in, it keeps its output: the stream decodes on.
    QQuickWindow window;
    view->setParentItem(window.contentItem());
    view->setVisible(false);
    QVERIFY(_thermal->videoOutput());

    _videoManager->thermalStream()->setStreamInfo(nullptr);
    QVERIFY(!_videoManager->hasThermal());
    _settle();
    QCOMPARE(_thermal->videoOutput(), nullptr);
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
