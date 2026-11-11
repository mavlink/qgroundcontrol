#include "GstVideoReceiverPipelineTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <gst/base/gstbasesink.h>

#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoSink>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

#include "Fixtures/RAIIFixtures.h"
#include "GStreamerDecoderRanks.h"
#include "GstScoped.h"
#include "MockVideoStreamServer.h"

namespace {

using GstReceiverTest::kFrameHeight;
using GstReceiverTest::kFrameWidth;
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
#endif
}

void GstVideoReceiverPipelineTest::_testDecodesMockLinkStreams()
{
#ifndef QGC_HAS_MOCK_VIDEO_STREAM_SERVER
    QSKIP("MockVideoStreamServer is only built into single-config Debug builds");
#else
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
#endif
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
    const GStreamer::GstElementPtr otherBin =
        GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(gst_bin_new(nullptr))));
    const GStreamer::GstPadPtr otherPad =
        GStreamer::adoptPad(GST_PAD(gst_object_ref_sink(gst_pad_new("other", GST_PAD_SINK))));

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
            // The display branch's only unlinked source pad is its valve's, where the decoder goes.
            for (const GStreamer::GstElementPtr& element : GstTest::childElements(GST_BIN(pipeline))) {
                const GStreamer::GstPadPtr src = GStreamer::adoptPad(gst_element_get_static_pad(element.get(), "src"));
                if (src && !gst_pad_is_linked(src.get()) &&
                    (GStreamer::branchOf(GST_OBJECT(element.get())) == GStreamer::PipelineBranch::Decoding)) {
                    linked = (gst_pad_link(src.get(), otherPad.get()) == GST_PAD_LINK_OK);
                }
            }
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

#endif

UT_REGISTER_TEST(GstVideoReceiverPipelineTest, TestLabel::Integration)
