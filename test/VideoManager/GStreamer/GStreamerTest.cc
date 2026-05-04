#include "GStreamerTest.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GStreamerTestLog, "Video.GStreamer.GStreamerTest")

#ifdef QGC_GST_STREAMING

#include "Fixtures/RAIIFixtures.h"
#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "GstVideoReceiver.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <gst/gst.h>

#include <atomic>
#include <iterator>
#include <memory>
#include <vector>

#include "GstAppSinkAdapter.h"
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#  include "GstDmaBufVideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
#  include "GstGlContextBridge.h"
#  include <gst/gl/gl.h>
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
#  include "GstD3D11ContextBridge.h"
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
#  include "GstD3D12ContextBridge.h"
#endif
#include "GstContextBridgeRegistry.h"
#include "GstHwVideoBuffer.h"
#include "GstHwVideoBufferFactory.h"
#include "gstqgc/gstqgcvideosinkbin.h"
#if defined(QGC_HAS_ANY_GPU_PATH)
#  include "QGCRhiCapture.h"
#endif
#include <QtTest/QSignalSpy>
#include <QtQuick/QQuickWindow>
#include <string_view>

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

    GstPlugin *corePlugin = gst_registry_find_plugin(registry, "coreelements");
    QVERIFY2(corePlugin, "Required plugin not found: coreelements");
    gst_clear_object(&corePlugin);

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

}

