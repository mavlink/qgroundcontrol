#include "GStreamerInitTest.h"

#include <algorithm>
#include <gst/gst.h>
#include <utility>

#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>

#include "Fixtures/RAIIFixtures.h"
#include "GStreamer.h"
#include "GStreamerDecoderRanks.h"
#include "GStreamerLibavWorkaround.h"
#include "GStreamerLogging.h"
#include "GStreamerSelfTest.h"
#include "GStreamerTestHelpers.h"
#include "GStreamerUdpSrcWorkaround.h"
#include "GstScoped.h"

namespace {

/// Saves and unsets the variables that ask for GStreamer's own debug output, and restores them on destruction.
struct NativeDebugLogEnvGuard
{
    TestFixtures::EnvVarFixture debug{"GST_DEBUG"};
    TestFixtures::EnvVarFixture debugFile{"GST_DEBUG_FILE"};
    TestFixtures::EnvVarFixture tracers{"GST_TRACERS"};

    NativeDebugLogEnvGuard()
    {
        qunsetenv("GST_DEBUG");
        qunsetenv("GST_DEBUG_FILE");
        qunsetenv("GST_TRACERS");
    }
};

}  // namespace

void GStreamerInitTest::_testInitializeLogsFromItsFirstLine()
{
    if (gst_is_initialized()) {
        QSKIP("GStreamer was initialized before this test could watch it");
    }
    const NativeDebugLogEnvGuard guard;
    // The category passes GStreamer's lines below a warning only once it is turned on.
    const TestFixtures::LoggingCategoryFixture apiLog(QStringLiteral("Video.GStreamer.GStreamerAPI"));
    const auto restore = qScopeGuard([] { gst_debug_set_default_threshold(GST_LEVEL_NONE); });

    // Whatever else GStreamer logs while it starts and scans this host's plugins.
    const QRegularExpression anything(QStringLiteral("."));
    for (const QtMsgType type : {QtDebugMsg, QtInfoMsg, QtWarningMsg, QtCriticalMsg}) {
        ignoreLogMessage("Video.GStreamer.GStreamerAPI", type, anything);
        ignoreLogMessage("Video.GStreamer.GStreamerLogging", type, anything);
    }
    // GStreamer's first log line: at the persisted level.
    expectLogMessage("Video.GStreamer.GStreamerAPI", QtInfoMsg,
                     QRegularExpression(QStringLiteral("Initializing GStreamer Core Library")));
    const VideoBackend::InitParams params{
        .environment = GStreamer::Environment::prepareEnvironment(),
        .debugLevel = static_cast<int>(GST_LEVEL_INFO),
    };
    QVERIFY(GStreamer::initialize(params));
    verifyExpectedLogMessage();

    // And the threshold once GStreamer runs.
    QCOMPARE(gst_debug_get_default_threshold(), GST_LEVEL_INFO);
}

void GStreamerInitTest::_testNativeDebugLogRequested_data()
{
    QTest::addColumn<QByteArray>("variable");
    QTest::addColumn<QByteArray>("value");
    QTest::addColumn<bool>("native");

    QTest::newRow("none") << QByteArray() << QByteArray() << false;
    QTest::newRow("empty GST_DEBUG") << QByteArrayLiteral("GST_DEBUG") << QByteArrayLiteral("") << false;
    QTest::newRow("GST_DEBUG") << QByteArrayLiteral("GST_DEBUG") << QByteArrayLiteral("3") << true;
    QTest::newRow("GST_DEBUG_FILE") << QByteArrayLiteral("GST_DEBUG_FILE") << QByteArrayLiteral("gst.log") << true;
    QTest::newRow("GST_TRACERS") << QByteArrayLiteral("GST_TRACERS") << QByteArrayLiteral("leaks") << true;
}

void GStreamerInitTest::_testNativeDebugLogRequested()
{
    QFETCH(QByteArray, variable);
    QFETCH(QByteArray, value);
    QFETCH(bool, native);

    const NativeDebugLogEnvGuard guard;
    if (!variable.isEmpty()) {
        qputenv(variable.constData(), value);
    }
    QCOMPARE(GStreamer::nativeDebugLogRequested(), native);
}

