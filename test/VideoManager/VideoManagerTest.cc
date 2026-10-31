#include "VideoManagerTest.h"

#include <algorithm>

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

#include "SettingsManager.h"
#include "VideoManager.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"
#include "VideoTestHelpers.h"

using VideoTestHelpers::makeStreamInfo;

namespace {

constexpr const char* kVideoOutputQml = R"QML(
import QtQuick
import QtMultimedia

VideoOutput {
    width: 64
    height: 48
}
)QML";

// FlightDisplayViewVideoOutput inside a container that tests move between windows.
constexpr const char* kStreamOutputQml = R"QML(
import QtQuick
import QGroundControl.FlyView

Item {
    id: root

    property var stream

    Item {
        objectName: "holder"

        FlightDisplayViewVideoOutput {
            objectName: "output"
            stream: root.stream
        }
    }
}
)QML";

constexpr const char* kStatsGroupQml = R"QML(
import QtQuick
import QGroundControl.Toolbar

VideoStreamStatsGroup {
}
)QML";

std::unique_ptr<QQuickItem> createVideoOutput(QQmlEngine& engine)
{
    QQmlComponent component(&engine);
    component.setData(kVideoOutputQml, QUrl(QStringLiteral("qrc:/VideoManagerTest.qml")));
    std::unique_ptr<QObject> object(component.create());
    return std::unique_ptr<QQuickItem>(qobject_cast<QQuickItem*>(object.release()));
}

void showFrame(QQuickItem* videoOutput, const QImage& image)
{
    videoOutput->property("videoSink").value<QVideoSink*>()->setVideoFrame(QVideoFrame(image));
}

/// Whether @p root shows a label reading @p text.
bool showsText(const QQuickItem* root, const QString& text)
{
    const QList<QQuickItem*> items = root->findChildren<QQuickItem*>();
    return std::ranges::any_of(
        items, [&text](const QQuickItem* item) { return item->isVisible() && (item->property("text") == text); });
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
    _facts->set(_videoSettings->streamEnabled(), true);
    _facts->set(_videoSettings->disableWhenDisarmed(), false);
    _facts->set(_videoSettings->lowLatencyMode(), false);
    _facts->set(_videoSettings->rtpJitterLatencyMs(), 80);
    _facts->set(_videoSettings->rtspAutoReconnect(), true);
    _facts->set(_videoSettings->rtspTimeout(), 8);

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
    for (FakeVideoReceiver* receiver : {_primary, _thermal}) {
        receiver->configChanges = 0;
        receiver->activeChanges = 0;
        receiver->restarts = 0;
        receiver->videoOutputChanges = 0;
    }
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

void VideoManagerTest::_testSinkSettingsRestartStreams_data()
{
    QTest::addColumn<QString>("setting");
    QTest::addColumn<QVariant>("value");

    QTest::newRow("videoConversionElement")
        << "videoConversionElement" << QVariant(QStringLiteral("videoconvertscale"));
    QTest::newRow("disablePixelAspectRatio") << "disablePixelAspectRatio" << QVariant(true);
}

void VideoManagerTest::_testSinkSettingsRestartStreams()
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

void VideoManagerTest::_testStreamEnabledControlsVideo()
{
    QVERIFY(_videoManager->hasVideo());
    QSignalSpy hasVideoSpy(_videoManager.get(), &VideoManager::hasVideoChanged);

    _facts->set(_videoSettings->streamEnabled(), false);
    QCOMPARE(hasVideoSpy.count(), 1);
    QVERIFY(!_videoManager->hasVideo());
    _settle();
    QVERIFY(!_primary->active());

    _videoSettings->streamEnabled()->setRawValue(true);
    QCOMPARE(hasVideoSpy.count(), 2);
    QVERIFY(_videoManager->hasVideo());
    _settle();
    QVERIFY(_primary->active());
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
    QVERIFY(_videoSettings->streamEnabled()->rawValue().toBool());

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
    stream->unregisterVideoOutput(&second);
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

    holder->setParentItem(root.get());
    root.reset();
    QCOMPARE(_primary->videoOutput(), nullptr);
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

    // The thermal output holds a frame but isn't streaming: only the primary stream is saved.
    const QString shot = tempDir.filePath(QStringLiteral("shot.png"));
    _videoManager->grabImage(shot);
    QCOMPARE(QImage(shot).size(), frame.size());
    QVERIFY(!QFile::exists(tempDir.filePath(QStringLiteral("shot.thermalVideo.png"))));

    _thermal->setState(VideoReceiver::State::Streaming);
    const QString second = tempDir.filePath(QStringLiteral("second.png"));
    _videoManager->grabImage(second);
    QVERIFY(QFile::exists(second));
    QCOMPARE(QImage(tempDir.filePath(QStringLiteral("second.thermalVideo.png"))).size(), frame.size());
}

void VideoManagerTest::_testStreamErrorsReachUser()
{
    const QRegularExpression message(QStringLiteral("stream failed"));
    expectLogMessage("Video.VideoManager", QtWarningMsg, message);
    expectAppMessage(message);
    emit _thermal->errorOccurred(QStringLiteral("stream failed"));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
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

    stats.packetsLost = 3;
    stats.lossPercent = 1.5;
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
    QCOMPARE(statsSpy.count(), 2);
    QVERIFY(showsText(group.get(), QStringLiteral("Packet loss")));
    QVERIFY(showsText(group.get(), QStringLiteral("1.5% (3 packet(s))")));
    QVERIFY(showsText(group.get(), QStringLiteral("1 / 0")));
    QVERIFY(showsText(group.get(), QStringLiteral("4 requested, 3 recovered")));
    QVERIFY(showsText(group.get(), QStringLiteral("1.0 s")));
    QVERIFY(showsText(group.get(), QStringLiteral("Decoder restarts")));
    QVERIFY(showsText(group.get(), QStringLiteral("1:02:05")));
    QVERIFY(showsText(group.get(), QStringLiteral("1:05, 2.0 MB")));

    // The same measurement again is no change, and the thermal stream's are its own.
    _primary->setStats(stats);
    _thermal->setStats(stats);
    QCOMPARE(statsSpy.count(), 2);

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
    QVERIFY(showsText(group.get(), QStringLiteral("0 kbit/s (camera 8.00 Mbit/s)")));
    QVERIFY(showsText(group.get(), QStringLiteral("H.264 (camera 1280 × 720)")));
    stream->setStreamInfo(nullptr);
    _settle();

    // A receiver going away takes its measurements with it.
    stream->setReceiver(nullptr);
    _primary = nullptr;
    QCOMPARE(statsSpy.count(), 3);
    QVERIFY(!group->isVisible());
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