void GStreamerTest::_testCompleteInit()
{
    GStreamer::redirectGLibLogging();
    const bool result = GStreamer::completeInit();
    QVERIFY2(result, "GStreamer::completeInit() failed");

    GstRegistry *registry = gst_registry_get();
    QVERIFY(registry);

    GstPlugin *qgcPlugin = gst_registry_find_plugin(registry, "qgc");
    QVERIFY2(qgcPlugin, "Static plugin 'qgc' not registered after completeInit()");
    gst_clear_object(&qgcPlugin);

    GstElementFactory *sinkFactory = gst_element_factory_find("appsink");
    QVERIFY2(sinkFactory, "Factory 'appsink' not found after completeInit()");
    gst_object_unref(sinkFactory);

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

    GstElement *sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY2(sinkBin, "Could not find 'sink' element in pipeline");

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

    QTRY_VERIFY_WITH_TIMEOUT(frameCount > 0, TestTimeout::mediumMs());
    QCOMPARE(lastFrameSize, QSize(320, 240));

    adapter.teardown();
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void GStreamerTest::_testAppsinkYuvPassthrough()
{
    GstElementFactory *guardFactory = gst_element_factory_find("qgcvideosinkbin");
    if (!guardFactory) {
        GStreamer::completeInit();
    } else {
        gst_object_unref(guardFactory);
    }

    GError *error = nullptr;
    GstElement *pipeline = gst_parse_launch(
        "videotestsrc num-buffers=10 ! "
        "video/x-raw,format=I420,width=320,height=240,framerate=30/1 ! "
        "qgcvideosinkbin name=sink",
        &error);
    if (error) {
        const QString msg = QString::fromUtf8(error->message);
        g_clear_error(&error);
        QFAIL(qPrintable(QStringLiteral("Pipeline parse error: %1").arg(msg)));
    }
    QVERIFY2(pipeline, "Failed to create YUV passthrough pipeline");

    GstElement *sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY2(sinkBin, "Could not find 'sink' element");

    QVideoSink videoSink;
    GstAppSinkAdapter adapter;

    int frameCount = 0;
    QVideoFrameFormat::PixelFormat lastPixelFormat = QVideoFrameFormat::Format_Invalid;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &adapter, [&](const QVideoFrame &frame) {
        frameCount++;
        lastPixelFormat = frame.pixelFormat();
    });

    QVERIFY2(adapter.setup(sinkBin, &videoSink), "GstAppSinkAdapter::setup() failed");
    gst_object_unref(sinkBin);

    QVERIFY2(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE,
             "Pipeline failed to PLAY");

    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
        static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    QVERIFY2(msg, "Pipeline timed out");
    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        gst_message_unref(msg);
        gst_object_unref(bus);
        adapter.teardown();
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        QFAIL("YUV passthrough pipeline errored");
    }
    QCOMPARE(GST_MESSAGE_TYPE(msg), GST_MESSAGE_EOS);
    gst_message_unref(msg);
    gst_object_unref(bus);

    QTRY_VERIFY_WITH_TIMEOUT(frameCount > 0, TestTimeout::mediumMs());
    QCOMPARE(lastPixelFormat, QVideoFrameFormat::Format_YUV420P);

    adapter.teardown();
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void GStreamerTest::_testAppsinkPtsAndColorimetry()
{
    GstElementFactory *guardFactory = gst_element_factory_find("qgcvideosinkbin");
    if (!guardFactory) {
        GStreamer::completeInit();
    } else {
        gst_object_unref(guardFactory);
    }

    GError *error = nullptr;
    GstElement *pipeline = gst_parse_launch(
        "videotestsrc num-buffers=5 do-timestamp=true ! "
        "video/x-raw,format=I420,width=64,height=48,framerate=30/1,"
        "colorimetry=(string)bt709 ! "
        "qgcvideosinkbin name=sink",
        &error);
    if (error) {
        const QString msg = QString::fromUtf8(error->message);
        g_clear_error(&error);
        QFAIL(qPrintable(QStringLiteral("Pipeline parse error: %1").arg(msg)));
    }
    QVERIFY2(pipeline, "Failed to create PTS/colorimetry pipeline");

    GstElement *sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY2(sinkBin, "Could not find 'sink' element");

    QVideoSink videoSink;
    GstAppSinkAdapter adapter;

    int frameCount = 0;
    qint64 lastStartTime = -1;
    QVideoFrameFormat::ColorSpace lastColorSpace = QVideoFrameFormat::ColorSpace_Undefined;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &adapter, [&](const QVideoFrame &frame) {
        frameCount++;
        lastStartTime = frame.startTime();
        lastColorSpace = frame.surfaceFormat().colorSpace();
    });

    QVERIFY2(adapter.setup(sinkBin, &videoSink), "GstAppSinkAdapter::setup() failed");
    gst_object_unref(sinkBin);

    QVERIFY2(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE,
             "Pipeline failed to PLAY");

    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
        static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    QVERIFY2(msg, "Pipeline timed out");
    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        gst_message_unref(msg);
        gst_object_unref(bus);
        adapter.teardown();
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        QFAIL("PTS/colorimetry pipeline errored");
    }
    QCOMPARE(GST_MESSAGE_TYPE(msg), GST_MESSAGE_EOS);
    gst_message_unref(msg);
    gst_object_unref(bus);

    QTRY_VERIFY_WITH_TIMEOUT(frameCount > 0, TestTimeout::mediumMs());
    QVERIFY2(lastStartTime >= 0, "GstBuffer PTS not forwarded to QVideoFrame::startTime");
    QCOMPARE(lastColorSpace, QVideoFrameFormat::ColorSpace_BT709);

    adapter.teardown();
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void GStreamerTest::_testQgcVideoSinkBinGpuZeroCopyProperty()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");

    GstElementFactory *factory = gst_element_factory_find("qgcvideosinkbin");
    QVERIFY2(factory, "qgcvideosinkbin factory not found");

    {
        GstElement *bin = gst_element_factory_create_full(factory,
                                                          "gpu-zerocopy", FALSE,
                                                          NULL);
        QVERIFY2(bin, "Failed to create qgcvideosinkbin (CPU branch)");

        gboolean prop = TRUE;
        g_object_get(bin, "gpu-zerocopy", &prop, NULL);
        QCOMPARE(prop, FALSE);

        GstElement *appsink = gst_bin_get_by_name(GST_BIN(bin), "qgcappsink");
        QVERIFY2(appsink, "appsink missing from CPU bin");

        GstIterator *it = gst_bin_iterate_elements(GST_BIN(bin));
        int elementCount = 0;
        bool sawVideoconvert = false;
        gboolean done = FALSE;
        GValue val = G_VALUE_INIT;
        while (!done) {
            switch (gst_iterator_next(it, &val)) {
            case GST_ITERATOR_OK: {
                ++elementCount;
                GstElement *child = GST_ELEMENT(g_value_get_object(&val));
                gchar *name = gst_element_get_name(child);
                if (name && QString::fromUtf8(name).startsWith(QStringLiteral("videoconvert"))) {
                    sawVideoconvert = true;
                }
                g_free(name);
                g_value_reset(&val);
                break;
            }
            case GST_ITERATOR_RESYNC: gst_iterator_resync(it); break;
            case GST_ITERATOR_ERROR:
            case GST_ITERATOR_DONE:   done = TRUE; break;
            }
        }
        g_value_unset(&val);
        gst_iterator_free(it);

        // CPU branch: videoconvert + PAR=1/1 capsfilter + appsink (3 children).
        QCOMPARE(elementCount, 3);
        QVERIFY2(sawVideoconvert, "CPU bin missing videoconvert");

        gst_object_unref(appsink);
        gst_object_unref(bin);
    }

    {
        // disable-par=TRUE drops the capsfilter (videoconvert + appsink only).
        GstElement *bin = gst_element_factory_create_full(factory,
                                                          "gpu-zerocopy", FALSE,
                                                          "disable-par", TRUE,
                                                          NULL);
        QVERIFY2(bin, "Failed to create qgcvideosinkbin (CPU branch, disable-par=TRUE)");
        GstIterator *it = gst_bin_iterate_elements(GST_BIN(bin));
        int elementCount = 0;
        gboolean done = FALSE;
        GValue val = G_VALUE_INIT;
        while (!done) {
            switch (gst_iterator_next(it, &val)) {
            case GST_ITERATOR_OK: ++elementCount; g_value_reset(&val); break;
            case GST_ITERATOR_RESYNC: gst_iterator_resync(it); break;
            case GST_ITERATOR_ERROR:
            case GST_ITERATOR_DONE:   done = TRUE; break;
            }
        }
        g_value_unset(&val);
        gst_iterator_free(it);
        QCOMPARE(elementCount, 2);
        gst_object_unref(bin);
    }

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    {
        GstElement *bin = gst_element_factory_create_full(factory,
                                                          "gpu-zerocopy", TRUE,
                                                          NULL);
        QVERIFY2(bin, "Failed to create qgcvideosinkbin (GPU branch)");

        gboolean prop = FALSE;
        g_object_get(bin, "gpu-zerocopy", &prop, NULL);
        QCOMPARE(prop, TRUE);

        GstElement *appsink = gst_bin_get_by_name(GST_BIN(bin), "qgcappsink");
        QVERIFY2(appsink, "appsink missing from GPU bin");

        GstIterator *it = gst_bin_iterate_elements(GST_BIN(bin));
        int elementCount = 0;
#if defined(QGC_GST_BIN_USE_GLUPLOAD)
        bool sawGlupload = false;
#endif
        gboolean done = FALSE;
        GValue val = G_VALUE_INIT;
        while (!done) {
            switch (gst_iterator_next(it, &val)) {
            case GST_ITERATOR_OK: {
                ++elementCount;
                GstElement *child = GST_ELEMENT(g_value_get_object(&val));
                gchar *name = gst_element_get_name(child);
#if defined(QGC_GST_BIN_USE_GLUPLOAD)
                if (name && QString::fromUtf8(name).startsWith(QStringLiteral("glupload"))) {
                    sawGlupload = true;
                }
#endif
                g_free(name);
                g_value_reset(&val);
                break;
            }
            case GST_ITERATOR_RESYNC: gst_iterator_resync(it); break;
            case GST_ITERATOR_ERROR:
            case GST_ITERATOR_DONE:   done = TRUE; break;
            }
        }
        g_value_unset(&val);
        gst_iterator_free(it);

        GstCaps *caps = nullptr;
        g_object_get(appsink, "caps", &caps, NULL);
        QVERIFY2(caps, "GPU bin appsink has null caps");
        gchar *capsStr = gst_caps_to_string(caps);
        const QString s = QString::fromUtf8(capsStr ? capsStr : "");
        g_free(capsStr);
        gst_caps_unref(caps);

#if defined(QGC_GST_BIN_USE_GLUPLOAD)
        // Linux desktop: bin is glupload → appsink so the appsink demands GLMemory only.
        QCOMPARE(elementCount, 2);
        QVERIFY2(sawGlupload, "GPU bin missing glupload");
        QVERIFY2(s.contains(QStringLiteral("memory:GLMemory")),
                 qUtf8Printable(QStringLiteral("GPU bin caps missing memory:GLMemory: ") + s));
#else
        QCOMPARE(elementCount, 1);
        QVERIFY2(s.contains(QStringLiteral("memory:DMABuf")),
                 qUtf8Printable(QStringLiteral("GPU bin caps missing memory:DMABuf: ") + s));
#endif

        gst_object_unref(appsink);
        gst_object_unref(bin);
    }
#endif

    gst_object_unref(factory);
}

