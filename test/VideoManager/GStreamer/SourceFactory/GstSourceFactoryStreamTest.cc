#include "GstSourceFactoryStreamTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <gst/gst.h>

#include <QtCore/QByteArray>
#include <QtNetwork/QUdpSocket>

#include "GStreamerTestHelpers.h"
#include "GstScoped.h"
#include "GstSourceFactory.h"
#include "PipelineStatsProbe.h"
#include "VideoStreamStats.h"

namespace {

/// RTP packets into rtph264depay and the units that reach h264parse after it.
struct DepayloaderOutput
{
    std::atomic<int> packetsIn{0};
    std::atomic<int> unitsOut{0};
    std::atomic<bool> firstUnitHasSps{false};
};

GstPadProbeReturn recordDepayloaderUnit([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    auto* output = static_cast<DepayloaderOutput*>(data);
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    GstMapInfo map;
    if ((output->unitsOut.fetch_add(1) == 0) && buffer && gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        const QByteArray unit = QByteArray::fromRawData(reinterpret_cast<const char*>(map.data), qsizetype(map.size));
        output->firstUnitHasSps = GstTest::nalTypes(unit, /*h265=*/false).contains(7);
        gst_buffer_unmap(buffer, &map);
    }
    return GST_PAD_PROBE_OK;
}

/// Counts the RTP packets into @p source's rtph264depay and records the units that reach its h264parse: at the
/// parser's sink rather than the depayloader's src, where the factory's own probe holds units back.
bool watchH264Depayloading(GstElement* source, DepayloaderOutput* output)
{
    const GStreamer::GstElementPtr depayloader = GstTest::findByFactory(source, "rtph264depay");
    const GStreamer::GstElementPtr parser = GstTest::findByFactory(source, "h264parse");
    const GStreamer::GstPadPtr depayloaderSink =
        GStreamer::adoptPad(depayloader ? gst_element_get_static_pad(depayloader.get(), "sink") : nullptr);
    const GStreamer::GstPadPtr parserSink =
        GStreamer::adoptPad(parser ? gst_element_get_static_pad(parser.get(), "sink") : nullptr);
    return depayloaderSink && parserSink &&
           gst_pad_add_probe(depayloaderSink.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers,
                             &output->packetsIn, nullptr) &&
           gst_pad_add_probe(parserSink.get(), GST_PAD_PROBE_TYPE_BUFFER, recordDepayloaderUnit, output, nullptr);
}

/// An RTP packet (payload type 96) carrying a slice of an H.264 picture that isn't a keyframe.
QByteArray makeH264RtpPacket(quint16 sequence, quint32 timestamp)
{
    QByteArray packet(16, '\0');
    auto* const bytes = reinterpret_cast<guint8*>(packet.data());
    bytes[0] = 0x80;
    bytes[1] = 96;
    GST_WRITE_UINT16_BE(bytes + 2, sequence);
    GST_WRITE_UINT32_BE(bytes + 4, timestamp);
    GST_WRITE_UINT32_BE(bytes + 8, 0x1234ABCD);
    bytes[12] = 0x41;
    return packet;
}

}  // namespace

void GstSourceFactoryStreamTest::_testSourceFactoryRtspSetsUpOnlyVideoStream()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("rtspsrc");

    // Audio listed first, as on many IP cameras; rtspsrc sets streams up in SDP order.
    GstTest::FakeRtspServer server(
        QByteArrayLiteral("v=0\r\n"
                          "o=- 0 0 IN IP4 127.0.0.1\r\n"
                          "s=QGC select-stream test\r\n"
                          "c=IN IP4 127.0.0.1\r\n"
                          "t=0 0\r\n"
                          "m=audio 0 RTP/AVP 97\r\n"
                          "a=rtpmap:97 OPUS/48000/2\r\n"
                          "a=control:trackID=0\r\n"
                          "m=video 0 RTP/AVP 96\r\n"
                          "a=rtpmap:96 H264/90000\r\n"
                          "a=control:trackID=1\r\n"));
    QVERIFY(server.listen());

