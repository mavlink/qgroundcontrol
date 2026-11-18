#include "GstVideoReceiverPipelineTest.h"

#ifdef QGC_GST_STREAMING

#include <algorithm>
#include <atomic>
#include <gst/base/gstbasesink.h>
#include <gst/pbutils/missing-plugins.h>
#include <memory>
#include <optional>
#include <utility>

#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoSink>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

#include "Fixtures/RAIIFixtures.h"
#include "GStreamerDecoderRanks.h"
#include "GstScoped.h"
#include "HwBuffers/HwBuffers.h"
#include "MockVideoStreamServer.h"

#if defined(QGC_HAS_ANY_GPU_PATH)
#include <QtCore/QLoggingCategory>

#include "GstContextBridgeBase.h"
#include "GstHwPathRegistry.h"
#endif

namespace {

using GstReceiverTest::branchHead;
using GstReceiverTest::kFrameHeight;
using GstReceiverTest::kFrameWidth;
using GstReceiverTest::kRecordFrames;
using GstReceiverTest::kWatchdogTimeout;
using GstReceiverTest::loopbackUri;
using GstReceiverTest::Receiver;
using GstReceiverTest::State;
using GstReceiverTest::udpListenUri;

constexpr const char* kStubDecoderName = "qgcreceiverstubh264dec";

/// An H.264 "hardware" decoder that takes the stream and gives nothing back: no frame, and no message either. When
/// set to fail, it posts an error for the first frame instead, as a hardware decoder may in a VM.
struct ReceiverStubDecoder
{
    GstElement parent;
};

struct ReceiverStubDecoderClass
{
    GstElementClass parentClass;
};

GstStaticPadTemplate s_receiverStubDecoderSink =
    GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS("video/x-h264"));
GstStaticPadTemplate s_receiverStubDecoderSrc =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS("video/x-raw"));

G_DEFINE_TYPE(ReceiverStubDecoder, receiver_stub_decoder, GST_TYPE_ELEMENT)

std::atomic<bool> s_receiverStubDecoderFails = false;

GstFlowReturn receiverStubDecoderChain(GstPad* /*pad*/, GstObject* parent, GstBuffer* buffer)
{
    gst_buffer_unref(buffer);
    if (!s_receiverStubDecoderFails.load()) {
        return GST_FLOW_OK;
    }
    GST_ELEMENT_ERROR(GST_ELEMENT(parent), STREAM, DECODE, ("Simulated hardware decoder failure"), (nullptr));
    return GST_FLOW_ERROR;
}

gboolean receiverStubDecoderEvent(GstPad* pad, GstObject* parent, GstEvent* event)
{
    // Takes the stream's caps without passing them on: nothing ever leaves the decoder.
    if (GST_EVENT_TYPE(event) == GST_EVENT_CAPS) {
        gst_event_unref(event);
        return TRUE;
    }
    return gst_pad_event_default(pad, parent, event);
}

void receiver_stub_decoder_class_init(ReceiverStubDecoderClass* klass)
{
    GstElementClass* const elementClass = GST_ELEMENT_CLASS(klass);
    gst_element_class_add_static_pad_template(elementClass, &s_receiverStubDecoderSink);
    gst_element_class_add_static_pad_template(elementClass, &s_receiverStubDecoderSrc);
    gst_element_class_set_static_metadata(elementClass, "QGC receiver test decoder", "Codec/Decoder/Video/Hardware",
                                          "Decodes nothing", "QGroundControl");
}

void receiver_stub_decoder_init(ReceiverStubDecoder* self)
{
    GstPad* const sink = gst_pad_new_from_static_template(&s_receiverStubDecoderSink, "sink");
    gst_pad_set_chain_function(sink, receiverStubDecoderChain);
    gst_pad_set_event_function(sink, receiverStubDecoderEvent);
    (void) gst_element_add_pad(GST_ELEMENT(self), sink);
    (void) gst_element_add_pad(GST_ELEMENT(self), gst_pad_new_from_static_template(&s_receiverStubDecoderSrc, "src"));
}

#if defined(QGC_HAS_ANY_GPU_PATH)
Q_LOGGING_CATEGORY(LostDeviceBridgeLog, "Video.GStreamer.Test.LostDeviceBridge")

constexpr const char* kLostDeviceContextTypes[] = {"qgc.test.lost-device"};

/// A context bridge whose device was reported removed, and which gets no other.
class LostDeviceBridge final : public GstContextBridgeBase
{
public:
    LostDeviceBridge()
        : GstContextBridgeBase("Test", kLostDeviceContextTypes, &LostDeviceBridgeLog)
    {
        noteDeviceRemoved();
    }

private:
    bool primeLocked() override { return retryLater(); }

    GstObject* refObject(const char* /*contextType*/) override { return nullptr; }

    GstContext* buildContext(const char* /*contextType*/, GstObject* /*object*/) override { return nullptr; }

    void clearLocked() override {}
};
#endif

/// Has decodebin3 plug the stub decoder before any other, until destroyed. The factory stays registered, unranked:
/// the element class points at it for good. Ranked through the rank applier, so that NONE is the rank a later decoder
/// option restores.
class StubDecoderRegistration
{
public:
    explicit StubDecoderRegistration(bool fails = false)
        : _registered(
              gst_element_register(nullptr, kStubDecoderName, GST_RANK_NONE, receiver_stub_decoder_get_type()) &&
              GStreamer::changeFeatureRank(gst_registry_get(), kStubDecoderName, GST_RANK_PRIMARY + 100))
    {
        s_receiverStubDecoderFails.store(fails);
    }

    ~StubDecoderRegistration()
    {
        (void) GStreamer::changeFeatureRank(gst_registry_get(), kStubDecoderName, GST_RANK_NONE);
        s_receiverStubDecoderFails.store(false);
    }

    StubDecoderRegistration(const StubDecoderRegistration&) = delete;
    StubDecoderRegistration& operator=(const StubDecoderRegistration&) = delete;

    bool registered() const { return _registered; }

private:
    const bool _registered;
};