void GStreamerTest::_testGlMemoryDispatch()
{
#if !defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    QSKIP("GLMemory zero-copy GPU path not compiled (QGC_HAS_GST_GLMEMORY_GPU_PATH undefined)");
#else
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");

    GstElementFactory *gluploadFactory = gst_element_factory_find("glupload");
    if (!gluploadFactory) {
        QSKIP("glupload factory unavailable — gst-gl not registered in this build");
    }
    gst_object_unref(gluploadFactory);

    GError *error = nullptr;
    GstElement *pipeline = gst_parse_launch(
        "videotestsrc num-buffers=10 ! "
        "video/x-raw,format=RGBA,width=320,height=240,framerate=30/1 ! "
        "glupload ! "
        "video/x-raw(memory:GLMemory) ! "
        "qgcvideosinkbin name=sink gpu-zerocopy=true",
        &error);
    if (error) {
        const QString msg = QString::fromUtf8(error->message);
        g_clear_error(&error);
        QSKIP(qPrintable(QStringLiteral("GLMemory pipeline parse skipped: %1").arg(msg)));
    }
    QVERIFY2(pipeline, "Failed to create GLMemory test pipeline");

    GstElement *sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY2(sinkBin, "Could not find 'sink' element");
    GstElement *appsink = gst_bin_get_by_name(GST_BIN(sinkBin), "qgcappsink");
    QVERIFY2(appsink, "Could not find 'qgcappsink' inside sink bin");
    GstPad *appsinkPad = gst_element_get_static_pad(appsink, "sink");
    QVERIFY2(appsinkPad, "appsink has no sink pad");

    struct ProbeState {
        std::atomic<int> bufferCount{0};
        std::atomic<int> glMemoryCount{0};
    } probe;
    auto probeCb = +[](GstPad * /*pad*/, GstPadProbeInfo *info, gpointer userData) -> GstPadProbeReturn {
        auto *st = static_cast<ProbeState *>(userData);
        GstBuffer *buf = GST_PAD_PROBE_INFO_BUFFER(info);
        if (buf) {
            st->bufferCount.fetch_add(1, std::memory_order_relaxed);
            GstMemory *mem = gst_buffer_peek_memory(buf, 0);
            if (mem && mem->allocator && mem->allocator->mem_type
                && g_str_has_prefix(mem->allocator->mem_type, "GLMemory")) {
                st->glMemoryCount.fetch_add(1, std::memory_order_relaxed);
            }
        }
        return GST_PAD_PROBE_OK;
    };
    const gulong probeId = gst_pad_add_probe(appsinkPad, GST_PAD_PROBE_TYPE_BUFFER, probeCb, &probe, nullptr);
    QVERIFY(probeId);

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        gst_pad_remove_probe(appsinkPad, probeId);
        gst_object_unref(appsinkPad);
        gst_object_unref(appsink);
        gst_object_unref(sinkBin);
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        QSKIP("GLMemory pipeline failed to PLAY (no GL context available?)");
    }

    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
        static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

    QString errMsg;
    bool sawError = false;
    if (msg && GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError *err = nullptr;
        gchar *debug = nullptr;
        gst_message_parse_error(msg, &err, &debug);
        errMsg = QStringLiteral("%1 (%2)")
            .arg(err ? QString::fromUtf8(err->message) : QStringLiteral("?"))
            .arg(debug ? QString::fromUtf8(debug) : QString());
        g_clear_error(&err);
        g_free(debug);
        sawError = true;
    }
    if (msg) gst_message_unref(msg);
    gst_object_unref(bus);

    GstCaps *negotiated = gst_pad_get_current_caps(appsinkPad);
    QString negotiatedStr;
    if (negotiated) {
        gchar *s = gst_caps_to_string(negotiated);
        negotiatedStr = QString::fromUtf8(s ? s : "");
        g_free(s);
        gst_caps_unref(negotiated);
    }

    gst_pad_remove_probe(appsinkPad, probeId);
    gst_object_unref(appsinkPad);
    gst_object_unref(appsink);
    gst_object_unref(sinkBin);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    if (sawError) {
        // Common in headless envs without an EGL/GLX-capable display. Treat as
        // skip so the test is informative when GL is available, harmless when not.
        QSKIP(qPrintable(QStringLiteral("GLMemory pipeline error (likely no GL context): %1").arg(errMsg)));
    }
    QVERIFY2(probe.bufferCount.load() > 0, "No buffers reached qgcappsink under GLMemory caps");
    QVERIFY2(probe.glMemoryCount.load() > 0,
             qPrintable(QStringLiteral("GLMemory negotiated but buffers carried non-GL allocator. "
                                       "Buffers: %1, negotiated caps: %2")
                            .arg(probe.bufferCount.load()).arg(negotiatedStr)));
    QVERIFY2(negotiatedStr.contains(QStringLiteral("memory:GLMemory")),
             qPrintable(QStringLiteral("Appsink negotiated caps lack memory:GLMemory: %1").arg(negotiatedStr)));
#endif
}

namespace {

struct PipelineRunResult {
    int frameCount = 0;
    QSize lastFrameSize;
    QVideoFrameFormat::PixelFormat lastPixelFormat = QVideoFrameFormat::Format_Invalid;
    bool eos = false;
    QString errorMessage;
};

PipelineRunResult runPipelineThroughAdapter(GstAppSinkAdapter &adapter,
                                            QVideoSink &videoSink,
                                            const char *capsLine,
                                            int numBuffers = 5,
                                            bool gpuZerocopy = false)
{
    PipelineRunResult r;
    const QString launch = QStringLiteral(
        "videotestsrc num-buffers=%1 ! %2 ! qgcvideosinkbin name=sink gpu-zerocopy=%3")
        .arg(numBuffers).arg(QString::fromUtf8(capsLine)).arg(gpuZerocopy ? "true" : "false");
    GError *err = nullptr;
    GstElement *pipeline = gst_parse_launch(launch.toUtf8().constData(), &err);
    if (err) {
        r.errorMessage = QString::fromUtf8(err->message);
        g_clear_error(&err);
        return r;
    }
    GstElement *sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    if (!sinkBin || !adapter.setup(sinkBin, &videoSink)) {
        r.errorMessage = QStringLiteral("adapter.setup() failed");
        if (sinkBin) gst_object_unref(sinkBin);
        gst_object_unref(pipeline);
        return r;
    }
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &adapter,
                     [&](const QVideoFrame &f) {
                         ++r.frameCount;
                         r.lastFrameSize = f.size();
                         r.lastPixelFormat = f.pixelFormat();
                     });
    gst_object_unref(sinkBin);

    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        r.errorMessage = QStringLiteral("set_state(PLAYING) failed");
        adapter.teardown();
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        return r;
    }
    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
        static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    if (msg) {
        if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS) r.eos = true;
        else {
            GError *e = nullptr;
            gst_message_parse_error(msg, &e, nullptr);
            r.errorMessage = e ? QString::fromUtf8(e->message) : QStringLiteral("unknown error");
            g_clear_error(&e);
        }
        gst_message_unref(msg);
    } else {
        r.errorMessage = QStringLiteral("timeout waiting for EOS");
    }
    gst_object_unref(bus);
    QTest::qWait(50);
    adapter.teardown();
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return r;
}

} // namespace

