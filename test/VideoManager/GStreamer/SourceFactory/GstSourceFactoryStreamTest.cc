#include "GstSourceFactoryStreamTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <gst/gst.h>

#include <QtCore/QByteArray>

#include "GStreamerTestHelpers.h"
#include "GstScoped.h"
#include "GstSourceFactory.h"

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

// Units are recorded at h264parse's sink rather than the depayloader's src: the source bin sees
// deep-element-added before parsebin's own handlers, so a probe added here would run before the factory's.
void watchH264Depayloading([[maybe_unused]] GstBin* bin, [[maybe_unused]] GstBin* subBin, GstElement* element,
                           gpointer data)
{
    GstElementFactory* factory = gst_element_get_factory(element);
    const char* factoryName = factory ? gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)) : nullptr;
    auto* output = static_cast<DepayloaderOutput*>(data);
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(element, "sink"));
    if (g_strcmp0(factoryName, "rtph264depay") == 0) {
        (void) gst_pad_add_probe(sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &output->packetsIn,
                                 nullptr);
    } else if (g_strcmp0(factoryName, "h264parse") == 0) {
        (void) gst_pad_add_probe(sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, recordDepayloaderUnit, output, nullptr);
    }
}

void linkToSink([[maybe_unused]] GstElement* element, GstPad* pad, gpointer data)
{
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(GST_ELEMENT(data), "sink"));
    if (sinkPad && !gst_pad_is_linked(sinkPad.get())) {
        (void) gst_pad_link(pad, sinkPad.get());
    }
}

}  // namespace

void GstSourceFactoryStreamTest::_testSourceFactoryRtspSetsUpOnlyVideoStream()
{
    if (!GstTest::firstMissingFactory({"rtspsrc"}).isEmpty()) {
        QSKIP("rtspsrc plugin unavailable");
    }

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
    const QString missing = GstTest::firstMissingFactory(
        {"appsrc", "h264parse", "rtph264pay", "udpsink", "udpsrc", "parsebin", "rtph264depay", "fakesink", "appsink"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 plugin unavailable").arg(missing)));
    }

    DepayloaderOutput output;
    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr source = GstTest::createSource(QStringLiteral("udp://127.0.0.1:0"), config);
    QVERIFY(source);
    (void) g_signal_connect(source.get(), "deep-element-added", G_CALLBACK(watchH264Depayloading), &output);

    GstTest::TestPipeline receiver(gst_pipeline_new("join-mid-gop"));
    GstElement* const sink = gst_element_factory_make("fakesink", nullptr);
    QVERIFY(sink);
    g_object_set(sink, "sync", FALSE, "async", FALSE, nullptr);
    QVERIFY(gst_bin_add(receiver.bin(), source.get()));
    QVERIFY(gst_bin_add(receiver.bin(), sink));
    (void) g_signal_connect(source.get(), "pad-added", G_CALLBACK(linkToSink), sink);
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

#endif

UT_REGISTER_TEST(GstSourceFactoryStreamTest, TestLabel::Integration)
