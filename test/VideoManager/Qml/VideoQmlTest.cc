#include "VideoQmlTest.h"

#include <array>
#include <functional>

#include <QtCore/QLineF>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>

#include "MavlinkCameraControlInterface.h"
#include "UVCCameraController.h"
#include "VideoBackend.h"
#include "VideoDecoderOptions.h"
#include "VideoManager.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"
#include "VideoTelemetryOverlay.h"
#include "VideoTestHelpers.h"

namespace {

using VideoTestHelpers::InlineQmlEngine;
using VideoTestHelpers::kStreamOutputQml;
using VideoTestHelpers::makeStreamInfo;
using VideoTestHelpers::showFrame;
using VideoTestHelpers::showsText;

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
        objectName: "overlay"
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

void VideoQmlTest::_testDecodingSettingsGroupRestartsStreams()
{
    Fact* const decoderFact = _videoSettings->forceVideoDecoder();
    Fact* const cpuPathFact = _videoSettings->forceCpuVideoPath();
    _facts->set(decoderFact, static_cast<int>(VideoDecoderOption::Default));
    _facts->set(cpuPathFact, false);
    _settle();
    _resetCounters();

    InlineQmlEngine engine;
    const std::unique_ptr<QObject> group = engine.create(kDecodingSettingsQml);
    QVERIFY2(group, qPrintable(engine.error()));

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

void VideoQmlTest::_testStreamSettingsGroupFollowsLowLatency()
{
    if (!VideoBackend::instance().capabilities().pipelineSettings) {
        QSKIP("The RTP jitter-buffer latency is a GStreamer setting");
    }
    _facts->set(_videoSettings->lowLatencyMode(), false);

    InlineQmlEngine engine;
    const std::unique_ptr<QObject> group = engine.create(kStreamSettingsQml);
    QVERIFY2(group, qPrintable(engine.error()));

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

void VideoQmlTest::_testPlaceholderTellsWhyThereIsNoVideo_data()
{
    QTest::addColumn<int>("reason");
    QTest::addColumn<QString>("text");

    QTest::newRow("awaited") << int(VideoManager::NoVideoReason::Waiting) << "WAITING FOR VIDEO";
    QTest::newRow("stopped") << int(VideoManager::NoVideoReason::Stopped) << "VIDEO STOPPED";
    QTest::newRow("disarmed") << int(VideoManager::NoVideoReason::DisabledWhileDisarmed)
                              << "VIDEO DISABLED WHILE DISARMED";
    QTest::newRow("stopped and disarmed") << int(VideoManager::NoVideoReason::Stopped) << "VIDEO STOPPED";
}

void VideoQmlTest::_testPlaceholderTellsWhyThereIsNoVideo()
{
    QFETCH(int, reason);
    QFETCH(QString, text);

    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> placeholder = engine.create<QQuickItem>(
        kPlaceholderQml, {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}});
    QVERIFY2(placeholder, qPrintable(engine.error()));
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

void VideoQmlTest::_testQmlVideoOutputRegistersItself()
{
    InlineQmlEngine engine;
    std::unique_ptr<QQuickItem> root = engine.create<QQuickItem>(
        kStreamOutputQml, {{QStringLiteral("stream"), QVariant::fromValue(_videoManager->primaryStream())}});
    QVERIFY2(root, qPrintable(engine.error()));
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

void VideoQmlTest::_testStreamStatsReachQml()
{
    VideoStreamController* const stream = _videoManager->primaryStream();
    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> group =
        engine.create<QQuickItem>(kStatsGroupQml, {{QStringLiteral("stream"), QVariant::fromValue(stream)}});
    QVERIFY2(group, qPrintable(engine.error()));
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

void VideoQmlTest::_testStatsGroupShowsCamera()
{
    const QString name = QStringLiteral("QGC Test Camera");
    VideoStreamController* const cameraSource = _videoManager->_cameraSource;
    UVCCameraController* const camera = _videoManager->uvcCamera();
    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> group =
        engine.create<QQuickItem>(kStatsGroupQml, {{QStringLiteral("stream"), QVariant::fromValue(cameraSource)}});
    QVERIFY2(group, qPrintable(engine.error()));
    // Items outside a scene are never visible.
    QQuickWindow window;
    group->setParentItem(window.contentItem());
    QVERIFY(!group->isVisible());

    // What a camera measures: its frames, and the frame rate it is set up for.
    VideoStreamStats stats;
    stats.valid = true;
    stats.fps = 30.0;
    stats.cameraFps = 30.0;
    stats.bitrateKbps = -1.0;
    stats.framesDropped = -1;
    stats.codec = QStringLiteral("MJPEG");
    stats.gpuPath = QStringLiteral("CPU");
    stats.uptimeMs = 1000;
    camera->setVideoSize(QSize(640, 480));
    camera->setDecoding(true);
    camera->setStats(stats);
    QVERIFY(group->isVisible());
    QVERIFY(showsText(group.get(), QStringLiteral("30.0 fps (camera 30 fps)")));
    QVERIFY(showsText(group.get(), QStringLiteral("640 × 480 MJPEG")));
    QVERIFY(showsText(group.get(), QStringLiteral("CPU")));
    QVERIFY(showsText(group.get(), QStringLiteral("0:01")));
    // Nothing of what only a stream tells, nor what a camera doesn't count.
    for (const char* const row : {"Source", "URL", "Dropped", "Bitrate", "Decoder", "Latency"}) {
        QVERIFY2(!showsText(group.get(), QString::fromLatin1(row)), row);
    }
    // Without a camera selected there is no row for its name.
    QVERIFY(!showsText(group.get(), QStringLiteral("Camera")));

    _videoManager->_cameraAttached = [name](const QString& camera) { return camera == name; };
    _facts->set(_videoSettings->videoSource(), name);
    // The camera the test attached isn't one Qt knows.
    expectLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("No camera named")));
    _settle();
    verifyExpectedLogMessage();
    camera->setStats(stats);
    QVERIFY(showsText(group.get(), QStringLiteral("Camera")));
    QVERIFY(showsText(group.get(), name));
}

void VideoQmlTest::_testIndicatorReportsMeasuredSource()
{
    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> toolbar = engine.create<QQuickItem>(
        kIndicatorQml, {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}});
    QVERIFY2(toolbar, qPrintable(engine.error()));
    // Items outside a scene are never visible.
    QQuickWindow window;
    toolbar->setParentItem(window.contentItem());
    const QQuickItem* const indicator = toolbar->findChild<QQuickItem*>(QStringLiteral("toolbar_videoStreamIndicator"));
    QVERIFY(indicator);

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
    // No source is measured, and the video is awaited: the indicator says so in place of the figures.
    QVERIFY(indicator->property("showIndicator").toBool());
    QVERIFY(showsText(indicator, QStringLiteral("Waiting")));
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

void VideoQmlTest::_testSourceStatsShowMeasuredSources()
{
    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> sourceStats = engine.create<QQuickItem>(
        kSourceStatsQml, {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}});
    QVERIFY2(sourceStats, qPrintable(engine.error()));
    // Items outside a scene are never visible.
    QQuickWindow window;
    sourceStats->setParentItem(window.contentItem());
    QVERIFY(showsText(sourceStats.get(), QStringLiteral("No video stream")));

    // Each source has a group.
    const auto sourceGroups = [&sourceStats] {
        QList<QQuickItem*> groups;
        for (QQuickItem* const child : sourceStats->childItems()) {
            if (child->property("stream").isValid()) {
                groups.append(child);
            }
        }
        return groups;
    };
    QList<QQuickItem*> groups = sourceGroups();
    QCOMPARE(groups.size(), 3);
    const auto group = [&groups](int index) { return groups.at(index); };
    QCOMPARE(group(0)->property("stream").value<VideoStreamController*>(), _videoManager->primaryStream());
    QCOMPARE(group(0)->property("heading").toString(), QStringLiteral("Video"));
    QCOMPARE(group(1)->property("stream").value<VideoStreamController*>(), _videoManager->thermalStream());
    QCOMPARE(group(1)->property("heading").toString(), QStringLiteral("Thermal"));
    UVCCameraController* const camera = _videoManager->uvcCamera();
    QCOMPARE(group(2)->property("stream").value<VideoStreamController*>(), _videoManager->_cameraSource);
    QCOMPARE(group(2)->property("heading").toString(), QStringLiteral("USB Camera"));

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
            QCOMPARE(groups.at(index)->isVisible(), index == shown);
        }
        setStats.at(shown)({});
    }
    QVERIFY(showsText(sourceStats.get(), QStringLiteral("No video stream")));

