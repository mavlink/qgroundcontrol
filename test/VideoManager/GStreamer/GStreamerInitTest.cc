#include "GStreamerInitTest.h"

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>

#include "Fixtures/RAIIFixtures.h"
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

#else

void GStreamerInitTest::_testGStreamerDisabled()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(GStreamerInitTest, TestLabel::Unit)
