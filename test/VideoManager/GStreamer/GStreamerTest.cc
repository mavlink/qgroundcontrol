#include "GStreamerTest.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GStreamerTestLog, "VideoManager.GStreamer.GStreamerTest")

#ifdef QGC_GST_STREAMING

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>
#include <QtConcurrent/QtConcurrentRun>
#include <QtTest/QSignalSpy>
#include <gst/gst.h>

#include "GStreamer.h"
#include "GStreamerLogging.h"
#include "GstNativeRecorder.h"
#include "GstRemuxPipeline.h"
#include "GstIngestSession.h"
#include "GstStreamDevice.h"
#include "VideoManager/FakeVideoReceiver.h"
#include "VideoFrameDelivery.h"
#include "VideoReceiver.h"
#include "VideoRecorder.h"
#include "VideoRecordingPolicy.h"

namespace {

bool factoryAvailable(const char* factoryName)
{
    GstElementFactory* factory = gst_element_factory_find(factoryName);
    if (!factory)
        return false;
    gst_object_unref(factory);
    return true;
}

}  // namespace

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
    // loaded *some* plugins and that the ingest-session factories are available.
    GList* plugins = gst_registry_get_plugin_list(registry);
    const int pluginCount = g_list_length(plugins);
    gst_plugin_list_free(plugins);
    QVERIFY2(pluginCount > 0, "GStreamer registry contains no plugins at all");

    for (const char* factoryName : {"parsebin", "queue", "h264parse", "h265parse", "mpegtsmux", "appsink"}) {
        GstElementFactory* factory = gst_element_factory_find(factoryName);
        QVERIFY2(factory, qPrintable(QStringLiteral("Required factory not found: %1").arg(factoryName)));
        gst_object_unref(factory);
    }
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

    for (const char* factoryName : {"parsebin", "queue", "h264parse", "h265parse", "mpegtsmux", "appsink"}) {
        GstElementFactory* factory = gst_element_factory_find(factoryName);
        QVERIFY2(factory, qPrintable(QStringLiteral("Factory not found after completeInit(): %1").arg(factoryName)));
        gst_object_unref(factory);
    }
    QVERIFY(GStreamer::isAvailable());
}

void GStreamerTest::_testStreamDeviceFeedsSequentialBytes()
{
    GstStreamDevice device;
    QVERIFY(device.isOpen());
    QVERIFY(device.isSequential());
    QVERIFY(!device.atEnd());

    const QByteArray payload("abcdef");
    QVERIFY(device.append(payload.constData(), payload.size()));
    QCOMPARE(device.bytesAvailable(), payload.size());
    QCOMPARE(device.read(3), QByteArray("abc"));
    QVERIFY(!device.atEnd());

    device.finishStream();
    QCOMPARE(device.readAll(), QByteArray("def"));
    QVERIFY(device.atEnd());
}

void GStreamerTest::_testStreamDeviceMatchesSequentialMediaPlayerContract()
{
    GstStreamDevice device;
    QVERIFY(device.isOpen());
    QVERIFY(device.isSequential());
    QVERIFY(!device.isWritable());
    QVERIFY(device.isReadable());
    expectLogMessage(QtWarningMsg, QRegularExpression(QStringLiteral("Cannot call seek on a sequential device")));
    QVERIFY(!device.seek(0));
    QVERIFY(!device.atEnd());

    QFuture<QByteArray> readFuture = QtConcurrent::run([&device]() {
        return device.read(1);
    });
    QTest::qWait(50);
    QVERIFY(!readFuture.isFinished());

    device.close();
    QTRY_VERIFY_WITH_TIMEOUT(readFuture.isFinished(), 1000);
    QCOMPARE(readFuture.result(), QByteArray());
    QVERIFY(device.atEnd());
}

void GStreamerTest::_testIngestSessionReportsBusError()
{
    GstIngestSession session;
    QSignalSpy errorSpy(&session, &GstIngestSession::errorOccurred);

    GError* error = g_error_new_literal(GST_CORE_ERROR, GST_CORE_ERROR_FAILED, "synthetic session error");
    GstMessage* message = gst_message_new_error(nullptr, error, "synthetic debug detail");
    g_clear_error(&error);

    session.handleBusMessageForTest(message);
    gst_message_unref(message);

    QCOMPARE(errorSpy.count(), 1);
    const QList<QVariant> args = errorSpy.takeFirst();
    QCOMPARE(args.at(0).value<VideoReceiver::ErrorCategory>(), VideoReceiver::ErrorCategory::Fatal);
    QVERIFY(args.at(1).toString().contains(QStringLiteral("synthetic session error")));
}