/// What the display queue is given once armed: a stream without keyframes, and without a mark that is not the
/// scenario's, in which the queue overran once. The overrun leaked a frame if so set, and upstream lost one a few
/// buffers later if so set.
struct DisplayQueueScenario
{
    bool leak = false;
    bool upstreamLoss = false;
    std::atomic<bool> armed = false;
    // Set once the queue has been given all of the above.
    std::atomic<bool> played = false;
    // Set by the first buffer the keyframe gate lets through.
    std::atomic<bool> gateOpen = false;
    // The longest stretch of the armed stream that the gate kept from the decoder.
    std::atomic<GstClockTime> longestGap = 0;
    // Each is used by one streaming thread only.
    int buffersSinceOverrun = -1;
    GstClockTime lastGatedPts = GST_CLOCK_TIME_NONE;
};

GstPadProbeReturn displayQueueScenarioProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    DisplayQueueScenario& scenario = **static_cast<std::shared_ptr<DisplayQueueScenario>*>(data);
    if (!scenario.armed.load()) {
        return GST_PAD_PROBE_OK;
    }
    // The tee shares its buffer with the recording branch.
    GstBuffer* const buffer = gst_buffer_make_writable(GST_PAD_PROBE_INFO_BUFFER(info));
    GST_PAD_PROBE_INFO_DATA(info) = buffer;
    GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    // Upstream marks the buffer after a packet that a loaded host lost, or had too late.
    GST_BUFFER_FLAG_UNSET(buffer, GST_BUFFER_FLAG_DISCONT);
    if (scenario.played.load()) {
        return GST_PAD_PROBE_OK;
    }

    if (scenario.buffersSinceOverrun >= 0) {
        // As upstream marks the buffer after a loss ahead of the tee.
        constexpr int kBuffersToUpstreamLoss = 3;
        if (++scenario.buffersSinceOverrun == kBuffersToUpstreamLoss) {
            GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_DISCONT);
            scenario.played.store(true);
        }
        return GST_PAD_PROBE_OK;
    }

    // The gate sees the buffer after an overrun next, as it does this one once the queue holds no other.
    const GStreamer::GstElementPtr queue = GStreamer::adoptElement(gst_pad_get_parent_element(pad));
    guint queued = 0;
    g_object_get(queue.get(), "current-level-buffers", &queued, nullptr);
    if (queued > 0) {
        return GST_PAD_PROBE_OK;
    }
    g_signal_emit_by_name(queue.get(), "overrun");
    // The mark a leak leaves on the buffer after it.
    if (scenario.leak) {
        GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_DISCONT);
    }
    scenario.buffersSinceOverrun = 0;
    scenario.played.store(!scenario.upstreamLoss);
    return GST_PAD_PROBE_OK;
}

/// Sees the buffers the keyframe gate lets through.
GstPadProbeReturn gatedStreamProbe([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    DisplayQueueScenario& scenario = **static_cast<std::shared_ptr<DisplayQueueScenario>*>(data);
    scenario.gateOpen.store(true);
    const GstClockTime pts = GST_BUFFER_PTS(GST_PAD_PROBE_INFO_BUFFER(info));
    if (!GST_CLOCK_TIME_IS_VALID(pts)) {
        return GST_PAD_PROBE_OK;
    }
    if (scenario.armed.load() && GST_CLOCK_TIME_IS_VALID(scenario.lastGatedPts) && (pts > scenario.lastGatedPts)) {
        scenario.longestGap.store(std::max(scenario.longestGap.load(), pts - scenario.lastGatedPts));
    }
    scenario.lastGatedPts = pts;
    return GST_PAD_PROBE_OK;
}

/// Has @p receiver's display queue take its input through @p scenario, which alone decides what the keyframe gate
/// sees from here on: the queue itself no longer overruns or leaks, however slowly this host decodes, and a gate
/// that a frame leaked while the decoder started had closed is open again.
bool stageDisplayQueue(GstVideoReceiver* receiver, const std::shared_ptr<DisplayQueueScenario>& scenario)
{
    const auto addProbe = [&scenario](GstPad* pad, GstPadProbeCallback probe) {
        return pad && gst_pad_add_probe(
                          pad, GST_PAD_PROBE_TYPE_BUFFER, probe, new std::shared_ptr<DisplayQueueScenario>(scenario),
                          [](gpointer data) { delete static_cast<std::shared_ptr<DisplayQueueScenario>*>(data); });
    };
    bool staged = false;
    (void) GstVideoReceiverTestBase::runOnPipeline(receiver, [&staged, &addProbe](GstElement* pipeline) {
        const GStreamer::GstElementPtr queue = branchHead(pipeline, GStreamer::PipelineBranch::Decoding, "queue");
        if (!queue) {
            return;
        }
        g_object_set(queue.get(), "max-size-buffers", 0U, nullptr);
        const GStreamer::GstPadPtr sink = GStreamer::adoptPad(gst_element_get_static_pad(queue.get(), "sink"));
        const GStreamer::GstPadPtr src = GStreamer::adoptPad(gst_element_get_static_pad(queue.get(), "src"));
        // The gate is a probe on the queue's source pad, so its peer sees what the gate let through.
        const GStreamer::GstPadPtr gated = GStreamer::adoptPad(src ? gst_pad_get_peer(src.get()) : nullptr);
        staged = addProbe(sink.get(), displayQueueScenarioProbe) && addProbe(gated.get(), gatedStreamProbe);
    });
    return staged && QTest::qWaitFor([&scenario]() { return scenario->gateOpen.load(); }, TestTimeout::mediumMs());
}

/// Buffer probe that fails the push with not-negotiated and posts nothing, as qtmux does for a caps change it refuses
/// and a decoder for an output format it can't negotiate.
GstPadProbeReturn refuseBufferSilently([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info,
                                       [[maybe_unused]] gpointer data)
{
    gst_buffer_unref(GST_PAD_PROBE_INFO_BUFFER(info));
    GST_PAD_PROBE_INFO_FLOW_RETURN(info) = GST_FLOW_NOT_NEGOTIATED;
    return GST_PAD_PROBE_HANDLED;
}

}  // namespace

void GstVideoReceiverPipelineTest::_testStreamingStatesAndDisplay()
{
    Receiver stream;
    QCOMPARE(stream.receiver->state(), State::Stopped);
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());

    // Every state on the way, each once, and streaming exactly while Streaming.
    QCOMPARE(stream.stateSpy.count(), 2);
    QCOMPARE(stream.stateSpy.at(0).constFirst().value<State>(), State::Connecting);
    QCOMPARE(stream.stateSpy.at(1).constFirst().value<State>(), State::Streaming);
    QCOMPARE(stream.streamingSpy.count(), 1);
    QVERIFY(stream.receiver->streaming());

    // The QVideoSink's size, so it is the displayed one.
    QVERIFY_SIGNAL_COUNT_WAIT(stream.sizeSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(stream.receiver->videoSize(), QSize(kFrameWidth, kFrameHeight));
    QVERIFY_TRUE_WAIT(!stream.receiver->decoderName().isEmpty(), TestTimeout::mediumMs());

    const quint16 port = stream.udpPort();
    stream.receiver->setActive(false);
    QVERIFY(stream.waitForState(State::Stopped));
    QVERIFY_TRUE_WAIT(!stream.receiver->decoding(), TestTimeout::shortMs());
    QVERIFY(!stream.receiver->streaming());
    QVERIFY(stream.errorSpy.isEmpty());

    QUdpSocket rebind;
    QVERIFY2(rebind.bind(QHostAddress::LocalHost, port, QAbstractSocket::DontShareAddress),
             "Stopped receiver still holds its UDP port");
}

void GstVideoReceiverPipelineTest::_testStaticSourcePadWaitsForData()
{
    // udp265 exposes its source pad before any data arrives, which alone isn't streaming.
    Receiver stream(loopbackUri(QLatin1StringView("udp265"), 0));
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForState(State::Connecting));
    QVERIFY(stream.streamingSpy.isEmpty());
    QVERIFY(!stream.receiver->streaming());
}

