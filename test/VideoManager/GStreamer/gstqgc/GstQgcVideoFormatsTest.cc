#include "GstQgcVideoFormatsTest.h"

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>
#include <gst/video/video.h>
#include <string>

#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstQgcFrameMap.h"
#include "GstScoped.h"
#include "gstqgc/GstQgcCaps.h"
#include "gstqgc/GstQgcVideoFormats.h"

void GstQgcVideoFormatsTest::_testColorimetryPixelFormatMapping()
{
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_NV12), QVideoFrameFormat::Format_NV12);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_NV21), QVideoFrameFormat::Format_NV21);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_I420), QVideoFrameFormat::Format_YUV420P);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_YV12), QVideoFrameFormat::Format_YV12);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_Y42B), QVideoFrameFormat::Format_YUV422P);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_P010_10LE), QVideoFrameFormat::Format_P010);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_AYUV), QVideoFrameFormat::Format_AYUV);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_YUY2), QVideoFrameFormat::Format_YUYV);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_UYVY), QVideoFrameFormat::Format_UYVY);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_GRAY8), QVideoFrameFormat::Format_Y8);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_GRAY16_LE), QVideoFrameFormat::Format_Y16);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_BGRA), QVideoFrameFormat::Format_BGRA8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_RGBA), QVideoFrameFormat::Format_RGBA8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_I420_10LE), QVideoFrameFormat::Format_YUV420P10);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_P016_LE), QVideoFrameFormat::Format_P016);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_Y444), QVideoFrameFormat::Format_Invalid);
}

void GstQgcVideoFormatsTest::_testCpuCapsFormatsRoundTripToQt()
{
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_caps_from_string(GstQgc::buildSinkCaps({}).c_str()));
    QVERIFY2(caps, "buildSinkCaps() did not parse");
    const GstStructure* s = gst_caps_get_structure(caps.get(), 0);
    const GValue* fmt = gst_structure_get_value(s, "format");
    QVERIFY2(fmt && GST_VALUE_HOLDS_LIST(fmt), "CPU caps format field is not a list");
    const guint n = gst_value_list_get_size(fmt);
    QVERIFY2(n > 0, "CPU caps format list empty");
    for (guint i = 0; i < n; ++i) {
        const char* name = g_value_get_string(gst_value_list_get_value(fmt, i));
        const GstVideoFormat gf = gst_video_format_from_string(name);
        QVERIFY2(gf != GST_VIDEO_FORMAT_UNKNOWN,
                 qPrintable(QStringLiteral("caps lists unknown gst format '%1'").arg(name)));
        QVERIFY2(toQtPixelFormat(gf) != QVideoFrameFormat::Format_Invalid,
                 qPrintable(QStringLiteral("format '%1' advertised but not Qt-renderable").arg(name)));
    }
}

void GstQgcVideoFormatsTest::_testColorimetryColorSpaceMapping()
{
    // Mirrors Qt 6.10.3 qgst.cpp: SMPTE240M maps to AdobeRgb, FCC remains Undefined.
    QCOMPARE(toQtColorSpace(GST_VIDEO_COLOR_MATRIX_SMPTE240M), QVideoFrameFormat::ColorSpace_AdobeRgb);
    QCOMPARE(toQtColorSpace(GST_VIDEO_COLOR_MATRIX_FCC), QVideoFrameFormat::ColorSpace_Undefined);
}

void GstQgcVideoFormatsTest::_testColorimetryResolutionHeuristicMatchesQt()
{
    const auto inferredColorSpace = [](int height) {
        GstVideoInfo info;
        gst_video_info_init(&info);
        gst_video_info_set_format(&info, GST_VIDEO_FORMAT_I420, 1280, height);
        info.colorimetry.matrix = GST_VIDEO_COLOR_MATRIX_UNKNOWN;
        info.colorimetry.range = GST_VIDEO_COLOR_RANGE_UNKNOWN;
        info.colorimetry.transfer = GST_VIDEO_TRANSFER_UNKNOWN;

        QVideoFrameFormat format(QSize(1280, height), QVideoFrameFormat::Format_YUV420P);
        applyColorimetry(format, info, nullptr);
        return format.colorSpace();
    };

    QCOMPARE(inferredColorSpace(576), QVideoFrameFormat::ColorSpace_BT601);
    QCOMPARE(inferredColorSpace(720), QVideoFrameFormat::ColorSpace_BT709);
}

void GstQgcVideoFormatsTest::_testColorimetryTransferMapping()
{
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_SRGB), QVideoFrameFormat::ColorTransfer_Gamma22);
    // Regression: BT601 used to fall through to BT709 — Qt's own backend maps it distinctly.
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_BT601), QVideoFrameFormat::ColorTransfer_BT601);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_BT709), QVideoFrameFormat::ColorTransfer_BT709);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_BT2020_10), QVideoFrameFormat::ColorTransfer_BT709);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_SMPTE2084), QVideoFrameFormat::ColorTransfer_ST2084);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_ARIB_STD_B67), QVideoFrameFormat::ColorTransfer_STD_B67);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_GAMMA10), QVideoFrameFormat::ColorTransfer_Linear);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_GAMMA28), QVideoFrameFormat::ColorTransfer_Gamma28);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_SMPTE240M), QVideoFrameFormat::ColorTransfer_Gamma22);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_ADOBERGB), QVideoFrameFormat::ColorTransfer_Gamma22);
    QCOMPARE(toQtColorTransfer(GST_VIDEO_TRANSFER_LOG100), QVideoFrameFormat::ColorTransfer_Unknown);
}