    GstTest::TestPipeline pipeline(gst_pipeline_new("rtsp-select-stream-test"));
    QVERIFY(pipeline.get());

    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr bin = GstTest::createSource(server.uri(), config);
    QVERIFY(bin);
    QVERIFY(gst_bin_add(pipeline.bin(), bin.get()));
    QVERIFY(gst_element_set_state(pipeline.get(), GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE);

    QVERIFY_TRUE_WAIT(!server.setupUrls().isEmpty(), TestTimeout::mediumMs());
    QVERIFY2(server.setupUrls().first().endsWith("/trackID=1"),
             qPrintable(QStringLiteral("first SETUP must target the video track, got %1")
                            .arg(QString::fromUtf8(server.setupUrls().first()))));
}

void GstSourceFactoryStreamTest::_testSourceFactoryDropsRtpUntilParameterSets()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("appsrc", "appsink");

    DepayloaderOutput output;
    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr source = GstTest::createSource(QStringLiteral("udp://127.0.0.1:0"), config);
    QVERIFY(source);
    QVERIFY(watchH264Depayloading(source.get(), &output));

    GstTest::TestPipeline receiver(gst_pipeline_new("join-mid-gop"));
    GstElement* const sink = gst_element_factory_make("fakesink", nullptr);
    QVERIFY(sink);
    g_object_set(sink, "sync", FALSE, "async", FALSE, nullptr);
    QVERIFY(gst_bin_add(receiver.bin(), source.get()));
    QVERIFY(gst_bin_add(receiver.bin(), sink));
    QVERIFY(gst_element_link(source.get(), sink));
    QVERIFY(receiver.play());
    gint port = 0;
    g_object_get(GstTest::findByFactory(source.get(), "udpsrc").get(), "port", &port, nullptr);
    QVERIFY(port > 0);

    // The receiver joins a stream whose keyframes, and the parameter sets sent with them, it missed.
    GstTest::FixtureRtpSender sender;
    QVERIFY2(sender.error().isEmpty(), qPrintable(sender.error()));
    sender.holdKeyframes(true);
    QVERIFY(sender.start(static_cast<quint16>(port)));

    QVERIFY_TRUE_WAIT(output.packetsIn.load() >= 20, TestTimeout::mediumMs());
    QCOMPARE(output.unitsOut.load(), 0);

    sender.holdKeyframes(false);
    QVERIFY_TRUE_WAIT(output.unitsOut.load() > 0, TestTimeout::mediumMs());
    QVERIFY2(output.firstUnitHasSps.load(), "the first unit reaching h264parse must carry the SPS");
}

