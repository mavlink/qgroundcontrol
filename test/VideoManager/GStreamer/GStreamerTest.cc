#include "GStreamerTest.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GStreamerTestLog, "Video.GStreamer.GStreamerTest")

#ifdef QGC_GST_STREAMING

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLoggingCategory>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QStandardPaths>
#include <gst/gst.h>
#include <memory>
#include <vector>

#include "Fixtures/RAIIFixtures.h"
#include "Fact.h"
#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "GstVideoReceiver.h"
#include "LogManager.h"
#include "VideoBackend.h"

void GStreamerTest::init()
{
#if defined(__SANITIZE_ADDRESS__) || (defined(__has_feature) && __has_feature(address_sanitizer))
    QSKIP("GStreamer init deadlocks under AddressSanitizer (bindtextdomain lock)");
#endif

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

    if (!gst_is_initialized()) {
        GStreamer::prepareEnvironment();
        GStreamer::redirectGLibLogging();

        GError* error = nullptr;
        if (!gst_init_check(nullptr, nullptr, &error)) {
            const QString msg = error ? QString::fromUtf8(error->message) : QStringLiteral("unknown error");
            g_clear_error(&error);
            QSKIP(qPrintable(QStringLiteral("GStreamer unavailable: %1").arg(msg)));
        }
    }
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

void GStreamerTest::_testIsHardwareDecoderFactory()
{
    GList* factories = gst_element_factory_list_get_elements(
        static_cast<GstElementFactoryListType>(GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO),
        GST_RANK_NONE);
    if (!factories) {
        QSKIP("No video decoder factories available on this system");
    }

    int total = 0;
    int hwCount = 0;
    int swCount = 0;

    for (GList* node = factories; node != nullptr; node = node->next) {
        GstElementFactory* factory = GST_ELEMENT_FACTORY(node->data);
        QVERIFY(factory != nullptr);

        const gchar* name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
        QVERIFY(name != nullptr);

        if (GStreamer::isHardwareDecoderFactory(factory)) {
            ++hwCount;
        } else {
            ++swCount;
        }
        ++total;
    }

    qCDebug(GStreamerTestLog) << "Decoder factory classification:" << total << "total," << hwCount << "hardware,"
                              << swCount << "software";

    QVERIFY(total > 0);
    QVERIFY(swCount > 0);
    QVERIFY(!GStreamer::isHardwareDecoderFactory(nullptr));

    gst_plugin_feature_list_free(factories);
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

    auto lookup = [registry](const char* featureName) {
        return gst_registry_lookup_feature(registry, featureName);
    };

    GstPluginFeature* software = lookup("avdec_h265");
    GstPluginFeature* d3d11 = lookup("d3d11h265dec");
    GstPluginFeature* d3d12 = lookup("d3d12h265dec");
    if (!software || !d3d11 || !d3d12) {
        if (software) {
            gst_object_unref(software);
        }
        if (d3d11) {
            gst_object_unref(d3d11);
        }
        if (d3d12) {
            gst_object_unref(d3d12);
        }
        QSKIP("Required H.265 software/D3D decoder factories are not installed");
    }

    const guint oldSoftwareRank = gst_plugin_feature_get_rank(software);
    const guint oldD3D11Rank = gst_plugin_feature_get_rank(d3d11);
    const guint oldD3D12Rank = gst_plugin_feature_get_rank(d3d12);
    const auto restoreRanks = qScopeGuard([software, d3d11, d3d12, oldSoftwareRank, oldD3D11Rank, oldD3D12Rank]() {
        gst_plugin_feature_set_rank(software, oldSoftwareRank);
        gst_plugin_feature_set_rank(d3d11, oldD3D11Rank);
        gst_plugin_feature_set_rank(d3d12, oldD3D12Rank);
        gst_object_unref(software);
        gst_object_unref(d3d11);
        gst_object_unref(d3d12);
    });

    const QByteArray oldRhiBackend = qgetenv("QSG_RHI_BACKEND");
    qputenv("QSG_RHI_BACKEND", QByteArray("d3d11"));
    const auto restoreRhiEnv = qScopeGuard([oldRhiBackend]() {
        if (oldRhiBackend.isEmpty()) {
            qunsetenv("QSG_RHI_BACKEND");
        } else {
            qputenv("QSG_RHI_BACKEND", oldRhiBackend);
        }
    });

    gst_plugin_feature_set_rank(software, GST_RANK_PRIMARY + 1);
    gst_plugin_feature_set_rank(d3d11, GST_RANK_MARGINAL);
    gst_plugin_feature_set_rank(d3d12, GST_RANK_PRIMARY + 2);

    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderDefault);

    const guint softwareRank = gst_plugin_feature_get_rank(software);
    const guint d3d11Rank = gst_plugin_feature_get_rank(d3d11);
    const guint d3d12Rank = gst_plugin_feature_get_rank(d3d12);

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

    const QByteArray oldLoggingRules = qgetenv("QT_LOGGING_RULES");
    QLoggingCategory::setFilterRules(QStringLiteral("*.debug=false\nVideo.GStreamer.GStreamerHelpers.debug=true"));
    const auto restoreLoggingRules = qScopeGuard([oldLoggingRules]() {
        QLoggingCategory::setFilterRules(QString::fromUtf8(oldLoggingRules));
    });

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
    const QByteArray oldRhiBackend = qgetenv("QSG_RHI_BACKEND");
    qputenv("QSG_RHI_BACKEND", QByteArray("d3d12"));
    const auto restoreRhiEnv = qScopeGuard([oldRhiBackend]() {
        if (oldRhiBackend.isEmpty()) {
            qunsetenv("QSG_RHI_BACKEND");
        } else {
            qputenv("QSG_RHI_BACKEND", oldRhiBackend);
        }
    });

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

    GList* plugins = gst_registry_get_plugin_list(registry);
    const int pluginCount = g_list_length(plugins);
    gst_plugin_list_free(plugins);
    QVERIFY2(pluginCount > 0, "GStreamer registry contains no plugins at all");

    GstElementFactory* playbinFactory = gst_element_factory_find("playbin");
    QVERIFY2(playbinFactory, "Required factory not found: playbin");
    gst_object_unref(playbinFactory);
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

void GStreamerTest::_testWritePipelineDotReturnsEmptyOnWriteFailure()
{
    if (QStandardPaths::writableLocation(QStandardPaths::CacheLocation).isEmpty()) {
        QSKIP("No writable cache location available");
    }

    const bool hadDumpDir = qEnvironmentVariableIsSet("GST_DEBUG_DUMP_DOT_DIR");
    const QByteArray oldDumpDir = qgetenv("GST_DEBUG_DUMP_DOT_DIR");
    qunsetenv("GST_DEBUG_DUMP_DOT_DIR");
    const auto restoreDumpDir = qScopeGuard([&] {
        if (hadDumpDir) {
            qputenv("GST_DEBUG_DUMP_DOT_DIR", oldDumpDir);
        } else {
            qunsetenv("GST_DEBUG_DUMP_DOT_DIR");
        }
    });

    GstElement* pipeline = gst_pipeline_new("dot-write-failure-test");
    QVERIFY(pipeline);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(pipeline); });

    const QString path = GStreamer::writePipelineDot(pipeline, "missing-dir/pipeline");
    QVERIFY2(path.isEmpty(), qPrintable(QStringLiteral("Expected empty path for failed dot write, got %1").arg(path)));
}