    // A stream in a tile gets a group too, under its name.
    const auto current = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5601");
    const auto other = makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5602");
    _videoManager->_setAdvertisedStreams({current.get(), nullptr, QStringLiteral("Main")}, nullptr,
                                         {{other.get(), nullptr, QStringLiteral("Gimbal: Wide")}});
    groups = sourceGroups();
    QCOMPARE(groups.size(), 4);
    QCOMPARE(group(3)->property("stream").value<VideoStreamController*>(), _videoManager->_sources.last());
    QCOMPARE(group(3)->property("heading").toString(), QStringLiteral("Gimbal: Wide"));
    QCOMPARE(group(0)->property("heading").toString(), QStringLiteral("Video"));
}

void VideoQmlTest::_testVideoViewFitsVideo_data()
{
    // For a stream of 64 × 48 frames in a square view: the ratio its picture is forced to, whether the fitted video
    // takes its width from the view's height, and its height from the view's width, and whether it is cropped to the
    // view.
    QTest::addColumn<int>("fitMode");
    QTest::addColumn<int>("forced");
    QTest::addColumn<bool>("widthFromHeight");
    QTest::addColumn<bool>("heightFromWidth");
    QTest::addColumn<bool>("crops");

    const int own = VideoSettings::AspectRatioAuto;
    QTest::newRow("fit width") << int(VideoSettings::VideoFitWidth) << own << false << true << false;
    QTest::newRow("fit height") << int(VideoSettings::VideoFitHeight) << own << true << false << true;
    QTest::newRow("fill") << int(VideoSettings::VideoFitFill) << own << true << false << true;
    QTest::newRow("no crop") << int(VideoSettings::VideoFitNoCrop) << own << false << true << false;
    const int forced = VideoSettings::AspectRatio16x9;
    QTest::newRow("forced, no crop") << int(VideoSettings::VideoFitNoCrop) << forced << false << true << false;
    QTest::newRow("forced, fill") << int(VideoSettings::VideoFitFill) << forced << true << false << true;
}