void GstQgcVideoFormatsTest::_testColorimetryColorRangeMapping()
{
    QCOMPARE(toQtColorRange(GST_VIDEO_COLOR_RANGE_0_255), QVideoFrameFormat::ColorRange_Full);
    QCOMPARE(toQtColorRange(GST_VIDEO_COLOR_RANGE_16_235), QVideoFrameFormat::ColorRange_Video);
    QCOMPARE(toQtColorRange(GST_VIDEO_COLOR_RANGE_UNKNOWN), QVideoFrameFormat::ColorRange_Unknown);
}

void GstQgcVideoFormatsTest::_testPixelFormatAcceptedButNotAdvertised()
{
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_BGRx), QVideoFrameFormat::Format_BGRX8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_RGBx), QVideoFrameFormat::Format_RGBX8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_ARGB), QVideoFrameFormat::Format_ARGB8888);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_xRGB), QVideoFrameFormat::Format_XRGB8888);

    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_BGR), QVideoFrameFormat::Format_Invalid);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_RGB), QVideoFrameFormat::Format_Invalid);
    QCOMPARE(toQtPixelFormat(GST_VIDEO_FORMAT_UNKNOWN), QVideoFrameFormat::Format_Invalid);
}

void GstQgcVideoFormatsTest::_testAdvertisedFormatListMatchesTable()
{
    const std::string list = GstQgc::advertisedFormatList();
    for (const auto& e : GstQgc::kVideoFormatTable) {
        if (e.capsToken) {
            QVERIFY2(
                list.find(e.capsToken) != std::string::npos,
                qPrintable(
                    QStringLiteral("advertised token '%1' missing from list").arg(QString::fromUtf8(e.capsToken))));
        }
    }
    QVERIFY2(list.find("BGRx") == std::string::npos, "accepted-only BGRx must not be advertised");
    QVERIFY2(list.find("I420_10LE") == std::string::npos, "accepted-only I420_10LE must not be advertised");
    QVERIFY2(list.rfind("{ ", 0) == 0 && list.find(" }") != std::string::npos, "list must be brace-wrapped");
}

void GstQgcVideoFormatsTest::_testAdvertisedFormatListForOffer_data()
{
    QTest::addColumn<uint>("offer");
    QTest::addColumn<QString>("expected");

    // Multi-plane GLMemory renders green through Qt's GL import, so glupload must stay single-plane RGB.
    QTest::newRow("glupload") << uint(GstQgc::kOfferGlUpload) << QStringLiteral("{ BGRA, RGBA }");
    QTest::newRow("linear-dma-drm") << uint(GstQgc::kOfferDmaDrm)
                                    << QStringLiteral("{ NV12, NV21, I420, YV12, P010_10LE, BGRA, RGBA }");
}

void GstQgcVideoFormatsTest::_testAdvertisedFormatListForOffer()
{
    QFETCH(uint, offer);
    QFETCH(QString, expected);
    QCOMPARE(QString::fromStdString(GstQgc::advertisedFormatList(offer)), expected);
}

void GstQgcVideoFormatsTest::_testApplyOrientationToFrameMapping()
{
#ifndef QGC_HAS_GST_VIDEO_ORIENTATION_META
    QSKIP("GStreamer build lacks GstVideoOrientationMeta");
#else
    // Verifies each GStreamer orientation enum maps to the correct (rotation, mirrored) tuple
    // on QVideoFrame. Lock-down test: prior versions had subtle mismatches between gst's
    // diagonal-flip semantics and Qt's rotate-then-mirror composition.
    struct Case
    {
        GstVideoOrientationMethod gst;
        QtVideo::Rotation expectedRot;
        bool expectedMirrored;
    };

    const Case cases[] = {
        {GST_VIDEO_ORIENTATION_IDENTITY, QtVideo::Rotation::None, false},
        {GST_VIDEO_ORIENTATION_90R, QtVideo::Rotation::Clockwise90, false},
        {GST_VIDEO_ORIENTATION_180, QtVideo::Rotation::Clockwise180, false},
        {GST_VIDEO_ORIENTATION_90L, QtVideo::Rotation::Clockwise270, false},
        {GST_VIDEO_ORIENTATION_HORIZ, QtVideo::Rotation::None, true},
        {GST_VIDEO_ORIENTATION_VERT, QtVideo::Rotation::Clockwise180, true},
        {GST_VIDEO_ORIENTATION_UL_LR, QtVideo::Rotation::Clockwise90, true},
        {GST_VIDEO_ORIENTATION_UR_LL, QtVideo::Rotation::Clockwise270, true},
    };
    for (const Case& c : cases) {
        QVideoFrame frame = QVideoFrame(QVideoFrameFormat(QSize(2, 2), QVideoFrameFormat::Format_BGRA8888));
        // Pre-poison so a no-op switch case (default branch) wouldn't accidentally match.
        frame.setRotation(QtVideo::Rotation::Clockwise90);
        frame.setMirrored(true);
        applyOrientationToFrame(frame, c.gst);
        QCOMPARE(frame.rotation(), c.expectedRot);
        QCOMPARE(frame.mirrored(), c.expectedMirrored);
    }
#endif
}

#endif

UT_REGISTER_TEST(GstQgcVideoFormatsTest, TestLabel::Unit)
