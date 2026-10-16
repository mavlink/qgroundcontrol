#include "GStreamerTest.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GStreamerTestLog, "Video.GStreamer.GStreamerTest")

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <memory>
#include <vector>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "GStreamerTestHelpers.h"
#include "GstVideoReceiver.h"
#include "LogManager.h"
#include "VideoBackend.h"

namespace {

struct DemuxPadContext
{
    GstElement* parser = nullptr;
    std::atomic<bool> linked{false};
};

void linkH265DemuxPad([[maybe_unused]] GstElement* demux, GstPad* pad, gpointer userData)
{
    auto* context = static_cast<DemuxPadContext*>(userData);
    GstCaps* caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, nullptr);
    }
    if (!caps || gst_caps_is_any(caps) || gst_caps_is_empty(caps)) {
        gst_clear_caps(&caps);
        return;
    }

    bool isH265 = false;
    for (guint i = 0; i < gst_caps_get_size(caps); ++i) {
        if (gst_structure_has_name(gst_caps_get_structure(caps, i), "video/x-h265")) {
            isH265 = true;
            break;
        }
    }
    gst_clear_caps(&caps);
    if (!isH265) {
        return;
    }

    GstPad* sinkPad = gst_element_get_static_pad(context->parser, "sink");
    if (sinkPad) {
        context->linked.store(gst_pad_link(pad, sinkPad) == GST_PAD_LINK_OK, std::memory_order_relaxed);
        gst_clear_object(&sinkPad);
    }
}

GstFlowReturn pushAccessUnit(GstElement* appsrc, const QByteArray& bytes, GstClockTime timestamp)
{
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, static_cast<gsize>(bytes.size()), nullptr);
    if (!buffer) {
        return GST_FLOW_ERROR;
    }
    (void) gst_buffer_fill(buffer, 0, bytes.constData(), static_cast<gsize>(bytes.size()));
    GST_BUFFER_PTS(buffer) = timestamp;
    GST_BUFFER_DTS(buffer) = timestamp;
    GST_BUFFER_DURATION(buffer) = GST_SECOND / 4;
    return gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
}

}  // namespace

void GStreamerTest::initTestCase()
{
    UnitTest::initTestCase();

    // Every test runs against a fully initialized runtime, as the app does once VideoBackend is up.
    GStreamer::redirectGLibLogging();
    if (GstTest::initGStreamer().isEmpty()) {
        QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");
    }
}

void GStreamerTest::init()
{
    const QString unavailable = GstTest::initGStreamer();
    if (!unavailable.isEmpty()) {
        QSKIP(qPrintable(unavailable));
    }

    UnitTest::init();

    static const QRegularExpression sGLibTypeRe(
        QStringLiteral("cannot register existing type|"
                       "g_type_add_interface_static.*G_TYPE_IS_INSTANTIATABLE|"
                       "g_once_init_leave.*result != 0"));
    // GStreamer/GLib type registration warnings are environment/startup-order dependent
    // and may occur 0..N times across test process lifetime.
    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtCriticalMsg, sGLibTypeRe);

    // On headless/software-GL CI the sink bin cannot honor construct-only GPU properties
    // and the GL bridge is disabled. These warnings are environment-dependent.
    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg,
                     QRegularExpression(QStringLiteral("gpu-zerocopy.*can't be set after construction")));
    ignoreLogMessage("Video.GStreamer.HwBuffers.GstGlBridge", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Qt GL context exposes neither EGL nor GLX")));
}

void GStreamerTest::_testIsValidRtspUri()
{
    QVERIFY(GStreamer::isValidRtspUri("rtsp://127.0.0.1:8554/test"));
    QVERIFY(GStreamer::isValidRtspUri("rtsp://user:pass@10.0.0.1/stream"));
    QVERIFY(GStreamer::isValidRtspUri("rtspu://192.168.1.1:554/video"));
    QVERIFY(GStreamer::isValidRtspUri("rtspt://example.com/live"));

    QVERIFY(!GStreamer::isValidRtspUri(nullptr));
    QVERIFY(!GStreamer::isValidRtspUri(""));
    QVERIFY(!GStreamer::isValidRtspUri("not-a-uri"));
    QVERIFY(!GStreamer::isValidRtspUri("http://example.com"));
    QVERIFY(!GStreamer::isValidRtspUri("udp://127.0.0.1:5600"));
    QVERIFY(!GStreamer::isValidRtspUri("rtsp://"));
}

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