void GstVideoReceiverPipelineTest::_testDecodesMockLinkStreams_data()
{
    QTest::addColumn<int>("streamType");

    using StreamType = MockVideoStreamServer::StreamType;
    QTest::newRow("rtp h264 udp") << int(StreamType::RtpUdpH264);
    QTest::newRow("rtp h265 udp") << int(StreamType::RtpUdpH265);
    QTest::newRow("mpeg-ts udp") << int(StreamType::MpegTsUdp);
    QTest::newRow("mpeg-ts tcp") << int(StreamType::MpegTsTcp);
#ifdef QGC_GST_RTSP_SERVER
    QTest::newRow("rtsp") << int(StreamType::RtspH264);
    QTest::newRow("rtsp h265") << int(StreamType::RtspH265);
#endif
}

void GstVideoReceiverPipelineTest::_testDecodesMockLinkStreams()
{
    QFETCH(int, streamType);

    using StreamType = MockVideoStreamServer::StreamType;
    const auto type = static_cast<StreamType>(streamType);
    if (!MockVideoStreamServer::isStreamTypeAvailable(type)) {
        QSKIP("This GStreamer install lacks the stream type's encoder or muxer");
    }

    // The rows cover the stream paths, not decoder choice. CI's macOS VMs can't hardware-decode H.265, and
    // decodebin3 doesn't fall back from a hardware decoder that fails to start. Outlives the receiver.
    const GstTest::DecoderRankSnapshot decoderRanks;
    GStreamer::setCodecPriorities(VideoDecoderOption::Software);

    // Servers listen on a free port; for UDP the receiver does, and the server sends to it.
    MockVideoStreamServer server;
    const bool receiverListens =
        (type == StreamType::RtpUdpH264) || (type == StreamType::RtpUdpH265) || (type == StreamType::MpegTsUdp);
    QString uri;
    if (receiverListens) {
        const QLatin1StringView scheme = (type == StreamType::RtpUdpH264)   ? QLatin1StringView("udp")
                                         : (type == StreamType::RtpUdpH265) ? QLatin1StringView("udp265")
                                                                            : QLatin1StringView("mpegts");
        uri = loopbackUri(scheme, 0);
    } else {
        QVERIFY(server.start(type, QStringLiteral("127.0.0.1"), 0));
        uri = server.servedUri();
        // gst-rtsp-server defect, seen once with GStreamer 1.28.8 on a loaded macOS host: a client's socket is closed
        // while a main context of the server still polls it. GLib polls with select() there, which fails for a closed
        // descriptor where Linux's poll() only flags it, and GLib warns of the failure.
        ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg,
                         QRegularExpression(QStringLiteral("poll\\(2\\) failed due to: Bad file descriptor")));
    }

    Receiver stream(uri);
    stream.receiver->setActive(true);
    if (receiverListens) {
        // Connecting lasts until the server sends. A server that already does can take the receiver on to
        // Streaming before this thread sees Connecting.
        QVERIFY(stream.waitForState(State::Connecting));
        QVERIFY(server.start(type, QStringLiteral("127.0.0.1"), stream.udpPort()));
    }
    QVERIFY(stream.waitForState(State::Streaming));
    QVERIFY(stream.display());
    // Shown at the stream's 30 frames a second, not merely decoded: a sink that takes every frame for late shows one
    // a second.
    QVERIFY(UnitTest::waitForCondition(
        [&stream] {
            const VideoStreamStats stats = stream.receiver->stats();
            return stats.valid && (stats.fps > 10.0);
        },
        TestTimeout::longDuration(), u"frame rate"));
}

void GstVideoReceiverPipelineTest::_testConfigChangeRestarts()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    const GStreamer::GstElementPtr first = stream.pipeline();

    // Live: no restart.
    VideoReceiver::Config config = stream.receiver->config();
    config.autoReconnect = !config.autoReconnect;
    stream.receiver->setConfig(config);
    QCOMPARE(stream.pipeline().get(), first.get());
    QCOMPARE(stream.receiver->state(), State::Streaming);

    // Anything else applies to a new pipeline at once.
    stream.stateSpy.clear();
    config.lowLatency = !config.lowLatency;
    stream.receiver->setConfig(config);
    QVERIFY(stream.resumeSending());
    QVERIFY(stream.pipeline().get() != first.get());
    QCOMPARE(stream.stateSpy.constFirst().constFirst().value<State>(), State::Connecting);
    QVERIFY(stream.errorSpy.isEmpty());

    // An empty URI stops the stream until one is set again.
    config.uri.clear();
    stream.receiver->setConfig(config);
    QVERIFY(stream.waitForState(State::Stopped));
    config.uri = udpListenUri();
    stream.receiver->setConfig(config);
    QVERIFY(stream.resumeSending());
}

