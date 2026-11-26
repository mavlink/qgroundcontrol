#include "GstSelfTestPipelineTest.h"

#include <gst/gst.h>

#include <QtCore/QHash>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>

#include "GStreamerDecoderRanks.h"
#include "GStreamerSelfTest.h"
#include "GStreamerTestHelpers.h"
#include "GstScoped.h"

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

    // A "hardware" decoder that starts but fails on the first frame, as one may in a VM.
    const GStreamer::GstFeaturePtr failingDecoder =
        GstTest::registerStubDecoder(GstTest::StubDecoderFailure::AtFirstFrame);
    QVERIFY(failingDecoder);
    gst_plugin_feature_set_rank(failingDecoder.get(), GST_RANK_PRIMARY + 100);
    const auto unrank = qScopeGuard([&failingDecoder] { gst_plugin_feature_set_rank(failingDecoder.get(), 0); });
    const QHash<QByteArray, guint> ranks = GstTest::videoDecoderRanks();

    ignoreLogMessage("Video.GStreamer.GStreamerAPI", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Simulated hardware decoder failure")));
    expectLogMessage("Video.GStreamer.SelfTest", QtWarningMsg,
                     QRegularExpression(
                         QStringLiteral("H\\.264 hardware decoding with %1 failed.*Simulated hardware decoder failure")
                             .arg(QLatin1String(GstTest::kStubDecoderName))));
    QVERIFY(GStreamer::SelfTest::run({}));
    verifyExpectedLogMessage();
    QCOMPARE(GstTest::videoDecoderRanks(), ranks);
}

UT_REGISTER_TEST(GstSelfTestPipelineTest, TestLabel::Integration)