void GStreamerInitTest::_testNativeDebugLogIsNotBridged()
{
    GSTTEST_REQUIRE_GSTREAMER();

    GST_DEBUG_CATEGORY_STATIC(probeCategory);
    GST_DEBUG_CATEGORY_INIT(probeCategory, "qgc-init-test", 0, "QGC GStreamer init test");
    if (!probeCategory) {
        QSKIP("GStreamer debug categories unavailable");
    }
    // After configureDebugLogging(), which may reset every category's threshold to the persisted level.
    const auto logProbe = [](const char* text) {
        gst_debug_category_set_threshold(probeCategory, GST_LEVEL_WARNING);
        gst_debug_log(probeCategory, GST_LEVEL_WARNING, __FILE__, Q_FUNC_INFO, __LINE__, nullptr, "%s", text);
    };
    const auto resetThreshold = qScopeGuard([] { gst_debug_category_set_threshold(probeCategory, GST_LEVEL_NONE); });

    {
        const NativeDebugLogEnvGuard guard;
        qputenv("GST_DEBUG_FILE", QByteArrayLiteral("qgc-init-test.log"));
        GStreamer::configureDebugLogging();
        // Strict log checking fails the test if this reaches Qt logging.
        logProbe("native GStreamer log probe");
    }

    const NativeDebugLogEnvGuard guard;
    GStreamer::configureDebugLogging();
    GStreamer::configureDebugLogging();
    // Strict log checking fails the test if a second bridge repeats the message.
    expectLogMessage("Video.GStreamer.GStreamerAPI", QtWarningMsg,
                     QRegularExpression(QStringLiteral("bridged GStreamer log probe")));
    logProbe("bridged GStreamer log probe");
    verifyExpectedLogMessage();
}

void GStreamerInitTest::_testBridgedLogNamesItsSource()
{
    GSTTEST_REQUIRE_GSTREAMER();

    const NativeDebugLogEnvGuard guard;
    GStreamer::configureDebugLogging();

    GST_DEBUG_CATEGORY_STATIC(probeCategory);
    GST_DEBUG_CATEGORY_INIT(probeCategory, "qgc-init-test-source", 0, "QGC GStreamer init test");
    if (!probeCategory) {
        QSKIP("GStreamer debug categories unavailable");
    }
    gst_debug_category_set_threshold(probeCategory, GST_LEVEL_WARNING);
    const auto resetThreshold = qScopeGuard([] { gst_debug_category_set_threshold(probeCategory, GST_LEVEL_NONE); });

    const GStreamer::GstElementPtr element = GStreamer::makeElement("identity", "probe-element");
    QVERIFY(element);
    expectLogMessage("Video.GStreamer.GStreamerAPI", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^<probe-element> logged by an object")));
    gst_debug_log(probeCategory, GST_LEVEL_WARNING, __FILE__, Q_FUNC_INFO, __LINE__, G_OBJECT(element.get()), "%s",
                  "logged by an object");
    verifyExpectedLogMessage();

#if GST_CHECK_VERSION(1, 22, 0)
    // As GST_WARNING_ID() logs, which decodebin3's multiqueue uses for most of its lines.
    expectLogMessage("Video.GStreamer.GStreamerAPI", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^<probe-id> logged by an id")));
    gst_debug_log_id(probeCategory, GST_LEVEL_WARNING, __FILE__, Q_FUNC_INFO, __LINE__, "probe-id", "%s",
                     "logged by an id");
    verifyExpectedLogMessage();
#endif
}

void GStreamerInitTest::_testConfigureDebugLoggingDefaultsToDisabled()
{
    GSTTEST_REQUIRE_GSTREAMER();

    const NativeDebugLogEnvGuard guard;
    const auto restore = qScopeGuard([] { gst_debug_set_default_threshold(GST_LEVEL_NONE); });
    // A GStreamer development build (nano > 0) starts at GST_LEVEL_ERROR.
    gst_debug_set_default_threshold(GST_LEVEL_ERROR);
    GStreamer::configureDebugLogging();
    QCOMPARE(gst_debug_get_default_threshold(), GST_LEVEL_NONE);

    GStreamer::configureDebugLogging(GST_LEVEL_WARNING);
    QCOMPARE(gst_debug_get_default_threshold(), GST_LEVEL_WARNING);
}

void GStreamerInitTest::_testRedirectGLibLogging()
{
    GStreamer::redirectGLibLogging();

    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtDebugMsg,
                     QRegularExpression(QStringLiteral("GStreamerInitTest debug message")));
    expectLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg,
                     QRegularExpression(QStringLiteral("GStreamerInitTest warning message")));
    g_log("TestDomain", G_LOG_LEVEL_DEBUG, "GStreamerInitTest debug message");
    g_log("TestDomain", G_LOG_LEVEL_WARNING, "GStreamerInitTest warning message");
    verifyExpectedLogMessage();
}