void GStreamerTest::_testSetCodecPrioritiesDefault()
{
    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderDefault);

    GstRegistry* registry = gst_registry_get();
    QVERIFY(registry != nullptr);
}

void GStreamerTest::_testSetCodecPrioritiesDefaultPrefersMatchingD3DDecoder()
{
#ifndef Q_OS_WIN
    QSKIP("D3D decoder rank steering is Windows-only");
#else
    GstRegistry* registry = gst_registry_get();
    QVERIFY(registry != nullptr);

    const GStreamer::GstFeaturePtr software =
        GStreamer::adoptFeature(gst_registry_lookup_feature(registry, "avdec_h265"));
    const GStreamer::GstFeaturePtr d3d11 =
        GStreamer::adoptFeature(gst_registry_lookup_feature(registry, "d3d11h265dec"));
    const GStreamer::GstFeaturePtr d3d12 =
        GStreamer::adoptFeature(gst_registry_lookup_feature(registry, "d3d12h265dec"));
    if (!software || !d3d11 || !d3d12) {
        QSKIP("Required H.265 software/D3D decoder factories are not installed");
    }

    const guint oldSoftwareRank = gst_plugin_feature_get_rank(software.get());
    const guint oldD3D11Rank = gst_plugin_feature_get_rank(d3d11.get());
    const guint oldD3D12Rank = gst_plugin_feature_get_rank(d3d12.get());
    const auto restoreRanks = qScopeGuard([&]() {
        gst_plugin_feature_set_rank(software.get(), oldSoftwareRank);
        gst_plugin_feature_set_rank(d3d11.get(), oldD3D11Rank);
        gst_plugin_feature_set_rank(d3d12.get(), oldD3D12Rank);
    });

    const TestFixtures::EnvVarFixture rhiBackend("QSG_RHI_BACKEND");
    qputenv("QSG_RHI_BACKEND", QByteArray("d3d11"));

    gst_plugin_feature_set_rank(software.get(), GST_RANK_PRIMARY + 1);
    gst_plugin_feature_set_rank(d3d11.get(), GST_RANK_MARGINAL);
    gst_plugin_feature_set_rank(d3d12.get(), GST_RANK_PRIMARY + 2);

    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderDefault);

    const guint softwareRank = gst_plugin_feature_get_rank(software.get());
    const guint d3d11Rank = gst_plugin_feature_get_rank(d3d11.get());
    const guint d3d12Rank = gst_plugin_feature_get_rank(d3d12.get());

    QVERIFY2(d3d11Rank > softwareRank, "Default Windows D3D11 RHI must prefer d3d11h265dec over avdec_h265");
    QCOMPARE(d3d12Rank, static_cast<guint>(GST_RANK_NONE));
#endif
}

void GStreamerTest::_testSetCodecPrioritiesSkipsAbsentD3DDecoders()
{
#ifndef Q_OS_WIN
    QSKIP("D3D decoder rank steering is Windows-only");
#else
    GstRegistry* registry = gst_registry_get();
    QVERIFY(registry != nullptr);

    const TestFixtures::LoggingCategoryFixture helpersLog(QStringLiteral("Video.GStreamer.GStreamerHelpers"));

    LogManager::clearCapturedMessages();
    QVERIFY(!GStreamer::changeFeatureRank(registry, "__qgc_missing_d3d_decoder_for_test__", GST_RANK_NONE));

    const QList<LogEntry> helperMessages =
        LogManager::capturedMessages(QStringLiteral("Video.GStreamer.GStreamerHelpers"));
    for (const LogEntry& entry : helperMessages) {
        QVERIFY2(!entry.message.contains(QStringLiteral("Feature does not exist")),
                 qPrintable(QStringLiteral("Optional D3D decoder factory was logged as a failure: %1")
                                 .arg(entry.message)));
    }
#endif
}