void GstVideoReceiverPipelineTest::_testOutputDetachKeepsPipeline()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    QVERIFY_TRUE_WAIT(!stream.receiver->decoderName().isEmpty(), TestTimeout::mediumMs());
    const GStreamer::GstElementPtr pipeline = stream.pipeline();
    stream.stateSpy.clear();

    stream.receiver->setVideoOutput(nullptr);
    QVERIFY_TRUE_WAIT(!stream.receiver->decoding(), TestTimeout::mediumMs());
    QVERIFY2(stream.receiver->decoderName().isEmpty(), "The decoder that was taken out is still named");

    // A destroyed output detaches too; a new one shows the stream again.
    auto* const replacement = new QQuickVideoOutput(stream.window.contentItem());
    replacement->setSize(QSizeF(kFrameWidth, kFrameHeight));
    stream.receiver->setVideoOutput(replacement);
    QVERIFY_TRUE_WAIT(stream.receiver->decoding(), TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(replacement->videoSink()->videoFrame().isValid(), TestTimeout::mediumMs());
    delete replacement;
    QCOMPARE(stream.receiver->videoOutput(), nullptr);
    QVERIFY_TRUE_WAIT(!stream.receiver->decoding(), TestTimeout::mediumMs());

    QVERIFY(stream.display());
    QVERIFY_TRUE_WAIT(!stream.receiver->decoderName().isEmpty(), TestTimeout::mediumMs());
    QCOMPARE(stream.pipeline().get(), pipeline.get());
    QVERIFY2(stream.stateSpy.isEmpty(), "Changing the output restarted the stream");
}

void GstVideoReceiverPipelineTest::_testRestartRebuildsVideoSink()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    const GStreamer::GstElementPtr firstSink =
        GStreamer::adoptElement(GST_ELEMENT(gst_object_ref(displaySink(stream.receiver.get()))));
    const GStreamer::GstElementPtr firstPipeline = stream.pipeline();

    stream.receiver->restart();
    QVERIFY(displaySink(stream.receiver.get()) != firstSink.get());
    QVERIFY(stream.resumeSending());
    QVERIFY(stream.pipeline().get() != firstPipeline.get());
    QVERIFY_TRUE_WAIT(stream.receiver->decoding(), TestTimeout::mediumMs());
    // Only the receiver's ref and this one were ever taken on the replaced sink; the pipeline let go of it.
    QCOMPARE(GST_OBJECT_REFCOUNT_VALUE(firstSink.get()), 1);
}

void GstVideoReceiverPipelineTest::_testDecoderQosDisabled()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());

    // The decoders decodebin3 plugged, inspected on the worker that owns the pipeline.
    int decoders = 0;
    QStringList qosEnabled;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&decoders, &qosEnabled](GstElement* pipeline) {
        for (const GStreamer::GstElementPtr& element : GstTest::childElements(GST_BIN(pipeline), /*recurse=*/true)) {
            GstElementFactory* const factory = gst_element_get_factory(element.get());
            if (!factory || !gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER) ||
                !g_object_class_find_property(G_OBJECT_GET_CLASS(element.get()), "qos")) {
                continue;
            }
            ++decoders;
            gboolean qos = TRUE;
            g_object_get(element.get(), "qos", &qos, nullptr);
            if (qos) {
                qosEnabled.append(QString::fromUtf8(GST_OBJECT_NAME(element.get())));
            }
        }
    }));
    QVERIFY(decoders > 0);
    QVERIFY2(qosEnabled.isEmpty(), qPrintable(qosEnabled.join(u',')));
}

void GstVideoReceiverPipelineTest::_testDecoderDecodesVideoStreamsOnly()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());

    // decodebin3 asks, for each stream of a collection, whether to decode it (1) or not (0); -1 leaves it the choice.
    gint video = -1;
    gint audio = -1;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&video, &audio](GstElement* pipeline) {
        const GStreamer::GstElementPtr decoder = GstTest::findByFactory(pipeline, "decodebin3");
        if (!decoder) {
            return;
        }
        const GStreamer::GstObjectPtr collection =
            GStreamer::adoptObject(GST_OBJECT_CAST(gst_stream_collection_new("test")));
        const auto selected = [&decoder, &collection](GstStreamType type) {
            const GStreamer::GstObjectPtr candidate =
                GStreamer::adoptObject(GST_OBJECT_CAST(gst_stream_new(nullptr, nullptr, type, GST_STREAM_FLAG_NONE)));
            gint answer = -1;
            g_signal_emit_by_name(decoder.get(), "select-stream", collection.get(), candidate.get(), &answer);
            return answer;
        };
        video = selected(GST_STREAM_TYPE_VIDEO);
        audio = selected(GST_STREAM_TYPE_AUDIO);
    }));
    QCOMPARE(video, 1);
    QCOMPARE(audio, 0);
}

void GstVideoReceiverPipelineTest::_testDecoderDelayFollowsSourceLiveness_data()
{
    QTest::addColumn<int>("streamType");
    QTest::addColumn<QString>("threadType");

    using StreamType = MockVideoStreamServer::StreamType;
    // Behind a live source libav picks slice threads by itself, so its setting is left alone.
    QTest::newRow("mpeg-ts udp is live") << int(StreamType::MpegTsUdp) << QStringLiteral("auto");
    // tcpclientsrc isn't live: left alone, libav would add frame threads, each of which holds a frame back.
    QTest::newRow("mpeg-ts tcp is not live") << int(StreamType::MpegTsTcp) << QStringLiteral("slice");
}