void GStreamerInitTest::_testExternalPluginLoaderFailed()
{
    GStreamer::redirectGLibLogging();
    GStreamer::resetExternalPluginLoaderFailed();
    const auto reset = qScopeGuard([] { GStreamer::resetExternalPluginLoaderFailed(); });

    expectLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to load plugin")));
    g_log("GStreamer", G_LOG_LEVEL_WARNING, "Failed to load plugin 'libgstfoo.so': file too short");
    verifyExpectedLogMessage();
    QVERIFY(!GStreamer::externalPluginLoaderFailed());

    // GStreamer's warning when its gst-plugin-scanner helper can't run (gstregistry.c).
    expectLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg,
                     QRegularExpression(QStringLiteral("External plugin loader failed")));
    g_log("GStreamer", G_LOG_LEVEL_WARNING,
          "External plugin loader failed. This most likely means that the plugin loader helper binary was not found "
          "or could not be run.");
    verifyExpectedLogMessage();
    QVERIFY(GStreamer::externalPluginLoaderFailed());

    GStreamer::resetExternalPluginLoaderFailed();
    QVERIFY(!GStreamer::externalPluginLoaderFailed());
}

void GStreamerInitTest::_testMissingPluginFeatures_data()
{
    QTest::addColumn<QStringList>("absent");
    QTest::addColumn<QStringList>("missingFactories");
    QTest::addColumn<int>("missingFeatures");
    QTest::addColumn<bool>("videoUnavailable");

    QTest::newRow("complete") << QStringList() << QStringList() << 0 << false;
    QTest::newRow("videoconvert stands in for videoconvertscale")
        << QStringList{"videoconvertscale"} << QStringList() << 0 << false;
    QTest::newRow("no converter") << QStringList{"videoconvertscale", "videoconvert"}
                                  << QStringList{"videoconvertscale|videoconvert"} << 1 << true;
    QTest::newRow("no udpsrc") << QStringList{"udpsrc"} << QStringList{"udpsrc"} << 1 << true;
    QTest::newRow("no matroska") << QStringList{"matroskamux"} << QStringList{"matroskamux"} << 1 << false;
    QTest::newRow("no isomp4") << QStringList{"mp4mux", "qtmux"} << QStringList{"mp4mux", "qtmux"} << 1 << false;
    QTest::newRow("no rtsp or tcp") << QStringList{"rtspsrc", "tcpclientsrc"} << QStringList{"rtspsrc", "tcpclientsrc"}
                                    << 2 << false;
}

void GStreamerInitTest::_testMissingPluginFeatures()
{
    QFETCH(QStringList, absent);
    QFETCH(QStringList, missingFactories);
    QFETCH(int, missingFeatures);
    QFETCH(bool, videoUnavailable);

    const QList<GStreamer::MissingPluginFeature> missing = GStreamer::missingPluginFeatures(
        [&absent](const char* factoryName) { return !absent.contains(QLatin1String(factoryName)); });

    QStringList factories;
    bool required = false;
    for (const GStreamer::MissingPluginFeature& feature : missing) {
        QVERIFY(!feature.feature.isEmpty());
        factories << feature.factories;
        required = required || feature.required;
    }
    QCOMPARE(missing.size(), missingFeatures);
    QCOMPARE(factories, missingFactories);
    QCOMPARE(required, videoUnavailable);
}

void GStreamerInitTest::_testUdpSrcUnlockSerializedBefore1_24_10()
{
    GSTTEST_REQUIRE_GSTREAMER();

    GStreamer::serializeUdpSrcUnlock();

    const GStreamer::GstFactoryPtr factory = GStreamer::adoptFactory(gst_element_factory_find("udpsrc"));
    if (!factory) {
        QSKIP("udpsrc is not available");
    }
    QCOMPARE(GStreamer::udpSrcUnlockSerialized(),
             !gst_plugin_feature_check_version(GST_PLUGIN_FEATURE(factory.get()), 1, 24, 10));
}