void GStreamerTest::_testCompleteInit()
{
    GStreamer::redirectGLibLogging();
    const bool result = GStreamer::completeInit();
    QVERIFY2(result, "GStreamer::completeInit() failed");

    GstRegistry* registry = gst_registry_get();
    QVERIFY(registry);

    GstPlugin* qgcPlugin = gst_registry_find_plugin(registry, "qgc");
    QVERIFY2(qgcPlugin, "Static plugin 'qgc' not registered after completeInit()");
    gst_clear_object(&qgcPlugin);

    GstElementFactory* sinkFactory = gst_element_factory_find("appsink");
    QVERIFY2(sinkFactory, "Factory 'appsink' not found after completeInit()");
    gst_object_unref(sinkFactory);

    GstElementFactory* binFactory = gst_element_factory_find("qgcvideosinkbin");
    QVERIFY2(binFactory, "Factory 'qgcvideosinkbin' not found after completeInit()");
    gst_object_unref(binFactory);
}

void GStreamerTest::_testCreateVideoReceiver()
{
    std::unique_ptr<VideoReceiver> receiver(GStreamer::createVideoReceiver(nullptr));
    QVERIFY2(receiver, "GStreamer::createVideoReceiver() returned nullptr");
    QVERIFY(qobject_cast<GstVideoReceiver*>(receiver.get()));
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

#ifdef QGC_GST_BUILD_VERSION_MAJOR
    if (major != QGC_GST_BUILD_VERSION_MAJOR || minor != QGC_GST_BUILD_VERSION_MINOR) {
        qCWarning(GStreamerTestLog) << "GStreamer version mismatch: built against" << QGC_GST_BUILD_VERSION_MAJOR << "."
                                    << QGC_GST_BUILD_VERSION_MINOR << "but runtime is" << major << "." << minor;
    }
#endif
}