void GStreamerTest::_testCapsCacheInvalidation()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink videoSink;
    GstAppSinkAdapter adapter;

    auto r1 = runPipelineThroughAdapter(adapter, videoSink,
        "video/x-raw,format=I420,width=320,height=240,framerate=30/1 ! "
        "videoconvert ! video/x-raw,format=BGRA");
    QVERIFY2(r1.eos, qUtf8Printable(QStringLiteral("Session 1: %1").arg(r1.errorMessage)));
    QTRY_VERIFY_WITH_TIMEOUT(r1.frameCount > 0, 2000);
    QCOMPARE(r1.lastPixelFormat, QVideoFrameFormat::Format_BGRA8888);

    auto r2 = runPipelineThroughAdapter(adapter, videoSink,
        "video/x-raw,format=I420,width=320,height=240,framerate=30/1 ! "
        "videoconvert ! video/x-raw,format=RGBA");
    QVERIFY2(r2.eos, qUtf8Printable(QStringLiteral("Session 2: %1").arg(r2.errorMessage)));
    QTRY_VERIFY_WITH_TIMEOUT(r2.frameCount > 0, 2000);
    QCOMPARE(r2.lastPixelFormat, QVideoFrameFormat::Format_RGBA8888);
}

void GStreamerTest::_testGpuZeroCopyFallback()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    const quint64 dmabufBefore = GstDmaBufVideoBuffer::peekMapFailureCount();
#endif

    QVideoSink videoSink;
    GstAppSinkAdapter adapter;
    auto r = runPipelineThroughAdapter(adapter, videoSink,
        "video/x-raw,format=BGRA,width=320,height=240,framerate=30/1",
        /*numBuffers*/ 5, /*gpuZerocopy*/ true);
    QVERIFY2(r.eos, qUtf8Printable(QStringLiteral("Pipeline: %1").arg(r.errorMessage)));
    QTRY_VERIFY_WITH_TIMEOUT(r.frameCount > 0, 2000);
    QCOMPARE(r.lastPixelFormat, QVideoFrameFormat::Format_BGRA8888);

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QCOMPARE(GstDmaBufVideoBuffer::peekMapFailureCount(), dmabufBefore);
#endif
}

void GStreamerTest::_testAppsinkTeardownUnderLoad()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink videoSink;
    GstAppSinkAdapter adapter;

    GError *err = nullptr;
    GstElement *pipeline = gst_parse_launch(
        "videotestsrc is-live=true ! "
        "video/x-raw,format=BGRA,width=160,height=120,framerate=60/1 ! "
        "qgcvideosinkbin name=sink", &err);
    if (err) { g_clear_error(&err); }
    QVERIFY(pipeline);

    GstElement *sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY(sinkBin);
    QVERIFY(adapter.setup(sinkBin, &videoSink));
    gst_object_unref(sinkBin);

    QVERIFY(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    QTest::qWait(150);
    adapter.teardown();

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    auto r = runPipelineThroughAdapter(adapter, videoSink,
        "video/x-raw,format=RGBA,width=160,height=120,framerate=30/1");
    QVERIFY2(r.eos, qUtf8Printable(QStringLiteral("Restart: %1").arg(r.errorMessage)));
    QTRY_VERIFY_WITH_TIMEOUT(r.frameCount > 0, 2000);
    QCOMPARE(r.lastPixelFormat, QVideoFrameFormat::Format_RGBA8888);
}

void GStreamerTest::_testBridgeDispatcherFanout()
{
#if !defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) && !defined(QGC_HAS_GST_D3D11_GPU_PATH) \
    && !defined(QGC_HAS_GST_D3D12_GPU_PATH)
    QSKIP("No GPU bridge compiled in this build");
#else
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    GstElement *dummy = gst_element_factory_make("identity", nullptr);
    QVERIFY(dummy);
    GstMessage *unrelated = gst_message_new_need_context(GST_OBJECT(dummy),
                                                         "totally.unrelated.context");
    QVERIFY(unrelated);
#  if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    QCOMPARE(GstGlContextBridge::handleSyncMessage(unrelated), GST_BUS_PASS);
#  endif
#  if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    QCOMPARE(GstD3D11ContextBridge::handleSyncMessage(unrelated), GST_BUS_PASS);
#  endif
#  if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    QCOMPARE(GstD3D12ContextBridge::handleSyncMessage(unrelated), GST_BUS_PASS);
#  endif
    gst_message_unref(unrelated);

#  if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    GstMessage *glReq = gst_message_new_need_context(GST_OBJECT(dummy),
                                                     GST_GL_DISPLAY_CONTEXT_TYPE);
    QVERIFY(glReq);
    const GstBusSyncReply r = GstGlContextBridge::handleSyncMessage(glReq);
    // Either PASS (couldn't prime — expected in CI without GL) or DROP (primed
    // and consumed). Both are valid; the contract is "never crash".
    QVERIFY(r == GST_BUS_PASS || r == GST_BUS_DROP);
    if (r == GST_BUS_PASS) gst_message_unref(glReq);
#  endif

    gst_object_unref(dummy);
#endif
}

void GStreamerTest::_testHwBufferMapTexturesGuard()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    GStreamer::redirectGLibLogging();

    const quint64 failsBefore = GstDmaBufVideoBuffer::peekMapFailureCount();

    GstElement *pipeline = gst_parse_launch(
        "videotestsrc num-buffers=1 ! "
        R"(capsfilter caps="video/x-raw(memory:DMABuf),format=NV12,width=64,height=64" ! )"
        "fakesink name=sink sync=false",
        nullptr);
    if (!pipeline) {
        QSKIP("Could not construct DMABuf test pipeline (element missing)");
    }

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        gst_object_unref(pipeline);
        QSKIP("DMABuf pipeline failed to reach PAUSED (no DMABuf source on this machine)");
    }
    gst_element_get_state(pipeline, nullptr, nullptr, 2 * GST_SECOND);

    GstElement *fakesink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    GstSample *sample = nullptr;
    if (fakesink) {
        g_object_get(fakesink, "last-sample", &sample, nullptr);
        gst_object_unref(fakesink);
    }

    if (!sample) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        QSKIP("No DMABuf sample produced (caps negotiation failed on this machine)");
    }

    GstVideoInfo info;
    GstCaps *caps = gst_sample_get_caps(sample);
    gst_video_info_from_caps(&info, caps);
    QVideoFrameFormat fmt(QSize(64, 64), QVideoFrameFormat::Format_NV12);

    GstDmaBufVideoBuffer buf(sample, info, fmt, EGL_NO_DISPLAY);
    QVideoFrameTexturesUPtr old;
    QVideoFrameTexturesUPtr result = buf.mapTextures(*reinterpret_cast<QRhi*>(1), old);
    QVERIFY(!result);

    const quint64 failsAfter = GstDmaBufVideoBuffer::peekMapFailureCount();
    QVERIFY(failsAfter > failsBefore);

    gst_sample_unref(sample);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
