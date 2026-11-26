#include "GstQgcSinkTest.h"

#include <array>
#include <atomic>
#include <cstring>
#include <gst/gst.h>
#include <gst/video/gstvideoaffinetransformationmeta.h>
#include <gst/video/gstvideometa.h>
#include <memory>
#include <thread>
#include <vector>

#include <QtCore/QCoreApplication>
#include <QtCore/QRect>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QSet>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>

#include "Fixtures/RAIIFixtures.h"
#include "GStreamer.h"
#include "GStreamerTestHelpers.h"
#include "GstHwPathTelemetry.h"
#include "HwBuffers/HwBuffers.h"
#include "gstqgc/GstQgcFrameDelivery.h"
#include "gstqgc/GstQgcFrameMap.h"
#include "gstqgc/GstQgcSinkPolicy.h"
#include "gstqgc/GstQgcSinkStats.h"
#include "gstqgc/GstQgcTestSupport.h"
#include "gstqgc/gstqgcqvideosink.h"
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include "GstDmaBufVideoBuffer.h"
#endif

namespace {

/// Sorted factory names of a sink bin holding @p others behind its default converter: videoconvertscale, or
/// videoconvert and videoscale before GStreamer 1.22.
QStringList withConverter(QStringList others)
{
    if (GstTest::firstMissingFactory({"videoconvertscale"}).isEmpty()) {
        others << QStringLiteral("videoconvertscale");
    } else {
        others << QStringLiteral("videoconvert") << QStringLiteral("videoscale");
    }
    others.sort();
    return others;
}

/// What @p element's sink pad accepts.
GStreamer::GstCapsPtr sinkPadCaps(GstElement* element)
{
    const GStreamer::GstPadPtr pad = GStreamer::adoptPad(gst_element_get_static_pad(element, "sink"));
    return GStreamer::adoptCaps(pad ? gst_pad_query_caps(pad.get(), nullptr) : nullptr);
}

constexpr GstQgc::RenderTarget kOpenGlOnEgl{.backend = GstQgc::RenderBackend::OpenGL, .egl = true};

/// Stands in for Qt's EGLDisplay (GstQgc::SinkEnvironment::dmaBufDisplay), so the sink offers DMABuf; never passed to
/// EGL by these tests.
void* const kFakeQtEglDisplay = reinterpret_cast<void*>(quintptr(1));

/// Memory features of @p caps' structures in order, consecutive duplicates collapsed.
QStringList capsMemoryFeatures(const GstCaps* caps)
{
    QStringList features;
    for (guint i = 0; i < gst_caps_get_size(caps); ++i) {
        const GstCapsFeatures* f = gst_caps_get_features(caps, i);
        const GStreamer::GCharPtr text = GStreamer::adoptGChar(f ? gst_caps_features_to_string(f) : nullptr);
        const QString feature = QString::fromUtf8(text ? text.get() : GST_CAPS_FEATURE_MEMORY_SYSTEM_MEMORY);
        if (features.isEmpty() || (features.last() != feature)) {
            features << feature;
        }
    }
    return features;
}

/// @p caps' first structure that doesn't ask for square pixels, as text; empty when all do.
QString firstStructureWithoutSquarePixels(const GstCaps* caps)
{
    for (guint i = 0; i < gst_caps_get_size(caps); ++i) {
        const GstStructure* const structure = gst_caps_get_structure(caps, i);
        gint parN = 0;
        gint parD = 0;
        if (!gst_structure_get_fraction(structure, "pixel-aspect-ratio", &parN, &parD) || (parN != parD)) {
            const GStreamer::GCharPtr text = GStreamer::adoptGChar(gst_structure_to_string(structure));
            return QString::fromUtf8(text.get());
        }
    }
    return {};
}

}  // namespace

void GstQgcSinkTest::_testSinkBinConfigReachesSink()
{
    {
        const GstQgc::SinkBin sink = GstQgc::makeSinkBin({});
        QVERIFY2(sink.bin && sink.qvideosink, "Failed to build the sink bin (CPU branch)");

        QVERIFY(!GstQgc::sinkGpuZeroCopy(sink.qvideosink.get()));

        // The bin leaves the deadline as the sink has it by itself (GstVideoSink's 15 ms): a live pipeline that syncs
        // to the clock adds it to its latency.
        guint64 sinkDeadline = 0;
        {
            const GStreamer::GstElementPtr standalone = GstQgcTest::makeVideoSink();
            QVERIFY(standalone);
            g_object_get(standalone.get(), "processing-deadline", &sinkDeadline, NULL);
        }

        // The sink doesn't sync until the receiver says so, and the bin turns off the QoS events a video sink sends by
        // default.
        gboolean sync = TRUE;
        gboolean qos = TRUE;
        guint64 processingDeadline = 0;
        g_object_get(sink.qvideosink.get(), "sync", &sync, "qos", &qos, "processing-deadline", &processingDeadline,
                     NULL);
        QCOMPARE(sync, FALSE);
        QCOMPARE(qos, FALSE);
        QCOMPARE(processingDeadline, sinkDeadline);

        // The converter and qgcqvideosink, whose caps ask for square pixels.
        QCOMPARE(GstTest::childFactoryNames(GST_BIN(sink.bin.get())),
                 withConverter({QStringLiteral("GstQgcQVideoSink")}));
        const GStreamer::GstCapsPtr caps = sinkPadCaps(sink.qvideosink.get());
        QVERIFY(caps && !gst_caps_is_empty(caps.get()));
        QCOMPARE(firstStructureWithoutSquarePixels(caps.get()), QString());

        // Two bins can share a pipeline: each has a name of its own.
        const GstQgc::SinkBin other = GstQgc::makeSinkBin({});
        QVERIFY(other.bin);
        const GStreamer::GCharPtr name = GStreamer::adoptGChar(gst_element_get_name(sink.bin.get()));
        const GStreamer::GCharPtr otherName = GStreamer::adoptGChar(gst_element_get_name(other.bin.get()));
        QVERIFY(QByteArray(name.get()) != QByteArray(otherName.get()));
    }

    {
        // disablePar lets the sink take any pixel aspect ratio.
        const GstQgc::SinkBin sink = GstQgc::makeSinkBin({.disablePar = true});
        QVERIFY2(sink.bin && sink.qvideosink, "Failed to build the sink bin (CPU branch, non-square pixels)");
        const GStreamer::GstCapsPtr caps = sinkPadCaps(sink.qvideosink.get());
        QVERIFY(caps && !gst_caps_is_empty(caps.get()));
        for (guint i = 0; i < gst_caps_get_size(caps.get()); ++i) {
            QVERIFY(!gst_structure_has_field(gst_caps_get_structure(caps.get(), i), "pixel-aspect-ratio"));
        }
    }

#if defined(QGC_HAS_ANY_GPU_PATH)
    {
        const GstQgc::SinkBin sink = GstQgc::makeSinkBin({.gpuZeroCopy = true});
        QVERIFY2(sink.bin && sink.qvideosink, "Failed to build the sink bin (GPU branch)");

        QVERIFY(GstQgc::sinkGpuZeroCopy(sink.qvideosink.get()));
    }
#endif
}

#if defined(QGC_HAS_ANY_GPU_PATH)
void GstQgcSinkTest::_testQgcVideoSinkBinGpuBinVariant_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("backend");
    QTest::addColumn<bool>("egl");
    QTest::addColumn<QStringList>("children");
    QTest::addColumn<QStringList>("memoryFeatures");
    QTest::addColumn<bool>("singlePlaneRgb2D");

    const QString systemMemory = QStringLiteral("memory:SystemMemory");
    const QStringList passthroughChildren = withConverter({QStringLiteral("GstQgcQVideoSink")});
#if defined(QGC_GST_BIN_HAS_GLUPLOAD) || defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    const int openGl = int(GstQgc::RenderBackend::OpenGL);
#endif
#if defined(QGC_GST_BIN_HAS_GLUPLOAD)
    const int glUpload = int(GstQgc::GpuBinMode::GlUpload);
    const QStringList gluploadChildren = withConverter(
        {QStringLiteral("glcolorconvert"), QStringLiteral("glupload"), QStringLiteral("GstQgcQVideoSink")});
    const QStringList glMemory{QStringLiteral("memory:GLMemory")};
    // glcolorconvert turns external-OES into texture-target=2D; Qt samples multi-plane GLMemory as green frames.
    QTest::newRow("glupload") << glUpload << openGl << true << gluploadChildren << glMemory << true;
    // GLMemory can't reach a renderer that isn't OpenGL: the bin would round-trip every frame.
    QTest::newRow("glupload-under-vulkan") << glUpload << int(GstQgc::RenderBackend::Vulkan) << false
                                           << passthroughChildren << QStringList{systemMemory} << false;
    // GLX, or EGL not known yet (Android before the scene graph runs): gst-gl shares Qt's OpenGL context either way.
    QTest::newRow("glupload-egl-unknown") << glUpload << openGl << false << gluploadChildren << glMemory << true;
#endif
#if defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    // DMA_DRM fragments carry memory:DMABuf too; GLMemory stays out so gst-gl can't take the import over.
    QTest::newRow("direct-dmabuf") << int(GstQgc::GpuBinMode::DirectDmaBuf) << openGl << true << passthroughChildren
                                   << QStringList{QStringLiteral("memory:DMABuf"), systemMemory} << false;
