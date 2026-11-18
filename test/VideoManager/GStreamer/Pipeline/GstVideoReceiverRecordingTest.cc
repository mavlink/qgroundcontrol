#include "GstVideoReceiverRecordingTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <cerrno>
#include <gst/video/video-event.h>
#include <memory>
#include <optional>

#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>

#include "DecodingBranch.h"
#include "GStreamerDecoderRanks.h"

namespace {

using GstReceiverTest::branchHead;
using GstReceiverTest::FileFormat;
using GstReceiverTest::Gate;
using GstReceiverTest::kFinalizeTimeoutMs;
using GstReceiverTest::kFrameRate;
using GstReceiverTest::kMinRecordedFrames;
using GstReceiverTest::kRecordFrames;
using GstReceiverTest::kShortWatchdogTimeout;
using GstReceiverTest::kWatchdogTimeout;
using GstReceiverTest::loopbackUri;
using GstReceiverTest::Receiver;
using GstReceiverTest::State;
using GstReceiverTest::udpListenUri;

// The test stream has a keyframe every 5 frames.
constexpr int kFramesPerGop = 5;

enum class RecordingEnd
{
    Stopped,
    RecorderError,
    Deactivated,
};

/// Why the recording at @p path isn't a finalized file, or an empty string when it is.
QString recordingProblem(const QString& path)
{
    QString failure;
    const int frames =
        GstTest::countPlayableFrames(path, failure, path.endsWith(QLatin1String(".mkv")) ? "matroskademux" : "qtdemux");
    return (frames >= kMinRecordedFrames)
               ? QString()
               : QStringLiteral("%1: %2 frames recorded %3").arg(path).arg(frames).arg(failure);
}

GstPadProbeReturn countRecordingKeyframeRequests([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    GstEvent* const event = GST_PAD_PROBE_INFO_EVENT(info);
    if (event && gst_video_event_is_force_key_unit(event)) {
        static_cast<std::atomic<int>*>(data)->fetch_add(1, std::memory_order_relaxed);
    }
    return GST_PAD_PROBE_OK;
}

/// Keeps the stream from the decoder while the flag passed as data is set, so that the decoder shows no frame.
GstPadProbeReturn starveDecoder([[maybe_unused]] GstPad* pad, [[maybe_unused]] GstPadProbeInfo* info, gpointer data)
{
    return static_cast<std::atomic<bool>*>(data)->load(std::memory_order_relaxed) ? GST_PAD_PROBE_DROP
                                                                                  : GST_PAD_PROBE_OK;
}

/// Keeps the stream's keyframes from the recorder while held, as a camera that sends none for a while.
struct KeyframeHold
{
    std::atomic<bool> held{false};
    std::atomic<int> dropped{0};
};

GstPadProbeReturn dropHeldKeyframes([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    auto* const hold = static_cast<KeyframeHold*>(data);
    const GstBuffer* const buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    if (buffer && !GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT) &&
        hold->held.load(std::memory_order_relaxed)) {
        hold->dropped.fetch_add(1, std::memory_order_relaxed);
        return GST_PAD_PROBE_DROP;
    }
    return GST_PAD_PROBE_OK;
}

/// Takes the parameter sets out of the stream, as a camera sends it that tells its parameter sets once, when the
/// stream starts: until the next @c keyframes keyframes have passed, or for good when that is negative. Out of the
/// delta frames in between too: upstream gives the parameter sets of a keyframe whose slices were lost on the way,
/// as on a loaded host, to the frame after it.
struct ParameterSetStrip
{
    std::atomic<int> keyframes{0};
    std::atomic<int> stripped{0};
};

/// A copy of the H.264 access unit @p buffer (Annex B) without its SPS and PPS.
GstBuffer* withoutParameterSets(GstBuffer* buffer)
{
    constexpr QByteArrayView kStartCode("\x00\x00\x01", 3);
    constexpr int kNalSps = 7;
    constexpr int kNalPps = 8;

    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        return gst_buffer_ref(buffer);
    }
    const QByteArrayView unit(reinterpret_cast<const char*>(map.data), static_cast<qsizetype>(map.size));
    QByteArray kept;
    for (qsizetype nal = unit.indexOf(kStartCode); (nal >= 0) && ((nal + 3) < unit.size());) {
        const qsizetype next = unit.indexOf(kStartCode, nal + 3);
        const int type = unit.at(nal + 3) & 0x1F;
        if ((type != kNalSps) && (type != kNalPps)) {
            kept.append(unit.sliced(nal, ((next < 0) ? unit.size() : next) - nal));
        }
        nal = next;
    }
    gst_buffer_unmap(buffer, &map);

    GstBuffer* const stripped = gst_buffer_new_memdup(kept.constData(), static_cast<gsize>(kept.size()));
    (void) gst_buffer_copy_into(stripped, buffer,
                                static_cast<GstBufferCopyFlags>(GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_TIMESTAMPS), 0,
                                static_cast<gsize>(-1));
    return stripped;
}

GstPadProbeReturn stripParameterSets([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    auto* const strip = static_cast<ParameterSetStrip*>(data);
    GstBuffer* const buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    const int keyframes = strip->keyframes.load(std::memory_order_relaxed);
    if (!buffer || (keyframes == 0)) {
        return GST_PAD_PROBE_OK;
    }
    const bool keyframe = !GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    GST_PAD_PROBE_INFO_DATA(info) = withoutParameterSets(buffer);
    gst_buffer_unref(buffer);
    if (keyframe) {
        if (keyframes > 0) {
            strip->keyframes.store(keyframes - 1, std::memory_order_relaxed);
        }
        strip->stripped.fetch_add(1, std::memory_order_relaxed);
    }
    return GST_PAD_PROBE_OK;
}

/// Has the sync of a finished file fail for a full disk: posts what filesink does when its fsync fails, in place of
/// the empty buffer that asks for the sync.
GstPadProbeReturn failSyncAtEnd(GstPad* pad, GstPadProbeInfo* info, [[maybe_unused]] gpointer data)
{
    GstBuffer* const buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    if (!GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_SYNC_AFTER) || (gst_buffer_get_size(buffer) != 0)) {
        return GST_PAD_PROBE_OK;
    }
    const GStreamer::GstElementPtr fileSink = GStreamer::adoptElement(gst_pad_get_parent_element(pad));
    const GStreamer::GErrorPtr error = GStreamer::adoptError(
        g_error_new_literal(GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_WRITE, "Error while writing to file."));
    (void) gst_element_post_message(fileSink.get(),
                                    gst_message_new_error(GST_OBJECT(fileSink.get()), error.get(), g_strerror(ENOSPC)));
    return GST_PAD_PROBE_DROP;
}

}  // namespace

