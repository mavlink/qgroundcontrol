#include "GStreamerTest.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GStreamerTestLog, "VideoManager.GStreamer.GStreamerTest")

#ifdef QGC_GST_STREAMING

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <gst/gst.h>

#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "GstFormatTable.h"
#include "GstVideoReceiver.h"

#ifdef Q_OS_MACOS
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#endif

void GStreamerTest::init()
{
    UnitTest::init();

    // In-process plugin scanning (GST_REGISTRY_FORK=no) can trigger harmless
    // GLib critical messages about duplicate type registration.  Whitelist them
    // so the UnitTest cleanup check doesn't treat them as test failures.
    static const QRegularExpression sGLibTypeRe(
        QStringLiteral("cannot register existing type|"
                       "g_type_add_interface_static.*G_TYPE_IS_INSTANTIATABLE|"
                       "g_once_init_leave.*result != 0"));
    expectLogMessage(QtCriticalMsg, sGLibTypeRe);

#if defined(__SANITIZE_ADDRESS__) || (defined(__has_feature) && __has_feature(address_sanitizer))
    QSKIP("GStreamer init deadlocks under AddressSanitizer (bindtextdomain lock)");
#endif

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

    qCDebug(GStreamerTestLog) << "Decoder factory classification:" << total << "total," << hwCount << "hardware," << swCount
                              << "software";

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

    g_log("TestDomain", G_LOG_LEVEL_DEBUG, "GStreamerTest debug message");
    g_log("TestDomain", G_LOG_LEVEL_WARNING, "GStreamerTest warning message");
}

void GStreamerTest::_testVerifyRequiredPlugins()
{
    GstRegistry* registry = gst_registry_get();
    QVERIFY(registry != nullptr);

    // coreelements comes from system/bundled GStreamer and should always be
    // available after gst_init_check().
    GstPlugin* corePlugin = gst_registry_find_plugin(registry, "coreelements");
    QVERIFY2(corePlugin, "Required plugin not found: coreelements");
    gst_clear_object(&corePlugin);

    // qml6 and qgc are QGC-built static plugins only registered by
    // _registerPlugins() during GStreamer::completeInit(). They aren't in the
    // registry from a bare gst_init_check(), so just verify the registry
    // loaded *some* plugins and that playbin is available (from system GStreamer).
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
    struct EnvBackup
    {
        const char* name;
        QByteArray value;
        bool wasSet;
    };

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
    QList<EnvBackup> backups;
    for (const char* var : envVars) {
        backups.append({var, qgetenv(var), qEnvironmentVariableIsSet(var)});
        qunsetenv(var);
    }

    GStreamer::prepareEnvironment();

    // On desktop builds with bundled plugins, env vars should be set.
    // On dev builds without bundles, they remain unset (which is correct).
    // Either way, if any plugin path is set, it should point to an existing directory.
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

    // Restore env vars
    for (const EnvBackup& backup : backups) {
        if (backup.wasSet) {
            qputenv(backup.name, backup.value);
        } else {
            qunsetenv(backup.name);
        }
    }
}

void GStreamerTest::_testCompleteInit()
{
    GStreamer::redirectGLibLogging();
    const bool result = GStreamer::completeInit();
    QVERIFY2(result, "GStreamer::completeInit() failed");

    GstRegistry* registry = gst_registry_get();
    QVERIFY(registry);

    // The current renderer is QGC-owned appsink -> QVideoFrame delivery, not
    // the old qgc/qgcvideosinkbin static plugin path.
    GstElementFactory* appsinkFactory = gst_element_factory_find("appsink");
    QVERIFY2(appsinkFactory, "Factory 'appsink' not found after completeInit()");
    gst_object_unref(appsinkFactory);

    GstElementFactory* playbinFactory = gst_element_factory_find("playbin");
    QVERIFY2(playbinFactory, "Factory 'playbin' not found after completeInit()");
    gst_object_unref(playbinFactory);
}

void GStreamerTest::_testCreateVideoReceiver()
{
    VideoReceiver* receiver = GStreamer::createVideoReceiver(nullptr);
    QVERIFY2(receiver, "GStreamer::createVideoReceiver() returned nullptr");
    QVERIFY(qobject_cast<GstVideoReceiver*>(receiver));
    delete receiver;
}