#endif
#if defined(QGC_GST_BIN_HAS_GLUPLOAD) && defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    const int dmaBufOrGlUpload = int(GstQgc::GpuBinMode::DmaBufOrGlUpload);
    // Without switchbin the variant is the glupload one.
    const bool memorySwitch = GstQgcTest::hasMemorySwitch();
    const QStringList switchChildren =
        memorySwitch ? withConverter({QStringLiteral("GstQgcQVideoSink"), QStringLiteral("switchbin")})
                     : gluploadChildren;
    // System memory stays out: the switchbin sends it through glupload, and DMABuf straight to the sink.
    QTest::newRow("dmabuf-or-glupload") << dmaBufOrGlUpload << openGl << true << switchChildren
                                        << (memorySwitch ? QStringList{QStringLiteral("memory:DMABuf")} + glMemory
                                                         : glMemory)
                                        << true;
    // GLX has no EGLImages to import DMABuf with.
    QTest::newRow("dmabuf-or-glupload-egl-unknown")
        << dmaBufOrGlUpload << openGl << false << switchChildren << glMemory << true;
#endif
#if !defined(QGC_GST_BIN_HAS_GLUPLOAD) && !defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    QStringList direct3D11Features;
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    // Offered where Qt's Direct3D 11 device opens it, as this test's environment says it does.
    direct3D11Features << QStringLiteral("memory:D3D12Memory");
#endif
    direct3D11Features << QStringLiteral("memory:D3D11Memory");
#endif
    QTest::newRow("passthrough-d3d11") << int(GstQgc::GpuBinMode::Passthrough) << int(GstQgc::RenderBackend::Direct3D11)
                                       << false << passthroughChildren << (direct3D11Features << systemMemory) << false;
    // vtdec's IOSurface-backed buffers arrive in system memory; GLMemory would be downloaded under Metal.
    QTest::newRow("passthrough-metal") << int(GstQgc::GpuBinMode::Passthrough) << int(GstQgc::RenderBackend::Metal)
                                       << false << passthroughChildren << QStringList{systemMemory} << false;
#endif
}

void GstQgcSinkTest::_testQgcVideoSinkBinGpuBinVariant()
{
    QFETCH(int, mode);
    QFETCH(int, backend);
    QFETCH(bool, egl);
    QFETCH(QStringList, children);
    QFETCH(QStringList, memoryFeatures);
    QFETCH(bool, singlePlaneRgb2D);

    const GstQgc::SinkEnvironment environment{
        .target = {.backend = static_cast<GstQgc::RenderBackend>(backend), .egl = egl},
        .gpuBin = {.mode = static_cast<GstQgc::GpuBinMode>(mode)},
        .dmaBufDisplay = kFakeQtEglDisplay,
        .d3d11OpensD3D12 = true};

    const GstQgc::SinkBin sinkBin = GstQgc::makeSinkBin({.gpuZeroCopy = true, .environment = environment});
    QVERIFY2(sinkBin, "Failed to build the sink bin (GPU branch)");
    QCOMPARE(GstTest::childFactoryNames(GST_BIN(sinkBin.bin.get())), children);

    const GStreamer::GstCapsPtr caps = sinkPadCaps(sinkBin.qvideosink.get());
    QVERIFY2(caps, "qgcqvideosink returned no caps");
    QCOMPARE(capsMemoryFeatures(caps.get()), memoryFeatures);
    // No element of the bin scales GPU memory, and the VideoOutput shows a frame's pixels as square.
    QCOMPARE(firstStructureWithoutSquarePixels(caps.get()), QString());

    if (!singlePlaneRgb2D) {
        return;
    }
    for (guint i = 0; i < gst_caps_get_size(caps.get()); ++i) {
        if (!gst_caps_features_contains(gst_caps_get_features(caps.get(), i), "memory:GLMemory")) {
            continue;
        }
        const GstStructure* structure = gst_caps_get_structure(caps.get(), i);
        QCOMPARE(QString::fromUtf8(gst_structure_get_string(structure, "texture-target")), QStringLiteral("2D"));
        const GValue* formats = gst_structure_get_value(structure, "format");
        QVERIFY2(formats && GST_VALUE_HOLDS_LIST(formats), "GLMemory caps format field is not a list");
        for (guint f = 0; f < gst_value_list_get_size(formats); ++f) {
            const char* name = g_value_get_string(gst_value_list_get_value(formats, f));
            const GstVideoFormatInfo* info = gst_video_format_get_info(gst_video_format_from_string(name));
            QVERIFY2(info && GST_VIDEO_FORMAT_INFO_IS_RGB(info) && (GST_VIDEO_FORMAT_INFO_N_PLANES(info) == 1),
                     qPrintable(QStringLiteral("GLMemory caps offer non-single-plane-RGB format %1").arg(name)));
        }
    }
}
#endif

void GstQgcSinkTest::_testQgcVideoSinkBinRefusesDrawingD3D11Converter_data()
{
    QTest::addColumn<QByteArray>("plugin");
    QTest::addColumn<QByteArray>("converter");
    QTest::addColumn<int>("backend");
    QTest::addColumn<bool>("suits");

    const QByteArray d3d11 = QByteArrayLiteral("d3d11");
    const int direct3D11 = int(GstQgc::RenderBackend::Direct3D11);
    QTest::newRow("d3d11convert") << d3d11 << QByteArrayLiteral("d3d11convert") << direct3D11 << false;
    QTest::newRow("d3d11deinterlace") << d3d11 << QByteArrayLiteral("d3d11deinterlace") << direct3D11 << false;
    // It only copies into the device's textures.
    QTest::newRow("d3d11upload") << d3d11 << QByteArrayLiteral("d3d11upload") << direct3D11 << true;
    // Qt renders on another device then.
    QTest::newRow("d3d11convert under Direct3D 12")
        << d3d11 << QByteArrayLiteral("d3d11convert") << int(GstQgc::RenderBackend::Direct3D12) << true;
    QTest::newRow("videoconvert") << QByteArrayLiteral("videoconvertscale") << QByteArrayLiteral("videoconvert")
                                  << direct3D11 << true;
}

void GstQgcSinkTest::_testQgcVideoSinkBinRefusesDrawingD3D11Converter()
{
    QFETCH(QByteArray, plugin);
    QFETCH(QByteArray, converter);
    QFETCH(int, backend);
    QFETCH(bool, suits);

    const GstQgc::RenderTarget target{.backend = static_cast<GstQgc::RenderBackend>(backend)};
    QCOMPARE(GstQgc::converterSuitsRenderTarget(plugin.constData(), converter.constData(), target), suits);

    // Where GStreamer has the element, the bin converts with its default instead, and says why.
    if (suits || !GstTest::firstMissingFactory({converter.constData()}).isEmpty()) {
        return;
    }
    expectLogMessage("Video.GStreamer.GstQgcGpuBin", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^conversion element %1 would draw on the device Qt renders")
                                            .arg(QString::fromLatin1(converter))));
    const GStreamer::GstElementPtr bin =
        GstQgc::makeSinkBin({.conversionElement = converter, .environment = GstQgc::SinkEnvironment{.target = target}})
            .bin;
    verifyExpectedLogMessage();
    QVERIFY2(bin, "Failed to build the sink bin");
    QCOMPARE(GstTest::childFactoryNames(GST_BIN(bin.get())), withConverter({QStringLiteral("GstQgcQVideoSink")}));
}

void GstQgcSinkTest::_testCpuZeroCopyFrameRejectsWritableMap()
{
    GstVideoInfo info;
    gst_video_info_init(&info);
    gst_video_info_set_format(&info, GST_VIDEO_FORMAT_BGRA, 4, 4);

    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&info), nullptr);
    QVERIFY(buffer);
    auto bufferGuard = qScopeGuard([&] { gst_buffer_unref(buffer); });

    GstMapInfo mapInfo;
    QVERIFY(gst_buffer_map(buffer, &mapInfo, GST_MAP_WRITE));
    std::memset(mapInfo.data, 0x7f, mapInfo.size);
    gst_buffer_unmap(buffer, &mapInfo);

    QVideoFrameFormat format(QSize(4, 4), QVideoFrameFormat::Format_BGRA8888);
    QVideoFrame frame = mapCpuFrame(buffer, info, format);
    QVERIFY(frame.isValid());

    QVERIFY(frame.map(QVideoFrame::ReadOnly));
    frame.unmap();

    QVERIFY(!frame.map(QVideoFrame::WriteOnly));
    QVERIFY(!frame.map(QVideoFrame::ReadWrite));
}

void GstQgcSinkTest::_testAllocationQueryHwMemoryPoolHint_data()
{
    QTest::addColumn<QByteArray>("minBuffersEnv");
    QTest::addColumn<int>("expectedMinBuffers");

    // Default: as many decoder buffers as one sink's frames hold in steady flow (kProposedMinBuffers).
    constexpr int kDefault = 5;
    QTest::newRow("default") << QByteArray() << kDefault;
    QTest::newRow("env-override") << QByteArrayLiteral("3") << 3;
    QTest::newRow("env-invalid") << QByteArrayLiteral("0") << kDefault;
}

void GstQgcSinkTest::_testAllocationQueryHwMemoryPoolHint()
{
    QFETCH(QByteArray, minBuffersEnv);
    QFETCH(int, expectedMinBuffers);
    const TestFixtures::EnvVarFixture envGuard("QGC_GST_PROPOSE_ALLOCATION_MIN_BUFFERS");
    if (minBuffersEnv.isEmpty()) {
        qunsetenv("QGC_GST_PROPOSE_ALLOCATION_MIN_BUFFERS");
    } else {
        qputenv("QGC_GST_PROPOSE_ALLOCATION_MIN_BUFFERS", minBuffersEnv);
    }

    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_caps_from_string(
        "video/x-raw(memory:DMABuf), format=(string)NV12, width=(int)64, height=(int)64, framerate=(fraction)30/1"));
    QVERIFY2(caps, "HW caps did not parse");

    const GStreamer::GstQueryPtr query = GStreamer::adoptQuery(gst_query_new_allocation(caps.get(), TRUE));
    QVERIFY(GstQgc::populateAllocationQuery(query.get()));

    QCOMPARE(gst_query_get_n_allocation_pools(query.get()), 1U);
    GstBufferPool* pool = nullptr;
    guint size = 0, minBuffers = 0, maxBuffers = 0;
    gst_query_parse_nth_allocation_pool(query.get(), 0, &pool, &size, &minBuffers, &maxBuffers);
    QVERIFY2(pool == nullptr, "GPU path must not propose a generic pool for native memory");
    QCOMPARE(int(minBuffers), expectedMinBuffers);
    QVERIFY2(gst_query_find_allocation_meta(query.get(), GST_VIDEO_META_API_TYPE, nullptr),
             "VideoMeta not advertised on GPU path");
    // No frame path reads it, and a producer that attaches it may skip uploading.
    QVERIFY(!gst_query_find_allocation_meta(query.get(), GST_VIDEO_GL_TEXTURE_UPLOAD_META_API_TYPE, nullptr));
}