void VideoQmlTest::_testVideoViewFitsVideo()
{
    QFETCH(int, fitMode);
    QFETCH(int, forced);
    QFETCH(bool, widthFromHeight);
    QFETCH(bool, heightFromWidth);
    QFETCH(bool, crops);

    _facts->set(_videoSettings->videoFit(), fitMode);
    _facts->set(_videoSettings->forcedAspectRatio(), forced);
    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> view = engine.create<QQuickItem>(
        kVideoViewQml, {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}});
    QVERIFY2(view, qPrintable(engine.error()));
    auto* const output = view->findChild<QQuickItem*>(QStringLiteral("videoViewOutput"));
    QVERIFY(output);

    // The video's own ratio is its frames', which the first one tells; until then the video fills the view.
    const bool isForced = forced != VideoSettings::AspectRatioAuto;
    const qreal aspectRatio = isForced ? VideoSettings::aspectRatioOf(forced) : (64.0 / 48.0);
    if (!isForced) {
        QCOMPARE(view->property("videoWidth").toReal(), 400.0);
        QCOMPARE(view->property("videoHeight").toReal(), 400.0);
    }
    showFrame(output, VideoTestHelpers::redTopLeftQuadrant());
    QTRY_COMPARE_WITH_TIMEOUT(output->property("implicitHeight").toReal(), 48.0, TestTimeout::shortMs());
    QCOMPARE(view->property("_ar").toReal(), aspectRatio);

    const QSizeF fitted(widthFromHeight ? (400 * aspectRatio) : 400, heightFromWidth ? (400 / aspectRatio) : 400);
    QCOMPARE(view->property("videoWidth").toReal(), fitted.width());
    QCOMPARE(view->property("videoHeight").toReal(), fitted.height());

    // Where the output draws. The video's own picture is fitted by the output, which fills the view; a forced one
    // is the whole video, stretched over the fitted rectangle.
    const QRectF drawn = output->mapRectToItem(view.get(), QRectF(0, 0, output->width(), output->height()));
    if (isForced) {
        QCOMPARE(output->property("fillMode").toInt(), static_cast<int>(Qt::IgnoreAspectRatio));
        // To the precision of the item's transform, which is single.
        QVERIFY2(qAbs(drawn.width() - fitted.width()) < 0.01, qPrintable(QString::number(drawn.width())));
        QVERIFY2(qAbs(drawn.height() - fitted.height()) < 0.01, qPrintable(QString::number(drawn.height())));
        QVERIFY(QLineF(drawn.center(), QPointF(200, 200)).length() < 0.01);
    } else {
        QCOMPARE(output->property("fillMode").toInt(),
                 static_cast<int>(crops ? Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio));
        QCOMPARE(drawn, QRectF(0, 0, 400, 400));
    }
}