void GStreamerTest::_testPipelineSmokeTest()
{
    GstElement* pipeline = gst_parse_launch("videotestsrc num-buffers=5 ! fakesink", nullptr);
    QVERIFY2(pipeline, "Failed to create videotestsrc ! fakesink pipeline");

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    QVERIFY2(ret != GST_STATE_CHANGE_FAILURE, "Pipeline failed to transition to PLAYING");

    GstBus* bus = gst_element_get_bus(pipeline);
    QVERIFY(bus);

    GstMessage* msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
                                                 static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    QVERIFY2(msg, "Pipeline timed out waiting for EOS or ERROR");

    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError* err = nullptr;
        gst_message_parse_error(msg, &err, nullptr);
        const QString errMsg = err ? QString::fromUtf8(err->message) : QStringLiteral("unknown");
        g_clear_error(&err);
        gst_message_unref(msg);
        gst_object_unref(bus);
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        QFAIL(qPrintable(QStringLiteral("Pipeline error: %1").arg(errMsg)));
    }

    QCOMPARE(GST_MESSAGE_TYPE(msg), GST_MESSAGE_EOS);
    gst_message_unref(msg);
    gst_object_unref(bus);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void GStreamerTest::_testRuntimeVersionCheck()
{
    guint major, minor, micro, nano;
    gst_version(&major, &minor, &micro, &nano);

    QVERIFY2(major == 1, "Unexpected GStreamer major version");
    QVERIFY2(minor >= 20, qPrintable(QStringLiteral("GStreamer runtime version %1.%2.%3 is below minimum 1.20.0")
                                         .arg(major)
                                         .arg(minor)
                                         .arg(micro)));

#ifdef QGC_GST_BUILD_VERSION_MAJOR
    if (major != QGC_GST_BUILD_VERSION_MAJOR || minor != QGC_GST_BUILD_VERSION_MINOR) {
        qCWarning(GStreamerTestLog) << "GStreamer version mismatch: built against" << QGC_GST_BUILD_VERSION_MAJOR << "."
                                    << QGC_GST_BUILD_VERSION_MINOR << "but runtime is" << major << "." << minor;
    }
#endif
}

void GStreamerTest::_testAppsinkFrameDelivery()
{
    // Superseded by VideoManagerIntegrationTest::_testFrameDeliveryReachesSinkAndBridge
    // after the GstAppSinkAdapter → GstAppsinkBridge refactor. The new test
    // exercises the full appsink → bridge → QVideoSink path end-to-end via
    // FakeVideoReceiver, without requiring a real GStreamer pipeline.
    QSKIP("Superseded by VideoManagerIntegrationTest::_testFrameDeliveryReachesSinkAndBridge");
}

void GStreamerTest::_testGray8FormatMapping()
{
    // Verify GRAY8 → Y8 mapping in format table
    const auto qtFmt = GstFormatTable::gstFormatToQt(GST_VIDEO_FORMAT_GRAY8);
    QCOMPARE(qtFmt, QVideoFrameFormat::Format_Y8);

    // Reverse mapping
    const auto gstFmt = GstFormatTable::qtFormatToGst(QVideoFrameFormat::Format_Y8);
    QCOMPARE(gstFmt, GST_VIDEO_FORMAT_GRAY8);

    // Verify GRAY8 appears in CPU caps
    const QByteArray cpuCaps = GstFormatTable::cpuCapsFormats();
    QVERIFY2(cpuCaps.contains("GRAY8"), "GRAY8 not in CPU caps");
}

void GStreamerTest::_testGray16FormatMapping()
{
    // Verify GRAY16 → Y16 mapping (endian-dependent)
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    const auto qtFmt = GstFormatTable::gstFormatToQt(GST_VIDEO_FORMAT_GRAY16_LE);
    QCOMPARE(qtFmt, QVideoFrameFormat::Format_Y16);
#else
    const auto qtFmt = GstFormatTable::gstFormatToQt(GST_VIDEO_FORMAT_GRAY16_BE);
    QCOMPARE(qtFmt, QVideoFrameFormat::Format_Y16);
#endif

    // Reverse mapping
    const auto gstFmt = GstFormatTable::qtFormatToGst(QVideoFrameFormat::Format_Y16);
    QVERIFY2(gstFmt != GST_VIDEO_FORMAT_UNKNOWN, "Y16 has no GStreamer reverse mapping");
}

