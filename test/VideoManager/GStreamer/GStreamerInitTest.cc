#include "GStreamerInitTest.h"

#ifdef QGC_GST_STREAMING

#include <algorithm>
#include <gst/gst.h>
#include <utility>
#include <vector>

#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QStringList>

#include "Fixtures/RAIIFixtures.h"
#include "GStreamer.h"
#include "GStreamerDecoderRanks.h"
#include "GStreamerLogging.h"
#include "GStreamerSelfTest.h"
#include "GStreamerTestHelpers.h"
#include "GStreamerUdpSrcWorkaround.h"
#include "GstScoped.h"
#include "LogManager.h"

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
    expectLogMessage("Video.GStreamer.GStreamerAPI", QtWarningMsg,
                     QRegularExpression(QStringLiteral("bridged GStreamer log probe")));
    logProbe("bridged GStreamer log probe");
    verifyExpectedLogMessage();
}

void GStreamerInitTest::_testConfigureDebugLoggingIsIdempotent()
{
    GSTTEST_REQUIRE_GSTREAMER();

    const NativeDebugLogEnvGuard guard;
    GStreamer::configureDebugLogging();
    GStreamer::configureDebugLogging();

    GST_DEBUG_CATEGORY_STATIC(probeCategory);
    GST_DEBUG_CATEGORY_INIT(probeCategory, "qgc-init-test-idempotent", 0, "QGC GStreamer init test");
    if (!probeCategory) {
        QSKIP("GStreamer debug categories unavailable");
    }
    gst_debug_category_set_threshold(probeCategory, GST_LEVEL_WARNING);
    const auto resetThreshold = qScopeGuard([] { gst_debug_category_set_threshold(probeCategory, GST_LEVEL_NONE); });

    // Strict log checking fails the test if a second bridge repeats the message.
    expectLogMessage("Video.GStreamer.GStreamerAPI", QtWarningMsg,
                     QRegularExpression(QStringLiteral("idempotent configureDebugLogging probe")));
    gst_debug_log(probeCategory, GST_LEVEL_WARNING, __FILE__, Q_FUNC_INFO, __LINE__, nullptr, "%s",
                  "idempotent configureDebugLogging probe");
    verifyExpectedLogMessage();
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
    QTest::addColumn<bool>("hinted");

    QTest::newRow("complete") << QStringList() << QStringList() << 0 << false << false;
    QTest::newRow("videoconvert stands in for videoconvertscale")
        << QStringList{"videoconvertscale"} << QStringList() << 0 << false << false;
    QTest::newRow("no converter") << QStringList{"videoconvertscale", "videoconvert"}
                                  << QStringList{"videoconvertscale|videoconvert"} << 1 << true << false;
    QTest::newRow("no udpsrc") << QStringList{"udpsrc"} << QStringList{"udpsrc"} << 1 << true << false;
    QTest::newRow("no matroska") << QStringList{"matroskamux"} << QStringList{"matroskamux"} << 1 << false << false;
    QTest::newRow("no isomp4") << QStringList{"mp4mux", "qtmux"} << QStringList{"mp4mux", "qtmux"} << 1 << false
                               << false;
    QTest::newRow("no rtsp or tcp") << QStringList{"rtspsrc", "tcpclientsrc"} << QStringList{"rtspsrc", "tcpclientsrc"}
                                    << 2 << false << false;
    QTest::newRow("qgc elements stripped")
        << QStringList{"qgcvideosinkbin"} << QStringList{"qgcvideosinkbin"} << 1 << true << true;
}

void GStreamerInitTest::_testMissingPluginFeatures()
{
    QFETCH(QStringList, absent);
    QFETCH(QStringList, missingFactories);
    QFETCH(int, missingFeatures);
    QFETCH(bool, videoUnavailable);
    QFETCH(bool, hinted);

    const QList<GStreamer::MissingPluginFeature> missing = GStreamer::missingPluginFeatures(
        [&absent](const char* factoryName) { return !absent.contains(QLatin1String(factoryName)); });

    QStringList factories;
    bool required = false;
    bool hint = false;
    for (const GStreamer::MissingPluginFeature& feature : missing) {
        QVERIFY(!feature.feature.isEmpty());
        factories << feature.factories;
        required = required || feature.required;
        hint = hint || !feature.hint.isEmpty();
    }
    QCOMPARE(missing.size(), missingFeatures);
    QCOMPARE(factories, missingFactories);
    QCOMPARE(required, videoUnavailable);
    QCOMPARE(hint, hinted);
}