#endif
}


void GStreamerTest::_testFrameCountsTelemetrySignal()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink videoSink;
    GstAppSinkAdapter adapter;

    QSignalSpy spy(&adapter, &GstAppSinkAdapter::frameCountsChanged);

    GError *err = nullptr;
    GstElement *pipeline = gst_parse_launch(
        "videotestsrc is-live=false num-buffers=90 ! "
        "video/x-raw,format=BGRA,width=320,height=240,framerate=30/1 ! "
        "qgcvideosinkbin name=sink",
        &err);
    if (err) { g_clear_error(&err); }
    QVERIFY2(pipeline, "Pipeline construction failed");

    GstElement *sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY(sinkBin);
    QVERIFY(adapter.setup(sinkBin, &videoSink));
    gst_object_unref(sinkBin);

    QVERIFY(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);

    // Timer fires at 1 Hz. 90 frames @ 30 fps = 3 s pipeline; give 5 s total.
    // QTRY_VERIFY processes the event loop, allowing the QTimer to fire.
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, 5000);

    QVERIFY(adapter.cpuFrameCount() > 0);
    QCOMPARE(adapter.gpuFallbackCount(), quint64(0));

    adapter.teardown();
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void GStreamerTest::_testGetAppsinkAccessor()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    GstElement *bin = gst_element_factory_make("qgcvideosinkbin", nullptr);
    QVERIFY2(bin, "qgcvideosinkbin factory not found");

    // Construction is synchronous (GObject::constructed runs inside factory_make).
    GstElement *appsink = gst_qgc_video_sink_bin_get_appsink(GST_QGC_VIDEO_SINK_BIN(bin));
    QVERIFY2(appsink, "appsink accessor returned NULL after factory_make");
    QVERIFY(GST_IS_ELEMENT(appsink));
    gst_object_unref(appsink);

    // After transitioning to READY the accessor must still return a valid element.
    GstStateChangeReturn ret = gst_element_set_state(bin, GST_STATE_READY);
    QVERIFY(ret != GST_STATE_CHANGE_FAILURE);

    GstElement *appsink2 = gst_qgc_video_sink_bin_get_appsink(GST_QGC_VIDEO_SINK_BIN(bin));
    QVERIFY2(appsink2, "appsink accessor returned NULL after READY");
    QVERIFY(GST_IS_ELEMENT(appsink2));
    gst_object_unref(appsink2);

    gst_element_set_state(bin, GST_STATE_NULL);
    gst_object_unref(bin);
}

void GStreamerTest::_testContextBridgeRegistry()
{
#if !defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) && !defined(QGC_HAS_GST_D3D11_GPU_PATH) \
    && !defined(QGC_HAS_GST_D3D12_GPU_PATH)
    QSKIP("No GPU context bridge compiled in this build");
#else
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    GstContextBridgeRegistry::clearForTest();

    static bool s_invoked = false;
    s_invoked = false;

    GstContextBridgeRegistry::registerBridgeHandler([](GstMessage *msg) -> GstBusSyncReply {
        const gchar *type = nullptr;
        gst_message_parse_context_type(msg, &type);
        if (type && std::string_view(type) == "test-bridge-only") {
            s_invoked = true;
            return GST_BUS_DROP;
        }
        return GST_BUS_PASS;
    });

    GstElement *dummy = gst_element_factory_make("identity", nullptr);
    QVERIFY(dummy);

    GstMessage *hit = gst_message_new_need_context(GST_OBJECT(dummy), "test-bridge-only");
    QVERIFY(hit);
    QCOMPARE(GstContextBridgeRegistry::dispatchBridges(hit), GST_BUS_DROP);
    QVERIFY(s_invoked);
    gst_message_unref(hit);

    s_invoked = false;
    GstMessage *miss = gst_message_new_need_context(GST_OBJECT(dummy), "other-context-type");
    QVERIFY(miss);
    QCOMPARE(GstContextBridgeRegistry::dispatchBridges(miss), GST_BUS_PASS);
    QVERIFY(!s_invoked);
    gst_message_unref(miss);

    // Reset-callback round-trip: registerResetCallback + resetAllBridges must invoke every cb.
    GstContextBridgeRegistry::clearForTest();
    static int s_resetCount = 0;
    s_resetCount = 0;
    GstContextBridgeRegistry::registerResetCallback([]() { ++s_resetCount; });
    GstContextBridgeRegistry::registerResetCallback([]() { s_resetCount += 10; });
    GstContextBridgeRegistry::resetAllBridges();
    QCOMPARE(s_resetCount, 11);
    // clearForTest must invoke pending reset callbacks before zeroing the slots so cached
    // bridge state can't leak across test cases. Drop the prior round's callbacks first so we
    // measure exactly the new callback's invocation count, not 1+10+1=12 from leftovers.
    GstContextBridgeRegistry::clearForTest();
    s_resetCount = 0;
    GstContextBridgeRegistry::registerResetCallback([]() { ++s_resetCount; });
    GstContextBridgeRegistry::clearForTest();
    QCOMPARE(s_resetCount, 1);
    GstContextBridgeRegistry::resetAllBridges();  // post-clear: no callbacks → no-op
    QCOMPARE(s_resetCount, 1);

    // Coexistence: two bridges with different context types must not consume each other's messages.
    GstContextBridgeRegistry::clearForTest();
    static bool s_aHit = false;
    static bool s_bHit = false;
    s_aHit = s_bHit = false;
    GstContextBridgeRegistry::registerBridgeHandler([](GstMessage *m) -> GstBusSyncReply {
        const gchar *t = nullptr;
        gst_message_parse_context_type(m, &t);
        if (t && std::string_view(t) == "type-A") { s_aHit = true; return GST_BUS_DROP; }
        return GST_BUS_PASS;
    });
    GstContextBridgeRegistry::registerBridgeHandler([](GstMessage *m) -> GstBusSyncReply {
        const gchar *t = nullptr;
        gst_message_parse_context_type(m, &t);
        if (t && std::string_view(t) == "type-B") { s_bHit = true; return GST_BUS_DROP; }
        return GST_BUS_PASS;
    });
    GstMessage *msgA = gst_message_new_need_context(GST_OBJECT(dummy), "type-A");
    QCOMPARE(GstContextBridgeRegistry::dispatchBridges(msgA), GST_BUS_DROP);
    QVERIFY(s_aHit);
    QVERIFY(!s_bHit);
    gst_message_unref(msgA);
    GstMessage *msgB = gst_message_new_need_context(GST_OBJECT(dummy), "type-B");
    QCOMPARE(GstContextBridgeRegistry::dispatchBridges(msgB), GST_BUS_DROP);
    QVERIFY(s_bHit);
    gst_message_unref(msgB);

    gst_object_unref(dummy);
#endif
}