void GstVideoReceiverPipelineTest::_testDecoderDelayFollowsSourceLiveness()
{
    QFETCH(int, streamType);
    QFETCH(QString, threadType);

    const auto type = static_cast<MockVideoStreamServer::StreamType>(streamType);
    if (!MockVideoStreamServer::isStreamTypeAvailable(type)) {
        QSKIP("This GStreamer install lacks the stream type's encoder or muxer");
    }
    GSTTEST_SKIP_UNLESS_FACTORIES("avdec_h264");
    // libav's decoder on every host. Outlives the receiver.
    const GstTest::DecoderRankSnapshot decoderRanks;
    GStreamer::setCodecPriorities(VideoDecoderOption::Software);

    MockVideoStreamServer server;
    const bool receiverListens = (type == MockVideoStreamServer::StreamType::MpegTsUdp);
    if (!receiverListens) {
        QVERIFY(server.start(type, QStringLiteral("127.0.0.1"), 0));
    }
    Receiver stream(receiverListens ? loopbackUri(QLatin1StringView("mpegts"), 0) : server.servedUri());
    stream.receiver->setActive(true);
    if (receiverListens) {
        QVERIFY(stream.waitForState(State::Connecting));
        QVERIFY(server.start(type, QStringLiteral("127.0.0.1"), stream.udpPort()));
    }
    QVERIFY(stream.waitForState(State::Streaming));
    QVERIFY(stream.display());
    QCOMPARE(stream.receiver->decoderName(), QStringLiteral("avdec_h264"));

    QString decoderThreadType;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&decoderThreadType](GstElement* pipeline) {
        const GStreamer::GstElementPtr decoder = GstTest::findByFactory(pipeline, "avdec_h264");
        GParamSpec* const spec =
            decoder ? g_object_class_find_property(G_OBJECT_GET_CLASS(decoder.get()), "thread-type") : nullptr;
        if (!spec || !G_IS_PARAM_SPEC_FLAGS(spec)) {
            return;
        }
        guint flags = 0;
        g_object_get(decoder.get(), "thread-type", &flags, nullptr);
        if (const GFlagsValue* const value = g_flags_get_first_value(G_PARAM_SPEC_FLAGS(spec)->flags_class, flags)) {
            decoderThreadType = QString::fromLatin1(value->value_nick);
        }
    }));
    QCOMPARE(decoderThreadType, threadType);
}

void GstVideoReceiverPipelineTest::_testDisplaySinkHasLatencyOnceLinked()
{
    // No LATENCY message follows the link: the stub decoder posts none, and the sink gets no frame to post one for.
    const StubDecoderRegistration stubDecoder;
    QVERIFY(stubDecoder.registered());

    // Nor does the watchdog replace the decoder that shows nothing, which would link the sink anew.
    Receiver stream(udpListenUri(), /*autoReconnect=*/false, std::chrono::minutes{1});
    stream.window.show();
    stream.receiver->setVideoOutput(stream.output);
    QVERIFY(stream.startStreaming());
    QVERIFY_TRUE_WAIT(stream.receiver->decoderName() == QLatin1StringView(kStubDecoderName), TestTimeout::mediumMs());

    // Synced to the clock, the sink shows a frame its latency after the frame's time. With none it would take the
    // first frames, which the jitter buffer alone held for its latency, for late ones and drop them.
    const auto sinkLatencyMs = [&stream] {
        guint64 latencyMs = 0;
        (void) runOnPipeline(stream.receiver.get(), [&latencyMs](GstElement* pipeline) {
            if (const GStreamer::GstElementPtr sink = GstTest::findByFactory(pipeline, "qgcqvideosink")) {
                latencyMs = GST_TIME_AS_MSECONDS(gst_base_sink_get_latency(GST_BASE_SINK(sink.get())));
            }
        });
        return latencyMs;
    };
    QVERIFY_TRUE_WAIT(sinkLatencyMs() >= static_cast<guint64>(stream.receiver->config().rtpJitterLatencyMs),
                      TestTimeout::mediumMs());
}

void GstVideoReceiverPipelineTest::_testFailingHardwareDecoderGivesWayToTheNext()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("avdec_h264");
    // Without the host's hardware decoders, so the decoder ranked after the failing one is the same everywhere.
    const GstTest::DecoderRankSnapshot decoderRanks;
    GStreamer::setCodecPriorities(VideoDecoderOption::Software);
    const StubDecoderRegistration failingDecoder(/*fails=*/true);
    QVERIFY(failingDecoder.registered());

    // The failure as GStreamer logs it and as the session reports it: for the first decoder and for its replacement.
    ignoreLogMessage("Video.GStreamer.GStreamerAPI", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Simulated hardware decoder failure")));
    ignoreLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Decoder error from .*Simulated hardware decoder failure")));
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^The hardware decoder \"%1\" brought no frame after a restart")
                                            .arg(QLatin1StringView(kStubDecoderName))));

    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    verifyExpectedLogMessage();
    QCOMPARE(stream.receiver->decoderName(), QStringLiteral("avdec_h264"));
    QCOMPARE(stream.receiver->state(), State::Streaming);
    QVERIFY(stream.errorSpy.isEmpty());
}

void GstVideoReceiverPipelineTest::_testFailedDecoderLeavesAutoplugOnlyForAnother_data()
{
    QTest::addColumn<QString>("caps");
    QTest::addColumn<QByteArray>("userRanks");
    QTest::addColumn<bool>("demoted");

    const QString h264 = QStringLiteral("video/x-h264, stream-format=(string)byte-stream, alignment=(string)au");
    QTest::newRow("another decoder takes the stream") << h264 << QByteArray() << true;
    // Only the stub's template is wide enough for it. Without its decoder the stream would not decode at all.
    QTest::newRow("no other decoder takes the stream")
        << QStringLiteral("video/x-h264, stream-format=(string)qgc-test") << QByteArray() << false;
    QTest::newRow("ranked by the user") << h264 << QByteArray(kStubDecoderName) + ":MAX" << false;
}

void GstVideoReceiverPipelineTest::_testFailedDecoderLeavesAutoplugOnlyForAnother()
{
    QFETCH(QString, caps);
    QFETCH(QByteArray, userRanks);
    QFETCH(bool, demoted);

    GSTTEST_SKIP_UNLESS_FACTORIES("avdec_h264");
    const StubDecoderRegistration stubDecoder;
    QVERIFY(stubDecoder.registered());
    const TestFixtures::EnvVarFixture userRankEnv("GST_PLUGIN_FEATURE_RANK");
    qputenv("GST_PLUGIN_FEATURE_RANK", userRanks);

    const GStreamer::GstCapsPtr streamCaps = GStreamer::adoptCaps(gst_caps_from_string(caps.toLatin1().constData()));
    QVERIFY(streamCaps);
    QCOMPARE(GStreamer::demoteFailedDecoder(kStubDecoderName, streamCaps.get()), demoted);

    const GStreamer::GstFeaturePtr stub =
        GStreamer::adoptFeature(gst_registry_lookup_feature(gst_registry_get(), kStubDecoderName));
    QVERIFY(stub);
    QCOMPARE(gst_plugin_feature_get_rank(stub.get()), guint(demoted ? GST_RANK_NONE : GST_RANK_PRIMARY + 100));
}