void GstQgcSinkTest::_testAllocationQueryOffersCropMeta_data()
{
    QTest::addColumn<QString>("memoryFeature");
    QTest::addColumn<bool>("offered");

    // gst-va then sends a stream cropped on its left or top without a copy.
    QTest::newRow("DMABuf") << QStringLiteral("memory:DMABuf") << true;
    // The D3D paths copy the picture from the texture's origin, so the decoder is left to crop one that starts
    // elsewhere.
    QTest::newRow("D3D11Memory") << QStringLiteral("memory:D3D11Memory") << false;
    QTest::newRow("D3D12Memory") << QStringLiteral("memory:D3D12Memory") << false;
}

void GstQgcSinkTest::_testAllocationQueryOffersCropMeta()
{
    QFETCH(QString, memoryFeature);
    QFETCH(bool, offered);

    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_caps_from_string(
        qPrintable(QStringLiteral("video/x-raw(%1), format=(string)NV12, width=(int)64, height=(int)64, "
                                  "framerate=(fraction)30/1")
                       .arg(memoryFeature))));
    QVERIFY2(caps, "HW caps did not parse");
    const GStreamer::GstQueryPtr query = GStreamer::adoptQuery(gst_query_new_allocation(caps.get(), TRUE));
    QVERIFY(GstQgc::populateAllocationQuery(query.get()));

    QCOMPARE(bool(gst_query_find_allocation_meta(query.get(), GST_VIDEO_CROP_META_API_TYPE, nullptr)), offered);
    QVERIFY(gst_query_find_allocation_meta(query.get(), GST_VIDEO_META_API_TYPE, nullptr));
}

#if defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
void GstQgcSinkTest::_testQgcVideoSinkBinAllocationQueryAdvertisesVideoMeta()
{
    const GstQgc::SinkEnvironment environment{.target = kOpenGlOnEgl,
                                              .gpuBin = {.mode = GstQgc::GpuBinMode::DirectDmaBuf},
                                              .dmaBufDisplay = kFakeQtEglDisplay};

    const GStreamer::GstElementPtr bin = GstQgc::makeSinkBin({.gpuZeroCopy = true, .environment = environment}).bin;
    QVERIFY2(bin, "Failed to build the sink bin");
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(bin.get(), "sink"));
    QVERIFY2(sinkPad, "the sink bin has no sink pad");
    const GStreamer::GstPadPtr src = GStreamer::adoptFloating(gst_pad_new("src", GST_PAD_SRC));
    QCOMPARE(gst_pad_link(src.get(), sinkPad.get()), GST_PAD_LINK_OK);
    QVERIFY(gst_pad_set_active(src.get(), TRUE));
    const auto stop = qScopeGuard([&bin, &src] {
        (void) gst_element_set_state(bin.get(), GST_STATE_NULL);
        (void) gst_pad_set_active(src.get(), FALSE);
    });
    QVERIFY(gst_element_set_state(bin.get(), GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);

    // Negotiated as a VA decoder would, so the converter passes the DMABuf frames and their ALLOCATION query through.
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_caps_from_string(
        "video/x-raw(memory:DMABuf), format=(string)NV12, width=(int)64, height=(int)64, framerate=(fraction)30/1"));
    QVERIFY2(caps, "HW caps did not parse");
    GstSegment segment;
    gst_segment_init(&segment, GST_FORMAT_TIME);
    QVERIFY(gst_pad_push_event(src.get(), gst_event_new_stream_start("qgc-allocation")));
    QVERIFY(gst_pad_push_event(src.get(), gst_event_new_caps(caps.get())));
    QVERIFY(gst_pad_push_event(src.get(), gst_event_new_segment(&segment)));

    const GStreamer::GstQueryPtr query = GStreamer::adoptQuery(gst_query_new_allocation(caps.get(), TRUE));
    QVERIFY2(query, "Could not allocate ALLOCATION query");
    QVERIFY2(gst_pad_peer_query(src.get(), query.get()), "the sink bin did not answer ALLOCATION query");
    QVERIFY2(gst_query_find_allocation_meta(query.get(), GST_VIDEO_META_API_TYPE, nullptr),
             "the sink bin's ALLOCATION query must advertise VideoMeta for gst-va DMABuf");

    QCOMPARE(gst_query_get_n_allocation_pools(query.get()), 1U);
    GstBufferPool* pool = nullptr;
    guint size = 0, minBuffers = 0, maxBuffers = 0;
    gst_query_parse_nth_allocation_pool(query.get(), 0, &pool, &size, &minBuffers, &maxBuffers);
    QVERIFY2(pool == nullptr, "the sink bin must not propose a generic pool for native memory");
    QVERIFY2(minBuffers > 0, "the sink bin's min-buffer hint must be non-zero");
}
#endif

void GstQgcSinkTest::_testQgcQVideoSinkSystemMemoryAllocation_data()
{
    QTest::addColumn<bool>("needPool");

    QTest::newRow("pool needed") << true;
    // A producer with a pool of its own still learns how many buffers the sink holds.
    QTest::newRow("no pool needed") << false;
}

void GstQgcSinkTest::_testQgcQVideoSinkSystemMemoryAllocation()
{
    QFETCH(bool, needPool);

    const GStreamer::GstElementPtr sink = GstQgcTest::makeVideoSink();
    QVERIFY(sink);
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(sink.get(), "sink"));
    QVERIFY(sinkPad);
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_caps_from_string(
        "video/x-raw, format=(string)BGRA, width=(int)64, height=(int)64, framerate=(fraction)30/1"));
    QVERIFY2(caps, "System-memory caps did not parse");
    const GStreamer::GstQueryPtr query = GStreamer::adoptQuery(gst_query_new_allocation(caps.get(), needPool));

    QVERIFY2(gst_pad_query(sinkPad.get(), query.get()), "qgcqvideosink did not answer ALLOCATION");
    QCOMPARE(gst_query_get_n_allocation_pools(query.get()), 1U);
    GstBufferPool* pool = nullptr;
    guint minBuffers = 0;
    gst_query_parse_nth_allocation_pool(query.get(), 0, &pool, nullptr, &minBuffers, nullptr);
    const GStreamer::GstObjectPtr ownedPool = GStreamer::adoptObject(pool ? GST_OBJECT(pool) : nullptr);
    QCOMPARE(int(minBuffers), 5);
    QCOMPARE(pool != nullptr, needPool);
    if (needPool) {
        QVERIFY2(gst_buffer_pool_has_option(pool, GST_BUFFER_POOL_OPTION_VIDEO_META),
                 "proposed pool must support VIDEO_META so decoders can write into it directly");
    }
    // Crop, orientation and video metadata must survive into GstQgcFrameMap either way.
    QVERIFY2(gst_query_find_allocation_meta(query.get(), GST_VIDEO_META_API_TYPE, nullptr),
             "VideoMeta not advertised for system-memory caps");
    QVERIFY2(gst_query_find_allocation_meta(query.get(), GST_VIDEO_CROP_META_API_TYPE, nullptr),
             "VideoCropMeta not advertised for system-memory caps");
}

void GstQgcSinkTest::_testQgcQVideoSinkAllocationNeedsCaps()
{
    const GStreamer::GstElementPtr sink = GstQgcTest::makeVideoSink();
    QVERIFY(sink);
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(sink.get(), "sink"));
    QVERIFY(sinkPad);

    // Nothing to propose for: the query fails, as in upstream's sinks, and its sender allocates by its defaults.
    const GStreamer::GstQueryPtr query = GStreamer::adoptQuery(gst_query_new_allocation(nullptr, TRUE));
    QVERIFY(!gst_pad_query(sinkPad.get(), query.get()));
    QCOMPARE(gst_query_get_n_allocation_pools(query.get()), 0U);
}

void GstQgcSinkTest::_testAdapterFlushDropsInFlightSamples()
{
    // Counts come from the element: latest-frame-only delivery coalesces videoFrameChanged emissions.
    const auto counts = [](GstElement* sink) {
        const GstQgc::SinkStats stats = GstQgc::sinkStats(sink);
        return std::array<quint64, 3>{stats.input, stats.delivered, stats.dropped};
    };

    QVideoSink videoSink;
    GstQgcTest::SinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());
    QCOMPARE(harness.pushFrame(GST_SECOND), GST_FLOW_OK);

    // While flushing, the base sink refuses buffers before show_frame.
    QVERIFY(harness.pushEvent(gst_event_new_flush_start()));
    const auto beforeFlushedFrame = counts(harness.element());
    QCOMPARE(harness.pushFrame(2 * GST_SECOND), GST_FLOW_FLUSHING);
    QCOMPARE(counts(harness.element()), beforeFlushedFrame);

    // After the flush, every buffer the sink saw is accounted for as delivered or dropped, none left in flight.
    QVERIFY(harness.pushEvent(gst_event_new_flush_stop(/*reset_time=*/TRUE)));
    QVERIFY(harness.pushSegment());
    QCOMPARE(harness.pushFrame(3 * GST_SECOND), GST_FLOW_OK);
    const auto [input, delivered, dropped] = counts(harness.element());
    QCOMPARE(input, quint64(2));
    QCOMPARE(delivered + dropped, input);
}