#else

void GStreamerTest::init()
{
    UnitTest::init();
}

#define QGC_GST_SKIP_TEST(fn) \
    void GStreamerTest::fn()  \
    {                         \
        QSKIP("GStreamer not enabled"); \
    }

QGC_GST_SKIP_TEST(_testIsValidRtspUri)
QGC_GST_SKIP_TEST(_testIsHardwareDecoderFactory)
QGC_GST_SKIP_TEST(_testSetCodecPrioritiesDefault)
QGC_GST_SKIP_TEST(_testSetCodecPrioritiesDefaultPrefersMatchingD3DDecoder)
QGC_GST_SKIP_TEST(_testSetCodecPrioritiesSkipsAbsentD3DDecoders)
QGC_GST_SKIP_TEST(_testSetCodecPrioritiesSoftware)
QGC_GST_SKIP_TEST(_testSetCodecPrioritiesHardware)
QGC_GST_SKIP_TEST(_testRedirectGLibLogging)
QGC_GST_SKIP_TEST(_testConfigureDebugLoggingIsIdempotent)
QGC_GST_SKIP_TEST(_testVerifyRequiredPlugins)
QGC_GST_SKIP_TEST(_testEnvironmentSetup)
QGC_GST_SKIP_TEST(_testWritePipelineDotReturnsEmptyOnWriteFailure)
QGC_GST_SKIP_TEST(_testCompleteInit)
QGC_GST_SKIP_TEST(_testCreateVideoReceiver)
QGC_GST_SKIP_TEST(_testBindDebugLevelFactRejectsNullContext)
QGC_GST_SKIP_TEST(_testRuntimeVersionCheck)
QGC_GST_SKIP_TEST(_testAppsinkFrameDelivery)
QGC_GST_SKIP_TEST(_testAppsinkYuvPassthrough)
QGC_GST_SKIP_TEST(_testAppsinkPtsAndColorimetry)
QGC_GST_SKIP_TEST(_testQgcVideoSinkBinGpuZeroCopyProperty)
QGC_GST_SKIP_TEST(_testQgcVideoSinkBinRejectsFailedAdopt)
QGC_GST_SKIP_TEST(_testGlMemoryDispatch)
QGC_GST_SKIP_TEST(_testDmaBufDispatch)
QGC_GST_SKIP_TEST(_testDmaDrmCapsRejectNonLinearModifiers)
QGC_GST_SKIP_TEST(_testDmaBufRejectsNonLinearDirectImport)
QGC_GST_SKIP_TEST(_testDmaBufTiledImportAvoidsTexStorage)
QGC_GST_SKIP_TEST(_testVulkanDispatchDemotesToCpu)
QGC_GST_SKIP_TEST(_testCapsCacheInvalidation)
QGC_GST_SKIP_TEST(_testGpuZeroCopyFallback)
QGC_GST_SKIP_TEST(_testAppsinkTeardownUnderLoad)
QGC_GST_SKIP_TEST(_testBridgeDispatcherFanout)
QGC_GST_SKIP_TEST(_testHwBufferMapTexturesGuard)
QGC_GST_SKIP_TEST(_testFrameCountsTelemetrySignal)
QGC_GST_SKIP_TEST(_testInactiveQgcQVideoSinkDropsAndCounts)
QGC_GST_SKIP_TEST(_testGetAppsinkAccessor)
QGC_GST_SKIP_TEST(_testQVideoSinkControllerClearsElementOnDestroy)
QGC_GST_SKIP_TEST(_testQVideoSinkControllerClearsElementWhenVideoSinkDestroyed)
QGC_GST_SKIP_TEST(_testQVideoSinkControllerNullSinkStillDeactivatesOnDestroy)
QGC_GST_SKIP_TEST(_testQVideoSinkControllerRepeatedSetupKeepsNewBindingActive)
QGC_GST_SKIP_TEST(_testQVideoSinkControllerNoWindowStartsInactive)
QGC_GST_SKIP_TEST(_testContextBridgeRegistry)
QGC_GST_SKIP_TEST(_testHwBufferLifecycleResetsNativeCaches)
QGC_GST_SKIP_TEST(_testHwBufferFactoryDispatchSystemMemory)
QGC_GST_SKIP_TEST(_testHwBufferFactoryCacheRejectsMemoryTypeChange)
QGC_GST_SKIP_TEST(_testDmaBufSingleFdImportEnvGate)
QGC_GST_SKIP_TEST(_testCpuZeroCopyFrameRejectsWritableMap)
QGC_GST_SKIP_TEST(_testCpuMemcpyActiveRowStrideHandling)
QGC_GST_SKIP_TEST(_testQGCRhiCaptureCacheLifecycle)
QGC_GST_SKIP_TEST(_testColorimetryPixelFormatMapping)
QGC_GST_SKIP_TEST(_testCpuCapsFormatsRoundTripToQt)
QGC_GST_SKIP_TEST(_testAllocationQueryHwMemoryPoolHint)
QGC_GST_SKIP_TEST(_testAllocationQuerySystemMemoryNoPoolStillAdvertisesMetas)
QGC_GST_SKIP_TEST(_testColorimetryColorSpaceMapping)
QGC_GST_SKIP_TEST(_testColorimetryResolutionHeuristicMatchesQt)
QGC_GST_SKIP_TEST(_testColorimetryTransferMapping)
QGC_GST_SKIP_TEST(_testColorimetryFrameRatePropagation)
QGC_GST_SKIP_TEST(_testHwBufferCropMatrixIdentityWithoutMeta)
QGC_GST_SKIP_TEST(_testHwBufferCropMatrixFromVideoCropMeta)
QGC_GST_SKIP_TEST(_testApplyOrientationToFrameMapping)
QGC_GST_SKIP_TEST(_testAdapterFlushDropsInFlightSamples)
QGC_GST_SKIP_TEST(_testSourceFactoryUdpRtpJitterBuffer)
QGC_GST_SKIP_TEST(_testSourceFactoryJitterBufferNone)
QGC_GST_SKIP_TEST(_testSourceFactoryNoRetransmission)
QGC_GST_SKIP_TEST(_testSourceFactoryRtspExcludesStaticJitterBuffer)
QGC_GST_SKIP_TEST(_testSourceFactoryRejectsBadUri)
QGC_GST_SKIP_TEST(_testSourceFactoryTcpMpegTs)
QGC_GST_SKIP_TEST(_testSourceFactoryRejectsBadTcpUri)
QGC_GST_SKIP_TEST(_testSourceFactoryUdp265Caps)
QGC_GST_SKIP_TEST(_testSourceFactoryUdpH264Caps)
QGC_GST_SKIP_TEST(_testSourceFactoryUdpMpegTs)
QGC_GST_SKIP_TEST(_testSourceFactorySchemeCaseInsensitive)
QGC_GST_SKIP_TEST(_testSourceFactoryNegativeLatencyClamped)
QGC_GST_SKIP_TEST(_testSourceFactoryDynamicRtpLinkFailureCleansJitterBuffer)
QGC_GST_SKIP_TEST(_testColorimetryColorRangeMapping)
QGC_GST_SKIP_TEST(_testPixelFormatAcceptedButNotAdvertised)
QGC_GST_SKIP_TEST(_testAdvertisedFormatListMatchesTable)
QGC_GST_SKIP_TEST(_testTelemetryMapDurationEwma)
QGC_GST_SKIP_TEST(_testTelemetryFallbackReasonMatrix)
QGC_GST_SKIP_TEST(_testTelemetrySyncWaitSplit)
QGC_GST_SKIP_TEST(_testTelemetryPathStatsFailuresAreNotDelivered)
QGC_GST_SKIP_TEST(_testTelemetryDmaBufExtraStatsDrain)

#undef QGC_GST_SKIP_TEST
#endif

UT_REGISTER_TEST(GStreamerTest, TestLabel::Integration)