void GstVideoReceiverPipelineTest::_testLateGraphicsDeviceReplacesHardwareDecoder_data()
{
    QTest::addColumn<bool>("hardware");

    QTest::newRow("hardware decoder") << true;
    // It holds no graphics device: replacing it would only have the stream wait for a keyframe.
    QTest::newRow("software decoder") << false;
}

void GstVideoReceiverPipelineTest::_testLateGraphicsDeviceReplacesHardwareDecoder()
{
    QFETCH(bool, hardware);

    GSTTEST_SKIP_UNLESS_FACTORIES("avdec_h264");
    // Without the host's hardware decoders, so the software row plugs the same decoder everywhere.
    const GstTest::DecoderRankSnapshot decoderRanks;
    GStreamer::setCodecPriorities(VideoDecoderOption::Software);
    std::optional<StubDecoderRegistration> stubDecoder;
    if (hardware) {
        stubDecoder.emplace();
        QVERIFY(stubDecoder->registered());
    }
    const QByteArray decoderName = hardware ? QByteArray(kStubDecoderName) : QByteArrayLiteral("avdec_h264");

    // The stub decoder shows nothing: the watchdog must not be what replaces it.
    Receiver stream(udpListenUri(), /*autoReconnect=*/false, std::chrono::minutes{1});
    stream.window.show();
    stream.receiver->setVideoOutput(stream.output);
    QVERIFY(stream.startStreaming());
    QVERIFY_TRUE_WAIT(stream.receiver->decoderName() == QLatin1StringView(decoderName), TestTimeout::mediumMs());
    if (!hardware) {
        QVERIFY_TRUE_WAIT(stream.receiver->decoding(), TestTimeout::mediumMs());
    }

    const auto pluggedDecoder = [&stream, &decoderName] {
        GStreamer::GstElementPtr decoder;
        (void) runOnPipeline(stream.receiver.get(), [&decoder, &decoderName](GstElement* pipeline) {
            decoder = pipeline ? GstTest::findByFactory(pipeline, decoderName.constData()) : nullptr;
        });
        return decoder;
    };
    // Held, so that the next decoder can't get its address.
    const GStreamer::GstElementPtr first = pluggedDecoder();
    QVERIFY(first);

    // What a context bridge posts on a pipeline it gives Qt's device after the pipeline's elements had asked for it:
    // a hardware decoder plugged by then made a device of its own, and keeps it.
    QVERIFY(runOnPipeline(stream.receiver.get(), [](GstElement* pipeline) {
        (void) gst_element_post_message(
            pipeline, gst_message_new_element(GST_OBJECT(pipeline),
                                              gst_structure_new_empty(HwBuffers::kGraphicsDeviceServedMessage)));
    }));

    if (hardware) {
        QVERIFY_TRUE_WAIT(pluggedDecoder() && (pluggedDecoder() != first), TestTimeout::mediumMs());
    } else {
        // The worker has taken the message off the bus long before the decoder has had this many more frames.
        QVERIFY(stream.framesSent(GstReceiverTest::kFrameRate / 2));
        QCOMPARE(pluggedDecoder().get(), first.get());
        QVERIFY(stream.receiver->decoding());
    }
}

void GstVideoReceiverPipelineTest::_testLostGraphicsDeviceRestartsTheStream()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    // Held, so that what replaces them can't get their addresses.
    const GStreamer::GstElementPtr pipeline = stream.pipeline();
    const GStreamer::GstElementPtr sink =
        GStreamer::adoptElement(GST_ELEMENT(gst_object_ref(displaySink(stream.receiver.get()))));

    // What HwBuffers::onGraphicsDeviceRemoved() posts on a pipeline that was given the device: its GPU elements and
    // its video sink hold a device that is gone.
    QVERIFY(runOnPipeline(stream.receiver.get(), [](GstElement* running) {
        (void) gst_element_post_message(
            running, gst_message_new_element(GST_OBJECT(running),
                                             gst_structure_new_empty(HwBuffers::kGraphicsDeviceLostMessage)));
    }));

    // Not a failure: the stream is back at once, on a new pipeline and a new video sink.
    QVERIFY(stream.resumeSending());
    QVERIFY_TRUE_WAIT(stream.receiver->decoding(), TestTimeout::mediumMs());
    QVERIFY(stream.pipeline().get() != pipeline.get());
    QVERIFY(displaySink(stream.receiver.get()) != sink.get());
    QVERIFY(stream.errorSpy.isEmpty());
}

void GstVideoReceiverPipelineTest::_testDecoderFailingOnALostGraphicsDeviceKeepsItsRank()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("No GPU path compiled in this build");
#else
    GSTTEST_SKIP_UNLESS_FACTORIES("avdec_h264");
    const GstTest::DecoderRankSnapshot decoderRanks;
    GStreamer::setCodecPriorities(VideoDecoderOption::Software);
    const StubDecoderRegistration failingDecoder(/*fails=*/true);
    QVERIFY(failingDecoder.registered());

    static LostDeviceBridge lostDeviceBridge;
    static const HwPathDescriptor kLostDevicePath = {
        .path = HwVideoBufferPath::GlMemory,
        .matches = [](GstMemory*) { return false; },
        .bridge = []() -> GstContextBridgeBase& { return lostDeviceBridge; },
    };
    const GstHwPathRegistry::ScopedPathsForTest paths({&kLostDevicePath});
    QVERIFY(HwBuffers::graphicsDeviceLost());

    ignoreLogMessage("Video.GStreamer.GStreamerAPI", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Simulated hardware decoder failure")));
    ignoreLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Decoder error from .*Simulated hardware decoder failure")));
    // No decoder works on a device that is gone, so none is taken out of autoplug for failing on it: the stream
    // gives up with the same decoder it began with, where it would otherwise go on with the next one.
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Video stream stopped:.*The video decoder keeps failing")));

    Receiver stream;
    stream.window.show();
    stream.receiver->setVideoOutput(stream.output);
    QVERIFY(stream.startStreaming());
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    const GStreamer::GstFeaturePtr stub =
        GStreamer::adoptFeature(gst_registry_lookup_feature(gst_registry_get(), kStubDecoderName));
    QVERIFY(stub);
    QCOMPARE(gst_plugin_feature_get_rank(stub.get()), guint(GST_RANK_PRIMARY + 100));
#endif
}