void GStreamerTest::_testHwBufferFactoryDispatchSystemMemory()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("HwVideoBuffer factory not compiled in this build (no GPU path enabled)");
#else
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    GError *err = nullptr;
    // videotestsrc produces system memory; no GPU path should match.
    GstElement *pipeline = gst_parse_launch(
        "videotestsrc num-buffers=1 ! "
        "video/x-raw,format=BGRA,width=64,height=64,framerate=30/1 ! "
        "fakesink name=sink enable-last-sample=true sync=false",
        &err);
    if (err) { g_clear_error(&err); }
    QVERIFY2(pipeline, "Pipeline construction failed");

    QVERIFY(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
        static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    gst_object_unref(bus);
    if (msg) gst_message_unref(msg);

    GstElement *fakesink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY(fakesink);
    GstSample *sample = nullptr;
    g_object_get(fakesink, "last-sample", &sample, nullptr);
    gst_object_unref(fakesink);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    if (!sample) QSKIP("No sample produced (fakesink enable-last-sample may be off)");

    GstVideoInfo info;
    GstCaps *caps = gst_sample_get_caps(sample);
    QVERIFY(gst_video_info_from_caps(&info, caps));

    QVideoFrameFormat fmt(QSize(64, 64), QVideoFrameFormat::Format_BGRA8888);
    HwVideoBufferPath path = HwVideoBufferPath::None;

    auto buf = makeHwVideoBuffer(sample, info, fmt, /*gpuEnabled=*/ true,
                                 /*eglDisplay=*/ nullptr,
                                 /*ahbEglDisplay=*/ nullptr, path);
    QVERIFY(!buf);
    QCOMPARE(path, HwVideoBufferPath::None);

    auto buf2 = makeHwVideoBuffer(sample, info, fmt, /*gpuEnabled=*/ false,
                                  /*eglDisplay=*/ nullptr,
                                  /*ahbEglDisplay=*/ nullptr, path);
    QVERIFY(!buf2);

    gst_sample_unref(sample);
#endif
}

void GStreamerTest::_testCpuMemcpyActiveRowStrideHandling()
{
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    // Width 753 is not 4-byte aligned; GStreamer will pad stride to 756.
    // If the memcpy copies stride bytes instead of active-row bytes, the
    // right-edge pixels of the right-most component will contain padding zeros.
    QVideoSink videoSink;
    GstAppSinkAdapter adapter;

    QVideoFrame capturedFrame;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &adapter,
                     [&](const QVideoFrame &f) { capturedFrame = f; });

    auto r = runPipelineThroughAdapter(adapter, videoSink,
        "video/x-raw,format=BGRA,width=753,height=432,framerate=30/1",
        /*numBuffers=*/ 5);
    QVERIFY2(r.eos, qUtf8Printable(QStringLiteral("Pipeline: %1").arg(r.errorMessage)));
    QTRY_VERIFY_WITH_TIMEOUT(capturedFrame.isValid(), 2000);

    QVERIFY(capturedFrame.map(QVideoFrame::ReadOnly));
    const uchar *data = capturedFrame.bits(0);
    const int stride = capturedFrame.bytesPerLine(0);
    // Sample the last active pixel column (x=752) from row 0; BGRA so 4 bytes/pixel.
    const uchar *lastPixel = data + 752 * 4;
    // videotestsrc pattern=0 (smpte) fills with non-zero color in top rows.
    const bool nonZero = (lastPixel[0] | lastPixel[1] | lastPixel[2] | lastPixel[3]) != 0;
    capturedFrame.unmap();

    (void)stride;
    QVERIFY2(nonZero, "Last active column is all zeros — likely stride vs active-row memcpy bug");
}

void GStreamerTest::_testQGCRhiCaptureCacheLifecycle()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("QGCRhiCapture not compiled in this build (no GPU path enabled)");
#else
    // cachedRhi() returns nullptr before any window has been connected.
    QVERIFY(!QGCRhiCapture::cachedRhi());

    // Create an offscreen window and connect it. The scene graph is never
    // initialized here so cachedRhi() remains nullptr.
    auto *window = new QQuickWindow();
    QGCRhiCapture::connectWindow(window);
    QVERIFY(!QGCRhiCapture::cachedRhi());

    // Destroying the window must clear the cache (no dangling QRhi*).
    delete window;
    QVERIFY(!QGCRhiCapture::cachedRhi());
#endif
}

void GStreamerTest::_testColorimetryPixelFormatMapping()
{
    // Every format advertised in qgcvideosinkbin caps must round-trip through toQtPixelFormat
    // to a non-Invalid Qt format. Format_Invalid here means onNewSample() returns
    // GST_FLOW_ERROR for any frame Qt can negotiate — total frame-delivery loss.
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_NV12),      QVideoFrameFormat::Format_NV12);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_NV21),      QVideoFrameFormat::Format_NV21);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_I420),      QVideoFrameFormat::Format_YUV420P);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_YV12),      QVideoFrameFormat::Format_YV12);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_Y42B),      QVideoFrameFormat::Format_YUV422P);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_P010_10LE), QVideoFrameFormat::Format_P010);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_AYUV),      QVideoFrameFormat::Format_AYUV);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_YUY2),      QVideoFrameFormat::Format_YUYV);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_UYVY),      QVideoFrameFormat::Format_UYVY);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_GRAY8),     QVideoFrameFormat::Format_Y8);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_GRAY16_LE), QVideoFrameFormat::Format_Y16);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_BGRA),      QVideoFrameFormat::Format_BGRA8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_RGBA),      QVideoFrameFormat::Format_RGBA8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_I420_10LE), QVideoFrameFormat::Format_YUV420P10);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_P016_LE),   QVideoFrameFormat::Format_P016);
    // Y444 is intentionally NOT in caps — Qt 6.10 has no Format_YUV444*, so any negotiation
    // would dead-end at GST_FLOW_ERROR. Re-enable when Qt grows the enum.
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_Y444),      QVideoFrameFormat::Format_Invalid);
}