void GstVideoReceiverRecordingTest::init()
{
    GstVideoReceiverTestBase::init();
    _tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(_tempDir->isValid());
}

void GstVideoReceiverRecordingTest::cleanup()
{
    _tempDir.reset();
    GstVideoReceiverTestBase::cleanup();
}

void GstVideoReceiverRecordingTest::_testDecoderFailureKeepsRecording_data()
{
    QTest::addColumn<bool>("givenUp");

    // The decoder that replaces the failed one decodes.
    QTest::newRow("decoder replaced") << false;
    // Each replacement fails too: the stream goes on without a picture, until a decoder tried later decodes.
    QTest::newRow("decoder given up") << true;
}

void GstVideoReceiverRecordingTest::_testDecoderFailureKeepsRecording()
{
    QFETCH(bool, givenUp);

    // Before the receiver, which they outlive.
    std::optional<GstTest::DecoderRankSnapshot> decoderRanks;
    if (givenUp) {
        // Failures in a row would take the host's hardware decoders out of autoplug, one after the other.
        GSTTEST_SKIP_UNLESS_FACTORIES("avdec_h264");
        decoderRanks.emplace();
        GStreamer::setCodecPriorities(VideoDecoderOption::Software);
    }

    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    const QString path = _tempDir->filePath(QStringLiteral("decoder-failure.mp4"));
    QVERIFY(stream.record(path));
    const int decodingChanges = stream.decodingSpy.count();
    stream.stateSpy.clear();

    // A replacement that gets no frame shows none, and so counts as failed when it posts an error too.
    std::atomic<bool> starved{givenUp};
    GStreamer::PadProbe starveProbe;
    QVERIFY(stream.probeBranchHead(starveProbe, GStreamer::PipelineBranch::Decoding, "valve", "sink",
                                   GST_PAD_PROBE_TYPE_BUFFER, starveDecoder, &starved));
    const auto currentDecoder = [&stream] {
        GStreamer::GstElementPtr decoder;
        (void) runOnPipeline(stream.receiver.get(), [&decoder](GstElement* pipeline) {
            decoder = GstTest::findByFactory(pipeline, "decodebin3");
        });
        return decoder;
    };

    const QRegularExpression decoderError(QStringLiteral("^Decoder error from .*injected decoder failure"));
    const int failures = givenUp ? (DecodingBranch::kMaxDecoderRestarts + 1) : 1;
    if (givenUp) {
        expectLogMessage(
            "Video.GStreamer.GstVideoReceiver", QtWarningMsg,
            QRegularExpression(QStringLiteral("^Video stream without a picture:.*injected decoder failure")));
    }
    // Held, so that the decoder that replaces it can't get its address.
    GStreamer::GstElementPtr failing = currentDecoder();
    for (int failure = 0; failure < failures; ++failure) {
        QVERIFY(failing);
        expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg, decoderError);
        QVERIFY(stream.postError("decodebin3", GST_STREAM_ERROR, GST_STREAM_ERROR_DECODE, "injected decoder failure"));
        // The worker has acted on the error once the decoder is another one, or gone.
        GStreamer::GstElementPtr next;
        QVERIFY_TRUE_WAIT((next = currentDecoder()).get() != failing.get(), TestTimeout::mediumMs());
        failing = std::move(next);
    }

    if (givenUp) {
        // Told once, with what the decoder said.
        QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
        verifyExpectedLogMessage();
        const QString message = stream.errorSpy.constFirst().constFirst().toString();
        QVERIFY2(message.contains(QLatin1String("injected decoder failure")), qPrintable(message));
        QVERIFY(!stream.receiver->decoding());
        QVERIFY(stream.framesSent(kRecordFrames));
        // The decoder tried next gets the stream.
        starved.store(false);
    }
    for (int failure = 0; failure < failures; ++failure) {
        verifyExpectedLogMessage();
    }

    // A new decoder takes over; the recording and the stream went on.
    QVERIFY_SIGNAL_COUNT_WAIT(stream.decodingSpy, decodingChanges + 2, TestTimeout::mediumMs());
    QVERIFY(stream.receiver->decoding());
    QVERIFY(stream.framesSent(kRecordFrames));
    QVERIFY2(stream.stateSpy.isEmpty(), "A failing decoder restarted the stream");
    QCOMPARE(stream.receiver->state(), State::Streaming);
    QVERIFY(stream.recordingFinishedSpy.isEmpty());
    QVERIFY_TRUE_WAIT(stream.receiver->stats().decoderRestarts == (givenUp ? DecodingBranch::kMaxDecoderRestarts : 1),
                      TestTimeout::mediumMs());
    QVERIFY(stream.receiver->stats().recordingMs >= 0);
    QVERIFY_TRUE_WAIT(stream.receiver->stats().recordingBytes > 0, TestTimeout::mediumMs());

    stream.receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    QCOMPARE(stream.recordingStartedSpy.count(), 1);
    QCOMPARE(stream.errorSpy.count(), givenUp ? 1 : 0);
    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void GstVideoReceiverRecordingTest::_testUndecodableStreamIsRecorded()
{
    // No decoder takes H.265 from here on, as on a machine that has none. Before the receiver, which it outlives.
    const GstTest::DecoderRankSnapshot decoderRanks;
    const GStreamer::GstCapsPtr h265 = GStreamer::adoptCaps(gst_caps_new_empty_simple("video/x-h265"));
    GStreamer::forEachVideoDecoderFactory(GST_RANK_MARGINAL, [&h265](GstElementFactory* factory) {
        if (gst_element_factory_can_sink_any_caps(factory, h265.get())) {
            gst_plugin_feature_set_rank(GST_PLUGIN_FEATURE(factory), GST_RANK_NONE);
        }
    });

    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Video stream without a picture:.*H\\.265")));

    Receiver stream(loopbackUri(QLatin1StringView("udp265"), 0), /*autoReconnect=*/false, kWatchdogTimeout,
                    QString::fromLatin1(GstTest::kH265Stream));
    QVERIFY2(stream.sender.error().isEmpty(), qPrintable(stream.sender.error()));
    stream.window.show();
    stream.receiver->setVideoOutput(stream.output);
    QVERIFY(stream.startStreaming());
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    const QString message = stream.errorSpy.constFirst().constFirst().toString();
    QVERIFY2(message.contains(QLatin1String("H.265")), qPrintable(message));

    // Received all the same, and so recorded.
    const QString path = _tempDir->filePath(QStringLiteral("undecodable.mp4"));
    QVERIFY(stream.record(path));
    QCOMPARE(stream.receiver->state(), State::Streaming);
    QVERIFY(!stream.receiver->decoding());
    stream.receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    QCOMPARE(stream.errorSpy.count(), 1);
    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void GstVideoReceiverRecordingTest::_testCodecMp4CannotHoldIsRecordedAsMatroska()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("videotestsrc", "avenc_mpeg2video", "mpegtsmux", "tsdemux", "matroskamux",
                                  "matroskademux");

    Receiver stream(loopbackUri(QLatin1StringView("mpegts"), 0));
    setMaxRecordingPartDuration(stream.receiver.get(), std::chrono::seconds(1));
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForState(State::Connecting));
    // MPEG-2 video, which neither MP4 nor MOV holds, as an MPEG-TS camera of that kind sends it.
    GstTest::TestPipeline camera;
    QVERIFY2(camera.launch(QStringLiteral("videotestsrc is-live=true ! video/x-raw,width=320,height=240,framerate=30/1 "
                                          "! avenc_mpeg2video gop-size=5 ! mpegtsmux ! "
                                          "udpsink host=127.0.0.1 port=%1")
                               .arg(stream.udpPort())),
             qPrintable(camera.error()));
    QVERIFY(camera.play());
    QVERIFY(stream.waitForState(State::Streaming));

    // The recording goes into the container that holds the stream, in files named for it: the first one, and the
    // parts that follow.
    const QStringList parts{_tempDir->filePath(QStringLiteral("mpeg2.mkv")),
                            _tempDir->filePath(QStringLiteral("mpeg2_part2.mkv"))};
    const QString asked = _tempDir->filePath(QStringLiteral("mpeg2.mp4"));
    stream.receiver->startRecording(asked, FileFormat::Mp4);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, parts.size(), TestTimeout::mediumMs());
    stream.receiver->stopRecording();
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::mediumMs());
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, stream.recordingStartedSpy.count(), TestTimeout::mediumMs());
    QVERIFY(stream.errorSpy.isEmpty());

    for (qsizetype part = 0; part < parts.size(); ++part) {
        QCOMPARE(stream.recordingStartedSpy.at(part).constFirst().toString(), parts.at(part));
        QCOMPARE(stream.recordingFinishedSpy.at(part).at(0).toString(), parts.at(part));
        QCOMPARE(stream.recordingFinishedSpy.at(part).at(1).toString(), QString());
    }
    QVERIFY(!QFile::exists(asked));
    const QString problem = recordingProblem(parts.constFirst());
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void GstVideoReceiverRecordingTest::_testReconnectResumesRecordingInNextPart()
{
    Receiver stream(udpListenUri(), /*autoReconnect=*/true, kShortWatchdogTimeout);
    QVERIFY(stream.startStreaming());
    const QString path = _tempDir->filePath(QStringLiteral("resumed.mp4"));
    QVERIFY(stream.record(path));

    // The camera drops out: the first part is closed and the recording waits for the stream to come back.
    stream.sender.stop();
    QVERIFY(stream.waitForState(State::Reconnecting));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    QVERIFY(stream.receiver->recording());

    QVERIFY(stream.resumeSending());
    const QString part2 = _tempDir->filePath(QStringLiteral("resumed_part2.mp4"));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(stream.recordingStartedSpy.constLast().constFirst().toString(), part2);
    QVERIFY(stream.framesSent(kRecordFrames));
    // The recording's size counts the closed first part too.
    const qint64 part1Bytes = QFileInfo(path).size();
    QVERIFY(part1Bytes > 0);
    QVERIFY_TRUE_WAIT(stream.receiver->stats().reconnects == 1, TestTimeout::mediumMs());
    QVERIFY(stream.receiver->stats().recordingBytes >= part1Bytes);

    stream.receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), part2);
    QVERIFY(stream.errorSpy.isEmpty());
    for (const QString& file : {path, part2}) {
        const QString problem = recordingProblem(file);
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
    }
}