void VideoQmlTest::_testTelemetryOverlayFollowsTelemetry()
{
    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> view = engine.create<QQuickItem>(
        kVideoViewQml, {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}});
    QVERIFY2(view, qPrintable(engine.error()));
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

void VideoQmlTest::_testCameraPinchZoomSteps()
{
    InlineQmlEngine engine;
    const std::unique_ptr<QObject> zoom = engine.create(kCameraPinchZoomQml);
    QVERIFY2(zoom, qPrintable(engine.error()));
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

void VideoQmlTest::_testVideoViewPlacesThermalPip()
{
    InlineQmlEngine engine;
    const std::unique_ptr<QObject> view = engine.create(kThermalVideoViewQml);
    QVERIFY2(view, qPrintable(engine.error()));
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

void VideoQmlTest::_testVideoViewShowsOtherSourcesInTiles()
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

    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> view = engine.create<QQuickItem>(
        kTilesVideoViewQml, {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}});
    QVERIFY2(view, qPrintable(engine.error()));
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

    // A view without room for tiles, as the picture-in-picture, shows none: their streams aren't decoded. For the
    // grace period it leaves the manager with the room there was, and so the tiles' sources as they are.
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

    // A view that stays without its tiles for longer than the grace period has no room for them any more: their
    // sources go, and with them the receivers of their streams. They are back, connecting anew, with the tiles.
    const QPointer<VideoReceiver> hiddenReceiver = secondReceiver;
    QVERIFY(tiles->setProperty("hiddenGrace", 1));
    (void) view->setProperty("showTiles", false);
    QTRY_COMPARE_WITH_TIMEOUT(_videoManager->tileRoom(), 0, TestTimeout::mediumMs());
    _settle();
    QCOMPARE(_videoManager->_sources.size(), 3);
    QVERIFY(hiddenReceiver.isNull());
    (void) view->setProperty("showTiles", true);
    _settle();
    QCOMPARE(_videoManager->_sources.size(), 5);
    // The new sources' updates were queued by the manager's.
    _settle();
    QVERIFY(_receiverOf(_videoManager->_sources.at(3))->active());
}

void VideoQmlTest::_testTappedTileBecomesMainPicture()
{
    const QString camera = QStringLiteral("QGC Test Camera");
    _videoManager->_cameraAttached = [camera](const QString& name) { return name == camera; };
    _facts->set(_videoSettings->uvcTileCamera(), camera);
    // The camera the test attached isn't one Qt knows.
    expectLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("No camera named")));
    _settle();
    verifyExpectedLogMessage();
    const UVCCameraController* const uvcCamera = _videoManager->uvcCamera();

    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> view = engine.create<QQuickItem>(
        kTilesVideoViewQml, {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}});
    QVERIFY2(view, qPrintable(engine.error()));
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
    // The camera plays into the main picture's output, which the stream left.
    QVERIFY(streamOutput);
    QCOMPARE(uvcCamera->videoOutput(), streamOutput.data());

    // A tap on the stream's tile gives it the main picture back.
    QVERIFY(tapTile());
    QVERIFY(!_videoManager->isUvc());
    _settle();
    QCOMPARE(shownTiles(tiles).first()->property("source").value<VideoStreamController*>(),
             _videoManager->_cameraSource);
    QCOMPARE(_primary->videoOutput(), streamOutput.data());
    QVERIFY(uvcCamera->videoOutput());
    QVERIFY(uvcCamera->videoOutput() != streamOutput);
    QCOMPARE(_primary->videoOutputChanges, 2);
    QCOMPARE(_primary->activeChanges, 0);
}

void VideoQmlTest::_testTilesOutlastViewOnItsWayToFullSize()
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
    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> flyView = engine.create<QQuickItem>(
        kFlyViewVideoQml, {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}});
    QVERIFY2(flyView, qPrintable(engine.error()));
    _settle();

    // The stream in the tile played on through that: it didn't have to connect anew.
    QVERIFY(_videoManager->tileRoom() > 0);
    QVERIFY(tileReceiver);
}