void GStreamerInitTest::_testUdpSrcUnlockSerializedBefore1_24_10()
{
    GStreamer::serializeUdpSrcUnlock();

    const GStreamer::GstFactoryPtr factory = GStreamer::adoptFactory(gst_element_factory_find("udpsrc"));
    if (!factory) {
        QSKIP("udpsrc is not available");
    }
    QCOMPARE(GStreamer::udpSrcUnlockSerialized(),
             !gst_plugin_feature_check_version(GST_PLUGIN_FEATURE(factory.get()), 1, 24, 10));
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

    // A static plugin has no file, so it is never blamed on the bundle.
    QList<GStreamer::SelfTest::RegisteredPlugin> plugins{
        {.name = QStringLiteral("qgc"), .filename = QString(), .blacklisted = false}};
    for (const QString& file : registered) {
        plugins.append(
            {.name = QFileInfo(file).fileName(), .filename = file, .blacklisted = blacklisted.contains(file)});
    }

    const QStringList found = GStreamer::SelfTest::bundledPluginIssues(plugins, {pluginDir}, shipped);
    QVERIFY2(found.size() == issues.size(), qPrintable(found.join(QLatin1Char('\n'))));
    for (qsizetype i = 0; i < issues.size(); ++i) {
        QVERIFY2(found.at(i).contains(QRegularExpression(issues.at(i))), qPrintable(found.at(i)));
    }
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
    QVERIFY(GStreamer::changeFeatureRank(registry, "identity", static_cast<uint16_t>(requestedRank)));
    QCOMPARE(gst_plugin_feature_get_rank(identity.get()), kept ? originalRank : requestedRank);
}

void GStreamerInitTest::_testSetCodecPrioritiesDefault()
{
    GSTTEST_REQUIRE_GSTREAMER();

    const GstTest::DecoderRankSnapshot decoderRanks;
    std::vector<std::pair<GStreamer::GstFeaturePtr, guint>> softwareRanks;
    GStreamer::forEachVideoDecoderFactory(GST_RANK_NONE, [&softwareRanks](GstElementFactory* factory) {
        if (!GStreamer::isHardwareDecoderFactory(factory)) {
            GstPluginFeature* const feature = GST_PLUGIN_FEATURE(factory);
            softwareRanks.emplace_back(GStreamer::adoptFeature(GST_PLUGIN_FEATURE(gst_object_ref(feature))),
                                       gst_plugin_feature_get_rank(feature));
        }
    });
    if (softwareRanks.empty()) {
        QSKIP("No software video decoder factories available on this system");
    }

    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderDefault);

    // The default only steers hardware decoders: zero-copy preferences and the D3D family matching the RHI.
    for (const auto& [feature, rank] : softwareRanks) {
        QCOMPARE(gst_plugin_feature_get_rank(feature.get()), rank);
    }
}

void GStreamerInitTest::_testSetCodecPrioritiesDefaultPrefersMatchingD3DDecoder()
{
#ifndef Q_OS_WIN
    QSKIP("D3D decoder rank steering is Windows-only");
#else
    GSTTEST_REQUIRE_GSTREAMER();

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

    const GstTest::DecoderRankSnapshot decoderRanks;
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

void GStreamerInitTest::_testSetCodecPrioritiesSkipsAbsentD3DDecoders()
{
#ifndef Q_OS_WIN
    QSKIP("D3D decoder rank steering is Windows-only");
#else
    GSTTEST_REQUIRE_GSTREAMER();

    GstRegistry* registry = gst_registry_get();
    QVERIFY(registry != nullptr);

    const TestFixtures::LoggingCategoryFixture ranksLog(QStringLiteral("Video.GStreamer.DecoderRanks"));

    LogManager::clearCapturedMessages();
    QVERIFY(!GStreamer::changeFeatureRank(registry, "__qgc_missing_d3d_decoder_for_test__", GST_RANK_NONE));

    const QList<LogEntry> helperMessages = LogManager::capturedMessages(QStringLiteral("Video.GStreamer.DecoderRanks"));
    for (const LogEntry& entry : helperMessages) {
        QVERIFY2(
            !entry.message.contains(QStringLiteral("Feature does not exist")),
            qPrintable(QStringLiteral("Optional D3D decoder factory was logged as a failure: %1").arg(entry.message)));
    }
#endif
}

void GStreamerInitTest::_testSetCodecPrioritiesSoftware()
{
    GSTTEST_REQUIRE_GSTREAMER();

    const GstTest::DecoderRankSnapshot decoderRanks;
    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderSoftware);

    int decoders = 0;
    bool foundPrioritizedSoftware = false;
    GStreamer::forEachVideoDecoderFactory(GST_RANK_NONE, [&](GstElementFactory* factory) {
        ++decoders;
        if (!GStreamer::isHardwareDecoderFactory(factory) &&
            (gst_plugin_feature_get_rank(GST_PLUGIN_FEATURE(factory)) > GST_RANK_MARGINAL)) {
            foundPrioritizedSoftware = true;
        }
    });
    if (decoders == 0) {
        QSKIP("No video decoder factories available on this system");
    }
    QVERIFY(foundPrioritizedSoftware);
}