void GstVideoReceiverRecordingTest::_testLongRecordingContinuesInNextPart()
{
    Receiver stream;
    // Without the jitter buffer, which may drop a late packet: every frame sent is to reach a file.
    VideoReceiver::Config config = stream.receiver->config();
    config.lowLatency = true;
    stream.receiver->setConfig(config);
    setMaxRecordingPartDuration(stream.receiver.get(), std::chrono::seconds(1));
    QVERIFY(stream.startStreaming());

    const QString path = _tempDir->filePath(QStringLiteral("long.mp4"));
    const QStringList parts{path, _tempDir->filePath(QStringLiteral("long_part2.mp4")),
                            _tempDir->filePath(QStringLiteral("long_part3.mp4"))};
    QVERIFY(stream.record(path));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, parts.size(), TestTimeout::mediumMs());
    QVERIFY(stream.receiver->recording());
    stream.receiver->stopRecording();
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::mediumMs());
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, stream.recordingStartedSpy.count(), TestTimeout::mediumMs());
    QVERIFY(stream.errorSpy.isEmpty());

    for (qsizetype part = 0; part < parts.size(); ++part) {
        QCOMPARE(stream.recordingStartedSpy.at(part).constFirst().toString(), parts.at(part));
        QCOMPARE(stream.recordingFinishedSpy.at(part).at(0).toString(), parts.at(part));
        QCOMPARE(stream.recordingFinishedSpy.at(part).at(1).toString(), QString());
    }
    // A part that the next one took over from ends right before the keyframe the next one starts with: it holds whole
    // groups of pictures, and no frame between the two is lost.
    for (const QString& part : parts.first(parts.size() - 1)) {
        QString failure;
        const int frames = GstTest::countPlayableFrames(part, failure);
        QVERIFY2(frames >= kMinRecordedFrames, qPrintable(failure));
        QVERIFY2((frames % kFramesPerGop) == 0, qPrintable(QStringLiteral("%1: %2 frames").arg(part).arg(frames)));
    }
}