void GstVideoReceiverPipelineTest::_testOverrunWithoutLeakLeavesKeyframeGateOpen()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    const auto scenario = std::make_shared<DisplayQueueScenario>();
    scenario->upstreamLoss = true;
    QVERIFY(stageDisplayQueue(stream.receiver.get(), scenario));
    QSignalSpy frameSpy(stream.videoSink(), &QVideoSink::videoFrameChanged);

    // The queue was full for a moment, but the decoder took a frame before the queue looked again, so nothing leaked
    // and the next buffer carries no DISCONT. A gate that closed for the overrun alone, or that took it for the leak
    // behind the DISCONT upstream sets on a later buffer, would keep its two seconds of this stream from the decoder,
    // as no keyframe opens it sooner.
    scenario->armed.store(true);
    QVERIFY_TRUE_WAIT(scenario->played.load(), TestTimeout::mediumMs());
    frameSpy.clear();
    QVERIFY_SIGNAL_COUNT_WAIT(frameSpy, GstReceiverTest::kFrameRate, TestTimeout::mediumMs());
    QCOMPARE_LT(scenario->longestGap.load(), GstClockTime{GST_SECOND});
}

void GstVideoReceiverPipelineTest::_testKeyframeGateOpensWithoutKeyframe()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    QVERIFY_TRUE_WAIT(stream.receiver->stats().valid, TestTimeout::mediumMs());
    const qint64 droppedBefore = stream.receiver->stats().framesDropped;
    const auto scenario = std::make_shared<DisplayQueueScenario>();
    scenario->leak = true;
    QVERIFY(stageDisplayQueue(stream.receiver.get(), scenario));
    QSignalSpy frameSpy(stream.videoSink(), &QVideoSink::videoFrameChanged);

    // The gate closes for the leaked frame and asks for a keyframe, which this stream never sends: UDP carries no
    // request to the sender, and every frame from here on is a delta frame.
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^No keyframe within \\d+ ms of the request")));
    scenario->armed.store(true);
    QVERIFY_TRUE_WAIT(stream.receiver->stats().framesDropped >= (droppedBefore + GstReceiverTest::kFrameRate),
                      TestTimeout::mediumMs());

    // Rather than hold the picture for good, the gate opens again after its two seconds, and says why.
    frameSpy.clear();
    QVERIFY_SIGNAL_COUNT_WAIT(frameSpy, GstReceiverTest::kFrameRate, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
}

void GstVideoReceiverPipelineTest::_testDecoderInsertionFailureStopsStream_data()
{
    QTest::addColumn<bool>("sinkTaken");
    QTest::addColumn<QString>("warning");

    QTest::newRow("video sink held by another bin") << true << QStringLiteral("^Failed to add the video sink");
    QTest::newRow("display valve already linked") << false << QStringLiteral("^Unable to insert the decoder");
}

void GstVideoReceiverPipelineTest::_testDecoderInsertionFailureStopsStream()
{
    QFETCH(bool, sinkTaken);
    QFETCH(QString, warning);

    // Before the receiver, which they outlive: what keeps its decoder out of the pipeline.
    const GStreamer::GstElementPtr otherBin = GStreamer::adoptFloating(gst_bin_new(nullptr));
    const GStreamer::GstPadPtr otherPad = GStreamer::adoptFloating(gst_pad_new("other", GST_PAD_SINK));

    Receiver stream;
    stream.window.show();
    stream.receiver->setVideoOutput(stream.output);
    if (sinkTaken) {
        QVERIFY(gst_bin_add(GST_BIN(otherBin.get()), displaySink(stream.receiver.get())));
    }
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForState(State::Connecting));
    if (!sinkTaken) {
        bool linked = false;
        QVERIFY(runOnPipeline(stream.receiver.get(), [&linked, &otherPad](GstElement* pipeline) {
            // The source pad of the display branch's valve is where the decoder goes.
            const GStreamer::GstElementPtr valve = branchHead(pipeline, GStreamer::PipelineBranch::Decoding, "valve");
            const GStreamer::GstPadPtr src =
                GStreamer::adoptPad(valve ? gst_element_get_static_pad(valve.get(), "src") : nullptr);
            linked = src && (gst_pad_link(src.get(), otherPad.get()) == GST_PAD_LINK_OK);
        }));
        QVERIFY(linked);
    }
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg, QRegularExpression(warning));
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Video stream stopped:")));

    // A stream that can't be shown fails like one whose decoder keeps failing, instead of running without a picture.
    QVERIFY(stream.sender.start(stream.udpPort()));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(stream.waitForState(State::Stopped));
}

void GstVideoReceiverPipelineTest::_testScreenshotSavesDisplayedFrame()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    const QString path = tempDir.filePath(QStringLiteral("screenshot.png"));

    expectLogMessage("Video.VideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^No video output to take a screenshot from")));
    QVERIFY(!stream.receiver->takeScreenshot(path));
    verifyExpectedLogMessage();

    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    QVERIFY_TRUE_WAIT(stream.videoSink()->videoFrame().isValid(), TestTimeout::mediumMs());

    // Qt Multimedia converts on the CPU when the platform (offscreen here) has no RHI, and says so.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    QVERIFY(stream.receiver->takeScreenshot(path));
    // Written off the GUI thread.
    QTRY_COMPARE_WITH_TIMEOUT(QImage(path).size(), QSize(kFrameWidth, kFrameHeight), TestTimeout::mediumMs());
}

void GstVideoReceiverPipelineTest::_testDeactivateReleasesDisplayedFrame()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    QVERIFY_TRUE_WAIT(stream.videoSink()->videoFrame().isValid(), TestTimeout::mediumMs());

    stream.receiver->setActive(false);
    QVERIFY_TRUE_WAIT(!stream.videoSink()->videoFrame().isValid(), TestTimeout::mediumMs());

    // Releasing the frame keeps the output bound: the next stream shows again.
    QVERIFY(stream.startStreaming());
    QVERIFY_TRUE_WAIT(stream.videoSink()->videoFrame().isValid(), TestTimeout::mediumMs());
}