void GstQgcSinkTest::_testQgcQVideoSinkDeliversLatestFrameOnly()
{
    int emissions = 0;
    qint64 lastStartUs = -1;
    QVideoSink videoSink;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &videoSink, [&](const QVideoFrame& frame) {
        ++emissions;
        lastStartUs = frame.startTime();
    });
    GstQgcTest::SinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());

    // Three frames while the GUI thread is "stalled": all are handed off, but only the newest is shown. The first is
    // also the preroll buffer, which must not be shown twice.
    for (int i = 1; i <= 3; ++i) {
        QCOMPARE(harness.pushFrame(i * GST_SECOND), GST_FLOW_OK);
    }
    QCOMPARE(GstQgc::sinkStats(harness.element()).input, quint64(3));
    QCOMPARE(harness.delivered(), guint64(3));
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    QCOMPARE(emissions, 1);
    QCOMPARE(lastStartUs, qint64(3 * GST_SECOND / GST_USECOND));
    QCOMPARE(GstQgc::sinkStats(harness.element()).superseded, quint64(2));

    // Delivery re-arms once the GUI side has taken the frame.
    QCOMPARE(harness.pushFrame(4 * GST_SECOND), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    QCOMPARE(emissions, 2);
    QCOMPARE(lastStartUs, qint64(4 * GST_SECOND / GST_USECOND));
    QCOMPARE(GstQgc::sinkStats(harness.element()).superseded, quint64(2));
}

void GstQgcSinkTest::_testQgcQVideoSinkDisplayedCountNeverFalls()
{
    QVideoSink videoSink;
    GstQgcTest::SinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());

    // Readers count the frames displayed as those delivered less those superseded (SinkStats::displayed()), so no
    // snapshot may see a delivery without the frame it superseded, or the other way round. With the GUI thread taking
    // none, each frame a streaming thread pushes supersedes the one before it.
    const auto displayed = [&harness] {
        const GstQgc::SinkStats stats = GstQgc::sinkStats(harness.element());
        quint64 delivered = 0;
        for (const quint64 frames : stats.pathFrames) {
            delivered += frames;
        }
        return static_cast<qint64>(delivered) - static_cast<qint64>(stats.superseded);
    };
    QCOMPARE(harness.pushFrame(GST_MSECOND), GST_FLOW_OK);
    QCOMPARE(displayed(), qint64(1));

    constexpr int kFrames = 20000;
    std::atomic<bool> done{false};
    std::thread streaming([&harness, &done] {
        for (int i = 2; i <= kFrames; ++i) {
            (void) harness.pushFrame(i * GST_MSECOND);
        }
        done.store(true);
    });
    int snapshots = 0;
    int wrong = 0;
    while (!done.load()) {
        ++snapshots;
        wrong += (displayed() != 1) ? 1 : 0;
    }
    streaming.join();
    QVERIFY(snapshots > 0);
    QCOMPARE(wrong, 0);
    QCOMPARE(displayed(), qint64(1));
}

void GstQgcSinkTest::_testQgcQVideoSinkHoldsTwoPoolBuffersWhileGuiStalls()
{
    // A decoder's pool of a fixed size: a producer that acquires from it once every buffer is out waits for one back.
    constexpr int kPoolBuffers = 6;
    const std::unique_ptr<GstBufferPool, GStreamer::GstObjectDeleter> pool{gst_video_buffer_pool_new()};
    QVERIFY(pool);
    GstVideoInfo info;
    QVERIFY(gst_video_info_set_format(&info, GST_VIDEO_FORMAT_BGRA, GstQgcTest::SinkHarness::kWidth,
                                      GstQgcTest::SinkHarness::kHeight));
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_video_info_to_caps(&info));
    GstStructure* const config = gst_buffer_pool_get_config(pool.get());
    gst_buffer_pool_config_set_params(config, caps.get(), static_cast<guint>(GST_VIDEO_INFO_SIZE(&info)), kPoolBuffers,
                                      kPoolBuffers);
    QVERIFY(gst_buffer_pool_set_config(pool.get(), config));
    QVERIFY(gst_buffer_pool_set_active(pool.get(), TRUE));
    const auto deactivate = qScopeGuard([&pool] { (void) gst_buffer_pool_set_active(pool.get(), FALSE); });
    // Null where a producer would wait.
    const auto acquire = [&pool](GstClockTime pts) -> GstBuffer* {
        GstBufferPoolAcquireParams params = {};
        params.flags = GST_BUFFER_POOL_ACQUIRE_FLAG_DONTWAIT;
        GstBuffer* buffer = nullptr;
        if (gst_buffer_pool_acquire_buffer(pool.get(), &buffer, &params) != GST_FLOW_OK) {
            return nullptr;
        }
        GST_BUFFER_PTS(buffer) = pts;
        return buffer;
    };
    const auto heldDownstream = [&acquire] {
        std::vector<GstBuffer*> available;
        while (GstBuffer* const buffer = acquire(GST_CLOCK_TIME_NONE)) {
            available.push_back(buffer);
        }
        for (GstBuffer* const buffer : available) {
            gst_buffer_unref(buffer);
        }
        return kPoolBuffers - static_cast<int>(available.size());
    };

    QVideoSink videoSink;
    GstQgcTest::SinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());
    QCOMPARE(harness.pushBuffer(acquire(GST_MSECOND)), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    QVERIFY(videoSink.videoFrame().isValid());

    // The GUI thread stalls while the decoder goes on: every frame maps in place and pins its buffer, yet the pool
    // never runs dry, as each frame lets the one it supersedes go.
    for (int i = 2; i <= 2000; ++i) {
        GstBuffer* const buffer = acquire(i * GST_MSECOND);
        QVERIFY2(buffer, "the pool ran dry: a decoder would wait for a buffer here");
        QCOMPARE(harness.pushBuffer(buffer), GST_FLOW_OK);
    }
    // The frame Qt shows and the one waiting for the GUI thread.
    QCOMPARE(heldDownstream(), 2);

    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    QCOMPARE(heldDownstream(), 1);
}

void GstQgcSinkTest::_testRenderBackendNames()
{
    using GstQgc::RenderBackend;
    QSet<QString> names;
    for (const RenderBackend backend :
         {RenderBackend::Unknown, RenderBackend::Null, RenderBackend::OpenGL, RenderBackend::Direct3D11,
          RenderBackend::Direct3D12, RenderBackend::Metal, RenderBackend::Vulkan}) {
        names.insert(QString::fromLatin1(GstQgc::toString(backend)));
    }
    QCOMPARE(names.size(), 7);
}

void GstQgcSinkTest::_testRenderTargetSharesGlContext()
{
    using GstQgc::RenderBackend;
    QVERIFY(kOpenGlOnEgl.sharesGlContext());
    // Qt's OpenGL off EGL is GLX on Linux, which gst-gl wraps too, but WGL or CGL elsewhere, which it doesn't.
    constexpr GstQgc::RenderTarget kOpenGlOffEgl{.backend = RenderBackend::OpenGL, .egl = false};
#ifdef Q_OS_LINUX
    QVERIFY(kOpenGlOffEgl.sharesGlContext());
#else
    QVERIFY(!kOpenGlOffEgl.sharesGlContext());
#endif
    QVERIFY(!GstQgc::RenderTarget{.backend = RenderBackend::Direct3D11}.sharesGlContext());
    QVERIFY(!GstQgc::RenderTarget{.backend = RenderBackend::Vulkan}.sharesGlContext());
}

void GstQgcSinkTest::_testCurrentRenderTarget()
{
    // Before a scene graph runs: the API Qt Quick was asked to use, which the GUI thread reads.
    QVERIFY(GstQgc::currentRenderTarget().backend != GstQgc::RenderBackend::Unknown);
}

void GstQgcSinkTest::_testQgcQVideoSinkAppliesCropMeta()
{
    QVideoSink videoSink;
    GstQgcTest::SinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());

    GstBuffer* const buffer = GstQgcTest::SinkHarness::makeFrame(GST_SECOND);
    GstVideoCropMeta* const crop = gst_buffer_add_video_crop_meta(buffer);
    crop->x = 1;
    crop->y = 1;
    crop->width = 2;
    crop->height = 3;
    QCOMPARE(harness.pushBuffer(buffer), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);

    const QVideoFrameFormat format = videoSink.videoFrame().surfaceFormat();
    QCOMPARE(format.viewport(), QRect(1, 1, 2, 3));
    QCOMPARE(format.frameSize(), QSize(GstQgcTest::SinkHarness::kWidth, GstQgcTest::SinkHarness::kHeight));
}

void GstQgcSinkTest::_testQgcQVideoSinkShowsPictureOfLargerBuffer()
{
    QVideoSink videoSink;
    GstQgcTest::SinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());

    // As gst-va sends a stream cropped on its left or top: the GstVideoMeta has the uncropped size, larger than the
    // caps, and the crop meta the picture in it.
    constexpr int kBufferWidth = GstQgcTest::SinkHarness::kWidth + 2;
    constexpr int kBufferHeight = GstQgcTest::SinkHarness::kHeight + 2;
    QByteArray pixels(kBufferWidth * kBufferHeight * 4, Qt::Uninitialized);
    for (qsizetype i = 0; i < pixels.size(); ++i) {
        pixels[i] = static_cast<char>(i);
    }
    GstBuffer* const buffer = GstQgcTest::SinkHarness::makeFrame(GST_SECOND, static_cast<gsize>(pixels.size()));
    gst_buffer_fill(buffer, 0, pixels.constData(), static_cast<gsize>(pixels.size()));
    gst_buffer_add_video_meta(buffer, GST_VIDEO_FRAME_FLAG_NONE, GST_VIDEO_FORMAT_BGRA, kBufferWidth, kBufferHeight);
    GstVideoCropMeta* const crop = gst_buffer_add_video_crop_meta(buffer);
    crop->x = 1;
    crop->y = 2;
    crop->width = GstQgcTest::SinkHarness::kWidth;
    crop->height = GstQgcTest::SinkHarness::kHeight;
    QCOMPARE(harness.pushBuffer(buffer), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);

    // Qt takes the viewport as a part of the frame, so the frame is the whole buffer.
    QVideoFrame frame = videoSink.videoFrame();
    QCOMPARE(frame.size(), QSize(kBufferWidth, kBufferHeight));
    QCOMPARE(frame.surfaceFormat().viewport(),
             QRect(1, 2, GstQgcTest::SinkHarness::kWidth, GstQgcTest::SinkHarness::kHeight));
    QVERIFY(frame.map(QVideoFrame::ReadOnly));
    const auto unmap = qScopeGuard([&frame] { frame.unmap(); });
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(frame.bits(0)), frame.mappedBytes(0)), pixels);
}