void GstVideoReceiverRecordingTest::_testFileSwitchRequestsOneKeyframe()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());

    std::atomic<int> requests{0};
    KeyframeHold keyframes;
    GStreamer::PadProbe requestProbe;
    GStreamer::PadProbe keyframeProbe;
    // Where the recording branch's requests leave it, and where the stream enters its recorder.
    QVERIFY(stream.probeBranchHead(requestProbe, GStreamer::PipelineBranch::Recording, "queue", "sink",
                                   GST_PAD_PROBE_TYPE_EVENT_UPSTREAM, countRecordingKeyframeRequests, &requests));
    QVERIFY(stream.probeBranchHead(keyframeProbe, GStreamer::PipelineBranch::Recording, "valve", "sink",
                                   GST_PAD_PROBE_TYPE_BUFFER, dropHeldKeyframes, &keyframes));

    QVERIFY(stream.record(_tempDir->filePath(QStringLiteral("first.mp4"))));
    QCOMPARE(requests.load(), 1);

    // No keyframe is on its way to the recorder any more once one was dropped.
    keyframes.held.store(true);
    QVERIFY_TRUE_WAIT(keyframes.dropped.load() > 0, TestTimeout::mediumMs());

    // The next file starts with a keyframe, which is asked for rather than waited for.
    const QString second = _tempDir->filePath(QStringLiteral("second.mp4"));
    stream.receiver->startRecording(second, FileFormat::Mp4);
    QVERIFY_TRUE_WAIT(requests.load() == 2, TestTimeout::mediumMs());
    QCOMPARE(stream.recordingStartedSpy.count(), 1);

    // That keyframe starts the file, which asks for no other.
    keyframes.held.store(false);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(stream.recordingStartedSpy.constLast().constFirst().toString(), second);
    QCOMPARE(requests.load(), 2);
}

void GstVideoReceiverRecordingTest::_testFramesArrivingTogetherStartNoNewPart()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());

    // The frame after the one a file starts with gets a time half a millisecond later, as two frames have that arrive
    // together without a jitter buffer. From the first frame alone, the muxer takes its header to last for seconds.
    struct Burst
    {
        GstClockTime first = GST_CLOCK_TIME_NONE;
        bool done = false;
    } burst;  // Touched only on the streaming thread

    GStreamer::PadProbe burstProbe;
    QVERIFY(stream.probeBranchHead(
        burstProbe, GStreamer::PipelineBranch::Recording, "valve", "src", GST_PAD_PROBE_TYPE_BUFFER,
        [](GstPad*, GstPadProbeInfo* info, gpointer data) -> GstPadProbeReturn {
            auto* const frames = static_cast<Burst*>(data);
            GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
            if (!buffer || frames->done) {
                return GST_PAD_PROBE_OK;
            }
            if (!GST_CLOCK_TIME_IS_VALID(frames->first)) {
                if (!GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT)) {
                    frames->first = GST_BUFFER_PTS(buffer);
                }
                return GST_PAD_PROBE_OK;
            }
            buffer = gst_buffer_make_writable(buffer);
            GST_BUFFER_PTS(buffer) = frames->first + (GST_MSECOND / 2);
            if (GST_BUFFER_DTS_IS_VALID(buffer)) {
                GST_BUFFER_DTS(buffer) = GST_BUFFER_PTS(buffer);
            }
            GST_PAD_PROBE_INFO_DATA(info) = buffer;
            frames->done = true;
            return GST_PAD_PROBE_OK;
        },
        &burst));

    // Recorded for longer than the muxer's estimate takes to be looked at: the recording stays in its file.
    const QString path = _tempDir->filePath(QStringLiteral("burst.mp4"));
    QVERIFY(stream.record(path));
    QVERIFY(stream.framesSent(2 * kFrameRate));
    stream.receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(stream.recordingStartedSpy.count(), 1);
}

void GstVideoReceiverRecordingTest::_testRecordingLeavesFileBeforeHeaderSpaceRunsOut()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());

    // The stream waits in front of the recorder until its muxer is set up.
    GStreamer::PadProbe streamHold;
    QVERIFY(stream.probeBranchHead(
        streamHold, GStreamer::PipelineBranch::Recording, "valve", "src",
        static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_BUFFER),
        [](GstPad*, GstPadProbeInfo*, gpointer) { return GST_PAD_PROBE_OK; }, nullptr));
    const QString path = _tempDir->filePath(QStringLiteral("header.mp4"));
    stream.receiver->startRecording(path, FileFormat::Mp4);

    // A file whose header has room for less than the recording wants ahead of it, as one that recorded for long at a
    // high frame rate: well under RecordingBranch::kHeaderSpaceMargin, whatever the muxer makes of the frames it gets.
    const auto reserveLittleHeaderSpace = [&stream] {
        bool reserved = false;
        (void) runOnPipeline(stream.receiver.get(), [&reserved](GstElement* pipeline) {
            if (const GStreamer::GstElementPtr muxer = GstTest::findByFactory(pipeline, "mp4mux")) {
                g_object_set(muxer.get(), "reserved-max-duration", G_GUINT64_CONSTANT(10) * GST_SECOND, nullptr);
                reserved = true;
            }
        });
        return reserved;
    };
    QVERIFY_TRUE_WAIT(reserveLittleHeaderSpace(), TestTimeout::mediumMs());
    streamHold.remove();

    // Once the muxer's estimate counts, the recording goes on in the next part, whose header has the regular room, and
    // the file is closed in good order.
    const QString part2 = _tempDir->filePath(QStringLiteral("header_part2.mp4"));
    const int estimateDelayMs =
        static_cast<int>(std::chrono::milliseconds(RecordingBranch::kHeaderEstimateDelay).count());
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, 2, estimateDelayMs + TestTimeout::mediumMs());
    QCOMPARE(stream.recordingStartedSpy.at(0).constFirst().toString(), path);
    QCOMPARE(stream.recordingStartedSpy.at(1).constFirst().toString(), part2);
    QCOMPARE(stream.recordingFinishedSpy.count(), 1);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    QVERIFY(stream.receiver->recording());

    QVERIFY(stream.framesSent(kRecordFrames));
    stream.receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(stream.recordingStartedSpy.count(), 2);
    QVERIFY(stream.errorSpy.isEmpty());
    for (const QString& file : {path, part2}) {
        const QString problem = recordingProblem(file);
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
    }
}