void GStreamerInitTest::_testHevcWavefrontDeadlockAvoided_data()
{
    QTest::addColumn<QByteArray>("factory");
    QTest::addColumn<QByteArray>("libavcodecVersion");
    QTest::addColumn<int>("maxThreads");

    QTest::newRow("HEVC, FFmpeg 7.1.0") << QByteArray("avdec_h265") << QByteArray("61.19.100") << 1;
    QTest::newRow("HEVC, FFmpeg 7.1.1") << QByteArray("avdec_h265") << QByteArray("61.19.101") << 0;
    QTest::newRow("HEVC, FFmpeg unknown") << QByteArray("avdec_h265") << QByteArray() << 0;
    QTest::newRow("H.264, FFmpeg 7.1.0") << QByteArray("avdec_h264") << QByteArray("61.19.100") << 0;
}

void GStreamerInitTest::_testHevcWavefrontDeadlockAvoided()
{
    GSTTEST_REQUIRE_GSTREAMER();
    QFETCH(QByteArray, factory);
    QFETCH(QByteArray, libavcodecVersion);
    QFETCH(int, maxThreads);

    GSTTEST_SKIP_UNLESS_FACTORIES(factory.constData());
    const GStreamer::GstElementPtr decoder = GStreamer::makeElement(factory.constData());
    QVERIFY(decoder);

    QCOMPARE(GStreamer::avoidHevcWavefrontDeadlock(decoder.get(), QLatin1StringView(libavcodecVersion)),
             maxThreads == 1);
    gint threads = -1;
    g_object_get(decoder.get(), "max-threads", &threads, nullptr);
    QCOMPARE(threads, maxThreads);
}

void GStreamerInitTest::_testBundledPluginIssues_data()
{
    QTest::addColumn<QString>("pluginDir");
    QTest::addColumn<QStringList>("shipped");
    QTest::addColumn<QStringList>("registered");
    QTest::addColumn<QStringList>("blacklisted");
    QTest::addColumn<QStringList>("issues");

    const QString dir = QStringLiteral("/app/lib/gstreamer-1.0");
    const QString app = dir + QStringLiteral("/libgstapp.so");
    const QString rtp = dir + QStringLiteral("/libgstrtp.so");
    const QStringList bundle{app, rtp};

    QTest::newRow("complete") << dir << bundle << bundle << QStringList() << QStringList();
    QTest::newRow("unnormalized plugin dir")
        << QStringLiteral("/app/bin/../lib/gstreamer-1.0/") << bundle << bundle << QStringList() << QStringList();
    QTest::newRow("blacklisted") << dir << bundle << bundle << QStringList{rtp}
                                 << QStringList{QStringLiteral("libgstrtp.so.*is blacklisted")};
    QTest::newRow("host plugin") << dir << bundle
                                 << QStringList{app, rtp, QStringLiteral("/usr/lib/gstreamer-1.0/libgstvpx.so")}
                                 << QStringList() << QStringList{QStringLiteral("libgstvpx.so, outside")};
    QTest::newRow("unregistered file") << dir << bundle << QStringList{app} << QStringList()
                                       << QStringList{QStringLiteral("libgstrtp.so is not in GStreamer's registry")};
    QTest::newRow("stale registry") << dir << QStringList{app} << bundle << QStringList()
                                    << QStringList{QStringLiteral("libgstrtp.so, which the bundle does not ship")};
}

void GStreamerInitTest::_testBundledPluginIssues()
{
    QFETCH(QString, pluginDir);
    QFETCH(QStringList, shipped);
    QFETCH(QStringList, registered);
    QFETCH(QStringList, blacklisted);
    QFETCH(QStringList, issues);

    // A static plugin, as every plugin of a mobile build is, has no file, so it is never blamed on the bundle.
    QList<GStreamer::SelfTest::RegisteredPlugin> plugins{
        {.name = QStringLiteral("coreelements"), .filename = QString(), .blacklisted = false}};
    for (const QString& file : registered) {
        plugins.append(
            {.name = QFileInfo(file).fileName(), .filename = file, .blacklisted = blacklisted.contains(file)});
    }

    const QStringList found = GStreamer::SelfTest::bundledPluginIssues(plugins, pluginDir, shipped);
    QVERIFY2(found.size() == issues.size(), qPrintable(found.join(QLatin1Char('\n'))));
    for (qsizetype i = 0; i < issues.size(); ++i) {
        QVERIFY2(found.at(i).contains(QRegularExpression(issues.at(i))), qPrintable(found.at(i)));
    }
}