void GStreamerTest::_testRemuxPipelineReportsMissingPluginMessage()
{
    GstRemuxPipeline pipeline(QStringLiteral("test-remux"));
    QSignalSpy errorSpy(&pipeline, &GstRemuxPipeline::errorOccurred);

    GstStructure* structure = gst_structure_new_empty("missing-plugin");
    gst_structure_set(structure, "type", G_TYPE_STRING, "decoder", nullptr);
    gst_structure_set(structure, "detail", GST_TYPE_CAPS, gst_caps_new_empty_simple("video/x-h999"), nullptr);
    gst_structure_set(structure, "name", G_TYPE_STRING, "Synthetic decoder", nullptr);
    GstMessage* message = gst_message_new_element(nullptr, structure);

    pipeline.handleBusMessageForTest(message);
    gst_message_unref(message);

    QCOMPARE(errorSpy.count(), 1);
    const QList<QVariant> args = errorSpy.takeFirst();
    QCOMPARE(args.at(0).value<VideoReceiver::ErrorCategory>(), VideoReceiver::ErrorCategory::MissingPlugin);
    QVERIFY(args.at(1).toString().contains(QStringLiteral("Missing GStreamer plugin")));
}

void GStreamerTest::_testNativeRecorderSelectedForIngestSources()
{
    const VideoSourceResolver::SourceDescriptor source =
        VideoSourceResolver::describeUri(QStringLiteral("rtsp://127.0.0.1:8554/test"));
    QVERIFY(source.needsIngestSession());

    FakeVideoReceiver receiver;
    VideoFrameDelivery delivery;
    std::unique_ptr<VideoRecorder> recorder =
        VideoRecordingPolicy::createRecorder(source, &receiver, &delivery, nullptr, nullptr, {});

    QVERIFY(recorder);
    const VideoRecorder::Capabilities capabilities = recorder->capabilities();
    QVERIFY(capabilities.lossless);
    QVERIFY(capabilities.description.contains(QStringLiteral("GStreamer")));
    QVERIFY(!capabilities.formats.isEmpty());
}

void GStreamerTest::_testRecordingPolicyReportsSelectedBackend()
{
    FakeVideoReceiver receiver;
    VideoFrameDelivery delivery;

    const VideoSourceResolver::SourceDescriptor ingested =
        VideoSourceResolver::describeUri(QStringLiteral("rtsp://127.0.0.1:8554/test"));
    QCOMPARE(VideoRecordingPolicy::selectBackend(ingested, &receiver, &delivery, {}),
             VideoRecordingPolicy::RecorderBackend::GStreamerNative);

    const VideoSourceResolver::SourceDescriptor direct =
        VideoSourceResolver::describeUri(QStringLiteral("hls://example.test/stream.m3u8"));
    QCOMPARE(VideoRecordingPolicy::selectBackend(direct, &receiver, &delivery, {}),
             VideoRecordingPolicy::RecorderBackend::FrameDelivery);
}

void GStreamerTest::_testNativeRecorderWritesFinitePipeline()
{
    if (!factoryAvailable("x264enc"))
        QSKIP("x264enc unavailable");
    if (!factoryAvailable("matroskamux"))
        QSKIP("matroskamux unavailable");

    const QString pipeline = QStringLiteral(
        "gstreamer-pipeline:videotestsrc num-buffers=5 is-live=false "
        "! video/x-raw,width=160,height=120,framerate=5/1 "
        "! x264enc tune=zerolatency key-int-max=5 "
        "! h264parse");
    const VideoSourceResolver::SourceDescriptor source = VideoSourceResolver::describeUri(pipeline);
    QVERIFY(source.needsIngestSession());

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("native-recording.mkv"));

    GstNativeRecorder recorder(source);
    QSignalSpy startedSpy(&recorder, &VideoRecorder::started);
    QSignalSpy stoppedSpy(&recorder, &VideoRecorder::stopped);
    QSignalSpy errorSpy(&recorder, &VideoRecorder::error);

    QVERIFY(recorder.start(path, QMediaFormat::Matroska));
    QCOMPARE(startedSpy.count(), 1);
    QTRY_COMPARE_WITH_TIMEOUT(stoppedSpy.count(), 1, 5000);
    QCOMPARE(errorSpy.count(), 0);
    QVERIFY(QFileInfo(path).exists());
    QVERIFY(QFileInfo(path).size() > 0);
    QCOMPARE(recorder.state(), VideoRecorder::State::Idle);
}