void GStreamerTest::_testGray8PipelineEndToEnd()
{
    // End-to-end: videotestsrc → GRAY8 → fakesink verifies the format
    // is accepted through the entire GStreamer pipeline negotiation.
    GstElement* pipeline = gst_parse_launch(
        "videotestsrc num-buffers=3 ! video/x-raw,format=GRAY8,width=160,height=120 ! fakesink", nullptr);
    QVERIFY2(pipeline, "Failed to create GRAY8 test pipeline");

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    QVERIFY2(ret != GST_STATE_CHANGE_FAILURE, "GRAY8 pipeline failed to start");

    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
                                                 static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    QVERIFY2(msg, "GRAY8 pipeline timed out");

    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError* err = nullptr;
        gst_message_parse_error(msg, &err, nullptr);
        const QString errMsg = err ? QString::fromUtf8(err->message) : QStringLiteral("unknown");
        g_clear_error(&err);
        gst_message_unref(msg);
        gst_object_unref(bus);
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        QFAIL(qPrintable(QStringLiteral("GRAY8 pipeline error: %1").arg(errMsg)));
    }

    QCOMPARE(GST_MESSAGE_TYPE(msg), GST_MESSAGE_EOS);
    gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void GStreamerTest::_testDmaBufDrmCapsWellFormed()
{
#ifndef QGC_GST_DMABUF
    QSKIP("DMA-BUF support not compiled in (QGC_GST_DMABUF undefined)");
#else
    // The GStreamer 1.24+ DRM caps form (format=DMA_DRM + drm-format=<fmt>:<modifier>)
    // must parse cleanly and restrict modifier to 0 (linear). Non-linear modifiers
    // mean GPU-tiled memory which the CPU-mmap path in GstDmaBufVideoBuffer can't
    // handle — verify our caps advertise modifier 0 explicitly so tiled decoders
    // are forced to downgrade to legacy DMABuf / CPU caps.
    const QByteArray drmCaps = GstFormatTable::dmaBufDrmCapsFormats();
    QVERIFY2(drmCaps.contains("format=DMA_DRM"), "DMA_DRM format tag missing");
    QVERIFY2(drmCaps.contains("drm-format"), "drm-format field missing");
    QVERIFY2(drmCaps.contains(":0"), "modifier 0 (linear) missing — non-linear modifier leaked");
    QVERIFY2(!drmCaps.contains(":0x"), "unexpected non-linear modifier in caps");

    GstCaps* caps = gst_caps_from_string(drmCaps.constData());
    QVERIFY2(caps != nullptr, "gst_caps_from_string failed on dmaBufDrmCapsFormats()");
    QVERIFY2(gst_caps_get_size(caps) >= 1, "DMA_DRM caps has no structures");
    gst_caps_unref(caps);

    // Sanity: the full appsink caps advertisement (DRM + legacy + CPU) parses too.
    const QByteArray fullCaps =
        GstFormatTable::dmaBufDrmCapsFormats() + ';' +
        GstFormatTable::dmaBufCapsFormats() + ';' +
        GstFormatTable::cpuCapsFormats();
    GstCaps* full = gst_caps_from_string(fullCaps.constData());
    QVERIFY2(full != nullptr, "gst_caps_from_string failed on composed caps");
    QVERIFY2(gst_caps_get_size(full) >= 3, "composed caps missing structures (DRM + legacy + CPU)");
    gst_caps_unref(full);
#endif
}

#else

void GStreamerTest::init()
{
    UnitTest::init();
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testIsValidRtspUri()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testIsHardwareDecoderFactory()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testSetCodecPrioritiesDefault()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testSetCodecPrioritiesSoftware()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testSetCodecPrioritiesHardware()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testRedirectGLibLogging()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testVerifyRequiredPlugins()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testEnvironmentSetup()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testCompleteInit()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testCreateVideoReceiver()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testPipelineSmokeTest()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testRuntimeVersionCheck()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testAppsinkFrameDelivery()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testGray8FormatMapping()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testGray16FormatMapping()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testGray8PipelineEndToEnd()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(GStreamerTest, TestLabel::Integration)
