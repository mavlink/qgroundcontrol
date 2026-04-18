#include "GStreamerTest.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GStreamerTestLog, "VideoManager.GStreamer.GStreamerTest")

#ifdef QGC_GST_STREAMING

#include "Fixtures/RAIIFixtures.h"
#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "GstVideoReceiver.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <gst/gst.h>

#include <iterator>
#include <memory>
#include <vector>

#ifdef Q_OS_MACOS
#include "GstAppSinkAdapter.h"
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#endif

void GStreamerTest::init()
{
    // Skip under AddressSanitizer BEFORE chaining to UnitTest::init(). QSKIP
    // causes Qt to skip the matching cleanup() call, which would leave the
    // UnitTest destructor's _cleanupCalled assertion tripped.
#if defined(__SANITIZE_ADDRESS__) || (defined(__has_feature) && __has_feature(address_sanitizer))
    QSKIP("GStreamer init deadlocks under AddressSanitizer (bindtextdomain lock)");
#endif

    UnitTest::init();

    // In-process plugin scanning (GST_REGISTRY_FORK=no) can trigger harmless
    // GLib critical messages about duplicate type registration.  Whitelist them
    // so the UnitTest cleanup check doesn't treat them as test failures.
    static const QRegularExpression sGLibTypeRe(
        QStringLiteral("cannot register existing type|"
                        "g_type_add_interface_static.*G_TYPE_IS_INSTANTIATABLE|"
                        "g_once_init_leave.*result != 0"));
    expectLogMessage(QtCriticalMsg, sGLibTypeRe);

    if (!gst_is_initialized()) {
        GStreamer::prepareEnvironment();
        GStreamer::redirectGLibLogging();

        GError *error = nullptr;
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
    GList *factories = gst_element_factory_list_get_elements(
        static_cast<GstElementFactoryListType>(GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO),
        GST_RANK_NONE);

    if (!factories) {
        QSKIP("No video decoder factories available on this system");
    }

    int total = 0;
    int hwCount = 0;
    int swCount = 0;

    for (GList *node = factories; node != nullptr; node = node->next) {
        GstElementFactory *factory = GST_ELEMENT_FACTORY(node->data);
        QVERIFY(factory != nullptr);

        const gchar *name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
        QVERIFY(name != nullptr);

        if (GStreamer::isHardwareDecoderFactory(factory)) {
            ++hwCount;
        } else {
            ++swCount;
        }
        ++total;
    }

    qCDebug(GStreamerTestLog) << "Decoder factory classification:" << total << "total,"
                          << hwCount << "hardware," << swCount << "software";

    QVERIFY(total > 0);
    QVERIFY(swCount > 0);
    QVERIFY(!GStreamer::isHardwareDecoderFactory(nullptr));

    gst_plugin_feature_list_free(factories);
}

void GStreamerTest::_testSetCodecPrioritiesDefault()
{
    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderDefault);

    GstRegistry *registry = gst_registry_get();
    QVERIFY(registry != nullptr);
}