void GstVideoReceiverPipelineTest::_testDecodesH265Stream()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("avdec_h265");
    // The stream path, not decoder choice, is under test; a hardware decoder that fails to start isn't replaced.
    const GstTest::DecoderRankSnapshot decoderRanks;
    GStreamer::setCodecPriorities(VideoDecoderOption::Software);

    Receiver stream(loopbackUri(QLatin1StringView("udp265"), 0), /*autoReconnect=*/false, kWatchdogTimeout,
                    QString::fromLatin1(GstTest::kH265Stream));
    QVERIFY2(stream.sender.error().isEmpty(), qPrintable(stream.sender.error()));
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    QVERIFY_TRUE_WAIT(stream.receiver->videoSize() == QSize(kFrameWidth, kFrameHeight), TestTimeout::mediumMs());
    QVERIFY(stream.errorSpy.isEmpty());
}

void GstVideoReceiverPipelineTest::_testResolutionChangeUpdatesVideoSize()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    QVERIFY_TRUE_WAIT(stream.receiver->videoSize() == QSize(kFrameWidth, kFrameHeight), TestTimeout::mediumMs());
    const qsizetype stateChanges = stream.stateSpy.count();
    const qsizetype sizeChanges = stream.sizeSpy.count();

    // A camera switching resolution mid-stream: its next keyframe carries the new parameter sets.
    QVERIFY2(stream.sender.switchTo(QString::fromLatin1(GstTest::kH264SmallStream)), qPrintable(stream.sender.error()));
    QVERIFY_TRUE_WAIT(stream.receiver->videoSize() == QSize(160, 120), TestTimeout::mediumMs());
    QVERIFY(stream.sizeSpy.count() > sizeChanges);

    // Taken in stride: no reconnect, no error.
    QCOMPARE(stream.stateSpy.count(), stateChanges);
    QCOMPARE(stream.receiver->state(), State::Streaming);
    QVERIFY(stream.errorSpy.isEmpty());
}

void GstVideoReceiverPipelineTest::_testMissingDecoderIsNamed()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());

    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Missing GStreamer plugin: .*H\\.265")));
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Decoder error from ")));
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Video stream stopped:")));
    // What decodebin3 posts for a stream no installed decoder takes: which one is missing, then that one is.
    QVERIFY(runOnPipeline(stream.receiver.get(), [](GstElement* pipeline) {
        const GStreamer::GstElementPtr decoder = GstTest::findByFactory(pipeline, "decodebin3");
        const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_caps_new_empty_simple("video/x-h265"));
        (void) gst_element_post_message(decoder.get(), gst_missing_decoder_message_new(decoder.get(), caps.get()));
        GError* const error = g_error_new_literal(GST_CORE_ERROR, GST_CORE_ERROR_MISSING_PLUGIN,
                                                  "Your GStreamer installation is missing a plug-in.");
        (void) gst_element_post_message(decoder.get(),
                                        gst_message_new_error(GST_OBJECT(decoder.get()), error, nullptr));
        g_error_free(error);
    }));

    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    const QString message = stream.errorSpy.constFirst().constFirst().toString();
    QVERIFY2(message.contains(QLatin1String("H.265")), qPrintable(message));
    QVERIFY(stream.waitForState(State::Stopped));
}

void GstVideoReceiverPipelineTest::_testSilentFlowErrorStaysInItsBranch_data()
{
    QTest::addColumn<bool>("recorder");

    QTest::newRow("decoder") << false;
    QTest::newRow("recorder") << true;
}

void GstVideoReceiverPipelineTest::_testSilentFlowErrorStaysInItsBranch()
{
    QFETCH(bool, recorder);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    if (recorder) {
        QVERIFY(stream.record(tempDir.filePath(QStringLiteral("flow-error.mp4"))));
    }
    const GStreamer::GstElementPtr pipeline = stream.pipeline();
    const qsizetype decodingChanges = stream.decodingSpy.count();
    stream.stateSpy.clear();

    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(recorder ? QStringLiteral("^Recording error: .*Internal data stream error")
                                                 : QStringLiteral("^Decoder error from .*Internal data stream error")));
    bool refusing = false;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&refusing, recorder](GstElement* streamPipeline) {
        // Behind the sink pad the branch's valve pushes into: the first pad inside the part the branch can replace.
        const GStreamer::GstElementPtr part =
            recorder ? GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(streamPipeline), "sinkbin"))
                     : GstTest::findByFactory(streamPipeline, "decodebin3");
        const GStreamer::GstPadPtr entrance =
            GStreamer::adoptPad(part ? gst_element_get_static_pad(part.get(), "sink") : nullptr);
        const GStreamer::GstPadPtr inside =
            GStreamer::adoptPad(entrance ? gst_ghost_pad_get_target(GST_GHOST_PAD(entrance.get())) : nullptr);
        refusing = inside && (gst_pad_add_probe(inside.get(), GST_PAD_PROBE_TYPE_BUFFER, refuseBufferSilently, nullptr,
                                                nullptr) != 0);
    }));
    QVERIFY(refusing);

    if (recorder) {
        QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
        QVERIFY(!Receiver::lastFinished(stream.recordingFinishedSpy, 1).isEmpty());
    } else {
        // A new decoder takes over.
        QVERIFY_SIGNAL_COUNT_WAIT(stream.decodingSpy, decodingChanges + 2, TestTimeout::mediumMs());
        QVERIFY(stream.receiver->decoding());
    }
    verifyExpectedLogMessage();

    // The tee never saw the failure: the stream runs on in the same pipeline.
    QVERIFY(stream.framesSent(kRecordFrames));
    QCOMPARE(stream.pipeline().get(), pipeline.get());
    QVERIFY2(stream.stateSpy.isEmpty(), "A branch's flow error restarted the stream");
    QVERIFY(stream.errorSpy.isEmpty());
}

#endif

UT_REGISTER_TEST(GstVideoReceiverPipelineTest, TestLabel::Integration)