void GstQgcSinkTest::_testQgcQVideoSinkAppliesAffineTransformationMeta_data()
{
    // SurfaceTexture's transform, which amcvideodec turns into the meta: the texture coordinate of the picture's
    // point (s, t), t upwards, is (xScale * s + xOffset, yScale * t + yOffset). turned: a quarter turn instead.
    QTest::addColumn<float>("xScale");
    QTest::addColumn<float>("xOffset");
    QTest::addColumn<float>("yScale");
    QTest::addColumn<float>("yOffset");
    QTest::addColumn<bool>("turned");
    QTest::addColumn<QString>("tag");
    QTest::addColumn<QRect>("viewport");
    QTest::addColumn<int>("rotation");
    QTest::addColumn<bool>("mirrored");

    const QRect whole(0, 0, GstQgcTest::SinkHarness::kWidth, GstQgcTest::SinkHarness::kHeight);
    const int none = int(QtVideo::Rotation::None);
    // The texture holds the picture top row first, which GL addresses from the bottom.
    QTest::newRow("upright") << 1.0F << 0.0F << -1.0F << 1.0F << false << QString() << whole << none << false;
    // The decoder's buffer is larger than the picture: its middle two columns and top three rows of four.
    QTest::newRow("cropped") << 0.5F << 0.25F << -0.75F << 0.75F << false << QString() << QRect(1, 0, 2, 3) << none
                             << false;
    QTest::newRow("mirrored") << -1.0F << 1.0F << -1.0F << 1.0F << false << QString() << whole << none << true;
    QTest::newRow("upside-down") << 1.0F << 0.0F << 1.0F << 0.0F << false << QString() << whole
                                 << int(QtVideo::Rotation::Clockwise180) << true;
    // The flip comes before the stream's own orientation: top to bottom, then a quarter turn clockwise.
    QTest::newRow("upside-down, stream rotated") << 1.0F << 0.0F << 1.0F << 0.0F << false << QStringLiteral("rotate-90")
                                                 << whole << int(QtVideo::Rotation::Clockwise90) << true;
    // No viewport shows a turned texture: the frame is left as it came.
    QTest::newRow("turned") << 1.0F << 0.0F << -1.0F << 1.0F << true << QString() << whole << none << false;
}

void GstQgcSinkTest::_testQgcQVideoSinkAppliesAffineTransformationMeta()
{
    QFETCH(float, xScale);
    QFETCH(float, xOffset);
    QFETCH(float, yScale);
    QFETCH(float, yOffset);
    QFETCH(bool, turned);
    QFETCH(QString, tag);
    QFETCH(QRect, viewport);
    QFETCH(int, rotation);
    QFETCH(bool, mirrored);

    QVideoSink videoSink;
    GstQgcTest::SinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());
    if (!tag.isEmpty()) {
        QVERIFY(harness.pushEvent(
            gst_event_new_tag(gst_tag_list_new(GST_TAG_IMAGE_ORIENTATION, qPrintable(tag), nullptr))));
    }

    // As amcvideodec composes it: the inverse of the texture transform, then a flip of the y axis.
    GstBuffer* const buffer = GstQgcTest::SinkHarness::makeFrame(GST_SECOND);
    GstVideoAffineTransformationMeta* const meta = gst_buffer_add_video_affine_transformation_meta(buffer);
    const gfloat inverse[16] = {1.0F / xScale, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F / yScale,     0.0F,
                                0.0F,          0.0F, 0.0F, 1.0F, 0.0F, -xOffset / xScale, -yOffset / yScale,
                                0.0F,          1.0F};
    const gfloat quarterTurn[16] = {0.0F, 1.0F, 0.0F, 0.0F, -1.0F, 0.0F, 0.0F, 0.0F,
                                    0.0F, 0.0F, 1.0F, 0.0F, 1.0F,  0.0F, 0.0F, 1.0F};
    const gfloat yFlip[16] = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, -1.0F, 0.0F, 0.0F,
                              0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F,  0.0F, 1.0F};
    gst_video_affine_transformation_meta_apply_matrix(meta, turned ? quarterTurn : inverse);
    gst_video_affine_transformation_meta_apply_matrix(meta, yFlip);
    if (turned) {
        // Once per process.
        ignoreLogMessage("Video.GStreamer.FrameMap", QtWarningMsg,
                         QRegularExpression(QStringLiteral("GstVideoAffineTransformationMeta")));
    }
    QCOMPARE(harness.pushBuffer(buffer), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);

    const QVideoFrame frame = videoSink.videoFrame();
    QVERIFY(frame.isValid());
    QCOMPARE(frame.surfaceFormat().viewport(), viewport);
    QCOMPARE(int(frame.rotation()), rotation);
    QCOMPARE(frame.mirrored(), mirrored);
    // What the viewport leaves out is the padding squeezed into the texture with the picture, which keeps its size.
    QCOMPARE(GstQgc::sinkPictureSize(harness.element()),
             QSize(GstQgcTest::SinkHarness::kWidth, GstQgcTest::SinkHarness::kHeight));
}

void GstQgcSinkTest::_testQgcQVideoSinkStatsArePerSink()
{
    QVideoSink videoSinkA;
    QVideoSink videoSinkB;
    GstQgcTest::SinkHarness harnessA(&videoSinkA);
    GstQgcTest::SinkHarness harnessB(&videoSinkB);
    QVERIFY(harnessA.isReady() && harnessB.isReady());

    QCOMPARE(harnessA.pushFrame(GST_SECOND), GST_FLOW_OK);
    QCOMPARE(harnessA.pushFrame(2 * GST_SECOND), GST_FLOW_OK);
    QCOMPARE(harnessB.pushFrame(GST_SECOND), GST_FLOW_OK);

    QCOMPARE(GstQgc::sinkStats(harnessA.element()).frames(HwVideoBufferPath::None), quint64(2));
    QCOMPARE(GstQgc::sinkStats(harnessB.element()).frames(HwVideoBufferPath::None), quint64(1));
    QCOMPARE(GstQgcTest::framesDelivered(harnessB.element()), guint64(1));

    const GstQgc::SinkStats stats = GstQgc::sinkStats(harnessA.element());
    const GstQgc::PathStats formatted = GstQgc::formatSinkStats(stats);
    QCOMPARE(formatted.line, QStringLiteral(" CPU:2"));
    QCOMPARE(formatted.totalDelivered, quint64(0));

    // A receiver reuses its sink across sessions and reports each session from its start.
    QCOMPARE(harnessA.pushFrame(3 * GST_SECOND), GST_FLOW_OK);
    const GstQgc::SinkStats later = GstQgc::sinkStats(harnessA.element());
    QCOMPARE(GstQgc::formatSinkStats(later - stats).line, QStringLiteral(" CPU:1"));
}

void GstQgcSinkTest::_testQgcQVideoSinkKeepsBaseSinkStats()
{
    const GStreamer::GstElementPtr sink = GstQgcTest::makeVideoSink();
    QVERIFY(sink);

    // Generic tools read GstBaseSink's "stats"; the sink keeps its own accounting apart (GstQgc::sinkStats()).
    GstStructure* rawStats = nullptr;
    g_object_get(sink.get(), "stats", &rawStats, nullptr);
    const GStreamer::GstStructurePtr stats = GStreamer::adoptStructure(rawStats);
    QVERIFY(stats);
    QVERIFY(gst_structure_has_field(stats.get(), "rendered"));
}

#if defined(QGC_HAS_ANY_GPU_PATH)
void GstQgcSinkTest::_testQgcQVideoSinkGpuPathSelection_data()
{
    QTest::addColumn<bool>("gpuZeroCopy");
    QTest::addColumn<QString>("memoryFeature");
    QTest::addColumn<int>("backend");
    QTest::addColumn<bool>("fdMemory");
    QTest::addColumn<qint64>("expectedGpuFallbacks");
    QTest::addColumn<quint64>("expectedNoPathVerdicts");

    const int openGl = int(GstQgc::RenderBackend::OpenGL);
    // Every row pushes three buffers, in system memory unless fdMemory.
    QTest::newRow("gpu-zerocopy-off") << false << QString() << openGl << true << qint64(0) << quint64(0);
    // Software-decoded frames never enter path selection.
    QTest::newRow("system-memory-caps") << true << QString() << openGl << false << qint64(0) << quint64(0);
    // Other memory under plain video/x-raw caps (vtdec's CoreVideo buffers, V4L2 dmabuf export) still tries the GPU
    // paths: the first frame finds no path for this allocator, the rest reuse that verdict.
    QTest::newRow("system-memory-caps-fd-buffers") << true << QString() << openGl << true << qint64(3) << quint64(1);
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QTest::newRow("gpu-caps-system-buffers")
        << true << QStringLiteral("memory:DMABuf") << openGl << false << qint64(3) << quint64(1);
#elif defined(QGC_HAS_GST_D3D11_GPU_PATH)
    QTest::newRow("gpu-caps-system-buffers")
        << true << QStringLiteral("memory:D3D11Memory") << int(GstQgc::RenderBackend::Direct3D11) << false << qint64(3)
        << quint64(1);
#endif
}