void GStreamerInitTest::_testSetCodecPrioritiesReplacesThePreviousOption()
{
    GSTTEST_REQUIRE_GSTREAMER();

    const GstTest::DecoderRankSnapshot decoderRanks;
    bool hasSoftwareDecoder = false;
    GStreamer::forEachVideoDecoderFactory(GST_RANK_NONE, [&hasSoftwareDecoder](GstElementFactory* factory) {
        hasSoftwareDecoder = hasSoftwareDecoder || !GStreamer::isHardwareDecoderFactory(factory);
    });
    if (!hasSoftwareDecoder) {
        QSKIP("No software video decoder factories available on this system");
    }

    const auto decoderRanksNow = [] {
        QHash<QByteArray, guint> ranks;
        GStreamer::forEachVideoDecoderFactory(GST_RANK_NONE, [&ranks](GstElementFactory* factory) {
            GstPluginFeature* const feature = GST_PLUGIN_FEATURE(factory);
            ranks.insert(QByteArray(gst_plugin_feature_get_name(feature)), gst_plugin_feature_get_rank(feature));
        });
        return ranks;
    };
    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderDefault);
    const QHash<QByteArray, guint> defaultRanks = decoderRanksNow();

    // The setting can change at runtime: going back to the default must not keep the software decoders promoted.
    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderSoftware);
    QVERIFY(decoderRanksNow() != defaultRanks);
    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderDefault);
    QCOMPARE(decoderRanksNow(), defaultRanks);
}

void GStreamerInitTest::_testSetCodecPrioritiesFamilyOutranksOtherDecoders()
{
    GSTTEST_REQUIRE_GSTREAMER();

    const GstTest::DecoderRankSnapshot decoderRanks;
    ignoreLogMessage("Video.GStreamer.DecoderRanks", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Forcing Vulkan video decoders")));
    QList<GStreamer::VideoDecoderOptions> families;
    GStreamer::forEachVideoDecoderFactory(GST_RANK_NONE, [&families](GstElementFactory* factory) {
        const auto family =
            GStreamer::decoderFamilyForFactory(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)));
        if ((family != GStreamer::ForceVideoDecoderDefault) && !families.contains(family)) {
            families.append(family);
        }
    });
    if (families.isEmpty()) {
        QSKIP("No decoder of a forceable family is installed");
    }

    // GStreamer ranks d3d11 and va decoders PRIMARY + 1 already: forcing another family must not tie with them.
    for (const GStreamer::VideoDecoderOptions family : std::as_const(families)) {
        GStreamer::setCodecPriorities(family);
        guint lowestFamilyRank = G_MAXUINT;
        guint highestOtherRank = GST_RANK_NONE;
        GStreamer::forEachVideoDecoderFactory(GST_RANK_NONE, [&](GstElementFactory* factory) {
            GstPluginFeature* const feature = GST_PLUGIN_FEATURE(factory);
            const guint rank = gst_plugin_feature_get_rank(feature);
            if (GStreamer::decoderFamilyForFactory(gst_plugin_feature_get_name(feature)) != family) {
                highestOtherRank = std::max(highestOtherRank, rank);
            } else if (rank > GST_RANK_NONE) {
                // A member at NONE was demoted on purpose, as the D3D decoders not matching the renderer are.
                lowestFamilyRank = std::min(lowestFamilyRank, rank);
            }
        });
        QVERIFY2(lowestFamilyRank > highestOtherRank, qPrintable(QStringLiteral("family %1: rank %2 <= %3")
                                                                     .arg(static_cast<int>(family))
                                                                     .arg(lowestFamilyRank)
                                                                     .arg(highestOtherRank)));
    }
}

void GStreamerInitTest::_testSetCodecPrioritiesHardware()
{
    GSTTEST_REQUIRE_GSTREAMER();

    const GstTest::DecoderRankSnapshot decoderRanks;
    // A host without GPU decoders, such as a CI runner, gets the warning that none could be preferred.
    bool hasHardwareDecoder = false;
    GStreamer::forEachVideoDecoderFactory(GST_RANK_NONE, [&hasHardwareDecoder](GstElementFactory* factory) {
        hasHardwareDecoder = hasHardwareDecoder || GStreamer::isHardwareDecoderFactory(factory);
    });
    if (!hasHardwareDecoder) {
        expectLogMessage("Video.GStreamer.DecoderRanks", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^No hardware video decoder factories found")));
    }
    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderHardware);
    if (!hasHardwareDecoder) {
        verifyExpectedLogMessage();
    }

    QStringList rankedSoftware;
    GStreamer::forEachVideoDecoderFactory(GST_RANK_NONE, [&rankedSoftware](GstElementFactory* factory) {
        GstPluginFeature* const feature = GST_PLUGIN_FEATURE(factory);
        if (!GStreamer::isHardwareDecoderFactory(factory) && (gst_plugin_feature_get_rank(feature) != GST_RANK_NONE)) {
            rankedSoftware.append(QString::fromUtf8(gst_plugin_feature_get_name(feature)));
        }
    });
    QVERIFY2(rankedSoftware.isEmpty(), qPrintable(rankedSoftware.join(QStringLiteral(", "))));
}

#else

void GStreamerInitTest::_testGStreamerDisabled()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(GStreamerInitTest, TestLabel::Unit)
