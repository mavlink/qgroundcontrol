#include "GstSourceFactoryHarnessTest.h"

#include <gst/check/gstharness.h>
#include <gst/gst.h>

#include <QtCore/QList>
#include <QtCore/QScopeGuard>

#include "GStreamerTestHelpers.h"
#include "GstScoped.h"
#include "GstSourceFactory.h"

namespace {

/// RTP packets of @p units, payloaded by @p harness from the given unit on, PTS at 30 fps.
QList<GstBuffer*> payload(GstHarness* harness, const QList<GstTest::AccessUnit>& units, qsizetype first,
                          qsizetype count)
{
    for (qsizetype i = first; i < first + count; ++i) {
        GstBuffer* const buffer =
            gst_buffer_new_memdup(units.at(i).bytes.constData(), static_cast<gsize>(units.at(i).bytes.size()));
        GST_BUFFER_PTS(buffer) = i * (GST_SECOND / GstTest::kStreamFrameRate);
        (void) gst_harness_push(harness, buffer);
    }
    QList<GstBuffer*> packets;
    while (GstBuffer* const packet = gst_harness_try_pull(harness)) {
        packets.append(packet);
    }
    return packets;
}

}  // namespace

void GstSourceFactoryHarnessTest::_testParameterSetGateOnDepayloadedStream_data()
{
    QTest::addColumn<QString>("resource");
    QTest::addColumn<bool>("h265");
    QTest::addColumn<QList<int>>("parameterSets");

    QTest::newRow("h264") << QString::fromLatin1(GstTest::kH264Stream) << false << QList<int>{7, 8};
    QTest::newRow("h265") << QString::fromLatin1(GstTest::kH265Stream) << true << QList<int>{32, 33, 34};
}

void GstSourceFactoryHarnessTest::_testParameterSetGateOnDepayloadedStream()
{
    QFETCH(QString, resource);
    QFETCH(bool, h265);
    QFETCH(QList<int>, parameterSets);

    const char* const codec = h265 ? "h265" : "h264";
    GSTTEST_SKIP_UNLESS_FACTORIES("appsrc", "appsink");
    QString loadError;
    const QList<GstTest::AccessUnit> units = GstTest::loadAccessUnits(resource, &loadError);
    QVERIFY2(units.size() > 5, qPrintable(loadError));
    QVERIFY(units.at(0).keyframe && !units.at(1).keyframe && units.at(5).keyframe);

    GstHarness* const payloader = gst_harness_new(h265 ? "rtph265pay" : "rtph264pay");
    const auto teardownPayloader = qScopeGuard([payloader] { gst_harness_teardown(payloader); });
    g_object_set(payloader->element, "config-interval", -1, "pt", 96, nullptr);
    gst_harness_set_src_caps_str(
        payloader,
        qPrintable(QStringLiteral("video/x-%1,stream-format=byte-stream,alignment=au").arg(QLatin1String(codec))));
    // A receiver joining mid-GOP: the rest of a GOP whose keyframe it missed, then the next keyframe.
    const QList<GstBuffer*> deltaPackets = payload(payloader, units, 1, 4);
    const QList<GstBuffer*> keyframePackets = payload(payloader, units, 5, 1);
    QVERIFY(!deltaPackets.isEmpty() && !keyframePackets.isEmpty());

    GstHarness* const depayloader = gst_harness_new(h265 ? "rtph265depay" : "rtph264depay");
    const auto teardownDepayloader = qScopeGuard([depayloader] { gst_harness_teardown(depayloader); });
    gst_harness_set_sink_caps_str(
        depayloader,
        qPrintable(QStringLiteral("video/x-%1,stream-format=byte-stream,alignment=au").arg(QLatin1String(codec))));
    gst_harness_set_src_caps_str(
        depayloader, qPrintable(QStringLiteral("application/x-rtp,media=video,clock-rate=90000,encoding-name=%1")
                                    .arg(QLatin1String(h265 ? "H265" : "H264"))));
    const GStreamer::GstPadPtr depayloaderSrc =
        GStreamer::adoptPad(gst_element_get_static_pad(depayloader->element, "src"));
    GStreamer::SourceFactory::gateOnParameterSets(depayloaderSrc.get(), h265);

    for (GstBuffer* const packet : deltaPackets) {
        QCOMPARE(gst_harness_push(depayloader, packet), GST_FLOW_OK);
    }
    QCOMPARE(gst_harness_buffers_received(depayloader), 0U);

    for (GstBuffer* const packet : keyframePackets) {
        QCOMPARE(gst_harness_push(depayloader, packet), GST_FLOW_OK);
    }
    GstBuffer* const first = gst_harness_try_pull(depayloader);
    QVERIFY2(first, "the keyframe never left the depayloader");
    GstMapInfo map;
    QVERIFY(gst_buffer_map(first, &map, GST_MAP_READ));
    const QList<int> types =
        GstTest::nalTypes(QByteArray(reinterpret_cast<const char*>(map.data), static_cast<qsizetype>(map.size)), h265);
    gst_buffer_unmap(first, &map);
    gst_buffer_unref(first);
    for (const int type : parameterSets) {
        QVERIFY2(types.contains(type), qPrintable(QStringLiteral("NAL type %1 missing from the first unit").arg(type)));
    }
}

UT_REGISTER_TEST(GstSourceFactoryHarnessTest, TestLabel::Unit)