void GStreamerInitTest::_testPluginScannerStartFailure_data()
{
    QTest::addColumn<QString>("program");
    QTest::addColumn<QString>("failure");

    // CI and a developer's test environment point GStreamer at the SDK's scanner.
    QTest::newRow("working scanner") << qEnvironmentVariable("GST_PLUGIN_SCANNER_1_0") << QString();
    QTest::newRow("missing scanner") << QStringLiteral("gst-plugin-scanner.exe")
                                     << QStringLiteral("could not be started");
    // Stands in for a scanner that ends otherwise, as one that can't find the GStreamer libraries does.
    QTest::newRow("another exit code") << QStandardPaths::findExecutable(QStringLiteral("whoami.exe"))
                                       << QStringLiteral("exited with code 0x0");
}

void GStreamerInitTest::_testPluginScannerStartFailure()
{
#if !defined(Q_OS_WIN)
    QSKIP("Only GStreamer for Windows tries the plugin scanner before it scans");
#else
    QFETCH(QString, program);
    QFETCH(QString, failure);

    if (program.isEmpty()) {
        QSKIP("This environment has no such program");
    }
    const QTemporaryDir emptyDir;
    QVERIFY(emptyDir.isValid());
    const QString path = QFileInfo(program).isAbsolute() ? program : emptyDir.filePath(program);

    const QString found = GStreamer::SelfTest::pluginScannerStartFailure(path);
    if (failure.isEmpty()) {
        QVERIFY2(found.isEmpty(), qPrintable(found));
    } else {
        QVERIFY2(found.startsWith(failure), qPrintable(found));
    }
#endif
}

void GStreamerInitTest::_testUserFeatureRankKept_data()
{
    QTest::addColumn<QByteArray>("userRanks");
    QTest::addColumn<bool>("kept");

    QTest::newRow("unset") << QByteArray() << false;
    QTest::newRow("named") << QByteArrayLiteral("fakesink:NONE, identity:MAX") << true;
    QTest::newRow("other feature") << QByteArrayLiteral("identityx:MAX") << false;
}

void GStreamerInitTest::_testUserFeatureRankKept()
{
    QFETCH(QByteArray, userRanks);
    QFETCH(bool, kept);

    GSTTEST_REQUIRE_GSTREAMER();

    GstRegistry* const registry = gst_registry_get();
    const GStreamer::GstFeaturePtr identity =
        GStreamer::adoptFeature(gst_registry_lookup_feature(registry, "identity"));
    if (!identity) {
        QSKIP("identity element unavailable");
    }
    const guint originalRank = gst_plugin_feature_get_rank(identity.get());
    const auto restoreRank = qScopeGuard([&] { gst_plugin_feature_set_rank(identity.get(), originalRank); });

    const TestFixtures::EnvVarFixture userRankEnv("GST_PLUGIN_FEATURE_RANK");
    if (userRanks.isNull()) {
        qunsetenv("GST_PLUGIN_FEATURE_RANK");
    } else {
        qputenv("GST_PLUGIN_FEATURE_RANK", userRanks);
    }

    const guint requestedRank = originalRank + 1;
    QVERIFY(
        GStreamer::DecoderRankApplier::instance().setRank(registry, "identity", static_cast<uint16_t>(requestedRank)));
    QCOMPARE(gst_plugin_feature_get_rank(identity.get()), kept ? originalRank : requestedRank);
}

namespace {

// The adapter Qt renders on, and another one, as the decoders of a machine with two GPUs name theirs.
constexpr qint64 kQtAdapterLuid = 1;
constexpr qint64 kOtherAdapterLuid = 2;

/// "name rank hw|sw [qt|other]" per decoder: the family comes from the name, as for an installed factory, and the
/// last field puts a Direct3D decoder on Qt's adapter or on another.
QList<GStreamer::DecoderInfo> decoderInfos(const QStringList& specs)
{
    QList<GStreamer::DecoderInfo> decoders;
    for (const QString& spec : specs) {
        const QStringList fields = spec.split(QLatin1Char(' '));
        const QByteArray name = fields.at(0).toLatin1();
        const QString adapter = fields.value(3);
        decoders.append({.name = name,
                         .rank = fields.at(1).toUInt(),
                         .hardware = (fields.at(2) == QLatin1String("hw")),
                         .family = GStreamer::decoderFamilyForFactory(name.constData()),
                         .adapterLuid = adapter.isEmpty()
                                            ? 0
                                            : ((adapter == QLatin1String("qt")) ? kQtAdapterLuid : kOtherAdapterLuid)});
    }
    return decoders;
}

/// "name=rank" per change of @p plan, in its order.
QStringList plannedChanges(const GStreamer::RankPlan& plan)
{
    QStringList planned;
    for (const GStreamer::RankChange& change : plan.changes) {
        planned.append(QStringLiteral("%1=%2").arg(QString::fromLatin1(change.feature)).arg(change.rank));
    }
    return planned;
}

}  // namespace