void GStreamerTest::_testD3D12RhiDisablesGpuZeroCopySink()
{
#if !defined(Q_OS_WIN) || !defined(QGC_HAS_GST_D3D12_GPU_PATH)
    QSKIP("D3D12 sink policy is Windows D3D12-only");
#else
    const TestFixtures::EnvVarFixture rhiBackend("QSG_RHI_BACKEND");
    qputenv("QSG_RHI_BACKEND", QByteArray("d3d12"));

    QVERIFY2(!VideoBackend::gpuZeroCopyAllowedForCurrentGraphicsApi(false, false),
             "D3D12 RHI must force the CPU sink path because gst-d3d12 cannot wrap Qt's ID3D12Device");
    QVERIFY(VideoBackend::gpuZeroCopyAllowedForCurrentGraphicsApi(false, true) == false);
    QVERIFY(VideoBackend::gpuZeroCopyAllowedForCurrentGraphicsApi(true, false) == false);

    qputenv("QSG_RHI_BACKEND", QByteArray("d3d11"));
    QVERIFY(VideoBackend::gpuZeroCopyAllowedForCurrentGraphicsApi(false, false));
#endif
}

void GStreamerTest::_testSetCodecPrioritiesSoftware()
{
    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderSoftware);

    GList* factories = gst_element_factory_list_get_elements(
        static_cast<GstElementFactoryListType>(GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO),
        GST_RANK_NONE);
    if (!factories) {
        QSKIP("No video decoder factories available on this system");
    }

    bool foundPrioritizedSoftware = false;
    for (GList* node = factories; node != nullptr; node = node->next) {
        GstElementFactory* factory = GST_ELEMENT_FACTORY(node->data);
        if (!factory)
            continue;

        if (!GStreamer::isHardwareDecoderFactory(factory)) {
            const guint rank = gst_plugin_feature_get_rank(GST_PLUGIN_FEATURE(factory));
            if (rank > GST_RANK_MARGINAL) {
                foundPrioritizedSoftware = true;
                break;
            }
        }
    }

    QVERIFY(foundPrioritizedSoftware);

    gst_plugin_feature_list_free(factories);
}

void GStreamerTest::_testSetCodecPrioritiesHardware()
{
    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderHardware);

    GList* factories = gst_element_factory_list_get_elements(
        static_cast<GstElementFactoryListType>(GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO),
        GST_RANK_NONE);
    if (!factories) {
        QSKIP("No video decoder factories available on this system");
    }

    for (GList* node = factories; node != nullptr; node = node->next) {
        GstElementFactory* factory = GST_ELEMENT_FACTORY(node->data);
        if (!factory)
            continue;

        if (!GStreamer::isHardwareDecoderFactory(factory)) {
            const guint rank = gst_plugin_feature_get_rank(GST_PLUGIN_FEATURE(factory));
            QCOMPARE(rank, static_cast<guint>(GST_RANK_NONE));
        }
    }

    gst_plugin_feature_list_free(factories);
}

void GStreamerTest::_testRedirectGLibLogging()
{
    GStreamer::redirectGLibLogging();

    // Debug-level GLib messages map to QtDebugMsg — suppress it.
    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtDebugMsg,
                     QRegularExpression(QStringLiteral("GStreamerTest debug message")));
    // Warning-level message is the redirect target being tested — expect and verify it.
    expectLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg,
                     QRegularExpression(QStringLiteral("GStreamerTest warning message")));
    g_log("TestDomain", G_LOG_LEVEL_DEBUG, "GStreamerTest debug message");
    g_log("TestDomain", G_LOG_LEVEL_WARNING, "GStreamerTest warning message");
    verifyExpectedLogMessage();
}

