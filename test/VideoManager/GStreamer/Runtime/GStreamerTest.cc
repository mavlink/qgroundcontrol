#include "GStreamerTest.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GStreamerTestLog, "Video.GStreamer.GStreamerTest")

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>
#include <memory>

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QVersionNumber>

#include "Fixtures/RAIIFixtures.h"
#include "GStreamer.h"
#include "GStreamerDecoderRanks.h"
#include "GStreamerHelpers.h"
#include "GStreamerTestHelpers.h"
#include "GStreamerVideoBackend.h"
#include "GstVideoReceiver.h"
#include "PipelineDotDumper.h"
#include "QtMultimediaVideoReceiver.h"
#include "RecordingBranch.h"
#include "SettingsManager.h"
#include "VideoBackend.h"
#include "VideoSettings.h"

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

    QTest::newRow("nvidia") << QStringLiteral("nvh264dec") << int(VideoDecoderOption::NVIDIA);
    QTest::newRow("va") << QStringLiteral("vah265dec") << int(VideoDecoderOption::VAAPI);
    // Legacy gstreamer-vaapi is demoted and never bumped by the VAAPI option.
    QTest::newRow("legacy-vaapi") << QStringLiteral("vaapih264dec") << int(VideoDecoderOption::Default);
    QTest::newRow("qsv") << QStringLiteral("qsvh265dec") << int(VideoDecoderOption::Intel);
    QTest::newRow("msdk") << QStringLiteral("msdkh264dec") << int(VideoDecoderOption::Intel);
    QTest::newRow("d3d11") << QStringLiteral("d3d11h264dec") << int(VideoDecoderOption::DirectX3D);
    QTest::newRow("d3d12") << QStringLiteral("d3d12h265dec") << int(VideoDecoderOption::DirectX3D);
    QTest::newRow("videotoolbox") << QStringLiteral("vtdec_hw") << int(VideoDecoderOption::VideoToolbox);
    // GStreamer 1.30 has one VideoToolbox decoder per codec.
    QTest::newRow("videotoolbox-per-codec") << QStringLiteral("vth264dec_hw") << int(VideoDecoderOption::VideoToolbox);
    QTest::newRow("vulkan") << QStringLiteral("vulkanh264dec") << int(VideoDecoderOption::Vulkan);
    QTest::newRow("software-with-nv") << QStringLiteral("avdec_wnv1") << int(VideoDecoderOption::Default);
    QTest::newRow("android-mediacodec") << QStringLiteral("amcviddec-c2qtiavcdecoder")
                                        << int(VideoDecoderOption::Default);
}

void GStreamerTest::_testDecoderFamilyForFactory()
{
    QFETCH(QString, factoryName);
    QFETCH(int, expectedFamily);

    QCOMPARE(int(GStreamer::decoderFamilyForFactory(factoryName.toUtf8().constData())), expectedFamily);
}

void GStreamerTest::_testAvailableDecoderFamilies()
{
    QList<VideoDecoderOption> installed;
    GStreamer::forEachVideoDecoderFactory(GST_RANK_NONE, [&installed](GstElementFactory* factory) {
        const auto family =
            GStreamer::decoderFamilyForFactory(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)));
        if ((family != VideoDecoderOption::Default) && !installed.contains(family)) {
            installed.append(family);
        }
    });
#ifndef QGC_HAS_GST_VULKAN_GPU_PATH
    // Without the Vulkan GPU path no build has a use for Vulkan decoders: no sink takes their VulkanImage memory.
    installed.removeAll(VideoDecoderOption::Vulkan);
#endif
    QCOMPARE(GStreamer::availableDecoderFamilies(), installed);

    // The decoder setting offers a hardware family where a decoder of it is installed, and nowhere else: no platform
    // has a list of its own.
    VideoSettings* const settings = SettingsManager::instance()->videoSettings();
    settings->pruneUnavailableDecoders();
    const QVariantList offered = settings->forceVideoDecoder()->enumValues();
    for (const VideoDecoderOption family :
         {VideoDecoderOption::NVIDIA, VideoDecoderOption::VAAPI, VideoDecoderOption::DirectX3D,
          VideoDecoderOption::VideoToolbox, VideoDecoderOption::Intel, VideoDecoderOption::Vulkan}) {
        QCOMPARE(offered.contains(QVariant(static_cast<int>(family))), installed.contains(family));
    }
}

void GStreamerTest::_testStoredDecoderOption_data()
{
    QTest::addColumn<int>("stored");
    QTest::addColumn<int>("option");

    QTest::newRow("first") << 0 << int(VideoDecoderOption::Default);
    QTest::newRow("software") << 1 << int(VideoDecoderOption::Software);
    QTest::newRow("last") << 8 << int(VideoDecoderOption::Hardware);
    // A stored value that names no option is the default.
    QTest::newRow("negative") << -1 << int(VideoDecoderOption::Default);
    QTest::newRow("past the last") << 9 << int(VideoDecoderOption::Default);
}

void GStreamerTest::_testStoredDecoderOption()
{
    QFETCH(int, stored);
    QFETCH(int, option);

    QCOMPARE(int(videoDecoderOptionFromSetting(stored)), option);
}

