#include "GStreamerTest.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GStreamerTestLog, "Video.GStreamer.GStreamerTest")

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>
#include <memory>

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QVersionNumber>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GStreamer.h"
#include "GStreamerDecoderRanks.h"
#include "GStreamerHelpers.h"
#include "GStreamerTestHelpers.h"
#include "GStreamerVideoBackend.h"
#include "GstVideoReceiver.h"
#include "PipelineDotDumper.h"
#include "RecordingBranch.h"
#include "VideoBackend.h"

void GStreamerTest::_testIsHardwareDecoderFactory_data()
{
    QTest::addColumn<QString>("factoryName");
    QTest::addColumn<bool>("expectedHardware");

    QTest::newRow("null-factory") << QString() << false;
    QTest::newRow("avdec_h264") << QStringLiteral("avdec_h264") << false;
    // Software decoders whose names merely contain a hardware family tag ("nv").
    QTest::newRow("avdec_wnv1") << QStringLiteral("avdec_wnv1") << false;
    QTest::newRow("avdec_dsicinvideo") << QStringLiteral("avdec_dsicinvideo") << false;
    QTest::newRow("vah264dec") << QStringLiteral("vah264dec") << true;
    QTest::newRow("vaapih264dec") << QStringLiteral("vaapih264dec") << true;
    QTest::newRow("vulkanh264dec") << QStringLiteral("vulkanh264dec") << true;
}

void GStreamerTest::_testIsHardwareDecoderFactory()
{
    QFETCH(QString, factoryName);
    QFETCH(bool, expectedHardware);

    if (factoryName.isEmpty()) {
        QCOMPARE(GStreamer::isHardwareDecoderFactory(nullptr), expectedHardware);
        return;
    }

    const GStreamer::GstFactoryPtr factory =
        GStreamer::adoptFactory(gst_element_factory_find(factoryName.toUtf8().constData()));
    if (!factory) {
        QSKIP(qPrintable(QStringLiteral("Decoder factory not installed: %1").arg(factoryName)));
    }
    QCOMPARE(GStreamer::isHardwareDecoderFactory(factory.get()), expectedHardware);
}

void GStreamerTest::_testDecoderFamilyForFactory_data()
{
    QTest::addColumn<QString>("factoryName");
    QTest::addColumn<int>("expectedFamily");

    QTest::newRow("nvidia") << QStringLiteral("nvh264dec") << int(GStreamer::ForceVideoDecoderNVIDIA);
    QTest::newRow("va") << QStringLiteral("vah265dec") << int(GStreamer::ForceVideoDecoderVAAPI);
    // Legacy gstreamer-vaapi is demoted and never bumped by the VAAPI option.
    QTest::newRow("legacy-vaapi") << QStringLiteral("vaapih264dec") << int(GStreamer::ForceVideoDecoderDefault);
    QTest::newRow("qsv") << QStringLiteral("qsvh265dec") << int(GStreamer::ForceVideoDecoderIntel);
    QTest::newRow("msdk") << QStringLiteral("msdkh264dec") << int(GStreamer::ForceVideoDecoderIntel);
    QTest::newRow("d3d11") << QStringLiteral("d3d11h264dec") << int(GStreamer::ForceVideoDecoderDirectX3D);
    QTest::newRow("d3d12") << QStringLiteral("d3d12h265dec") << int(GStreamer::ForceVideoDecoderDirectX3D);
    QTest::newRow("videotoolbox") << QStringLiteral("vtdec_hw") << int(GStreamer::ForceVideoDecoderVideoToolbox);
    QTest::newRow("vulkan") << QStringLiteral("vulkanh264dec") << int(GStreamer::ForceVideoDecoderVulkan);
    QTest::newRow("software-with-nv") << QStringLiteral("avdec_wnv1") << int(GStreamer::ForceVideoDecoderDefault);
    QTest::newRow("android-mediacodec") << QStringLiteral("amcviddec-c2qtiavcdecoder")
                                        << int(GStreamer::ForceVideoDecoderDefault);
}

void GStreamerTest::_testDecoderFamilyForFactory()
{
    QFETCH(QString, factoryName);
    QFETCH(int, expectedFamily);

    QCOMPARE(int(GStreamer::decoderFamilyForFactory(factoryName.toUtf8().constData())), expectedFamily);
}

void GStreamerTest::_testAvailableDecoderFamilies()
{
    QList<GStreamer::VideoDecoderOptions> installed;
    GStreamer::forEachVideoDecoderFactory(GST_RANK_NONE, [&installed](GstElementFactory* factory) {
        const auto family =
            GStreamer::decoderFamilyForFactory(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)));
        if ((family != GStreamer::ForceVideoDecoderDefault) && !installed.contains(family)) {
            installed.append(family);
        }
    });