void GStreamerTest::_testConfigureDebugLoggingIsIdempotent()
{
    GStreamer::configureDebugLogging();
    GStreamer::configureDebugLogging();

    GST_DEBUG_CATEGORY_STATIC(qgcTestDebug);
    GST_DEBUG_CATEGORY_INIT(qgcTestDebug, "qgc-test-debug", 0, "QGC GStreamer test debug category");
    if (!qgcTestDebug) {
        QSKIP("GStreamer debug categories unavailable");
    }
    gst_debug_category_set_threshold(qgcTestDebug, GST_LEVEL_WARNING);

    expectLogMessage("Video.GStreamer.GStreamerAPI", QtWarningMsg,
                     QRegularExpression(QStringLiteral("idempotent configureDebugLogging probe")));
    gst_debug_log(qgcTestDebug, GST_LEVEL_WARNING, __FILE__, Q_FUNC_INFO, __LINE__, nullptr, "%s",
                  "idempotent configureDebugLogging probe");
    verifyExpectedLogMessage();

    gst_debug_category_set_threshold(qgcTestDebug, GST_LEVEL_NONE);
}

void GStreamerTest::_testVerifyRequiredPlugins()
{
    GstRegistry* registry = gst_registry_get();
    QVERIFY(registry != nullptr);

    GstPlugin* corePlugin = gst_registry_find_plugin(registry, "coreelements");
    QVERIFY2(corePlugin, "Required plugin not found: coreelements");
    gst_clear_object(&corePlugin);

    GstPlugin* parserPlugin = gst_registry_find_plugin(registry, "videoparsersbad");
    QVERIFY2(parserPlugin, "Required plugin not found: videoparsersbad");
    gst_clear_object(&parserPlugin);

    GList* plugins = gst_registry_get_plugin_list(registry);
    const int pluginCount = g_list_length(plugins);
    gst_plugin_list_free(plugins);
    QVERIFY2(pluginCount > 0, "GStreamer registry contains no plugins at all");

    QVERIFY2(GStreamer::adoptFactory(gst_element_factory_find("playbin")), "Required factory not found: playbin");
}

void GStreamerTest::_testEnvironmentSetup()
{
    // Save and clear relevant env vars
    static constexpr const char* envVars[] = {
        "GIO_EXTRA_MODULES",
        "GIO_MODULE_DIR",
        "GIO_USE_VFS",
        "GST_PTP_HELPER",
        "GST_PTP_HELPER_1_0",
        "GST_PLUGIN_PATH",
        "GST_PLUGIN_PATH_1_0",
        "GST_PLUGIN_SYSTEM_PATH",
        "GST_PLUGIN_SYSTEM_PATH_1_0",
        "GST_PLUGIN_SCANNER",
        "GST_PLUGIN_SCANNER_1_0",
        "GST_REGISTRY_REUSE_PLUGIN_SCANNER",
        "GTK_PATH",
        "PYTHONHOME",
        "PYTHONPATH",
        "PYTHONUSERBASE",
        "VIRTUAL_ENV",
        "CONDA_PREFIX",
        "CONDA_DEFAULT_ENV",
        "PYTHONNOUSERSITE",
    };
    std::vector<TestFixtures::EnvVarFixture> envBackups;
    envBackups.reserve(std::size(envVars));
    for (const char* var : envVars) {
        envBackups.emplace_back(var);
        qunsetenv(var);
    }

    GStreamer::prepareEnvironment();

    for (const char* var :
         {"GST_PLUGIN_PATH", "GST_PLUGIN_PATH_1_0", "GST_PLUGIN_SYSTEM_PATH", "GST_PLUGIN_SYSTEM_PATH_1_0"}) {
        if (qEnvironmentVariableIsSet(var)) {
            const QString path = qEnvironmentVariable(var);
            // Paths may be colon-separated; check each component
            const QStringList parts = path.split(QDir::listSeparator(), Qt::SkipEmptyParts);
            for (const QString& part : parts) {
                QVERIFY2(QDir(part).exists(),
                         qPrintable(QStringLiteral("%1 contains non-existent path: %2").arg(var, part)));
            }
        }
    }

    for (const char* var : {"GST_PLUGIN_SCANNER", "GST_PLUGIN_SCANNER_1_0"}) {
        if (qEnvironmentVariableIsSet(var)) {
            const QString path = qEnvironmentVariable(var);
            QVERIFY2(QFileInfo(path).isExecutable(),
                     qPrintable(QStringLiteral("%1 is not executable: %2").arg(var, path)));
        }
    }
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
        QVERIFY2(GStreamer::adoptFactory(gst_element_factory_find(name)),
                 qPrintable(QStringLiteral("Factory '%1' not found after completeInit()").arg(QLatin1String(name))));
    }
}