void GStreamerTest::_testSetCodecPrioritiesSoftware()
{
    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderSoftware);

    GList *factories = gst_element_factory_list_get_elements(
        static_cast<GstElementFactoryListType>(GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO),
        GST_RANK_NONE);

    if (!factories) {
        QSKIP("No video decoder factories available on this system");
    }

    bool foundPrioritizedSoftware = false;
    for (GList *node = factories; node != nullptr; node = node->next) {
        GstElementFactory *factory = GST_ELEMENT_FACTORY(node->data);
        if (!factory) continue;

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

    GList *factories = gst_element_factory_list_get_elements(
        static_cast<GstElementFactoryListType>(GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO),
        GST_RANK_NONE);

    if (!factories) {
        QSKIP("No video decoder factories available on this system");
    }

    for (GList *node = factories; node != nullptr; node = node->next) {
        GstElementFactory *factory = GST_ELEMENT_FACTORY(node->data);
        if (!factory) continue;

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
    GstRegistry *registry = gst_registry_get();
    QVERIFY(registry != nullptr);

    // coreelements comes from system/bundled GStreamer and should always be
    // available after gst_init_check().
    GstPlugin *corePlugin = gst_registry_find_plugin(registry, "coreelements");
    QVERIFY2(corePlugin, "Required plugin not found: coreelements");
    gst_clear_object(&corePlugin);

    // qml6 and qgc are QGC-built static plugins only registered by
    // _registerPlugins() during GStreamer::completeInit(). They aren't in the
    // registry from a bare gst_init_check(), so just verify the registry
    // loaded *some* plugins and that playbin is available (from system GStreamer).
    GList *plugins = gst_registry_get_plugin_list(registry);
    const int pluginCount = g_list_length(plugins);
    gst_plugin_list_free(plugins);
    QVERIFY2(pluginCount > 0, "GStreamer registry contains no plugins at all");

    GstElementFactory *playbinFactory = gst_element_factory_find("playbin");
    QVERIFY2(playbinFactory, "Required factory not found: playbin");
    gst_object_unref(playbinFactory);
}

void GStreamerTest::_testEnvironmentSetup()
{
    // Save and clear relevant env vars
    static constexpr const char *envVars[] = {
        "GIO_EXTRA_MODULES", "GIO_MODULE_DIR", "GIO_USE_VFS",
        "GST_PTP_HELPER", "GST_PTP_HELPER_1_0",
        "GST_PLUGIN_PATH", "GST_PLUGIN_PATH_1_0",
        "GST_PLUGIN_SYSTEM_PATH", "GST_PLUGIN_SYSTEM_PATH_1_0",
        "GST_PLUGIN_SCANNER", "GST_PLUGIN_SCANNER_1_0",
        "GST_REGISTRY_REUSE_PLUGIN_SCANNER",
        "GTK_PATH",
        "PYTHONHOME", "PYTHONPATH", "PYTHONUSERBASE",
        "VIRTUAL_ENV", "CONDA_PREFIX", "CONDA_DEFAULT_ENV",
        "PYTHONNOUSERSITE",
    };
    std::vector<TestFixtures::EnvVarFixture> envBackups;
    envBackups.reserve(std::size(envVars));
    for (const char *var : envVars) {
        envBackups.emplace_back(var);
        qunsetenv(var);
    }

    GStreamer::prepareEnvironment();

    // On desktop builds with bundled plugins, env vars should be set.
    // On dev builds without bundles, they remain unset (which is correct).
    // Either way, if any plugin path is set, it should point to an existing directory.
    for (const char *var : {"GST_PLUGIN_PATH", "GST_PLUGIN_PATH_1_0",
                            "GST_PLUGIN_SYSTEM_PATH", "GST_PLUGIN_SYSTEM_PATH_1_0"}) {
        if (qEnvironmentVariableIsSet(var)) {
            const QString path = qEnvironmentVariable(var);
            // Paths may be colon-separated; check each component
            const QStringList parts = path.split(QDir::listSeparator(), Qt::SkipEmptyParts);
            for (const QString &part : parts) {
                QVERIFY2(QDir(part).exists(),
                    qPrintable(QStringLiteral("%1 contains non-existent path: %2").arg(var, part)));
            }
        }
    }

    for (const char *var : {"GST_PLUGIN_SCANNER", "GST_PLUGIN_SCANNER_1_0"}) {
        if (qEnvironmentVariableIsSet(var)) {
            const QString path = qEnvironmentVariable(var);
            QVERIFY2(QFileInfo(path).isExecutable(),
                qPrintable(QStringLiteral("%1 is not executable: %2").arg(var, path)));
        }
    }

    // EnvVarFixture destructors auto-restore env vars
}

void GStreamerTest::_testCompleteInit()
{
    GStreamer::redirectGLibLogging();
    const bool result = GStreamer::completeInit();
    QVERIFY2(result, "GStreamer::completeInit() failed");

    GstRegistry *registry = gst_registry_get();
    QVERIFY(registry);

    // qml6 and qgc are custom static plugins registered by completeInit
    GstPlugin *qml6Plugin = gst_registry_find_plugin(registry, "qml6");
    QVERIFY2(qml6Plugin, "Static plugin 'qml6' not registered after completeInit()");
    gst_clear_object(&qml6Plugin);

    GstPlugin *qgcPlugin = gst_registry_find_plugin(registry, "qgc");
    QVERIFY2(qgcPlugin, "Static plugin 'qgc' not registered after completeInit()");
    gst_clear_object(&qgcPlugin);

    // qml6glsink is the critical factory that renders video in QML
    GstElementFactory *sinkFactory = gst_element_factory_find("qml6glsink");
    QVERIFY2(sinkFactory, "Factory 'qml6glsink' not found after completeInit()");
    gst_object_unref(sinkFactory);

    // qgcvideosinkbin is the wrapper bin used by createVideoSink
    GstElementFactory *binFactory = gst_element_factory_find("qgcvideosinkbin");
    QVERIFY2(binFactory, "Factory 'qgcvideosinkbin' not found after completeInit()");
    gst_object_unref(binFactory);
}

void GStreamerTest::_testCreateVideoReceiver()
{
    std::unique_ptr<VideoReceiver> receiver(GStreamer::createVideoReceiver(nullptr));
    QVERIFY2(receiver, "GStreamer::createVideoReceiver() returned nullptr");
    QVERIFY(qobject_cast<GstVideoReceiver*>(receiver.get()));
}

void GStreamerTest::_testPipelineSmokeTest()
{
    GstElement *pipeline = gst_parse_launch("videotestsrc num-buffers=5 ! fakesink", nullptr);
    QVERIFY2(pipeline, "Failed to create videotestsrc ! fakesink pipeline");

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    QVERIFY2(ret != GST_STATE_CHANGE_FAILURE, "Pipeline failed to transition to PLAYING");

    GstBus *bus = gst_element_get_bus(pipeline);
    QVERIFY(bus);

    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
        static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    QVERIFY2(msg, "Pipeline timed out waiting for EOS or ERROR");

    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError *err = nullptr;
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
    QVERIFY2(minor >= 20, qPrintable(QStringLiteral(
        "GStreamer runtime version %1.%2.%3 is below minimum 1.20.0")
        .arg(major).arg(minor).arg(micro)));

#ifdef QGC_GST_BUILD_VERSION_MAJOR
    if (major != QGC_GST_BUILD_VERSION_MAJOR || minor != QGC_GST_BUILD_VERSION_MINOR) {
        qCWarning(GStreamerTestLog) << "GStreamer version mismatch: built against"
            << QGC_GST_BUILD_VERSION_MAJOR << "." << QGC_GST_BUILD_VERSION_MINOR
            << "but runtime is" << major << "." << minor;
    }
#endif
}

void GStreamerTest::_testAppsinkFrameDelivery()
{
#ifndef Q_OS_MACOS
    QSKIP("Appsink frame delivery test is macOS-only");
#else
    // Ensure the qgc plugin (including qgcvideosinkbin) is registered.
    // _testCompleteInit runs before this slot, but guard against reorder.
    GstElementFactory *guardFactory = gst_element_factory_find("qgcvideosinkbin");
    if (!guardFactory) {
        GStreamer::completeInit();
    } else {
        gst_object_unref(guardFactory);
    }

    GstElementFactory *factory = gst_element_factory_find("qgcvideosinkbin");
    QVERIFY2(factory, "qgcvideosinkbin factory not found");
    gst_object_unref(factory);

    // Build pipeline: videotestsrc → videoconvert → qgcvideosinkbin(appsink)
    GError *error = nullptr;
    GstElement *pipeline = gst_parse_launch(
        "videotestsrc num-buffers=10 ! "
        "video/x-raw,format=I420,width=320,height=240,framerate=30/1 ! "
        "videoconvert ! "
        "video/x-raw,format=BGRA ! "
        "qgcvideosinkbin name=sink",
        &error);
    if (error) {
        const QString msg = QString::fromUtf8(error->message);
        g_clear_error(&error);
        QFAIL(qPrintable(QStringLiteral("Pipeline parse error: %1").arg(msg)));
    }
    QVERIFY2(pipeline, "Failed to create appsink test pipeline");

    // Get the sink bin element
    GstElement *sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY2(sinkBin, "Could not find 'sink' element in pipeline");

    // Create a QVideoSink and adapter
    QVideoSink videoSink;
    GstAppSinkAdapter adapter;

    int frameCount = 0;
    QSize lastFrameSize;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &adapter, [&](const QVideoFrame &frame) {
        frameCount++;
        lastFrameSize = frame.size();
    });

    const bool setupOk = adapter.setup(sinkBin, &videoSink);
    QVERIFY2(setupOk, "GstAppSinkAdapter::setup() failed");

    gst_object_unref(sinkBin);

    // Run the pipeline to completion
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    QVERIFY2(ret != GST_STATE_CHANGE_FAILURE, "Pipeline failed to transition to PLAYING");

    GstBus *bus = gst_element_get_bus(pipeline);
    QVERIFY(bus);

    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
        static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    QVERIFY2(msg, "Pipeline timed out waiting for EOS or ERROR");

    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError *err = nullptr;
        gchar *debug = nullptr;
        gst_message_parse_error(msg, &err, &debug);
        const QString errMsg = QStringLiteral("%1 (%2)")
            .arg(err ? QString::fromUtf8(err->message) : QStringLiteral("unknown"))
            .arg(debug ? QString::fromUtf8(debug) : QString());
        g_clear_error(&err);
        g_free(debug);
        gst_message_unref(msg);
        gst_object_unref(bus);
        adapter.teardown();
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        QFAIL(qPrintable(QStringLiteral("Pipeline error: %1").arg(errMsg)));
    }

    QCOMPARE(GST_MESSAGE_TYPE(msg), GST_MESSAGE_EOS);
    gst_message_unref(msg);
    gst_object_unref(bus);

    // Wait for frames to be delivered via queued videoFrameChanged signals
    QTRY_VERIFY_WITH_TIMEOUT(frameCount > 0, TestTimeout::mediumMs());

    // Verify frames have the expected size
    QCOMPARE(lastFrameSize, QSize(320, 240));

    adapter.teardown();
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
#endif // Q_OS_MACOS
}

#else

void GStreamerTest::init() { UnitTest::init(); QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testIsValidRtspUri() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testIsHardwareDecoderFactory() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testSetCodecPrioritiesDefault() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testSetCodecPrioritiesSoftware() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testSetCodecPrioritiesHardware() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testRedirectGLibLogging() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testVerifyRequiredPlugins() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testEnvironmentSetup() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testCompleteInit() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testCreateVideoReceiver() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testPipelineSmokeTest() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testRuntimeVersionCheck() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testAppsinkFrameDelivery() { QSKIP("GStreamer not enabled"); }

#endif

UT_REGISTER_TEST(GStreamerTest, TestLabel::Integration)
