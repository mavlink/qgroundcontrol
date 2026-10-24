#include "GStreamerInitTest.h"

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QStringList>

#include "Fixtures/RAIIFixtures.h"
#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "GStreamerTestHelpers.h"

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
    const QString unavailable = GstTest::initGStreamer();
    if (!unavailable.isEmpty()) {
        QSKIP(qPrintable(unavailable));
    }

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

    const QString unavailable = GstTest::initGStreamer();
    if (!unavailable.isEmpty()) {
        QSKIP(qPrintable(unavailable));
    }

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

#else

void GStreamerInitTest::_testGStreamerDisabled()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(GStreamerInitTest, TestLabel::Unit)