void GStreamerTest::_testDecoderPriorityWarnings()
{
    const auto restoreRanks = qScopeGuard([] { GStreamer::setCodecPriorities(VideoDecoderOption::Default); });

    // Vulkan decoders are ranked first, though the video sink takes none of their frames.
    expectLogMessage("Video.GStreamer.DecoderRanks", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Forcing Vulkan video decoders")));
    GStreamer::setCodecPriorities(VideoDecoderOption::Vulkan);
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
    constexpr auto kSecretLocation = "/tmp/qgc-dot-secret-token";
    GstTest::TestPipeline pipeline;
    const QString description = QStringLiteral("filesrc name=source location=%1 ! fakesink name=sink");
    QVERIFY2(pipeline.launch(description.arg(QLatin1String(kSecretLocation))), qPrintable(pipeline.error()));

    const GStreamer::GCharPtr dotData =
        GStreamer::adoptGChar(gst_debug_bin_to_dot_data(pipeline.bin(), PipelineDotDumper::kDiagnosticDotGraphDetails));
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

void GStreamerTest::_testCompleteInitNamesBlacklistedPlugins()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("tcpclientsrc");
    GstRegistry* const registry = gst_registry_get();

    // A feature's element is missing, as when its plugin could not be loaded: GStreamer keeps such a file in the
    // registry as a plugin with the blacklisted flag.
    const GStreamer::GstFeaturePtr feature =
        GStreamer::adoptFeature(gst_registry_lookup_feature(registry, "tcpclientsrc"));
    QVERIFY(feature);
    auto* const blacklisted = GST_PLUGIN(g_object_new(GST_TYPE_PLUGIN, nullptr));
    GST_OBJECT_FLAG_SET(blacklisted, GST_PLUGIN_FLAG_BLACKLISTED);
    gst_registry_remove_feature(registry, feature.get());
    const bool added = gst_registry_add_plugin(registry, blacklisted);
    const auto restore = qScopeGuard([registry, &feature, blacklisted, added] {
        if (added) {
            gst_registry_remove_plugin(registry, blacklisted);
        }
        (void) gst_registry_add_feature(registry, feature.get());
    });
    QVERIFY(added);

    expectLogMessage("Video.GStreamer.GStreamer", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^TCP video unavailable.*tcpclientsrc")));
    // The fake plugin has no file; the host's own blacklisted plugins have one.
    ignoreLogMessage("Video.GStreamer.GStreamer", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Blacklisted plugin: .*file: [^(]")));
    expectLogMessage("Video.GStreamer.GStreamer", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Blacklisted plugin: .*file: \\(null\\)")));
    QVERIFY(GStreamer::completeInit());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void GStreamerTest::_testVideoBackendForwards()
{
    GStreamerVideoBackend backend;
    const VideoBackend::Capabilities capabilities = backend.capabilities();
    QVERIFY(capabilities.streaming);
    QVERIFY(capabilities.pipelineSettings);
    QVERIFY(capabilities.asyncInit);
#ifdef QGC_HAS_ANY_GPU_PATH
    QVERIFY(capabilities.gpuVideoPath);
#else
    QVERIFY(!capabilities.gpuVideoPath);
#endif

    const std::unique_ptr<VideoReceiver> receiver(backend.createReceiver(nullptr));
    QVERIFY(qobject_cast<GstVideoReceiver*>(receiver.get()));
    QCOMPARE(backend.availableDecoderFamilies(), GStreamer::availableDecoderFamilies());

    // The default decoder priorities and debug threshold change nothing a test sees.
    const GstDebugLevel threshold = gst_debug_get_default_threshold();
    backend.setDebugLevel(static_cast<int>(threshold));
    QCOMPARE(gst_debug_get_default_threshold(), threshold);
    backend.applyDecoderPriorities(VideoDecoderOption::Default);
}

void GStreamerTest::_testVideoBackendRunsInOrder()
{
    GStreamerVideoBackend backend;
    QCOMPARE(backend.state(), GStreamerVideoBackend::State::Unprepared);

    // Nothing to self-test before GStreamer runs.
    expectLogMessage("Video.GStreamer.VideoBackend", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^GStreamer self-test")));
    QVERIFY(!backend.selfTest());
    verifyExpectedLogMessage();

    // A setup that fails isn't retried: gst_init() can't be undone.
    VideoBackend::InitParams params;
    params.environment = {.ok = false, .error = QStringLiteral("test")};
    expectLogMessage("Video.GStreamer.VideoBackend", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Initializing GStreamer without preparing")));
    expectLogMessage("Video.GStreamer.GStreamer", QtCriticalMsg,
                     QRegularExpression(QStringLiteral("^Invalid GStreamer environment configuration")));
    QVERIFY(!backend.initialize(params));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(backend.state(), GStreamerVideoBackend::State::Failed);
    QVERIFY(!backend.initialize(VideoBackend::InitParams()));
    QCOMPARE(backend.state(), GStreamerVideoBackend::State::Failed);

    expectLogMessage("Video.GStreamer.VideoBackend", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^GStreamer self-test")));
    QVERIFY(!backend.selfTest());
    verifyExpectedLogMessage();

    // The streams then go to the receiver that needs no setup.
    const std::unique_ptr<VideoReceiver> receiver(backend.createReceiver(nullptr));
    QVERIFY(qobject_cast<QtMultimediaVideoReceiver*>(receiver.get()));
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
    const GStreamer::GstElementPtr source = GStreamer::adoptFloating(capsfilter);
    g_object_set(source.get(), "caps", inputCaps.get(), nullptr);

    const GStreamer::GstElementPtr sink =
        RecordingBranch::makeFileSink(QStringLiteral("recording-test"), fileFormat, inputCaps.get());
    QVERIFY(sink);

    QVERIFY2(gst_element_link(source.get(), sink.get()),
             qPrintable(QStringLiteral("Recording sink rejected %1").arg(capsString)));
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