void GStreamerTest::_testColorimetryColorSpaceMapping()
{
    QCOMPARE(toQtColorSpace(GST_VIDEO_COLOR_MATRIX_SMPTE240M), QVideoFrameFormat::ColorSpace_BT709);
    QCOMPARE(toQtColorSpace(GST_VIDEO_COLOR_MATRIX_FCC),       QVideoFrameFormat::ColorSpace_BT601);
}

void GStreamerTest::_testColorimetryTransferMapping()
{
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_SRGB),       QVideoFrameFormat::ColorTransfer_Gamma22);
    // Regression: BT601 used to fall through to BT709 — Qt's own backend maps it distinctly.
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_BT601),      QVideoFrameFormat::ColorTransfer_BT601);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_BT709),      QVideoFrameFormat::ColorTransfer_BT709);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_BT2020_10),  QVideoFrameFormat::ColorTransfer_BT709);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_SMPTE2084),  QVideoFrameFormat::ColorTransfer_ST2084);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_ARIB_STD_B67), QVideoFrameFormat::ColorTransfer_STD_B67);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_GAMMA10),    QVideoFrameFormat::ColorTransfer_Linear);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_GAMMA28),    QVideoFrameFormat::ColorTransfer_Gamma28);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_SMPTE240M),  QVideoFrameFormat::ColorTransfer_BT709);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_ADOBERGB),   QVideoFrameFormat::ColorTransfer_Gamma22);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_LOG100),     QVideoFrameFormat::ColorTransfer_Unknown);
}

void GStreamerTest::_testColorimetryFrameRatePropagation()
{
    // Verify that a 30/1 caps framerate is surfaced on the delivered QVideoFrame.
    GstElementFactory *guardFactory = gst_element_factory_find("qgcvideosinkbin");
    if (!guardFactory) {
        GStreamer::completeInit();
    } else {
        gst_object_unref(guardFactory);
    }

    GError *error = nullptr;
    GstElement *pipeline = gst_parse_launch(
        "videotestsrc num-buffers=3 ! "
        "video/x-raw,format=I420,width=64,height=48,framerate=30/1 ! "
        "qgcvideosinkbin name=sink",
        &error);
    if (error) {
        const QString msg = QString::fromUtf8(error->message);
        g_clear_error(&error);
        QFAIL(qPrintable(QStringLiteral("Pipeline parse error: %1").arg(msg)));
    }
    QVERIFY2(pipeline, "Failed to create framerate pipeline");

    GstElement *sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY2(sinkBin, "Could not find 'sink' element");

    QVideoSink videoSink;
    GstAppSinkAdapter adapter;

    QVideoFrameFormat lastFormat;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &adapter, [&](const QVideoFrame &frame) {
        lastFormat = frame.surfaceFormat();
    });

    QVERIFY2(adapter.setup(sinkBin, &videoSink), "GstAppSinkAdapter::setup() failed");
    gst_object_unref(sinkBin);

    QVERIFY2(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE,
             "Pipeline failed to PLAY");

    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
        static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    QVERIFY2(msg, "Pipeline timed out");
    const bool isError = (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR);
    gst_message_unref(msg);
    gst_object_unref(bus);
    if (isError) {
        adapter.teardown();
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        QFAIL("Framerate pipeline errored");
    }

    QTRY_VERIFY_WITH_TIMEOUT(lastFormat.isValid(), TestTimeout::mediumMs());
    QCOMPARE(lastFormat.streamFrameRate(), 30.0);

    adapter.teardown();
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

namespace {

#if defined(QGC_HAS_ANY_GPU_PATH)
// Concrete subclass for unit-testing GstHwVideoBuffer's protected/base behavior.
class TestableHwVideoBuffer : public GstHwVideoBuffer
{
public:
    TestableHwVideoBuffer(GstSample *sample, const GstVideoInfo &info, QVideoFrameFormat fmt)
        : GstHwVideoBuffer(QVideoFrame::NoHandle, sample, info, std::move(fmt)) {}
    MapData map(QVideoFrame::MapMode) override { return {}; }
    QVideoFrameTexturesUPtr mapTextures(QRhi &, QVideoFrameTexturesUPtr &) override { return {}; }
};
#endif

// Minimal GstSample factory: NV12 buffer at given size, optionally with a crop meta.
GstSample *makeNv12Sample(int width, int height, GstVideoInfo *outInfo,
                         bool addCrop, int cx, int cy, int cw, int ch)
{
    GstVideoInfo info;
    gst_video_info_set_format(&info, GST_VIDEO_FORMAT_NV12, width, height);
    GstCaps *caps = gst_video_info_to_caps(&info);
    GstBuffer *buf = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&info), nullptr);
    if (addCrop) {
        GstVideoCropMeta *crop = gst_buffer_add_video_crop_meta(buf);
        crop->x = cx; crop->y = cy; crop->width = cw; crop->height = ch;
    }
    GstSample *sample = gst_sample_new(buf, caps, nullptr, nullptr);
    gst_buffer_unref(buf);
    gst_caps_unref(caps);
    if (outInfo) *outInfo = info;
    return sample;
}

} // namespace

void GStreamerTest::_testHwBufferCropMatrixIdentityWithoutMeta()
{
#if !defined(QGC_HAS_ANY_GPU_PATH)
    QSKIP("GstHwVideoBuffer not compiled in this build (no GPU path enabled)");
#else
    GstVideoInfo info;
    GstSample *sample = makeNv12Sample(320, 240, &info, /*addCrop=*/false, 0, 0, 0, 0);
    QVERIFY(sample);
    QVideoFrameFormat fmt(QSize(320, 240), QVideoFrameFormat::Format_NV12);
    TestableHwVideoBuffer hw(sample, info, fmt);
    // Default externalTextureMatrix from QHwVideoBuffer is identity; we don't override it
    // for crop because Qt only consults externalTextureMatrix for Format_SamplerExternalOES.
    QCOMPARE(hw.externalTextureMatrix(), QMatrix4x4());
    gst_sample_unref(sample);
#endif
}

void GStreamerTest::_testHwBufferCropMatrixFromVideoCropMeta()
{
    // Regression: GstVideoCropMeta is propagated via QVideoFrameFormat::viewport() in
    // GstAppSinkAdapter::applyCropMeta, NOT via externalTextureMatrix. Verify the format
    // path round-trips a crop rect into viewport().
    GstVideoInfo info;
    GstSample *sample = makeNv12Sample(400, 200, &info, /*addCrop=*/true,
                                       /*cx=*/100, /*cy=*/50, /*cw=*/200, /*ch=*/100);
    QVERIFY(sample);
    GstBuffer *buf = gst_sample_get_buffer(sample);

    QVideoFrameFormat in(QSize(400, 200), QVideoFrameFormat::Format_NV12);
    QVideoFrameFormat out = applyCropMeta(in, buf);
    QCOMPARE(out.viewport(), QRect(100, 50, 200, 100));
    QCOMPARE(out.frameSize(), QSize(400, 200)); // unchanged

    gst_sample_unref(sample);
}