void GstVideoReceiverRecordingTest::_testRecordingStartsAtKeyframeWithParameterSets()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("matroskamux", "matroskademux");

    Receiver stream;
    QVERIFY(stream.startStreaming());

    // A recorder's parser can't pass on the first keyframes it is given.
    constexpr int kStrippedKeyframes = 3;
    ParameterSetStrip strip;
    strip.keyframes.store(kStrippedKeyframes);
    GStreamer::PadProbe stripProbe;
    QVERIFY(stream.probeBranchHead(stripProbe, GStreamer::PipelineBranch::Recording, "valve", "src",
                                   GST_PAD_PROBE_TYPE_BUFFER, stripParameterSets, &strip));

    const QString path = _tempDir->filePath(QStringLiteral("late-start.mkv"));
    QVERIFY(stream.record(path, FileFormat::Mkv));
    stream.receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    QCOMPARE(strip.stripped.load(), kStrippedKeyframes);

    // The file starts with the first frame it holds, not with a keyframe that never reached it.
    QString failure;
    GstClockTime firstPts = GST_CLOCK_TIME_NONE;
    const int frames = GstTest::countPlayableFrames(path, failure, "matroskademux", &firstPts);
    QVERIFY2(frames >= kMinRecordedFrames, qPrintable(failure));
    QCOMPARE(firstPts, GstClockTime{0});
}

void GstVideoReceiverRecordingTest::_testRecordingFailsWithoutKeyframeWithParameterSets()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());

    ParameterSetStrip strip;
    strip.keyframes.store(-1);
    GStreamer::PadProbe stripProbe;
    QVERIFY(stream.probeBranchHead(stripProbe, GStreamer::PipelineBranch::Recording, "valve", "src",
                                   GST_PAD_PROBE_TYPE_BUFFER, stripParameterSets, &strip));

    // Nothing such a stream sends can start a file: the recording says so rather than go on recording nothing.
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^No keyframe to start the recording with")));
    const QString path = _tempDir->filePath(QStringLiteral("no-start.mp4"));
    stream.receiver->startRecording(path, FileFormat::Mp4);
    const int startTimeoutMs = static_cast<int>(std::chrono::milliseconds(RecordingBranch::kStartTimeout).count());
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, startTimeoutMs + TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(strip.stripped.load() > 0);
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());
    QVERIFY(stream.recordingStartedSpy.isEmpty());
    QVERIFY(stream.recordingFinishedSpy.isEmpty());
    QVERIFY(!QFile::exists(path));
    QCOMPARE(stream.receiver->state(), State::Streaming);
}

void GstVideoReceiverRecordingTest::_testFormatChangeContinuesRecordingInNextPart_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<QString>("suffix");

    // matroskamux fails on other caps in a file it has begun; mp4mux would go on in the file at another resolution.
    QTest::newRow("mkv") << int(FileFormat::Mkv) << QStringLiteral("mkv");
    QTest::newRow("mp4") << int(FileFormat::Mp4) << QStringLiteral("mp4");
}

void GstVideoReceiverRecordingTest::_testFormatChangeContinuesRecordingInNextPart()
{
    QFETCH(int, format);
    QFETCH(QString, suffix);
    GSTTEST_SKIP_UNLESS_FACTORIES("matroskamux", "matroskademux");
    const auto fileFormat = static_cast<FileFormat>(format);

    Receiver stream;
    QVERIFY(stream.startStreaming());
    const QString path = _tempDir->filePath(QStringLiteral("resized.") + suffix);
    const QString part2 = _tempDir->filePath(QStringLiteral("resized_part2.") + suffix);
    QVERIFY(stream.record(path, fileFormat));

    // The camera goes on at another resolution.
    QVERIFY2(stream.sender.switchTo(QString::fromLatin1(GstTest::kH264SmallStream)), qPrintable(stream.sender.error()));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(stream.recordingStartedSpy.constLast().constFirst().toString(), part2);
    QCOMPARE(stream.recordingFinishedSpy.count(), 1);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    QVERIFY(stream.receiver->recording());

    QVERIFY(stream.framesSent(kRecordFrames));
    stream.receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    QVERIFY(stream.errorSpy.isEmpty());
    QCOMPARE(stream.receiver->state(), State::Streaming);
    QCOMPARE(stream.receiver->stats().reconnects, 0);
    for (const QString& file : {path, part2}) {
        const QString problem = recordingProblem(file);
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
    }
}

void GstVideoReceiverRecordingTest::_testSlowRecordingDoesNotStallDisplay()
{
    Receiver stream;
    // Opened before the receiver goes, so a failing test can't leave the recorder held.
    const auto gate = std::make_shared<Gate>();
    const auto releaseGate = qScopeGuard([gate] { gate->release(); });

    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());

    // A small backlog, so storage that stops taking data shows quickly.
    constexpr guint kBacklogBytes = 16 * 1024;
    std::atomic<int> displayed{0};
    GStreamer::PadProbe displayProbe;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&displayed, &displayProbe](GstElement* pipeline) {
        const GStreamer::GstElementPtr queue = branchHead(pipeline, GStreamer::PipelineBranch::Recording, "queue");
        g_object_set(queue.get(), "max-size-bytes", kBacklogBytes, nullptr);
        const GStreamer::GstElementPtr valve = branchHead(pipeline, GStreamer::PipelineBranch::Decoding, "valve");
        const GStreamer::GstPadPtr valveSrc = GStreamer::adoptPad(gst_element_get_static_pad(valve.get(), "src"));
        (void) displayProbe.add(valveSrc.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &displayed);
    }));

    const QString path = _tempDir->filePath(QStringLiteral("slow.mp4"));
    QVERIFY(stream.record(path));
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording storage too slow")));
    // Whether the drained file closes before the finalize timeout depends on when the gate opens.
    ignoreLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording not finalized within")));
    QVERIFY(stream.blockRecorder(gate));

    // The backlog fills; the recording gives up on it instead of holding up the tee.
    const auto recorderLeaks = [&stream] {
        gint leaky = 0;
        (void) runOnPipeline(stream.receiver.get(), [&leaky](GstElement* pipeline) {
            const GStreamer::GstElementPtr queue = branchHead(pipeline, GStreamer::PipelineBranch::Recording, "queue");
            g_object_get(queue.get(), "leaky", &leaky, nullptr);
        });
        return leaky != 0;
    };
    QVERIFY_TRUE_WAIT(recorderLeaks(), TestTimeout::mediumMs());

    // The display keeps getting frames while the storage stays stuck.
    const int displayedBefore = displayed.load(std::memory_order_relaxed);
    QVERIFY_TRUE_WAIT(displayed.load(std::memory_order_relaxed) >= (displayedBefore + kFrameRate),
                      TestTimeout::mediumMs());
    QCOMPARE(stream.receiver->state(), State::Streaming);
    verifyExpectedLogMessage();

    // The file had frames, so its recordingFinished() carries the error, and nothing else does.
    gate->release();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(!Receiver::lastFinished(stream.recordingFinishedSpy, 1).isEmpty());
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());
    QVERIFY(stream.errorSpy.isEmpty());
    QCOMPARE(stream.receiver->state(), State::Streaming);
}

