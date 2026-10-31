#include "GstSelfTestPipelineTest.h"

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>

#include <QtCore/QMap>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>

#include "GStreamerHelpers.h"
#include "GStreamerSelfTest.h"
#include "GstScoped.h"

namespace {

constexpr const char* kFailingDecoderName = "qgcselftestfailingh264dec";

/// An H.264 "hardware" decoder that starts but fails on the first frame, as one may in a VM.
struct SelfTestFailingDecoder
{
    GstElement parent;
};

struct SelfTestFailingDecoderClass
{
    GstElementClass parentClass;
};

GstStaticPadTemplate s_failingDecoderSink =
    GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("video/x-h264, stream-format=(string)byte-stream, alignment=(string)au"));
GstStaticPadTemplate s_failingDecoderSrc =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS("video/x-raw"));

G_DEFINE_TYPE(SelfTestFailingDecoder, self_test_failing_decoder, GST_TYPE_ELEMENT)

GstFlowReturn failingDecoderChain([[maybe_unused]] GstPad* pad, GstObject* parent, GstBuffer* buffer)
{
    gst_buffer_unref(buffer);
    GST_ELEMENT_ERROR(GST_ELEMENT(parent), STREAM, DECODE, ("Simulated hardware decoder failure"), (nullptr));
    return GST_FLOW_ERROR;
}

gboolean failingDecoderEvent(GstPad* pad, GstObject* parent, GstEvent* event)
{
    // Takes the stream's caps without passing them on: the decoder never outputs anything.
    if (GST_EVENT_TYPE(event) == GST_EVENT_CAPS) {
        gst_event_unref(event);
        return TRUE;
    }
    return gst_pad_event_default(pad, parent, event);
}

void self_test_failing_decoder_class_init(SelfTestFailingDecoderClass* klass)
{
    GstElementClass* const elementClass = GST_ELEMENT_CLASS(klass);
    gst_element_class_add_static_pad_template(elementClass, &s_failingDecoderSink);
    gst_element_class_add_static_pad_template(elementClass, &s_failingDecoderSrc);
    gst_element_class_set_static_metadata(elementClass, "QGC self-test failing decoder", "Codec/Decoder/Video/Hardware",
                                          "Fails on the first frame", "QGroundControl");
}

void self_test_failing_decoder_init(SelfTestFailingDecoder* self)
{
    GstPad* const sink = gst_pad_new_from_static_template(&s_failingDecoderSink, "sink");
    gst_pad_set_chain_function(sink, failingDecoderChain);
    gst_pad_set_event_function(sink, failingDecoderEvent);
    (void) gst_element_add_pad(GST_ELEMENT(self), sink);
    (void) gst_element_add_pad(GST_ELEMENT(self), gst_pad_new_from_static_template(&s_failingDecoderSrc, "src"));
}

QMap<QString, guint> videoDecoderRanks()
{
    QMap<QString, guint> ranks;
    GStreamer::forEachVideoDecoderFactory(GST_RANK_NONE, [&ranks](GstElementFactory* factory) {
        GstPluginFeature* const feature = GST_PLUGIN_FEATURE(factory);
        ranks.insert(QString::fromUtf8(gst_plugin_feature_get_name(feature)), gst_plugin_feature_get_rank(feature));
    });
    return ranks;
}

}  // namespace

void GstSelfTestPipelineTest::_testSystemRuntimePasses()
{
    // A host whose hardware decoder fails mid-stream (a VM's) still passes, with this warning.
    ignoreLogMessage("Video.GStreamer.SelfTest", QtWarningMsg,
                     QRegularExpression(QStringLiteral("hardware decoding with .* failed")));

    QVERIFY(GStreamer::SelfTest::run({}));
}

void GstSelfTestPipelineTest::_testFailingHardwareDecoderFallsBackToSoftware()
{
    const GStreamer::GstCapsPtr h264 = GStreamer::adoptCaps(gst_caps_new_empty_simple("video/x-h264"));
    bool softwareDecoder = false;
    GStreamer::forEachVideoDecoderFactory(GST_RANK_MARGINAL, [&h264, &softwareDecoder](GstElementFactory* factory) {
        softwareDecoder = softwareDecoder || (!GStreamer::isHardwareDecoderFactory(factory) &&
                                              gst_element_factory_can_sink_any_caps(factory, h264.get()));
    });
    if (!softwareDecoder) {
        QSKIP("No software H.264 decoder to fall back to");
    }

    QVERIFY(gst_element_register(nullptr, kFailingDecoderName, GST_RANK_PRIMARY + 100,
                                 self_test_failing_decoder_get_type()));
    const GStreamer::GstFeaturePtr failingDecoder =
        GStreamer::adoptFeature(gst_registry_lookup_feature(gst_registry_get(), kFailingDecoderName));
    QVERIFY(failingDecoder);
    const auto unrank = qScopeGuard([&failingDecoder] { gst_plugin_feature_set_rank(failingDecoder.get(), 0); });
    const QMap<QString, guint> ranks = videoDecoderRanks();

    ignoreLogMessage("Video.GStreamer.GStreamerAPI", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Simulated hardware decoder failure")));
    expectLogMessage("Video.GStreamer.SelfTest", QtWarningMsg,
                     QRegularExpression(
                         QStringLiteral("H\\.264 hardware decoding with %1 failed.*Simulated hardware decoder failure")
                             .arg(QLatin1String(kFailingDecoderName))));
    QVERIFY(GStreamer::SelfTest::run({}));
    verifyExpectedLogMessage();
    QCOMPARE(videoDecoderRanks(), ranks);
}

#endif

UT_REGISTER_TEST(GstSelfTestPipelineTest, TestLabel::Integration)