void GstQgcSinkTest::_testQgcQVideoSinkGpuPathSelection()
{
    QFETCH(bool, gpuZeroCopy);
    QFETCH(QString, memoryFeature);
    QFETCH(int, backend);
    QFETCH(bool, fdMemory);
    QFETCH(qint64, expectedGpuFallbacks);
    QFETCH(quint64, expectedNoPathVerdicts);
#if !defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
    if (fdMemory) {
        QSKIP("fd-backed test buffers need memfd (desktop Linux)");
    }
#endif
    // A render target that imports the row's GPU memory, so the sink accepts its caps.
    const GstQgc::SinkEnvironment environment{
        .target = {.backend = static_cast<GstQgc::RenderBackend>(backend), .egl = true},
        .dmaBufDisplay = kFakeQtEglDisplay};

    QVideoSink videoSink;
    const QByteArray feature = memoryFeature.toUtf8();
    GstQgcTest::SinkHarness harness(&videoSink, gpuZeroCopy, feature.isEmpty() ? nullptr : feature.constData(),
                                    environment);
    QVERIFY(harness.isReady());

    using GstHwPathTelemetry::HwFallbackReason;
    const quint64 noPathBefore =
        GstHwPathTelemetry::snapshot().count(HwVideoBufferPath::None, HwFallbackReason::UnknownMemType);
    for (int i = 1; i <= 3; ++i) {
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
        if (fdMemory) {
            GstBuffer* const buffer = GstQgcTest::SinkHarness::makeFdFrame(i * GST_SECOND);
            QVERIFY(buffer);
            QCOMPARE(harness.pushBuffer(buffer), GST_FLOW_OK);
            continue;
        }
#endif
        QCOMPARE(harness.pushFrame(i * GST_SECOND), GST_FLOW_OK);
    }
    const quint64 noPathAfter =
        GstHwPathTelemetry::snapshot().count(HwVideoBufferPath::None, HwFallbackReason::UnknownMemType);

    const GstQgc::SinkStats stats = GstQgc::sinkStats(harness.element());
    QCOMPARE(stats.frames(HwVideoBufferPath::None), quint64(3));
    QCOMPARE(qint64(stats.gpuFallbacks), expectedGpuFallbacks);
    QCOMPARE(qint64(stats.streamDemotions), qint64(expectedGpuFallbacks > 0 ? 1 : 0));
    QCOMPARE(noPathAfter - noPathBefore, expectedNoPathVerdicts);
}
#endif

void GstQgcSinkTest::_testQgcQVideoSinkCapsFollowRenderTarget_data()
{
    QTest::addColumn<bool>("gpuZeroCopy");
    QTest::addColumn<int>("backend");
    QTest::addColumn<bool>("egl");
    QTest::addColumn<QStringList>("memoryFeatures");

    const QString systemMemory = QStringLiteral("memory:SystemMemory");
    const QStringList systemOnly{systemMemory};
    QStringList openGlOnEgl;
    // GLX on Linux; WGL or CGL elsewhere, which gst-gl can't share.
    QStringList openGlNotEgl;
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    openGlOnEgl << QStringLiteral("memory:DMABuf");
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    openGlOnEgl << QStringLiteral("memory:GLMemory");
#if defined(Q_OS_LINUX)
    openGlNotEgl << QStringLiteral("memory:GLMemory");
#endif
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    openGlOnEgl << QStringLiteral("memory:AHardwareBuffer");
#endif
    // No D3D12Memory: Qt's device isn't known to open it here.
    QStringList direct3D11;
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    direct3D11 << QStringLiteral("memory:D3D11Memory");
#endif

    using GstQgc::RenderBackend;
    QTest::newRow("cpu-sink") << false << int(RenderBackend::OpenGL) << true << systemOnly;
    QTest::newRow("unknown") << true << int(RenderBackend::Unknown) << false << systemOnly;
    QTest::newRow("null") << true << int(RenderBackend::Null) << false << systemOnly;
    QTest::newRow("opengl-egl") << true << int(RenderBackend::OpenGL) << true << (openGlOnEgl << systemMemory);
    QTest::newRow("opengl-glx") << true << int(RenderBackend::OpenGL) << false << (openGlNotEgl << systemMemory);
    QTest::newRow("vulkan") << true << int(RenderBackend::Vulkan) << false << systemOnly;
    QTest::newRow("metal") << true << int(RenderBackend::Metal) << false << systemOnly;
    QTest::newRow("d3d11") << true << int(RenderBackend::Direct3D11) << false << (direct3D11 << systemMemory);
    QTest::newRow("d3d12") << true << int(RenderBackend::Direct3D12) << false << systemOnly;
}

void GstQgcSinkTest::_testQgcQVideoSinkCapsFollowRenderTarget()
{
    QFETCH(bool, gpuZeroCopy);
    QFETCH(int, backend);
    QFETCH(bool, egl);
    QFETCH(QStringList, memoryFeatures);

    const GStreamer::GstElementPtr sink = GstQgcTest::makeVideoSink(gpuZeroCopy);
    QVERIFY(sink);
    GstQgc::setSinkEnvironment(sink.get(),
                               {.target = {.backend = static_cast<GstQgc::RenderBackend>(backend), .egl = egl},
                                .dmaBufDisplay = kFakeQtEglDisplay});
    const GStreamer::GstCapsPtr caps = sinkPadCaps(sink.get());
    QVERIFY(caps);
    QCOMPARE(capsMemoryFeatures(caps.get()), memoryFeatures);
    QCOMPARE(firstStructureWithoutSquarePixels(caps.get()), QString());
    // The GL path samples only GL_TEXTURE_2D; amcvideodec's external-OES textures would show black.
    for (guint i = 0; i < gst_caps_get_size(caps.get()); ++i) {
        const GstCapsFeatures* const features = gst_caps_get_features(caps.get(), i);
        if (features && gst_caps_features_contains(features, "memory:GLMemory")) {
            QCOMPARE(
                QString::fromUtf8(gst_structure_get_string(gst_caps_get_structure(caps.get(), i), "texture-target")),
                QStringLiteral("2D"));
        }
    }
}

void GstQgcSinkTest::_testQgcQVideoSinkReconfiguresOnRenderTargetChange()
{
    QVideoSink videoSink;
    const GstQgc::SinkEnvironment vulkan{.target = {.backend = GstQgc::RenderBackend::Vulkan}};
    GstQgcTest::SinkHarness harness(&videoSink, /*gpuZeroCopy=*/true, /*memoryFeature=*/nullptr, vulkan);
    QVERIFY(harness.isReady());
    (void) harness.takeReconfigure();  // linking marks a source pad for reconfiguration

    QCOMPARE(harness.pushFrame(GST_SECOND), GST_FLOW_OK);
    QVERIFY(!harness.takeReconfigure());

    // E.g. the scene graph came up on OpenGL after the stream negotiated: GPU memory became importable.
    GstQgc::setSinkEnvironment(harness.element(), {.target = kOpenGlOnEgl});
    QCOMPARE(harness.pushFrame(2 * GST_SECOND), GST_FLOW_OK);
    QVERIFY(harness.takeReconfigure());
    QCOMPARE(harness.pushFrame(3 * GST_SECOND), GST_FLOW_OK);
    QVERIFY2(!harness.takeReconfigure(), "one change must ask for one renegotiation");

    const GStreamer::GstCapsPtr caps = sinkPadCaps(harness.element());
    QVERIFY(caps);
    QCOMPARE(harness.pushFrame(4 * GST_SECOND), GST_FLOW_OK);
    QVERIFY(!harness.takeReconfigure());

    GstQgc::setSinkEnvironment(harness.element(), vulkan);
    QCOMPARE(harness.pushFrame(5 * GST_SECOND), GST_FLOW_OK);
    QVERIFY(harness.takeReconfigure());
}

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
void GstQgcSinkTest::_testQgcQVideoSinkReconfiguresOnDmaBufImportChange()
{
    GstDmaBufVideoBuffer::resetCachedState();
    const auto resetLatch = qScopeGuard([] { GstDmaBufVideoBuffer::resetCachedState(); });
    const auto offersDmaBuf = [](GstElement* sink) {
        const GStreamer::GstCapsPtr caps = sinkPadCaps(sink);
        return caps && capsMemoryFeatures(caps.get()).contains(QStringLiteral("memory:DMABuf"));
    };

    QVideoSink videoSink;
    GstQgcTest::SinkHarness harness(
        &videoSink, /*gpuZeroCopy=*/true, /*memoryFeature=*/nullptr,
        GstQgc::SinkEnvironment{.target = kOpenGlOnEgl, .dmaBufDisplay = kFakeQtEglDisplay});
    QVERIFY(harness.isReady());
    (void) harness.takeReconfigure();  // linking marks a source pad for reconfiguration
    QVERIFY(offersDmaBuf(harness.element()));
    QCOMPARE(harness.pushFrame(GST_SECOND), GST_FLOW_OK);
    QVERIFY(!harness.takeReconfigure());

    // An import failure no later frame avoids: the sink must stop offering DMABuf (DMA_DRM included) at once, or a
    // tiled stream would stay negotiated and its frames could only be dropped.
    expectLogMessage("Video.GStreamer.HwBuffers.GstDmaBuf", QtWarningMsg,
                     QRegularExpression(QStringLiteral("EGL_BAD_MATCH")));
    GstDmaBufVideoBuffer::recordImportFailureForTest(/*unrecoverable=*/true);
    verifyExpectedLogMessage();
    QCOMPARE(harness.pushFrame(2 * GST_SECOND), GST_FLOW_OK);
    QVERIFY2(harness.takeReconfigure(), "disabling the DMABuf import must ask upstream to renegotiate");
    QVERIFY(!offersDmaBuf(harness.element()));
    QCOMPARE(harness.pushFrame(3 * GST_SECOND), GST_FLOW_OK);
    QVERIFY2(!harness.takeReconfigure(), "one change must ask for one renegotiation");

    // A recreated scene graph clears the latch.
    GstDmaBufVideoBuffer::resetCachedState();
    QCOMPARE(harness.pushFrame(4 * GST_SECOND), GST_FLOW_OK);
    QVERIFY(harness.takeReconfigure());
    QVERIFY(offersDmaBuf(harness.element()));
}
#endif