void GstVideoReceiverRecordingTest::_testStopRecordsFramesStillQueued()
{
    Receiver stream;
    // Opened before the receiver goes, so a failing test can't leave the recorder held.
    const auto gate = std::make_shared<Gate>();
    const auto releaseGate = qScopeGuard([gate] { gate->release(); });

    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    QVERIFY(stream.record(_tempDir->filePath(QStringLiteral("stopped.mkv")), FileFormat::Mkv));

    // Frames into the recording's queue, and out of its valve into the recorder.
    std::atomic<int> queued{0};
    std::atomic<int> recorded{0};
    GStreamer::PadProbe queuedProbe;
    GStreamer::PadProbe recordedProbe;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&](GstElement* pipeline) {
        const GStreamer::GstElementPtr queue = branchHead(pipeline, GStreamer::PipelineBranch::Recording, "queue");
        const GStreamer::GstPadPtr queueSink = GStreamer::adoptPad(gst_element_get_static_pad(queue.get(), "sink"));
        (void) queuedProbe.add(queueSink.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &queued);
        const GStreamer::GstElementPtr valve = branchHead(pipeline, GStreamer::PipelineBranch::Recording, "valve");
        const GStreamer::GstPadPtr valveSrc = GStreamer::adoptPad(gst_element_get_static_pad(valve.get(), "src"));
        (void) recordedProbe.add(valveSrc.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &recorded);
    }));

    // Storage that takes nothing for a while: the frames wait in the queue.
    QVERIFY(stream.blockRecorder(gate));
    QVERIFY(gate->waitUntilReached(TestTimeout::mediumDuration()));
    QVERIFY_TRUE_WAIT(queued.load(std::memory_order_relaxed) >= (recorded.load(std::memory_order_relaxed) + kFrameRate),
                      TestTimeout::mediumMs());

    stream.receiver->stopRecording();
    // The worker got to the request.
    QVERIFY(runOnPipeline(stream.receiver.get(), [](GstElement*) {}));
    const int queuedAtStop = queued.load(std::memory_order_relaxed);
    QVERIFY(recorded.load(std::memory_order_relaxed) < queuedAtStop);
    QVERIFY(stream.recordingFinishedSpy.isEmpty());

    // The file ends behind what was queued when it was stopped, and is finalized.
    gate->release();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY2(Receiver::lastFinished(stream.recordingFinishedSpy, 1).isEmpty(),
             qPrintable(Receiver::lastFinished(stream.recordingFinishedSpy, 1)));
    QVERIFY(recorded.load(std::memory_order_relaxed) >= queuedAtStop);
}

void GstVideoReceiverRecordingTest::_testDeactivateFinishesRecording()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    const QString path = _tempDir->filePath(QStringLiteral("deactivated.mp4"));
    QVERIFY(stream.record(path));
    QVERIFY(stream.receiver->recording());

    stream.receiver->setActive(false);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());
    QVERIFY(stream.waitForState(State::Stopped));

    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    // Activating again doesn't bring the recording back.
    QVERIFY(stream.startStreaming());
    QVERIFY(!stream.receiver->recording());
    QCOMPARE(stream.recordingStartedSpy.count(), 1);
}

void GstVideoReceiverRecordingTest::_testRecordingPlayableWhileInProgress_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<QString>("suffix");

    QTest::newRow("mp4") << int(FileFormat::Mp4) << QStringLiteral("mp4");
    QTest::newRow("mov") << int(FileFormat::Mov) << QStringLiteral("mov");
}

void GstVideoReceiverRecordingTest::_testRecordingPlayableWhileInProgress()
{
    QFETCH(int, format);
    QFETCH(QString, suffix);

    Receiver stream;
    QVERIFY(stream.startStreaming());

    const QString path = _tempDir->filePath(QStringLiteral("in-progress.") + suffix);
    QVERIFY(stream.record(path, static_cast<FileFormat>(format)));

    // A copy taken while recording is what a crash or power loss would leave on disk.
    const QString snapshot = _tempDir->filePath(QStringLiteral("snapshot.") + suffix);
    QString failure;
    const bool playable = UnitTest::waitForCondition(
        [&] {
            (void) QFile::remove(snapshot);
            return QFile::copy(path, snapshot) && (GstTest::countPlayableFrames(snapshot, failure) > 0);
        },
        TestTimeout::mediumDuration(), u"mid-recording copy is playable");
    QVERIFY2(playable, qPrintable(failure));
}

void GstVideoReceiverRecordingTest::_testSecondRecordingIsNotTruncated()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());

    const QString first = _tempDir->filePath(QStringLiteral("first.mp4"));
    QVERIFY(stream.record(first));
    stream.receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());

    const QString second = _tempDir->filePath(QStringLiteral("second.mp4"));
    QVERIFY(stream.record(second));
    stream.receiver->setActive(false);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 2, TestTimeout::mediumMs());

    for (const QString& path : {first, second}) {
        const QString problem = recordingProblem(path);
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
    }
}