void VideoQmlTest::_testFlyViewLeavesTapsOnTilesToThem()
{
    const QString camera = QStringLiteral("QGC Test Camera");
    _videoManager->_cameraAttached = [camera](const QString& name) { return name == camera; };
    _facts->set(_videoSettings->uvcTileCamera(), camera);
    // The camera the test attached isn't one Qt knows.
    expectLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("No camera named")));
    _settle();
    verifyExpectedLogMessage();

    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> flyView = engine.create<QQuickItem>(
        kFlyViewVideoQml, {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}});
    QVERIFY2(flyView, qPrintable(engine.error()));
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

void VideoQmlTest::_testVideoViewReportsClicksAndDrags()
{
    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> view = engine.create<QQuickItem>(
        kVideoViewQml, {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}});
    QVERIFY2(view, qPrintable(engine.error()));
    QVERIFY(view->setProperty("interactive", true));
    QQuickWindow window;
    window.resize(400, 400);
    view->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* const clickTimer = view->findChild<QObject*>(QStringLiteral("videoViewClickTimer"));
    QVERIFY(clickTimer);
    QSignalSpy clicked(view.get(), SIGNAL(clicked(double, double)));
    QSignalSpy dragStarted(view.get(), SIGNAL(dragStarted(double, double)));
    QSignalSpy dragMoved(view.get(), SIGNAL(dragMoved(double, double)));
    QSignalSpy dragEnded(view.get(), SIGNAL(dragEnded(double, double)));

    // A press that moves past the drag threshold is a drag, from where it was pressed to where it was released, and
    // no click.
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 200));
    QTest::mouseMove(&window, QPoint(130, 200));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(130, 200));
    QCOMPARE(dragStarted.count(), 1);
    QCOMPARE(dragStarted.first(), QVariantList({100.0, 200.0}));
    QCOMPARE(dragMoved.last(), QVariantList({130.0, 200.0}));
    QCOMPARE(dragEnded.count(), 1);
    QCOMPARE(dragEnded.first(), QVariantList({130.0, 200.0}));
    QVERIFY(!clickTimer->property("running").toBool());

    // A double-click toggles full screen and leaves no click to report.
    _videoManager->_fullScreen = true;
    QTest::mouseDClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(200, 300));
    QVERIFY(!_videoManager->fullScreen());
    QVERIFY(!clickTimer->property("running").toBool());

    // A click is reported once no second click followed it.
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(300, 300));
    QVERIFY_SIGNAL_WAIT(clicked, TestTimeout::shortMs());
    QCOMPARE(clicked.count(), 1);
    QCOMPARE(clicked.first(), QVariantList({300.0, 300.0}));
}

void VideoQmlTest::_testThermalDecodesOnlyWhileShown()
{
    const auto thermalInfo =
        makeStreamInfo(VIDEO_STREAM_TYPE_RTSP, "rtsp://10.0.0.1/thermal", VIDEO_STREAM_ENCODING_H264, true);
    _videoManager->thermalStream()->setStreamInfo(thermalInfo.get());
    QVERIFY(_videoManager->hasThermal());

    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> view = engine.create<QQuickItem>(
        kThermalOverlayQml, {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}});
    QVERIFY2(view, qPrintable(engine.error()));
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

void VideoQmlTest::_testThermalOverlayHasThePicturesShape()
{
    const auto thermalInfo =
        makeStreamInfo(VIDEO_STREAM_TYPE_RTSP, "rtsp://10.0.0.1/thermal", VIDEO_STREAM_ENCODING_H264, true);
    _videoManager->thermalStream()->setStreamInfo(thermalInfo.get());

    InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> view = engine.create<QQuickItem>(
        kThermalOverlayQml, {{QStringLiteral("videoManager"), QVariant::fromValue(_videoManager.get())}});
    QVERIFY2(view, qPrintable(engine.error()));
    const auto* const overlay = view->findChild<QQuickItem*>(QStringLiteral("overlay"));
    QVERIFY(overlay);
    const auto shape = [overlay] { return overlay->width() / overlay->height(); };

    // Until a frame arrives, the advertised resolution (1280 × 720) shapes the overlay.
    QCOMPARE(shape(), 1280.0 / 720.0);

    // Then the picture its output shows does, as for the main picture: here of another shape than the stream tells.
    QVERIFY(_thermal->videoOutput());
    showFrame(_thermal->videoOutput(), VideoTestHelpers::redTopLeftQuadrant());
    QTRY_COMPARE_WITH_TIMEOUT(shape(), 64.0 / 48.0, TestTimeout::shortMs());
}

UT_REGISTER_TEST(VideoQmlTest, TestLabel::Integration)