#ifndef QGC_HAS_GST_VULKAN_GPU_PATH
    // Without the Vulkan GPU path, forcing Vulkan decoders would only add a CPU copy of every frame.
    installed.removeAll(GStreamer::ForceVideoDecoderVulkan);
#endif
    QCOMPARE(GStreamer::availableDecoderFamilies(), installed);
}

void GStreamerTest::_testDecoderPriorityWarnings()
{
    const auto restoreRanks = qScopeGuard([] { GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderDefault); });

    // A stored value outside the options changes nothing.
    expectLogMessage("Video.GStreamer.DecoderRanks", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Ignoring invalid decode option: 99$")));
    GStreamer::setCodecPriorities(99);
    verifyExpectedLogMessage();
    expectLogMessage("Video.GStreamer.DecoderRanks", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Ignoring invalid decode option: -1$")));
    GStreamer::setCodecPriorities(-1);
    verifyExpectedLogMessage();

    // Vulkan decoders are ranked first, though their frames are copied through the CPU.
    expectLogMessage("Video.GStreamer.DecoderRanks", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Forcing Vulkan video decoders")));
    GStreamer::setCodecPriorities(static_cast<int>(GStreamer::ForceVideoDecoderVulkan));
    verifyExpectedLogMessage();
}

void GStreamerTest::_testLogDecoderRanks()
{
    const TestFixtures::LoggingCategoryFixture ranksLog(QStringLiteral("Video.GStreamer.DecoderRanks"));

    // A header, then a line per decoder, best ranked first.
    ignoreLogMessage("Video.GStreamer.DecoderRanks", QtDebugMsg, QRegularExpression(QStringLiteral("^  ")));
    expectLogMessage("Video.GStreamer.DecoderRanks", QtDebugMsg,
                     QRegularExpression(QStringLiteral("^Video decoder ranks:$")));
    GStreamer::logDecoderRanks();
    verifyExpectedLogMessage();
}

void GStreamerTest::_testPipelineDotOmitsElementProperties()
{
    const GStreamer::GstElementPtr pipeline =
        GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(gst_pipeline_new("safe-dot-test"))));

    GstElement* source = gst_element_factory_make("filesrc", "source");
    GstElement* sink = gst_element_factory_make("fakesink", "sink");
    QVERIFY(source);
    QVERIFY(sink);

    constexpr auto kSecretLocation = "/tmp/qgc-dot-secret-token";
    g_object_set(source, "location", kSecretLocation, nullptr);
    gst_bin_add_many(GST_BIN(pipeline.get()), source, sink, nullptr);
    QVERIFY(gst_element_link(source, sink));

    const GStreamer::GCharPtr dotData = GStreamer::adoptGChar(
        gst_debug_bin_to_dot_data(GST_BIN(pipeline.get()), PipelineDotDumper::kDiagnosticDotGraphDetails));
    QVERIFY(dotData);
    const QByteArray dot(dotData.get());

    QVERIFY(dot.contains("source"));
    QVERIFY(dot.contains("sink"));
    QVERIFY(!dot.contains(kSecretLocation));
    QVERIFY(!dot.contains("qgc-dot-secret-token"));
}

void GStreamerTest::_testCompleteInit()
{
    QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");

    GstRegistry* registry = gst_registry_get();
    QVERIFY(registry);

    GstPlugin* qgcPlugin = gst_registry_find_plugin(registry, "qgc");
    QVERIFY2(qgcPlugin, "Static plugin 'qgc' not registered after completeInit()");
    gst_clear_object(&qgcPlugin);

    for (const char* const name : {"appsink", "qgcvideosinkbin"}) {
        QVERIFY2(GStreamer::hasElementFactory(name),
                 qPrintable(QStringLiteral("Factory '%1' not found after completeInit()").arg(QLatin1String(name))));
    }
}

void GStreamerTest::_testCreateVideoReceiver()
{
    std::unique_ptr<VideoReceiver> receiver(GStreamer::createVideoReceiver(nullptr));
    QVERIFY2(receiver, "GStreamer::createVideoReceiver() returned nullptr");
    QVERIFY(qobject_cast<GstVideoReceiver*>(receiver.get()));
}