void GstQgcSinkTest::_testQgcQVideoSinkDropsUndeliveredFrame_data()
{
    QTest::addColumn<bool>("viaFlush");
    QTest::newRow("flush-start") << true;
    QTest::newRow("stop") << false;
}

void GstQgcSinkTest::_testQgcQVideoSinkDropsUndeliveredFrame()
{
    QFETCH(bool, viaFlush);

    int emissions = 0;
    QVideoSink videoSink;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &videoSink, [&](const QVideoFrame&) { ++emissions; });
    GstQgcTest::SinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());

    QCOMPARE(harness.pushFrame(GST_SECOND), GST_FLOW_OK);
    if (viaFlush) {
        QVERIFY(harness.pushEvent(gst_event_new_flush_start()));
    } else {
        QCOMPARE(gst_element_set_state(harness.element(), GST_STATE_NULL), GST_STATE_CHANGE_SUCCESS);
    }
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    QCOMPARE(emissions, 0);
    QVERIFY(!videoSink.videoFrame().isValid());
}

void GstQgcSinkTest::_testQgcQVideoSinkForwardsTimestamps()
{
    QVideoSink videoSink;
    GstQgcTest::SinkHarness harness(&videoSink);
    QVERIFY(harness.isReady());
    QCOMPARE(harness.pushFrame(10 * GST_SECOND), GST_FLOW_OK);

    // Timestamps going backwards without DISCONT, as after an encoder restart or an RTP resync; Qt's display path
    // ignores timestamps, so the frame is shown, timestamped as it came.
    constexpr GstClockTime kRegressed = 9900 * GST_MSECOND;
    QCOMPARE(harness.pushFrame(kRegressed), GST_FLOW_OK);
    QCOMPARE(harness.delivered(), guint64(2));
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    const QVideoFrame frame = videoSink.videoFrame();
    QCOMPARE(frame.startTime(), qint64(kRegressed / GST_USECOND));
    // Without a buffer duration the end stays unknown rather than collapsing onto the start.
    QCOMPARE(frame.endTime(), qint64(-1));
}

void GstQgcSinkTest::_testQgcQVideoSinkMapFailuresPostElementError_data()
{
    QTest::addColumn<QString>("memoryFeature");  // empty: a sink without gpuZeroCopy, on system memory caps
    QTest::addColumn<int>("backend");
    QTest::addColumn<QString>("stage");

    QTest::newRow("cpu") << QString() << int(GstQgc::RenderBackend::OpenGL) << QStringLiteral("CPU path");
    // The frames a GPU path was asked for end up on the CPU too, which the error must not pass off as the CPU path.
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QTest::newRow("gpu caps") << QStringLiteral("memory:DMABuf") << int(GstQgc::RenderBackend::OpenGL)
                              << QStringLiteral("CPU fallback from a GPU path");
#elif defined(QGC_HAS_GST_D3D11_GPU_PATH)
    QTest::newRow("gpu caps") << QStringLiteral("memory:D3D11Memory") << int(GstQgc::RenderBackend::Direct3D11)
                              << QStringLiteral("CPU fallback from a GPU path");
#endif
}

void GstQgcSinkTest::_testQgcQVideoSinkMapFailuresPostElementError()
{
    QFETCH(QString, memoryFeature);
    QFETCH(int, backend);
    QFETCH(QString, stage);

    // A render target that imports the row's GPU memory, so the sink accepts its caps.
    const GstQgc::SinkEnvironment environment{
        .target = {.backend = static_cast<GstQgc::RenderBackend>(backend), .egl = true},
        .dmaBufDisplay = kFakeQtEglDisplay};
    QVideoSink videoSink;
    const QByteArray feature = memoryFeature.toUtf8();
    GstQgcTest::SinkHarness harness(&videoSink, /*gpuZeroCopy=*/!feature.isEmpty(),
                                    feature.isEmpty() ? nullptr : feature.constData(), environment);
    QVERIFY(harness.isReady());

    // A run of failures is warned of on its first and every 64th failure (GstQgc::PathSession::onMapFailed()).
    ignoreLogMessage("Video.GStreamer.QgcQVideoSink", QtWarningMsg,
                     QRegularExpression(QStringLiteral("mapping failed")));
    expectLogMessage("Video.GStreamer.QgcQVideoSink", QtWarningMsg,
                     QRegularExpression(QStringLiteral("consecutive map failures")));

    // Undersized buffers can never be mapped; the sink must eventually give up with a descriptive element error.
    GstFlowReturn ret = GST_FLOW_OK;
    for (int i = 0; (i < 1000) && (ret == GST_FLOW_OK); ++i) {
        ret = harness.pushFrame(GST_CLOCK_TIME_NONE, /*size=*/4);
    }
    verifyExpectedLogMessage();
    QCOMPARE(ret, GST_FLOW_ERROR);

    const GStreamer::GstMessagePtr msg =
        GStreamer::adoptMessage(gst_bus_pop_filtered(harness.bus(), GST_MESSAGE_ERROR));
    QVERIFY2(msg, "map-failure escalation must post an element error before returning GST_FLOW_ERROR");
    const auto [error, debug] = GStreamer::parseMessageError(msg.get());
    QVERIFY(g_error_matches(error.get(), GST_STREAM_ERROR, GST_STREAM_ERROR_FAILED));
    const QString details = QString::fromUtf8(debug ? debug.get() : "");
    QVERIFY2(details.contains(QStringLiteral("consecutive map failures (%1, format BGRA)").arg(stage)),
             qPrintable(details));
}

void GstQgcSinkTest::_testFrameDeliveryKeepsTheLatestFrame()
{
    const auto frameOfWidth = [](int width) {
        return QVideoFrame(QVideoFrameFormat(QSize(width, 2), QVideoFrameFormat::Format_RGBA8888));
    };
    GstQgc::FrameDelivery delivery;

    // Nothing is delivered without a QVideoSink.
    QVERIFY(!delivery.deliver(frameOfWidth(2)).delivered);

    QVideoSink first;
    delivery.setSink(&first);
    QCOMPARE(delivery.sink(), &first);
    const GstQgc::FrameDelivery::Result delivered = delivery.deliver(frameOfWidth(2));
    QVERIFY(delivered.delivered);
    QVERIFY(!delivered.superseded);
    // A newer frame replaces one the QVideoSink hasn't taken, which then only gets the newest.
    QVERIFY(delivery.deliver(frameOfWidth(4)).superseded);
    QTRY_COMPARE_WITH_TIMEOUT(first.videoFrame().width(), 4, TestTimeout::shortMs());
    QVERIFY(!delivery.deliver(frameOfWidth(6)).superseded);

    // A frame bound for the previous sink reaches neither.
    QVideoSink second;
    delivery.setSink(&second);
    QCoreApplication::processEvents();
    QCOMPARE(first.videoFrame().width(), 4);
    QVERIFY(!second.videoFrame().isValid());

    // A dropped frame is never shown, and the next one is delivered anew.
    QVERIFY(delivery.deliver(frameOfWidth(8)).delivered);
    delivery.dropPending();
    QCoreApplication::processEvents();
    QVERIFY(!second.videoFrame().isValid());
    QVERIFY(!delivery.deliver(frameOfWidth(10)).superseded);
    QTRY_COMPARE_WITH_TIMEOUT(second.videoFrame().width(), 10, TestTimeout::shortMs());

    delivery.setSink(nullptr);
    QVERIFY(!delivery.deliver(frameOfWidth(12)).delivered);
}

void GstQgcSinkTest::_testPathSessionFailurePolicy()
{
    using Verdict = GstQgc::PathSession::Verdict;
    constexpr const char* kSinkLog = "Video.GStreamer.QgcQVideoSink";
    GstQgc::PathSession session;
    GstQgc::SinkCounters counters;
    const MappedFrame failed;

    // Failures are dropped, logged on the first of every 64, until a sustained run fails the stream.
    for (quint64 failure = 1; failure < GstQgc::PathSession::kMaxConsecutiveMapFailures; ++failure) {
        const bool logged = (failure % 64) == 1;
        if (logged) {
            expectLogMessage(kSinkLog, QtWarningMsg, QRegularExpression(QStringLiteral("^mapping failed")));
        }
        QCOMPARE(session.onMapFailed(failed, counters), Verdict::Drop);
        if (logged) {
            verifyExpectedLogMessage();
        }
    }
    expectLogMessage(kSinkLog, QtWarningMsg, QRegularExpression(QStringLiteral("consecutive map failures")));
    QCOMPARE(session.onMapFailed(failed, counters), Verdict::Error);
    verifyExpectedLogMessage();

    // New caps keep the run going; a frame that maps, or a stop, ends it.
    session.startEpoch();
    QCOMPARE(session.consecutiveMapFailures(), GstQgc::PathSession::kMaxConsecutiveMapFailures);
    session.onMapped();
    QCOMPARE(session.consecutiveMapFailures(), quint64(0));

    // Tiled frames the CPU can't read are counted, and warned about once per epoch.
    MappedFrame tiled;
    tiled.noCpuLayout = true;
    expectLogMessage(kSinkLog, QtWarningMsg, QRegularExpression(QStringLiteral("^tiled DMA_DRM frame")));
    QCOMPARE(session.onMapFailed(tiled, counters), Verdict::Drop);
    QCOMPARE(session.onMapFailed(tiled, counters), Verdict::Drop);
    verifyExpectedLogMessage();
    session.stop();
    QCOMPARE(session.consecutiveMapFailures(), quint64(0));
    expectLogMessage(kSinkLog, QtWarningMsg, QRegularExpression(QStringLiteral("^tiled DMA_DRM frame")));
    QCOMPARE(session.onMapFailed(tiled, counters), Verdict::Drop);
    verifyExpectedLogMessage();

    // A CPU fallback from a GPU path counts every time, as a stream demotion once per epoch.
    MappedFrame demoted;
    demoted.demoted = true;
    session.onDelivered(demoted, counters);
    session.onDelivered(demoted, counters);
    session.onDelivered(MappedFrame{}, counters);
    session.startEpoch();
    session.onDelivered(demoted, counters);
    const GstQgc::SinkStats stats = counters.snapshot();
    QCOMPARE(stats.gpuFallbacks, quint64(3));
    QCOMPARE(stats.streamDemotions, quint64(2));
    QCOMPARE(stats.tiledDrops, quint64(3));
    // As the receiver's "GPU path" debug lines show them, after the per-path counts.
    QCOMPARE(GstQgc::formatSinkStats(stats).line, QStringLiteral(" gpu-fallbacks:3 stream-demotions:2 tiled-drops:3"));
}