void GStreamerInitTest::_testPlanRanks_data()
{
    QTest::addColumn<int>("option");
    QTest::addColumn<bool>("zeroCopy");
    QTest::addColumn<int>("d3d");              // 0: no RHI D3D API, else 11, 12, or 1112: 11 that opens D3D12 frames
    QTest::addColumn<QStringList>("decoders");
    QTest::addColumn<QStringList>("changes");  // "name=rank", in input order
    QTest::addColumn<QString>("warning");      // a regex one warning must match; empty for none
    QTest::addColumn<bool>("softwareRendering");

    const QStringList linuxDecoders{"vaapih264dec 256 hw", "vah264dec 257 hw", "v4l2slh264dec 256 hw",
                                    "avdec_h264 256 sw"};
    const QStringList windowsDecoders{"d3d11h264dec 257 hw", "d3d12h264dec 256 hw", "nvh264dec 256 hw",
                                      "avdec_h264 256 sw"};
    // With GStreamer's ranks, Qt rendering on the second of two GPUs.
    const QStringList twoGpuDecoders{"d3d11h264dec 257 hw other", "d3d11h264device1dec 257 hw qt",
                                     "d3d12h264dec 258 hw other", "d3d12h264device1dec 258 hw qt", "avdec_h264 256 sw"};

    QTest::newRow("default without platform rules")
        << int(VideoDecoderOption::Default) << false << 0 << linuxDecoders << QStringList() << QString() << false;
    QTest::newRow("default on Linux prefers va over vaapi")
        << int(VideoDecoderOption::Default) << true << 0 << linuxDecoders
        << QStringList{"vaapih264dec=0", "vah264dec=258", "v4l2slh264dec=258"} << QString() << false;
    QTest::newRow("default on Linux leaves va decoders GStreamer ranks NONE")
        << int(VideoDecoderOption::Default) << true << 0
        << QStringList{"vah264dec 257 hw", "vajpegdec 0 hw", "vah266dec 0 hw", "jpegdec 256 sw"}
        << QStringList{"vah264dec=258"} << QString() << false;
    // GStreamer before 1.24 ranks every va decoder NONE.
    QTest::newRow("default on Linux raises va decoders all ranked NONE")
        << int(VideoDecoderOption::Default) << true << 0
        << QStringList{"vah264dec 0 hw", "vah265dec 0 hw", "avdec_h264 256 sw"}
        << QStringList{"vah264dec=258", "vah265dec=258"} << QString() << false;
    QTest::newRow("default on D3D11 RHI") << int(VideoDecoderOption::Default) << false << 11 << windowsDecoders
                                          << QStringList{"d3d11h264dec=258", "d3d12h264dec=0"} << QString() << false;
    QTest::newRow("default on D3D12 RHI") << int(VideoDecoderOption::Default) << false << 12 << windowsDecoders
                                          << QStringList{"d3d11h264dec=0", "d3d12h264dec=258"} << QString() << false;
    // The d3d12 decoders go first, on a device of their own; the d3d11 ones stay behind them at GStreamer's rank.
    QTest::newRow("default on D3D11 RHI that opens D3D12 frames")
        << int(VideoDecoderOption::Default) << false << 1112 << windowsDecoders << QStringList{"d3d12h264dec=258"}
        << QString() << false;
    QTest::newRow("D3D family on D3D11 RHI that opens D3D12 frames")
        << int(VideoDecoderOption::DirectX3D) << false << 1112 << windowsDecoders
        << QStringList{"d3d11h264dec=257", "d3d12h264dec=258"} << QString() << false;
    // A decoder on the other GPU ranks below the software decoders: its frames reach Qt through the CPU as well.
    QTest::newRow("default prefers the decoders on Qt's adapter")
        << int(VideoDecoderOption::Default) << false << 11 << twoGpuDecoders
        << QStringList{"d3d11h264dec=128", "d3d11h264device1dec=258", "d3d12h264dec=0", "d3d12h264device1dec=0"}
        << QString() << false;
    QTest::newRow("default prefers the d3d12 decoder on Qt's adapter")
        << int(VideoDecoderOption::Default) << false << 1112 << twoGpuDecoders
        << QStringList{"d3d11h264dec=128", "d3d12h264dec=128", "d3d12h264device1dec=258"} << QString() << false;
    // The software decoders keep the order GStreamer ranks them in.
    QTest::newRow("software lowers autoplugged hardware")
        << int(VideoDecoderOption::Software) << false << 0
        << QStringList{"avdec_h264 256 sw", "openh264dec 64 sw", "vah264dec 257 hw", "nvh264dec 0 hw"}
        << QStringList{"vah264dec=0"} << QString() << false;
    // A decoder GStreamer ranks NONE stays out of autoplug, as vulkanh264dec and msdkh264dec are.
    QTest::newRow("hardware lowers autoplugged software")
        << int(VideoDecoderOption::Hardware) << false << 0
        << QStringList{"nvh264dec 256 hw", "vulkanh264dec 0 hw", "msdkh264dec 0 hw", "avdec_h264 256 sw"}
        << QStringList{"avdec_h264=0"} << QString() << false;
    QTest::newRow("hardware without autoplugged hardware decoders")
        << int(VideoDecoderOption::Hardware) << false << 0
        << QStringList{"avdec_h264 256 sw", "openh264dec 32 sw", "vulkanh264dec 0 hw"} << QStringList{"avdec_h264=0"}
        << QStringLiteral("^No hardware video decoder that GStreamer autoplugs is installed") << false;
    QTest::newRow("hardware on Linux prefers va over vaapi")
        << int(VideoDecoderOption::Hardware) << true << 0 << linuxDecoders
        << QStringList{"vah264dec=258", "v4l2slh264dec=258", "avdec_h264=0"} << QString() << false;
    QTest::newRow("hardware on D3D11 RHI")
        << int(VideoDecoderOption::Hardware) << false << 11 << windowsDecoders
        << QStringList{"d3d11h264dec=258", "d3d12h264dec=0", "avdec_h264=0"} << QString() << false;
    QTest::newRow("family outranks a PRIMARY + 1 decoder")
        << int(VideoDecoderOption::NVIDIA) << false << 0 << windowsDecoders << QStringList{"nvh264dec=258"} << QString()
        << false;
    QTest::newRow("D3D family keeps the other API demoted")
        << int(VideoDecoderOption::DirectX3D) << false << 12 << windowsDecoders
        << QStringList{"d3d11h264dec=0", "d3d12h264dec=257"} << QString() << false;
    // GPU Safe Mode keeps off the GPU: the hardware decoders the platform rule raised rank below the software ones.
    QTest::newRow("software rendering ranks hardware decoders below software ones")
        << int(VideoDecoderOption::Default) << false << 11 << windowsDecoders
        << QStringList{"d3d11h264dec=128", "d3d12h264dec=0", "nvh264dec=128"} << QString() << true;
    // What the user chose in the video settings stays.
    QTest::newRow("software rendering keeps the hardware option")
        << int(VideoDecoderOption::Hardware) << false << 11 << windowsDecoders
        << QStringList{"d3d11h264dec=258", "d3d12h264dec=0", "avdec_h264=0"} << QString() << true;
    QTest::newRow("software rendering keeps a forced family")
        << int(VideoDecoderOption::NVIDIA) << false << 11 << windowsDecoders
        << QStringList{"d3d12h264dec=0", "nvh264dec=258"} << QString() << true;
}

