#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"

#include <gst/gst.h>

void GStreamerTest::init()
{
    UnitTest::init();

    if (!gst_is_initialized()) {
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

    qCDebug(GStreamerLog) << "Decoder factory classification:" << total << "total,"
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

#else

void GStreamerTest::init() { UnitTest::init(); QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testIsValidRtspUri() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testIsHardwareDecoderFactory() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testSetCodecPrioritiesDefault() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testSetCodecPrioritiesSoftware() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testSetCodecPrioritiesHardware() { QSKIP("GStreamer not enabled"); }
void GStreamerTest::_testRedirectGLibLogging() { QSKIP("GStreamer not enabled"); }

#endif

UT_REGISTER_TEST(GStreamerTest, TestLabel::Integration)