void GstVideoReceiverRecordingTest::_testRecordingSwitchWithoutDisplay_data()
{
    QTest::addColumn<int>("ending");

    QTest::newRow("stopped") << int(RecordingEnd::Stopped);
    QTest::newRow("recorder error") << int(RecordingEnd::RecorderError);
    QTest::newRow("deactivated") << int(RecordingEnd::Deactivated);
}

void GstVideoReceiverRecordingTest::_testRecordingSwitchWithoutDisplay()
{
    QFETCH(int, ending);

    // Without a display sink the first recorder's EOS ends the whole pipeline too. That EOS can still be queued when
    // the second recording starts, and must not be taken for the second recorder's.
    Receiver stream;
    QVERIFY(stream.startStreaming());
    const QString first = _tempDir->filePath(QStringLiteral("first.mp4"));
    const QString second = _tempDir->filePath(QStringLiteral("second.mp4"));
    QVERIFY(stream.record(first));
    // Switched without a stop: the second file starts once the first one is finalized.
    QVERIFY(stream.record(second));
    QCOMPARE(stream.recordingFinishedSpy.count(), 1);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), first);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());

    QElapsedTimer clock;
    clock.start();
    switch (static_cast<RecordingEnd>(ending)) {
        case RecordingEnd::Stopped:
            stream.receiver->stopRecording();
            break;
        case RecordingEnd::RecorderError:
            expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                             QRegularExpression(QStringLiteral("^Recording error: .*injected recorder failure")));
            QVERIFY(stream.postError("filesink", GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_WRITE,
                                     "injected recorder failure"));
            break;
        case RecordingEnd::Deactivated:
            stream.receiver->setActive(false);
            break;
    }
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 2, TestTimeout::mediumMs());
    QVERIFY2(clock.elapsed() < kFinalizeTimeoutMs,
             qPrintable(QStringLiteral("The second recording closed after %1 ms").arg(clock.elapsed())));
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), second);
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());
    // A file that opened reports its error through recordingFinished() only.
    QVERIFY(stream.errorSpy.isEmpty());

    if (static_cast<RecordingEnd>(ending) == RecordingEnd::RecorderError) {
        verifyExpectedLogMessage();
        QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QStringLiteral("injected recorder failure"));
        return;
    }
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    const QString problem = recordingProblem(second);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void GstVideoReceiverRecordingTest::_testStreamEndClosesRecordingAtOnce()
{
    Receiver stream(udpListenUri(), /*autoReconnect=*/true, kShortWatchdogTimeout);
    QVERIFY(stream.startStreaming());
    const QString path = _tempDir->filePath(QStringLiteral("ended.mp4"));
    QVERIFY(stream.record(path));

    // The stream's EOS reaches the recorder before the receiver acts on the end of the stream, as an RTSP/TCP source's
    // EOS can. Sent past the source, which is cut off so only the watchdog ends the session, a second or two later.
    QElapsedTimer clock;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&clock](GstElement* pipeline) {
        const GStreamer::GstElementPtr tee = GstTest::findByFactory(pipeline, "tee");
        const GStreamer::GstPadPtr teeSink = GStreamer::adoptPad(gst_element_get_static_pad(tee.get(), "sink"));
        const GStreamer::GstPadPtr sourcePad = GStreamer::adoptPad(gst_pad_get_peer(teeSink.get()));
        (void) gst_pad_add_probe(
            sourcePad.get(), GST_PAD_PROBE_TYPE_DATA_DOWNSTREAM,
            [](GstPad*, GstPadProbeInfo*, gpointer) { return GST_PAD_PROBE_DROP; }, nullptr, nullptr);
        clock.start();
        (void) gst_pad_send_event(teeSink.get(), gst_event_new_eos());
    }));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY2(clock.elapsed() < kFinalizeTimeoutMs,
             qPrintable(QStringLiteral("The recording closed after %1 ms").arg(clock.elapsed())));
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    // The closed file is the first part; the recording goes on in the next one once the stream is back.
    QVERIFY(stream.receiver->recording());
    QVERIFY(stream.resumeSending());
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(stream.recordingStartedSpy.constLast().constFirst().toString(),
             _tempDir->filePath(QStringLiteral("ended_part2.mp4")));
    QCOMPARE(stream.recordingFinishedSpy.count(), 1);
    QVERIFY(stream.errorSpy.isEmpty());
}

void GstVideoReceiverRecordingTest::_testRecordingErrorKeepsStreaming()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());

    // GStreamer adds follow-up errors (failed state change) whose number isn't ours to pin.
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording error: .*Could not open file")));
    ignoreLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording error:")));
    stream.receiver->startRecording(_tempDir->filePath(QStringLiteral("missing/unwritable.mp4")), FileFormat::Mp4);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());
    verifyExpectedLogMessage();
    // Never opened, so never finished.
    QVERIFY(stream.recordingFinishedSpy.isEmpty());
    QCOMPARE(stream.receiver->state(), State::Streaming);

    const QString path = _tempDir->filePath(QStringLiteral("recovered.mp4"));
    QVERIFY(stream.record(path));
    stream.receiver->setActive(false);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());

    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void GstVideoReceiverRecordingTest::_testFullStorageNamedInRecordingError_data()
{
    QTest::addColumn<int>("code");
    QTest::addColumn<int>("detailErrno");
    QTest::addColumn<bool>("namedFull");

    // filesink's two reports of a full disk: a write that fails, and the fsync after a header update that does.
    QTest::newRow("write") << static_cast<int>(GST_RESOURCE_ERROR_NO_SPACE_LEFT) << 0 << true;
    QTest::newRow("fsync") << static_cast<int>(GST_RESOURCE_ERROR_WRITE) << ENOSPC << true;
    QTest::newRow("another write error") << static_cast<int>(GST_RESOURCE_ERROR_WRITE) << EIO << false;
}