void GStreamerInitTest::_testPlanRanks()
{
    QFETCH(int, option);
    QFETCH(bool, zeroCopy);
    QFETCH(int, d3d);
    QFETCH(QStringList, decoders);
    QFETCH(QStringList, changes);
    QFETCH(QString, warning);
    QFETCH(bool, softwareRendering);

    const QList<GStreamer::DecoderInfo> decoderList = decoderInfos(decoders);
    GStreamer::RankTraits traits;
    traits.preferZeroCopyDecoders = zeroCopy;
    traits.softwareRendering = softwareRendering;
    if (d3d != 0) {
        traits.rhiD3DApi = (d3d == 12) ? GStreamer::D3DApi::D3D12 : GStreamer::D3DApi::D3D11;
        traits.d3d11OpensD3D12 = (d3d == 1112);
    }
    if (std::ranges::any_of(decoderList, [](const GStreamer::DecoderInfo& decoder) { return decoder.adapterLuid; })) {
        traits.rhiAdapterLuid = kQtAdapterLuid;
    }
    const GStreamer::RankPlan plan = GStreamer::planRanks(static_cast<VideoDecoderOption>(option), decoderList, traits);

    QCOMPARE(plannedChanges(plan), changes);
    if (warning.isEmpty()) {
        QVERIFY2(plan.warnings.isEmpty(), qPrintable(plan.warnings.join(QStringLiteral("; "))));
    } else {
        QCOMPARE(plan.warnings.size(), 1);
        QVERIFY2(QRegularExpression(warning).match(plan.warnings.first()).hasMatch(),
                 qPrintable(plan.warnings.first()));
    }
}