void GStreamerTest::_testCreateVideoReceiver()
{
    std::unique_ptr<VideoReceiver> receiver(GStreamer::createVideoReceiver(nullptr));
    QVERIFY2(receiver, "GStreamer::createVideoReceiver() returned nullptr");
    QVERIFY(qobject_cast<GstVideoReceiver*>(receiver.get()));
}

void GStreamerTest::_testRecordingSinkAcceptsElementaryStreams_data()
{
    QTest::addColumn<QString>("capsString");
    QTest::addColumn<int>("format");

    constexpr const char* h264Caps = "video/x-h264,stream-format=byte-stream,alignment=au";
    constexpr const char* h265Caps = "video/x-h265,stream-format=byte-stream,alignment=au";

    QTest::newRow("h264-mkv") << QString::fromLatin1(h264Caps) << int(VideoReceiver::FILE_FORMAT_MKV);
    QTest::newRow("h264-mov") << QString::fromLatin1(h264Caps) << int(VideoReceiver::FILE_FORMAT_MOV);
    QTest::newRow("h264-mp4") << QString::fromLatin1(h264Caps) << int(VideoReceiver::FILE_FORMAT_MP4);
    QTest::newRow("h265-mkv") << QString::fromLatin1(h265Caps) << int(VideoReceiver::FILE_FORMAT_MKV);
    QTest::newRow("h265-mov") << QString::fromLatin1(h265Caps) << int(VideoReceiver::FILE_FORMAT_MOV);
    QTest::newRow("h265-mp4") << QString::fromLatin1(h265Caps) << int(VideoReceiver::FILE_FORMAT_MP4);
}

void GStreamerTest::_testRecordingSinkAcceptsElementaryStreams()
{
    QFETCH(QString, capsString);
    QFETCH(int, format);

    const auto fileFormat = static_cast<VideoReceiver::FILE_FORMAT>(format);
    const char* parserFactory = capsString.startsWith(QLatin1String("video/x-h265")) ? "h265parse" : "h264parse";
    const char* muxerFactory = (fileFormat == VideoReceiver::FILE_FORMAT_MKV)
                                   ? "matroskamux"
                                   : ((fileFormat == VideoReceiver::FILE_FORMAT_MOV) ? "qtmux" : "mp4mux");

    const QString missing = GstTest::firstMissingFactory({"capsfilter", parserFactory, "splitmuxsink", muxerFactory});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("Required GStreamer factory is unavailable: %1").arg(missing)));
    }

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