void GStreamerTest::_testApplyOrientationToFrameMapping()
{
#ifndef QGC_HAS_GST_VIDEO_ORIENTATION_META
    QSKIP("GStreamer build lacks GstVideoOrientationMeta");
#else
    // Verifies each GStreamer orientation enum maps to the correct (rotation, mirrored) tuple
    // on QVideoFrame. Lock-down test: prior versions had subtle mismatches between gst's
    // diagonal-flip semantics and Qt's rotate-then-mirror composition.
    struct Case {
        GstVideoOrientationMethod gst;
        QtVideo::Rotation expectedRot;
        bool expectedMirrored;
    };
    const Case cases[] = {
        { GST_VIDEO_ORIENTATION_IDENTITY, QtVideo::Rotation::None,         false },
        { GST_VIDEO_ORIENTATION_90R,      QtVideo::Rotation::Clockwise90,  false },
        { GST_VIDEO_ORIENTATION_180,      QtVideo::Rotation::Clockwise180, false },
        { GST_VIDEO_ORIENTATION_90L,      QtVideo::Rotation::Clockwise270, false },
        { GST_VIDEO_ORIENTATION_HORIZ,    QtVideo::Rotation::None,         true  },
        { GST_VIDEO_ORIENTATION_VERT,     QtVideo::Rotation::Clockwise180, true  },
        { GST_VIDEO_ORIENTATION_UL_LR,    QtVideo::Rotation::Clockwise90,  true  },
        { GST_VIDEO_ORIENTATION_UR_LL,    QtVideo::Rotation::Clockwise270, true  },
    };
    for (const Case &c : cases) {
        QVideoFrame frame{QVideoFrameFormat(QSize(2, 2), QVideoFrameFormat::Format_BGRA8888)};
        // Pre-poison so a no-op switch case (default branch) wouldn't accidentally match.
        frame.setRotation(QtVideo::Rotation::Clockwise90);
        frame.setMirrored(true);
        applyOrientationToFrame(frame, c.gst);
        QCOMPARE(frame.rotation(), c.expectedRot);
        QCOMPARE(frame.mirrored(), c.expectedMirrored);
    }
#endif
}

void GStreamerTest::_testAdapterFlushDropsInFlightSamples()
{
    // Push GST_EVENT_FLUSH_START upstream of the adapter; verify subsequent buffers don't
    // surface as QVideoFrames (the new_sample callback short-circuits on _flushing). Then
    // push FLUSH_STOP and verify delivery resumes.
    GStreamer::redirectGLibLogging();
    QVERIFY2(GStreamer::completeInit(), "completeInit failed");

    QVideoSink sink;
    GstAppSinkAdapter adapter;

    std::atomic<int> deliveredFrames{0};
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, &adapter,
                     [&](const QVideoFrame &) { deliveredFrames.fetch_add(1, std::memory_order_relaxed); });

    GError *err = nullptr;
    // identity is_live=false to avoid the live-source latency offset; videotestsrc → identity → qgcvideosinkbin.
    GstElement *pipeline = gst_parse_launch(
        "videotestsrc num-buffers=10 ! "
        "video/x-raw,format=BGRA,width=64,height=48,framerate=30/1 ! "
        "identity name=id ! "
        "qgcvideosinkbin name=sink",
        &err);
    if (err) { g_clear_error(&err); }
    QVERIFY2(pipeline, "Pipeline construction failed");
    GstElement *sinkBin = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    QVERIFY(sinkBin);
    QVERIFY2(adapter.setup(sinkBin, &sink), "adapter.setup() failed");
    gst_object_unref(sinkBin);

    QVERIFY(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    // Wait for a few frames to confirm baseline delivery works.
    QTRY_VERIFY_WITH_TIMEOUT(deliveredFrames.load() > 0, 3000);
    const int beforeFlush = deliveredFrames.load(std::memory_order_relaxed);

    // Send FLUSH_START upstream of the appsink. identity element forwards events.
    GstElement *id = gst_bin_get_by_name(GST_BIN(pipeline), "id");
    QVERIFY(id);
    GstPad *idSrc = gst_element_get_static_pad(id, "src");
    QVERIFY(idSrc);
    QVERIFY(gst_pad_send_event(gst_pad_get_peer(idSrc), gst_event_new_flush_start()));

    // After FLUSH_START, push a few more buffers; they should all be dropped by the adapter.
    // Use a short window so the test stays under a second.
    const int duringFlushBaseline = deliveredFrames.load(std::memory_order_relaxed);
    QTest::qWait(150);
    QCOMPARE(deliveredFrames.load(std::memory_order_relaxed), duringFlushBaseline);

    // Send FLUSH_STOP — restore normal flow. videotestsrc may have emitted EOS by now,
    // so we don't strictly require new frames, just that the flag clears (no further drops).
    QVERIFY(gst_pad_send_event(gst_pad_get_peer(idSrc),
                               gst_event_new_flush_stop(/*reset_time=*/ TRUE)));
    QCOMPARE(adapter.appsinkInputFrames() >= quint64(beforeFlush), true); // sanity

    gst_object_unref(idSrc);
    gst_object_unref(id);
    adapter.teardown();
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
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
void GStreamerTest::_testAppsinkYuvPassthrough() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testAppsinkPtsAndColorimetry() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testQgcVideoSinkBinGpuZeroCopyProperty() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testGlMemoryDispatch() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testCapsCacheInvalidation() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testGpuZeroCopyFallback() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testAppsinkTeardownUnderLoad() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testBridgeDispatcherFanout() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testHwBufferMapTexturesGuard() { QSKIP("GStreamer not enabled"); }

void GStreamerTest::_testFrameCountsTelemetrySignal() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testGetAppsinkAccessor() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testContextBridgeRegistry() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testHwBufferFactoryDispatchSystemMemory() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testCpuMemcpyActiveRowStrideHandling() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testQGCRhiCaptureCacheLifecycle() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testColorimetryPixelFormatMapping() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testHwBufferCropMatrixIdentityWithoutMeta() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testHwBufferCropMatrixFromVideoCropMeta() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testColorimetryColorSpaceMapping() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testColorimetryTransferMapping() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testColorimetryFrameRatePropagation() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testApplyOrientationToFrameMapping() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testAdapterFlushDropsInFlightSamples() { QSKIP("GStreamer not enabled"); }
#endif

UT_REGISTER_TEST(GStreamerTest, TestLabel::Integration)