void GstVideoReceiverRecordingTest::_testFullStorageNamedInRecordingError()
{
    QFETCH(int, code);
    QFETCH(int, detailErrno);
    QFETCH(bool, namedFull);

    Receiver stream;
    QVERIFY(stream.startStreaming());
    const QString path = _tempDir->filePath(QStringLiteral("full.mp4"));
    QVERIFY(stream.record(path));

    const QString text = QStringLiteral("Error while writing to file.");
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording error:")));
    QVERIFY(stream.postError("filesink", GST_RESOURCE_ERROR, code, qUtf8Printable(text),
                             detailErrno ? g_strerror(detailErrno) : nullptr));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1),
             namedFull ? QStringLiteral("The storage is full.") : text);
}

void GstVideoReceiverRecordingTest::_testFailedSyncAtEndFailsRecording()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    const QString path = _tempDir->filePath(QStringLiteral("unsynced.mkv"));
    QVERIFY(stream.record(path, FileFormat::Mkv));

    // Storage that caches writes can be full for what a muxer wrote last, which only the sync of the finished file
    // tells.
    bool watched = false;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&watched](GstElement* pipeline) {
        const GStreamer::GstElementPtr fileSink = GstTest::findByFactory(pipeline, "filesink");
        const GStreamer::GstPadPtr fileSinkPad =
            GStreamer::adoptPad(fileSink ? gst_element_get_static_pad(fileSink.get(), "sink") : nullptr);
        watched = fileSinkPad &&
                  gst_pad_add_probe(fileSinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, failSyncAtEnd, nullptr, nullptr);
    }));
    QVERIFY(watched);

    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording error:")));
    stream.receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QStringLiteral("The storage is full."));
}

void GstVideoReceiverRecordingTest::_testFailedRecordingStartLeavesBranchUsable()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());

    // A child holding the recording sink's name makes the pipeline refuse the sink.
    GstElement* nameTaken = nullptr;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&nameTaken](GstElement* pipeline) {
        nameTaken = gst_bin_new("sinkbin");
        if (!gst_bin_add(GST_BIN(pipeline), nameTaken)) {
            nameTaken = nullptr;
        }
    }));
    QVERIFY(nameTaken);

    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to add the recording sink")));
    stream.receiver->startRecording(_tempDir->filePath(QStringLiteral("refused.mp4")), FileFormat::Mp4);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());
    verifyExpectedLogMessage();

    QVERIFY(runOnPipeline(stream.receiver.get(),
                          [nameTaken](GstElement* pipeline) { (void) gst_bin_remove(GST_BIN(pipeline), nameTaken); }));
    QVERIFY(stream.record(_tempDir->filePath(QStringLiteral("accepted.mp4"))));
}

void GstVideoReceiverRecordingTest::_testRecordingStartKeepsPipelinePlaying()
{
    Receiver stream;

    // Read on the worker that changes it; "PAUSED->PLAYING" while a change is pending.
    const auto pipelineState = [&stream]() {
        QString state;
        (void) runOnPipeline(stream.receiver.get(), [&state](GstElement* pipeline) {
            GstState current = GST_STATE_VOID_PENDING;
            GstState pending = GST_STATE_VOID_PENDING;
            (void) gst_element_get_state(pipeline, &current, &pending, 0);
            state = QString::fromUtf8(gst_element_state_get_name(current));
            if (pending != GST_STATE_VOID_PENDING) {
                state += QStringLiteral("->") + QString::fromUtf8(gst_element_state_get_name(pending));
            }
        });
        return state;
    };

    QVERIFY(stream.startStreaming());
    QCOMPARE_TRUE_WAIT(pipelineState(), QStringLiteral("PLAYING"), TestTimeout::mediumMs());

    // The file sink can't preroll before the next keyframe; the live pipeline must not wait for it meanwhile.
    stream.receiver->startRecording(_tempDir->filePath(QStringLiteral("recording.mp4")), FileFormat::Mp4);
    QCOMPARE(pipelineState(), QStringLiteral("PLAYING"));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(pipelineState(), QStringLiteral("PLAYING"));
}

void GstVideoReceiverRecordingTest::_testDestroyWhileRecordingFinalizesFile()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());

    const QString path = _tempDir->filePath(QStringLiteral("destroyed.mp4"));
    QVERIFY(stream.record(path));

    QElapsedTimer teardown;
    teardown.start();
    stream.receiver.reset();
    QVERIFY2(teardown.elapsed() < TestTimeout::mediumMs(),
             qPrintable(QStringLiteral("Destructor took %1 ms").arg(teardown.elapsed())));

    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void GstVideoReceiverRecordingTest::_testDestroyWithHungRecorderIsBounded()
{
    // The finalize timeout, then the receiver's margin for the pipeline to stop, then scheduling slack (more on CI).
    const qint64 kMaxDestructionMs =
        std::chrono::milliseconds(RecordingBranch::kFinalizeTimeout + std::chrono::seconds{2}).count() +
        (2 * TestTimeout::shortMs());

    const auto gate = std::make_shared<Gate>();
    const auto releaseGate = qScopeGuard([gate] { gate->release(); });
    GWeakRef pipelineRef;
    g_weak_ref_init(&pipelineRef, nullptr);
    const auto clearRef = qScopeGuard([&pipelineRef] { g_weak_ref_clear(&pipelineRef); });

    {
        Receiver stream;
        QVERIFY(stream.startStreaming());
        QVERIFY(stream.record(_tempDir->filePath(QStringLiteral("hung.mp4"))));
        g_weak_ref_set(&pipelineRef, stream.pipeline().get());
        // Storage that never returns, as a hung network mount.
        QVERIFY(stream.blockRecorder(gate));

        expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Recording not finalized within")));
        expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Worker still busy after")));
        QElapsedTimer teardown;
        teardown.start();
        stream.receiver.reset();
        QVERIFY2(teardown.elapsed() < kMaxDestructionMs,
                 qPrintable(QStringLiteral("Destructor took %1 ms").arg(teardown.elapsed())));
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
    }

    // The abandoned worker finishes on its own once the storage returns.
    gate->release();
    QVERIFY(UnitTest::waitForCondition(
        [&pipelineRef] {
            GstElement* const pipeline = static_cast<GstElement*>(g_weak_ref_get(&pipelineRef));
            if (pipeline) {
                gst_object_unref(pipeline);
            }
            return pipeline == nullptr;
        },
        TestTimeout::mediumDuration(), u"abandoned pipeline released"));
}

#endif

UT_REGISTER_TEST(GstVideoReceiverRecordingTest, TestLabel::Integration)