void GStreamerTest::_testRecordingSinkFinalizesMidStreamH265Mp4()
{
    const QString missing =
        GstTest::firstMissingFactory({"appsrc", "h265parse", "tee", "queue", "fakesink", "valve", "splitmuxsink",
                                      "mp4mux", "filesrc", "qtdemux", "avdec_h265"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("Required GStreamer factory is unavailable: %1").arg(missing)));
    }

    // Synthetic 32x32 Annex-B H.265 access units. Only the first AU contains VPS/SPS/PPS; the
    // later IDRs intentionally contain just a slice. This reproduces a camera that sends codec
    // configuration once at session startup, before the user begins recording.
    const QByteArray initialAu = QByteArray::fromBase64(QByteArrayLiteral(
        "AAAAAUABDAH//wQIAAADAJgoAAADAAAeugJAAAAAAUIBAQQIAAADAJgoAAADAAAekAhBCKUt0lJhf/gACAALUGBgYEAAAAMAQ"
        "AAAAwECAAAAAUQBwHAwYBEgAAAAASgBrC2AE6/7X1g="));
    const QByteArray secondAu = QByteArray::fromBase64(QByteArrayLiteral("AAAAASgBrC2AE6/7X1g="));
    const QByteArray thirdAu = QByteArray::fromBase64(QByteArrayLiteral("AAAAASgBrCyAE6/7X1g="));
    QVERIFY(!initialAu.isEmpty());
    QVERIFY(!secondAu.isEmpty());
    QVERIFY(!thirdAu.isEmpty());

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString outputPath = QDir(tempDir.path()).filePath(QStringLiteral("mid-stream.mp4"));

    GstTest::TestPipeline pipeline(gst_pipeline_new("mid-stream-recording-test"));
    QVERIFY(pipeline.get());

    GstElement* source = gst_element_factory_make("appsrc", nullptr);
    GstElement* upstreamParser = gst_element_factory_make("h265parse", nullptr);
    GstElement* tee = gst_element_factory_make("tee", nullptr);
    GstElement* displayQueue = gst_element_factory_make("queue", nullptr);
    GstElement* displaySink = gst_element_factory_make("fakesink", nullptr);
    GstElement* recorderQueue = gst_element_factory_make("queue", nullptr);
    GstElement* recorderValve = gst_element_factory_make("valve", nullptr);
    QVERIFY(source);
    QVERIFY(upstreamParser);
    QVERIFY(tee);
    QVERIFY(displayQueue);
    QVERIFY(displaySink);
    QVERIFY(recorderQueue);
    QVERIFY(recorderValve);

    GstCaps* sourceCaps =
        gst_caps_from_string("video/x-h265,stream-format=byte-stream,alignment=au,width=32,height=32,framerate=4/1");
    QVERIFY(sourceCaps);
    g_object_set(source, "caps", sourceCaps, "format", GST_FORMAT_TIME, nullptr);
    gst_clear_caps(&sourceCaps);
    g_object_set(upstreamParser, "config-interval", -1, nullptr);
    g_object_set(displaySink, "sync", FALSE, "async", FALSE, nullptr);
    g_object_set(recorderValve, "drop", TRUE, nullptr);
    gst_util_set_object_arg(G_OBJECT(recorderValve), "drop-mode", "forward-sticky-events");

    gst_bin_add_many(pipeline.bin(), source, upstreamParser, tee, displayQueue, displaySink, recorderQueue,
                     recorderValve, nullptr);
    QVERIFY(gst_element_link_many(source, upstreamParser, tee, nullptr));
    QVERIFY(gst_element_link_many(tee, displayQueue, displaySink, nullptr));
    QVERIFY(gst_element_link_many(tee, recorderQueue, recorderValve, nullptr));

    std::atomic<int> displayedBuffers{0};
    const GStreamer::GstPadPtr displayPad = GStreamer::adoptPad(gst_element_get_static_pad(displaySink, "sink"));
    QVERIFY(displayPad);
    GStreamer::PadProbe displayProbe;
    QVERIFY(displayProbe.add(displayPad.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &displayedBuffers));

    QVERIFY(pipeline.play());
    QVERIFY(pushAccessUnit(source, initialAu, 0) == GST_FLOW_OK);
    QTRY_VERIFY_WITH_TIMEOUT(displayedBuffers.load(std::memory_order_relaxed) >= 1, TestTimeout::shortMs());

    const GStreamer::GstPadPtr valveSrcPad = GStreamer::adoptPad(gst_element_get_static_pad(recorderValve, "src"));
    QVERIFY(valveSrcPad);
    const GStreamer::GstCapsPtr recordingCaps = GStreamer::adoptCaps(gst_pad_get_current_caps(valveSrcPad.get()));
    QVERIFY2(recordingCaps, "Closed recording valve did not preserve negotiated caps");

    const GStreamer::GstElementPtr fileSink =
        RecordingBranch::makeFileSink(outputPath, VideoReceiver::FILE_FORMAT_MP4, recordingCaps.get());
    QVERIFY(fileSink);
    QVERIFY(gst_bin_add(pipeline.bin(), fileSink.get()));
    QVERIFY(gst_element_link(recorderValve, fileSink.get()));
    QVERIFY(gst_element_sync_state_with_parent(fileSink.get()));
    g_object_set(recorderValve, "drop", FALSE, nullptr);

    QVERIFY(pushAccessUnit(source, secondAu, GST_SECOND / 4) == GST_FLOW_OK);
    QVERIFY(pushAccessUnit(source, thirdAu, GST_SECOND / 2) == GST_FLOW_OK);
    QVERIFY(gst_app_src_end_of_stream(GST_APP_SRC(source)) == GST_FLOW_OK);

    const QString failure = pipeline.runToEos();
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
    (void) gst_element_set_state(pipeline.get(), GST_STATE_NULL);
    QVERIFY2(QFileInfo(outputPath).size() > 0, "Recording did not produce an MP4 file");

    GstTest::TestPipeline verifyPipeline(gst_pipeline_new("verify-mid-stream-recording"));
    QVERIFY(verifyPipeline.get());

    GstElement* fileSource = gst_element_factory_make("filesrc", nullptr);
    GstElement* demux = gst_element_factory_make("qtdemux", nullptr);
    GstElement* verifyParser = gst_element_factory_make("h265parse", nullptr);
    GstElement* verifyDecoder = gst_element_factory_make("avdec_h265", nullptr);
    GstElement* verifySink = gst_element_factory_make("fakesink", nullptr);
    QVERIFY(fileSource);
    QVERIFY(demux);
    QVERIFY(verifyParser);
    QVERIFY(verifyDecoder);
    QVERIFY(verifySink);

    const QByteArray outputPathBytes = QFile::encodeName(outputPath);
    g_object_set(fileSource, "location", outputPathBytes.constData(), nullptr);
    g_object_set(verifySink, "sync", FALSE, "async", FALSE, nullptr);
    gst_bin_add_many(verifyPipeline.bin(), fileSource, demux, verifyParser, verifyDecoder, verifySink, nullptr);
    QVERIFY(gst_element_link(fileSource, demux));
    QVERIFY(gst_element_link_many(verifyParser, verifyDecoder, verifySink, nullptr));

    DemuxPadContext demuxContext{verifyParser};
    const gulong padAddedHandler =
        g_signal_connect(demux, "pad-added", G_CALLBACK(linkH265DemuxPad), &demuxContext);
    QVERIFY(padAddedHandler != 0);
    const auto padAddedCleanup = qScopeGuard([&] { g_signal_handler_disconnect(demux, padAddedHandler); });

    std::atomic<int> verifiedBuffers{0};
    const GStreamer::GstPadPtr verifyPad = GStreamer::adoptPad(gst_element_get_static_pad(verifySink, "sink"));
    QVERIFY(verifyPad);
    GStreamer::PadProbe verifyProbe;
    QVERIFY(verifyProbe.add(verifyPad.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &verifiedBuffers));

    QVERIFY(verifyPipeline.play());
    const QString verifyFailure = verifyPipeline.runToEos();
    QVERIFY2(verifyFailure.isEmpty(), qPrintable(verifyFailure));
    QVERIFY2(demuxContext.linked.load(std::memory_order_relaxed), "MP4 has no H.265 video track");
    QVERIFY2(verifiedBuffers.load(std::memory_order_relaxed) >= 2,
             "Finalized MP4 did not decode the two mid-stream H.265 access units");
}