void GStreamerTest::_testNativeRecorderRejectsPipelineWithoutVideoPad()
{
    if (!factoryAvailable("matroskamux"))
        QSKIP("matroskamux unavailable");

    const VideoSourceResolver::SourceDescriptor source =
        VideoSourceResolver::describeUri(QStringLiteral("gstreamer-pipeline:audiotestsrc num-buffers=1 ! audioconvert"));
    QVERIFY(source.needsIngestSession());

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("no-video.mkv"));

    GstNativeRecorder recorder(source);
    QSignalSpy startedSpy(&recorder, &VideoRecorder::started);
    QSignalSpy stoppedSpy(&recorder, &VideoRecorder::stopped);
    QSignalSpy errorSpy(&recorder, &VideoRecorder::error);

    QVERIFY(recorder.start(path, QMediaFormat::Matroska));
    QTRY_COMPARE_WITH_TIMEOUT(errorSpy.count(), 1, 5000);
    QCOMPARE(startedSpy.count(), 0);
    QCOMPARE(stoppedSpy.count(), 0);
    QVERIFY(!QFileInfo(path).exists() || QFileInfo(path).size() == 0);
    QCOMPARE(recorder.state(), VideoRecorder::State::Idle);
}

void GStreamerTest::_testNativeRecorderReportsMissingPluginMessage()
{
    const VideoSourceResolver::SourceDescriptor source =
        VideoSourceResolver::describeUri(QStringLiteral("rtsp://127.0.0.1:8554/test"));
    GstNativeRecorder recorder(source);
    QSignalSpy errorSpy(&recorder, &VideoRecorder::error);
    QSignalSpy stoppedSpy(&recorder, &VideoRecorder::stopped);

    GstStructure* structure = gst_structure_new_empty("missing-plugin");
    gst_structure_set(structure, "type", G_TYPE_STRING, "decoder", nullptr);
    gst_structure_set(structure, "detail", GST_TYPE_CAPS, gst_caps_new_empty_simple("video/x-h999"), nullptr);
    gst_structure_set(structure, "name", G_TYPE_STRING, "Synthetic decoder", nullptr);
    GstMessage* message = gst_message_new_element(nullptr, structure);

    recorder.handleBusMessageForTest(message);
    gst_message_unref(message);

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(stoppedSpy.count(), 0);
    QVERIFY(errorSpy.takeFirst().at(0).toString().contains(QStringLiteral("Missing GStreamer plugin")));
}

void GStreamerTest::_testIngestSessionCanRecordSharedIngest()
{
    if (!factoryAvailable("x264enc"))
        QSKIP("x264enc unavailable");
    if (!factoryAvailable("matroskamux"))
        QSKIP("matroskamux unavailable");

    const QString pipeline = QStringLiteral(
        "gstreamer-pipeline:videotestsrc num-buffers=10 is-live=false "
        "! video/x-raw,width=160,height=120,framerate=5/1 "
        "! x264enc tune=zerolatency key-int-max=5 "
        "! h264parse");
    const VideoSourceResolver::SourceDescriptor source = VideoSourceResolver::describeUri(pipeline);
    QVERIFY(source.needsIngestSession());

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("shared-ingest-recording.mkv"));

    GstIngestSession session;
    QSignalSpy eosSpy(&session, &GstIngestSession::endOfStream);
    QSignalSpy errorSpy(&session, &GstIngestSession::errorOccurred);

    QVERIFY(session.start(source, false));
    QVERIFY(session.running());
    QVERIFY(session.startRecording(path, QMediaFormat::Matroska));
    QVERIFY(session.isRecording());

    QTRY_VERIFY_WITH_TIMEOUT(eosSpy.count() > 0 || errorSpy.count() > 0, 5000);
    QCOMPARE(errorSpy.count(), 0);

    session.stopRecording();
    QVERIFY(!session.isRecording());
    QVERIFY(QFileInfo(path).exists());
    QVERIFY(QFileInfo(path).size() > 0);
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

#else

void GStreamerTest::init()
{
    UnitTest::init();
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

void GStreamerTest::_testIngestSessionReportsBusError()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testRemuxPipelineReportsMissingPluginMessage()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testNativeRecorderSelectedForIngestSources()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testRecordingPolicyReportsSelectedBackend()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testNativeRecorderWritesFinitePipeline()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testNativeRecorderRejectsPipelineWithoutVideoPad()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testNativeRecorderReportsMissingPluginMessage()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerTest::_testIngestSessionCanRecordSharedIngest()
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

void GStreamerTest::_testGray8PipelineEndToEnd()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(GStreamerTest, TestLabel::Integration)