void GStreamerTest::_testVideoBackendForwards()
{
    GStreamerVideoBackend backend;
    QVERIFY(backend.hasStreaming());
    QVERIFY(backend.hasPipelineSettings());
    QVERIFY(backend.needsAsyncInit());
#ifdef QGC_HAS_ANY_GPU_PATH
    QVERIFY(backend.hasGpuVideoPath());
#else
    QVERIFY(!backend.hasGpuVideoPath());
#endif

    const std::unique_ptr<VideoReceiver> receiver(backend.createReceiver(nullptr));
    QVERIFY(qobject_cast<GstVideoReceiver*>(receiver.get()));
    QCOMPARE(backend.availableDecoderFamilies(), GStreamer::availableDecoderFamilies());

    // The default decoder priorities and debug threshold change nothing a test sees.
    const GstDebugLevel threshold = gst_debug_get_default_threshold();
    backend.setDebugLevel(static_cast<int>(threshold));
    QCOMPARE(gst_debug_get_default_threshold(), threshold);
    backend.applyDecoderPriorities(GStreamer::ForceVideoDecoderDefault);
}

void GStreamerTest::_testRecordingSinkAcceptsElementaryStreams_data()
{
    QTest::addColumn<QString>("capsString");
    QTest::addColumn<int>("format");

    constexpr const char* h264Caps = "video/x-h264,stream-format=byte-stream,alignment=au";
    constexpr const char* h265Caps = "video/x-h265,stream-format=byte-stream,alignment=au";

    QTest::newRow("h264-mkv") << QString::fromLatin1(h264Caps) << static_cast<int>(VideoReceiver::FileFormat::Mkv);
    QTest::newRow("h264-mov") << QString::fromLatin1(h264Caps) << static_cast<int>(VideoReceiver::FileFormat::Mov);
    QTest::newRow("h264-mp4") << QString::fromLatin1(h264Caps) << static_cast<int>(VideoReceiver::FileFormat::Mp4);
    QTest::newRow("h265-mkv") << QString::fromLatin1(h265Caps) << static_cast<int>(VideoReceiver::FileFormat::Mkv);
    QTest::newRow("h265-mov") << QString::fromLatin1(h265Caps) << static_cast<int>(VideoReceiver::FileFormat::Mov);
    QTest::newRow("h265-mp4") << QString::fromLatin1(h265Caps) << static_cast<int>(VideoReceiver::FileFormat::Mp4);
}

void GStreamerTest::_testRecordingSinkAcceptsElementaryStreams()
{
    QFETCH(QString, capsString);
    QFETCH(int, format);

    const auto fileFormat = static_cast<VideoReceiver::FileFormat>(format);
    const char* muxerFactory = (fileFormat == VideoReceiver::FileFormat::Mkv)
                                   ? "matroskamux"
                                   : ((fileFormat == VideoReceiver::FileFormat::Mov) ? "qtmux" : "mp4mux");

    GSTTEST_SKIP_UNLESS_FACTORIES(muxerFactory);

    const GStreamer::GstCapsPtr inputCaps = GStreamer::adoptCaps(gst_caps_from_string(capsString.toUtf8().constData()));
    QVERIFY(inputCaps);

    GstElement* const capsfilter = gst_element_factory_make("capsfilter", nullptr);
    QVERIFY(capsfilter);
    const GStreamer::GstElementPtr source = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(capsfilter)));
    g_object_set(source.get(), "caps", inputCaps.get(), nullptr);

    const GStreamer::GstElementPtr sink =
        RecordingBranch::makeFileSink(QStringLiteral("recording-test"), fileFormat, inputCaps.get());
    QVERIFY(sink);

    QVERIFY2(gst_element_link(source.get(), sink.get()),
             qPrintable(QStringLiteral("Recording sink rejected %1").arg(capsString)));
}

void GStreamerTest::_testBindDebugLevelFactRejectsNullContext()
{
    Fact fact;
    VideoBackend::bindDebugLevelFact(&fact, nullptr);
}

void GStreamerTest::_testRuntimeVersionCheck()
{
    guint major, minor, micro, nano;
    gst_version(&major, &minor, &micro, &nano);

    QVERIFY2(major == 1, "Unexpected GStreamer major version");
    const QVersionNumber runtime(static_cast<int>(major), static_cast<int>(minor), static_cast<int>(micro));
    const QVersionNumber minimum = QVersionNumber::fromString(QStringLiteral(QGC_GST_RUNTIME_MINIMUM_VERSION));
    QVERIFY(!minimum.isNull());
    QVERIFY2(runtime >= minimum, qPrintable(QStringLiteral("GStreamer runtime version %1 is below the minimum %2")
                                                .arg(runtime.toString(), minimum.toString())));

    if (major != GST_VERSION_MAJOR || minor != GST_VERSION_MINOR) {
        qCWarning(GStreamerTestLog) << "GStreamer version mismatch: built against" << GST_VERSION_MAJOR << "."
                                    << GST_VERSION_MINOR << "but runtime is" << major << "." << minor;
    }
}

#endif

UT_REGISTER_TEST(GStreamerTest, TestLabel::Unit)