void GStreamerTest::_testBindDebugLevelFactRejectsNullContext()
{
    Fact fact;
    GStreamer::bindDebugLevelFact(&fact, nullptr);
}

void GStreamerTest::_testRuntimeVersionCheck()
{
    guint major, minor, micro, nano;
    gst_version(&major, &minor, &micro, &nano);

    QVERIFY2(major == 1, "Unexpected GStreamer major version");
#if defined(Q_OS_LINUX)
    QVERIFY2(minor >= 20, qPrintable(QStringLiteral("GStreamer runtime version %1.%2.%3 is below minimum 1.20.0")
                                         .arg(major)
                                         .arg(minor)
                                         .arg(micro)));
#else
    QVERIFY2(minor >= 28, qPrintable(QStringLiteral("GStreamer runtime version %1.%2.%3 is below bundled SDK minimum 1.28.0")
                                         .arg(major)
                                         .arg(minor)
                                         .arg(micro)));
#endif

    if (major != GST_VERSION_MAJOR || minor != GST_VERSION_MINOR) {
        qCWarning(GStreamerTestLog) << "GStreamer version mismatch: built against" << GST_VERSION_MAJOR << "."
                                    << GST_VERSION_MINOR << "but runtime is" << major << "." << minor;
    }
}

#else

void GStreamerTest::_testGStreamerDisabled()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(GStreamerTest, TestLabel::Integration)