void GStreamerInitTest::_testScopedDecoderOptionRestoresThePreviousOption()
{
    GSTTEST_REQUIRE_GSTREAMER();

    const GstTest::DecoderRankSnapshot decoderRanks;
    GStreamer::setCodecPriorities(VideoDecoderOption::Default);
    const QHash<QByteArray, guint> defaultRanks = GstTest::videoDecoderRanks();
    {
        const GStreamer::ScopedDecoderOption software(VideoDecoderOption::Software);
        QCOMPARE(GStreamer::DecoderRankApplier::instance().applied(), std::optional(VideoDecoderOption::Software));
    }
    QCOMPARE(GStreamer::DecoderRankApplier::instance().applied(), std::optional(VideoDecoderOption::Default));
    QCOMPARE(GstTest::videoDecoderRanks(), defaultRanks);
}

void GStreamerInitTest::_testSetCodecPrioritiesSkipsAbsentD3DDecoders()
{
#ifndef Q_OS_WIN
    QSKIP("D3D decoder rank steering is Windows-only");
#else
    GSTTEST_REQUIRE_GSTREAMER();

    GstRegistry* registry = gst_registry_get();
    QVERIFY(registry != nullptr);

    // Nothing is logged either, not even with the category on: strict log checking fails the test on any line.
    const TestFixtures::LoggingCategoryFixture ranksLog(QStringLiteral("Video.GStreamer.DecoderRanks"));
    QVERIFY(!GStreamer::DecoderRankApplier::instance().setRank(registry, "__qgc_missing_d3d_decoder_for_test__",
                                                               GST_RANK_NONE));
#endif
}

void GStreamerInitTest::_testSetCodecPrioritiesReplacesThePreviousOption()
{
    GSTTEST_REQUIRE_GSTREAMER();

    const GstTest::DecoderRankSnapshot decoderRanks;
    bool hasSoftwareDecoder = false;
    GStreamer::forEachVideoDecoderFactory(GST_RANK_MARGINAL, [&hasSoftwareDecoder](GstElementFactory* factory) {
        hasSoftwareDecoder = hasSoftwareDecoder || !GStreamer::isHardwareDecoderFactory(factory);
    });
    if (!hasSoftwareDecoder) {
        QSKIP("No autoplugged software video decoder factories available on this system");
    }
    // A host without GPU decoders is told that the hardware option leaves nothing to decode with.
    ignoreLogMessage("Video.GStreamer.DecoderRanks", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^No hardware video decoder that GStreamer autoplugs")));

    GStreamer::setCodecPriorities(VideoDecoderOption::Default);
    const QHash<QByteArray, guint> defaultRanks = GstTest::videoDecoderRanks();

    // The setting can change at runtime: going back to the default must not keep the software decoders demoted.
    GStreamer::setCodecPriorities(VideoDecoderOption::Hardware);
    QVERIFY(GstTest::videoDecoderRanks() != defaultRanks);
    GStreamer::setCodecPriorities(VideoDecoderOption::Default);
    QCOMPARE(GstTest::videoDecoderRanks(), defaultRanks);
}

UT_REGISTER_TEST(GStreamerInitTest, TestLabel::Unit)