#if defined(QGC_GST_BIN_HAS_GLUPLOAD) && defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
void GstQgcSinkTest::_testGpuBinPreferenceParse_data()
{
    QTest::addColumn<QByteArray>("value");
    QTest::addColumn<int>("expectedMode");
    QTest::addColumn<bool>("explicitChoice");
    QTest::addColumn<bool>("warns");

    const int glUpload = int(GstQgc::GpuBinMode::GlUpload);
    const int directDmaBuf = int(GstQgc::GpuBinMode::DirectDmaBuf);
    const int dmaBufOrGlUpload = int(GstQgc::GpuBinMode::DmaBufOrGlUpload);
    QTest::newRow("unset") << QByteArray() << dmaBufOrGlUpload << false << false;
    QTest::newRow("glupload") << QByteArray("glupload") << glUpload << true << false;
    QTest::newRow("dmabuf-padded-mixed-case") << QByteArray(" DmaBuf ") << directDmaBuf << true << false;
    QTest::newRow("unknown") << QByteArray("vulkan") << dmaBufOrGlUpload << false << true;
}

void GstQgcSinkTest::_testGpuBinPreferenceParse()
{
    QFETCH(QByteArray, value);
    QFETCH(int, expectedMode);
    QFETCH(bool, explicitChoice);
    QFETCH(bool, warns);

    if (warns) {
        expectLogMessage("Video.GStreamer.GstQgcGpuBin", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Unknown QGC_GST_GPU_BIN value.*default bin")));
    }
    const GstQgc::GpuBinPreference preference = GstQgc::parseGpuBinPreference(value);
    QCOMPARE(int(preference.mode), expectedMode);
    QCOMPARE(preference.explicitChoice, explicitChoice);
    if (warns) {
        verifyExpectedLogMessage();
    }
}
#endif

void GstQgcSinkTest::_testGpuBinSelection_data()
{
    QTest::addColumn<int>("preferred");
    QTest::addColumn<bool>("explicitChoice");
    QTest::addColumn<int>("backend");
    QTest::addColumn<bool>("egl");
    QTest::addColumn<int>("expected");

    using GstQgc::GpuBinMode;
    using GstQgc::RenderBackend;
    const int glUpload = int(GpuBinMode::GlUpload);
    const int directDmaBuf = int(GpuBinMode::DirectDmaBuf);
    const int dmaBufOrGlUpload = int(GpuBinMode::DmaBufOrGlUpload);
    const int passthrough = int(GpuBinMode::Passthrough);
    const int openGl = int(RenderBackend::OpenGL);
    const int vulkan = int(RenderBackend::Vulkan);
    // The DMABuf-or-glupload bin's sink offers DMABuf only on EGL, so under GLX it is the glupload bin in effect. WGL
    // and CGL contexts aren't wrapped, so there it is no GPU bin.
#if defined(Q_OS_LINUX)
    const int withoutEgl = dmaBufOrGlUpload;
#else
    const int withoutEgl = passthrough;
#endif
    QTest::newRow("dmabuf-or-glupload-opengl-egl") << dmaBufOrGlUpload << false << openGl << true << dmaBufOrGlUpload;
    QTest::newRow("dmabuf-or-glupload-opengl-not-egl") << dmaBufOrGlUpload << false << openGl << false << withoutEgl;
    QTest::newRow("dmabuf-or-glupload-vulkan") << dmaBufOrGlUpload << false << vulkan << false << passthrough;
    QTest::newRow("glupload-opengl-egl") << glUpload << false << openGl << true << glUpload;
    QTest::newRow("glupload-vulkan") << glUpload << false << vulkan << false << passthrough;
    QTest::newRow("glupload-vulkan-explicit") << glUpload << true << vulkan << false << glUpload;
    QTest::newRow("dmabuf-opengl-egl") << directDmaBuf << false << openGl << true << directDmaBuf;
    QTest::newRow("dmabuf-opengl-not-egl") << directDmaBuf << false << openGl << false << passthrough;
    QTest::newRow("dmabuf-opengl-not-egl-explicit") << directDmaBuf << true << openGl << false << directDmaBuf;
    QTest::newRow("passthrough-vulkan") << passthrough << false << vulkan << false << passthrough;
    const int direct3D12 = int(RenderBackend::Direct3D12);
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    // Direct3D12 has one GPU variant, whatever was asked for.
    const int d3d12Convert = int(GpuBinMode::D3D12Convert);
    QTest::newRow("passthrough-d3d12") << passthrough << false << direct3D12 << false << d3d12Convert;
    QTest::newRow("glupload-d3d12-explicit") << glUpload << true << direct3D12 << false << d3d12Convert;
#else
    QTest::newRow("passthrough-d3d12") << passthrough << false << direct3D12 << false << passthrough;
#endif
}

void GstQgcSinkTest::_testGpuBinSelection()
{
    QFETCH(int, preferred);
    QFETCH(bool, explicitChoice);
    QFETCH(int, backend);
    QFETCH(bool, egl);
    QFETCH(int, expected);

    const GstQgc::GpuBinPreference preference{.mode = static_cast<GstQgc::GpuBinMode>(preferred),
                                              .explicitChoice = explicitChoice};
    const GstQgc::RenderTarget target{.backend = static_cast<GstQgc::RenderBackend>(backend), .egl = egl};
    QCOMPARE(int(GstQgc::selectGpuBin(preference, target)), expected);
}

void GstQgcSinkTest::_testFrameGeometryFollowsBufferMetas_data()
{
    QTest::addColumn<QSize>("metaSize");  // invalid: no GstVideoMeta
    QTest::addColumn<QRect>("crop");      // null: no GstVideoCropMeta
    QTest::addColumn<QSize>("frameSize");
    QTest::addColumn<QRect>("viewport");

    // Under 400x200 caps.
    const QSize capsSize(400, 200);
    const QRect capsRect(QPoint(0, 0), capsSize);
    QTest::newRow("no-metas") << QSize() << QRect() << capsSize << capsRect;
    QTest::newRow("meta-of-caps-size") << capsSize << QRect() << capsSize << capsRect;
    // v4l2 stateless decoders: the meta has the coded size, and the picture is its top-left part.
    QTest::newRow("padded") << QSize(400, 208) << QRect() << QSize(400, 208) << capsRect;
    // gst-va with a left or top crop: the meta has the uncropped size, and the crop meta the picture in it.
    QTest::newRow("padded-and-cropped") << QSize(416, 208) << QRect(8, 4, 400, 200) << QSize(416, 208)
                                        << QRect(8, 4, 400, 200);
    QTest::newRow("cropped") << QSize() << QRect(100, 50, 200, 100) << capsSize << QRect(100, 50, 200, 100);
    // No frame gst_video_frame_map() would map.
    QTest::newRow("meta-smaller-than-caps") << QSize(400, 192) << QRect() << capsSize << capsRect;
}

void GstQgcSinkTest::_testFrameGeometryFollowsBufferMetas()
{
    QFETCH(QSize, metaSize);
    QFETCH(QRect, crop);
    QFETCH(QSize, frameSize);
    QFETCH(QRect, viewport);

    // The geometry reaches the frame as QVideoFrameFormat's frame size and viewport, not through
    // externalTextureMatrix.
    GstVideoInfo info;
    const GstTest::SamplePtr sample(GstTest::makeVideoSample(GST_VIDEO_FORMAT_NV12, 400, 200, &info));
    QVERIFY(sample);
    GstBuffer* const buffer = gst_sample_get_buffer(sample.get());
    if (metaSize.isValid()) {
        gst_buffer_add_video_meta(buffer, GST_VIDEO_FRAME_FLAG_NONE, GST_VIDEO_FORMAT_NV12,
                                  static_cast<guint>(metaSize.width()), static_cast<guint>(metaSize.height()));
    }
    if (!crop.isNull()) {
        GstVideoCropMeta* const cropMeta = gst_buffer_add_video_crop_meta(buffer);
        cropMeta->x = static_cast<guint>(crop.x());
        cropMeta->y = static_cast<guint>(crop.y());
        cropMeta->width = static_cast<guint>(crop.width());
        cropMeta->height = static_cast<guint>(crop.height());
    }

    const QVideoFrameFormat format = withFrameGeometry(
        QVideoFrameFormat(QSize(400, 200), QVideoFrameFormat::Format_NV12), frameGeometry(buffer, info));
    QCOMPARE(format.frameSize(), frameSize);
    QCOMPARE(format.viewport(), viewport);
}

UT_REGISTER_TEST(GstQgcSinkTest, TestLabel::Unit)