void GstSourceFactoryStreamTest::_testStatsCountPacketsDroppedOnLatency()
{
    // The RTP timestamps of the packets are a second apart, so the third one is the jitter buffer's latency after the
    // first.
    constexpr quint32 kPacketSpacing = 90000;

    GStreamer::SourceFactory::Config config;
    config.latencyMs = 2000;
    const GStreamer::GstElementPtr source = GstTest::createSource(QStringLiteral("udp://127.0.0.1:0"), config);
    QVERIFY(source);
    const GStreamer::GstElementPtr jitterBuffer = GstTest::findByFactory(source.get(), "rtpjitterbuffer");
    QVERIFY(jitterBuffer);

    GstTest::TestPipeline receiver(gst_pipeline_new("drop-on-latency"));
    GstElement* const sink = gst_element_factory_make("fakesink", nullptr);
    QVERIFY(sink);
    g_object_set(sink, "sync", FALSE, "async", FALSE, nullptr);
    QVERIFY(gst_bin_add(receiver.bin(), source.get()));
    QVERIFY(gst_bin_add(receiver.bin(), sink));
    QVERIFY(gst_element_link(source.get(), sink));
    QVERIFY(receiver.play());
    gint port = 0;
    g_object_get(GstTest::findByFactory(source.get(), "udpsrc").get(), "port", &port, nullptr);
    QVERIFY(port > 0);

    QUdpSocket sender;
    int sent = 0;
    const auto sendPacket = [&sender, &sent, port]() {
        const QByteArray packet =
            makeH264RtpPacket(static_cast<quint16>(100 + sent), static_cast<quint32>(sent) * kPacketSpacing);
        ++sent;
        return sender.writeDatagram(packet, QHostAddress::LocalHost, static_cast<quint16>(port)) == packet.size();
    };

    // As PipelineSession does with its bus.
    const GStreamer::GstBusPtr bus = GStreamer::adoptBus(gst_element_get_bus(receiver.get()));
    PipelineStatsProbe probe;
    const auto packetsLost = [&]() {
        while (const GStreamer::GstMessagePtr message =
                   GStreamer::adoptMessage(gst_bus_pop_filtered(bus.get(), GST_MESSAGE_ELEMENT))) {
            PipelineStatsProbe::noteElementMessage(message.get());
        }
        VideoStreamStatsMeter::Counters counters;
        VideoStreamStats stats;
        probe.sample(nullptr, source.get(), nullptr, counters, stats);
        return counters.rtp ? counters.rtp->packetsLost : 0;
    };
    // A burst, as after a stalled link: the packets of many seconds arrive within the two that the jitter buffer holds
    // the stream's first packets for. None is missing, but it keeps no more than its latency and drops the oldest ones.
    // The packets go on until it has: it drops none while the events that start the stream are still ahead of them.
    QVERIFY_TRUE_WAIT(sendPacket() && (packetsLost() > 0), TestTimeout::mediumMs());

    // Its own count of lost packets is of those that never arrived.
    GstStructure* rawStats = nullptr;
    g_object_get(jitterBuffer.get(), "stats", &rawStats, nullptr);
    const GStreamer::GstStructurePtr stats = GStreamer::adoptStructure(rawStats);
    guint64 neverArrived = 1;
    QVERIFY(stats && gst_structure_get_uint64(stats.get(), "num-lost", &neverArrived));
    QCOMPARE(neverArrived, guint64(0));
}

void GstSourceFactoryStreamTest::_testUdpSourceRefusesPortInUse()
{
    const GStreamer::SourceFactory::Config config;
    const GstTest::TestPipeline listener(GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:0"), config));
    QVERIFY(listener.get());
    // udpsrc binds its socket on the way to READY.
    QCOMPARE(gst_element_set_state(listener.get(), GST_STATE_READY), GST_STATE_CHANGE_SUCCESS);
    gint port = 0;
    g_object_get(GstTest::findByFactory(listener.get(), "udpsrc").get(), "port", &port, nullptr);
    QVERIFY(port > 0);

    // A second listener on the port would get some streams' packets, the first one the others', and neither an error.
    const GstTest::TestPipeline second(
        GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:%1").arg(port), config));
    QVERIFY(second.get());
    QCOMPARE(gst_element_set_state(second.get(), GST_STATE_READY), GST_STATE_CHANGE_FAILURE);
}

void GstSourceFactoryStreamTest::_testUdpSourceStopsRightAfterPlaying()
{
    // An unfixed udpsrc corrupts the heap in most runs of a few thousand cycles; each takes well under a millisecond.
    constexpr int kImmediateStopCycles = 10000;

    // The factory's UDP sources are udpsrc. Before gst-plugins-good 1.24.10, leaving PLAYING while its streaming thread
    // still clears the previous unlock cancels a freed GCancellable and corrupts the heap; GStreamer::completeInit()
    // serializes the two. Downstream elements delay the source's own state change and hide the race, so the source
    // runs bare.
    for (int cycle = 0; cycle < kImmediateStopCycles; ++cycle) {
        GstTest::TestPipeline pipeline;
        QVERIFY2(pipeline.launch(QStringLiteral("udpsrc address=127.0.0.1 port=0 ! fakesink")),
                 qPrintable(pipeline.error()));
        QVERIFY(pipeline.play());
        QCOMPARE(gst_element_set_state(pipeline.get(), GST_STATE_NULL), GST_STATE_CHANGE_SUCCESS);
    }
}

#endif

UT_REGISTER_TEST(GstSourceFactoryStreamTest, TestLabel::Integration)
