#include "GStreamerGlReadbackTest.h"

#include <rhi/qrhi.h>

#include <QtGui/QOffscreenSurface>

#ifdef QGC_GST_STREAMING

#include <array>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtGui/QGuiApplication>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtMultimedia/QVideoFrameFormat>

#include "GStreamer.h"
#include "GStreamerLogging.h"
#include "GStreamerTestHelpers.h"
#include "GstHwVideoInfo.h"
#include "HwBuffers/dmabuf/GstDmaDrmCaps.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include <fcntl.h>
#include <gst/allocators/gstdmabuf.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#if __has_include(<linux/udmabuf.h>)
#include <linux/udmabuf.h>
#endif

#include "GstDmaBufVideoBuffer.h"
#include "GstQtDmaBufImport.h"
#include "QGCRhiCapture.h"
#endif

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) && defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#define QGC_TEST_GLMEMORY_READBACK 1
#include <gst/app/gstappsink.h>

#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>

#include "CpuVideoFramePool.h"
#include "GstGlContextBridge.h"
#include "GstGlVideoBuffer.h"
#include "GstHwVideoBufferFactory.h"
#include "HwBuffers/GstHwImageReadback.h"
#include "HwBuffers/common/HwBuffers.h"
#include "gstqgc/GstQgcGpuBin.h"
#include "gstqgc/GstQgcRenderTarget.h"
#endif

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) || defined(QGC_TEST_GLMEMORY_READBACK)
#include <QtGui/qopenglcontext_platform.h>
#endif

#endif  // QGC_GST_STREAMING

GStreamerGlReadbackTest::GStreamerGlReadbackTest() = default;
GStreamerGlReadbackTest::~GStreamerGlReadbackTest() = default;

QRhi* GStreamerGlReadbackTest::_glRhi()
{
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) || defined(QGC_TEST_GLMEMORY_READBACK)
    if (_rhiProbed) {
        return _rhi.get();
    }
    _rhiProbed = true;
    _surface.reset(QRhiGles2InitParams::newFallbackSurface());
    QRhiGles2InitParams params;
    params.fallbackSurface = _surface.get();
    // Shares with QOpenGLContext::globalShareContext(), as Qt Quick's GL QRhi and the GL bridge's wrap do.
    _rhi.reset(QRhi::create(QRhi::OpenGLES2, &params));
    QOpenGLContext* const shared = QOpenGLContext::globalShareContext();
    if (!_rhi) {
        _rhiUnavailable = QStringLiteral("QRhi::create(OpenGLES2) failed");
    } else if (!shared || !shared->nativeInterface<QNativeInterface::QEGLContext>()) {
        _rhiUnavailable =
            QStringLiteral("Qt renders without EGL on the %1 platform").arg(QGuiApplication::platformName());
        _rhi.reset();
    }
    return _rhi.get();
#else
    _rhiUnavailable = QStringLiteral("No GL GPU path compiled in this build");
    return nullptr;
#endif
}

#ifdef QGC_GST_STREAMING

void GStreamerGlReadbackTest::initTestCase()
{
    UnitTest::initTestCase();
    GStreamer::redirectGLibLogging();
    if (GstTest::initGStreamer().isEmpty()) {
        QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");
    }
}

void GStreamerGlReadbackTest::init()
{
    const QString unavailable = GstTest::initGStreamer();
    if (!unavailable.isEmpty()) {
        QSKIP(qPrintable(unavailable));
    }
    UnitTest::init();
    // The xcb platform this class runs on registers with the desktop portal when one is around.
    ignoreLogMessage("qt.qpa.services", QtWarningMsg, QRegularExpression(QStringLiteral("host portal")));
}

void GStreamerGlReadbackTest::cleanupTestCase()
{
    _rhi.reset();
    _surface.reset();
    UnitTest::cleanupTestCase();
}

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) || defined(QGC_TEST_GLMEMORY_READBACK)
namespace {

/// @p texture's pixels as tightly packed RGBA8 rows (R8/RG8 planes read back with G/B/A filled in); empty when the
/// readback failed. Plain GL: QRhi's GL readback returns alpha for R8 on desktop GL.
QByteArray readBack(QRhi& rhi, QRhiTexture* texture)
{
    if (!texture) {
        return {};
    }
    rhi.makeThreadLocalNativeContextCurrent();
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (!context) {
        return {};
    }
    QOpenGLFunctions* gl = context->functions();
    GLuint fbo = 0;
    gl->glGenFramebuffers(1, &fbo);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               static_cast<GLuint>(texture->nativeTexture().object), 0);
    QByteArray rgba;
    if (gl->glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        const QSize size = texture->pixelSize();
        rgba.resize(qsizetype(size.width()) * size.height() * 4);
        gl->glPixelStorei(GL_PACK_ALIGNMENT, 1);
        gl->glReadPixels(0, 0, size.width(), size.height(), GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    }
    gl->glBindFramebuffer(GL_FRAMEBUFFER, context->defaultFramebufferObject());
    gl->glDeleteFramebuffers(1, &fbo);
    return rgba;
}

/// Where the first @p channels of @p rgba first differ from @p expected(x, y, channel) over a @p width x @p height
/// plane; empty when they match.
template <class Expected>
QString firstMismatch(const QByteArray& rgba, int width, int height, int channels, Expected&& expected)
{
    if (rgba.size() != qsizetype(width) * height * 4) {
        return QStringLiteral("read back %1 bytes, expected %2").arg(rgba.size()).arg(width * height * 4);
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int c = 0; c < channels; ++c) {
                const quint8 got = quint8(rgba[(((y * width) + x) * 4) + c]);
                const quint8 want = expected(x, y, c);
                if (got != want) {
                    return QStringLiteral("(%1,%2) channel %3: got %4, expected %5")
                        .arg(x)
                        .arg(y)
                        .arg(c)
                        .arg(got)
                        .arg(want);
                }
            }
        }
    }
    return {};
}

}  // namespace
#endif

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
namespace {

constexpr int kWidth = 64;
constexpr int kHeight = 48;
// Padded rows and a gap before chroma: a wrong pitch or plane offset samples the padding bytes.
constexpr int kStride = 128;
constexpr gsize kLumaSize = gsize(kStride) * kHeight;
constexpr gsize kChromaOffset = kLumaSize + 4096;
constexpr gsize kChromaSize = gsize(kStride) * (kHeight / 2);
constexpr char kPadding = char(0xEE);

quint8 lumaAt(int x, int y)
{
    return quint8((x * 3) + (y * 5));
}

quint8 cbAt(int x, int y)
{
    return quint8(16 + (x * 7) + y);
}

quint8 crAt(int x, int y)
{
    return quint8(240 - x - (y * 3));
}

QByteArray lumaPlane()
{
    QByteArray bytes(qsizetype(kLumaSize), kPadding);
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            bytes[(y * kStride) + x] = char(lumaAt(x, y));
        }
    }
    return bytes;
}

QByteArray chromaPlane()
{
    QByteArray bytes(qsizetype(kChromaSize), kPadding);
    for (int y = 0; y < kHeight / 2; ++y) {
        for (int x = 0; x < kWidth / 2; ++x) {
            bytes[(y * kStride) + (2 * x)] = char(cbAt(x, y));
            bytes[(y * kStride) + (2 * x) + 1] = char(crAt(x, y));
        }
    }
    return bytes;
}

/// A dma-buf fd holding @p bytes: a udmabuf over a sealed memfd when /dev/udmabuf is usable, else the memfd itself,
/// which only Mesa's software rasterizer imports (it maps the fd). -1 on failure.
int makeDmaBufFd(const QByteArray& bytes, bool& realDmaBuf)
{
    realDmaBuf = false;
    const long page = sysconf(_SC_PAGESIZE);
    const off_t size = static_cast<off_t>(((bytes.size() + page - 1) / page) * page);
    const int memfd = memfd_create("qgc-dmabuf-readback", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (memfd < 0) {
        return -1;
    }
    if ((ftruncate(memfd, size) != 0) || (pwrite(memfd, bytes.constData(), size_t(bytes.size()), 0) != bytes.size())) {
        ::close(memfd);
        return -1;
    }
#if __has_include(<linux/udmabuf.h>)
    const int device = ::open("/dev/udmabuf", O_RDWR | O_CLOEXEC);
    if (device >= 0) {
        udmabuf_create create = {};
        create.memfd = static_cast<__u32>(memfd);
        create.flags = UDMABUF_FLAGS_CLOEXEC;
        create.size = static_cast<__u64>(size);
        const int dmaBuf =
            (fcntl(memfd, F_ADD_SEALS, F_SEAL_SHRINK) == 0) ? ioctl(device, UDMABUF_CREATE, &create) : -1;
        ::close(device);
        if (dmaBuf >= 0) {
            ::close(memfd);
            realDmaBuf = true;
            return dmaBuf;
        }
    }
#endif
    return memfd;
}

GstMemory* wrapDmaBuf(GstAllocator* allocator, const QByteArray& bytes, bool& realDmaBuf)
{
    const int fd = makeDmaBufFd(bytes, realDmaBuf);
    if (fd < 0) {
        return nullptr;
    }
    GstMemory* memory = gst_dmabuf_allocator_alloc(allocator, fd, gsize(bytes.size()));
    if (!memory) {
        ::close(fd);
    }
    return memory;
}

}  // namespace
#endif

void GStreamerGlReadbackTest::_testNv12PlanesReadBack_data()
{
    QTest::addColumn<bool>("dmaBufPerPlane");
    QTest::addColumn<QString>("caps");

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    const QString legacy = QStringLiteral("video/x-raw(memory:DMABuf),format=NV12,width=%1,height=%2");
    QTest::newRow("one dma-buf, implicit modifier") << false << legacy.arg(kWidth).arg(kHeight);
    QTest::newRow("dma-buf per plane, implicit modifier") << true << legacy.arg(kWidth).arg(kHeight);
#if GST_CHECK_VERSION(1, 24, 0)
    const QString linearDrm =
        QStringLiteral("video/x-raw(memory:DMABuf),format=DMA_DRM,drm-format=NV12,width=%1,height=%2");
    QTest::newRow("one dma-buf, LINEAR modifier") << false << linearDrm.arg(kWidth).arg(kHeight);
#endif
#endif
}

void GStreamerGlReadbackTest::_testNv12PlanesReadBack()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("DMABuf GPU path not compiled in this build");
#else
    QFETCH(bool, dmaBufPerPlane);
    QFETCH(QString, caps);

    QRhi* rhi = _glRhi();
    if (!rhi) {
        QSKIP(qPrintable(_rhiUnavailable));
    }
    if (!QGCRhiCapture::qtEglDisplay() || (GstQtDmaBufImport::importDisplay(*rhi) == EGL_NO_DISPLAY)) {
        QSKIP("Qt's GL context can't bind EGLImages");
    }
    GstDmaBufVideoBuffer::resetCachedState();

    GstAllocator* allocator = gst_dmabuf_allocator_new();
    const auto allocatorGuard = qScopeGuard([allocator] { gst_object_unref(allocator); });
    GstBuffer* buffer = gst_buffer_new();
    const auto bufferGuard = qScopeGuard([buffer] { gst_buffer_unref(buffer); });
    bool realDmaBuf = false;
    gsize offsets[GST_VIDEO_MAX_PLANES] = {0, kChromaOffset};
    if (dmaBufPerPlane) {
        bool chromaRealDmaBuf = false;
        GstMemory* luma = wrapDmaBuf(allocator, lumaPlane(), realDmaBuf);
        GstMemory* chroma = wrapDmaBuf(allocator, chromaPlane(), chromaRealDmaBuf);
        QVERIFY(luma && chroma);
        gst_buffer_append_memory(buffer, luma);
        gst_buffer_append_memory(buffer, chroma);
        offsets[1] = kLumaSize;  // meta offsets count across the buffer's memories
    } else {
        QByteArray bytes = lumaPlane();
        bytes.append(qsizetype(kChromaOffset - kLumaSize), kPadding);
        bytes.append(chromaPlane());
        GstMemory* memory = wrapDmaBuf(allocator, bytes, realDmaBuf);
        QVERIFY(memory);
        gst_buffer_append_memory(buffer, memory);
    }
    if (!realDmaBuf && !rhi->driverInfo().deviceName.contains("llvmpipe")) {
        QSKIP("No /dev/udmabuf access, and only Mesa's software rasterizer imports a plain memfd");
    }
    gint strides[GST_VIDEO_MAX_PLANES] = {kStride, kStride};
    QVERIFY(gst_buffer_add_video_meta_full(buffer, GST_VIDEO_FRAME_FLAG_NONE, GST_VIDEO_FORMAT_NV12, kWidth, kHeight, 2,
                                           offsets, strides));

    const GStreamer::GstCapsPtr sampleCaps = GStreamer::adoptCaps(gst_caps_from_string(qPrintable(caps)));
    QVERIFY(sampleCaps);
    GstVideoInfo info;
    QVERIFY(GstHw::dmaDrmAwareVideoInfo(sampleCaps.get(), &info));
    GstSample* sample = gst_sample_new(buffer, sampleCaps.get(), nullptr, nullptr);
    GstDmaBufVideoBuffer frame(sample, info, QVideoFrameFormat(QSize(kWidth, kHeight), QVideoFrameFormat::Format_NV12),
                               static_cast<EGLDisplay>(QGCRhiCapture::qtEglDisplay()));
    gst_sample_unref(sample);
    QVERIFY(frame.validatePlaneHandles());

    QVideoFrameTexturesUPtr none;
    const QVideoFrameTexturesUPtr textures = frame.mapTextures(*rhi, none);
    QVERIFY2(textures, "Qt's DMABuf importer rejected the frame");

    const QString luma = firstMismatch(readBack(*rhi, textures->texture(0)), kWidth, kHeight, 1,
                                       [](int x, int y, int) { return lumaAt(x, y); });
    QVERIFY2(luma.isEmpty(), qPrintable(QStringLiteral("Y plane %1").arg(luma)));
    const QString chroma = firstMismatch(readBack(*rhi, textures->texture(1)), kWidth / 2, kHeight / 2, 2,
                                         [](int x, int y, int c) { return (c == 0) ? cbAt(x, y) : crAt(x, y); });
    QVERIFY2(chroma.isEmpty(), qPrintable(QStringLiteral("UV plane %1").arg(chroma)));
#endif
}

void GStreamerGlReadbackTest::_testVaDecodedFrameReadBack()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH) || !GST_CHECK_VERSION(1, 24, 0)
    QSKIP("DMABuf DMA_DRM import not compiled in this build");
#else
    QRhi* rhi = _glRhi();
    if (!rhi) {
        QSKIP(qPrintable(_rhiUnavailable));
    }
    if (!QGCRhiCapture::qtEglDisplay() || (GstQtDmaBufImport::importDisplay(*rhi) == EGL_NO_DISPLAY)) {
        QSKIP("Qt's GL context can't bind EGLImages");
    }
    const QString missingVa = GstTest::firstMissingFactory({"vah264enc", "vah264dec"});
    if (!missingVa.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("No VA-API stack: %1 is not registered").arg(missingVa)));
    }
    constexpr std::array kNv12{GST_VIDEO_FORMAT_NV12};
    std::string drmCaps = GstHw::buildSupportedDmaDrmCaps(kNv12, QGCRhiCapture::qtEglDisplay());
    if (drmCaps.empty()) {
        QSKIP("Qt's EGL display offers no importable NV12 modifier");
    }
    drmCaps.resize(drmCaps.find_last_not_of("; ") + 1);
    GstDmaBufVideoBuffer::resetCachedState();

    // One bitstream decoded twice: into the driver's own (usually tiled) DMABuf layout, and downloaded to system
    // memory as the reference.
    constexpr int kVaWidth = 320;
    constexpr int kVaHeight = 240;
    GstTest::TestPipeline pipeline;
    if (!pipeline.launch(QStringLiteral("videotestsrc num-buffers=1 pattern=smpte ! "
                                        "video/x-raw,format=NV12,width=%1,height=%2,framerate=30/1 ! "
                                        "vah264enc ! h264parse ! tee name=t "
                                        "t. ! queue ! vah264dec ! capsfilter name=gpucaps ! fakesink name=gpu "
                                        "t. ! queue ! vah264dec ! video/x-raw,format=NV12 ! fakesink name=cpu")
                             .arg(kVaWidth)
                             .arg(kVaHeight))) {
        QSKIP(qPrintable(QStringLiteral("VA pipeline parse skipped: %1").arg(pipeline.error())));
    }
    const GStreamer::GstCapsPtr gpuCaps = GStreamer::adoptCaps(gst_caps_from_string(drmCaps.c_str()));
    QVERIFY(gpuCaps);
    g_object_set(pipeline.element("gpucaps").get(), "caps", gpuCaps.get(), nullptr);
    // VA hands out DMA_DRM memory only to a peer that takes GstVideoMeta, as qgcqvideosink does.
    const GStreamer::GstPadPtr gpuPad =
        GStreamer::adoptPad(gst_element_get_static_pad(pipeline.element("gpu").get(), "sink"));
    QVERIFY(gpuPad);
    GStreamer::PadProbe allocationProbe;
    QVERIFY(allocationProbe.add(
        gpuPad.get(), static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM | GST_PAD_PROBE_TYPE_PUSH),
        +[](GstPad*, GstPadProbeInfo* info, gpointer) -> GstPadProbeReturn {
            GstQuery* query = GST_PAD_PROBE_INFO_QUERY(info);
            if (GST_QUERY_TYPE(query) != GST_QUERY_ALLOCATION) {
                return GST_PAD_PROBE_OK;
            }
            gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, nullptr);
            return GST_PAD_PROBE_HANDLED;
        },
        nullptr));
    if (!pipeline.play()) {
        QSKIP("VA pipeline failed to PLAY (no VA driver / DRI device?)");
    }
    const QString failure = pipeline.runToEos();
    if (!failure.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("VA driver can't decode into the offered modifiers (%1): %2")
                             .arg(QString::fromStdString(drmCaps), failure)));
    }

    using SamplePtr = std::unique_ptr<GstSample, decltype(&gst_sample_unref)>;
    const auto lastSample = [&pipeline](const char* name) {
        GstSample* sample = nullptr;
        g_object_get(pipeline.element(name).get(), "last-sample", &sample, nullptr);
        return SamplePtr(sample, &gst_sample_unref);
    };
    const SamplePtr gpuSampleRef = lastSample("gpu");
    const SamplePtr cpuSampleRef = lastSample("cpu");
    QVERIFY(gpuSampleRef && cpuSampleRef);
    GstSample* const gpuSample = gpuSampleRef.get();
    GstSample* const cpuSample = cpuSampleRef.get();

    GstVideoInfo gpuInfo;
    QVERIFY(GstHw::dmaDrmAwareVideoInfo(gst_sample_get_caps(gpuSample), &gpuInfo));
    GstDmaBufVideoBuffer frame(gpuSample, gpuInfo,
                               QVideoFrameFormat(QSize(kVaWidth, kVaHeight), QVideoFrameFormat::Format_NV12),
                               static_cast<EGLDisplay>(QGCRhiCapture::qtEglDisplay()));
    QVERIFY(frame.validatePlaneHandles());
    QVideoFrameTexturesUPtr none;
    const QVideoFrameTexturesUPtr textures = frame.mapTextures(*rhi, none);
    const GStreamer::GCharPtr negotiated = GStreamer::adoptGChar(gst_caps_to_string(gst_sample_get_caps(gpuSample)));
    QVERIFY2(textures, negotiated.get());

    GstVideoInfo cpuInfo;
    QVERIFY(gst_video_info_from_caps(&cpuInfo, gst_sample_get_caps(cpuSample)));
    GstVideoFrame reference;
    QVERIFY(gst_video_frame_map(&reference, &cpuInfo, gst_sample_get_buffer(cpuSample), GST_MAP_READ));
    const auto unmap = qScopeGuard([&reference] { gst_video_frame_unmap(&reference); });
    const auto referenceAt = [&reference](int plane, int x, int y, int c) {
        const auto* row = static_cast<const quint8*>(GST_VIDEO_FRAME_PLANE_DATA(&reference, plane)) +
                          (y * GST_VIDEO_FRAME_PLANE_STRIDE(&reference, plane));
        return row[(x * ((plane == 0) ? 1 : 2)) + c];
    };

    const QString luma = firstMismatch(readBack(*rhi, textures->texture(0)), kVaWidth, kVaHeight, 1,
                                       [&](int x, int y, int c) { return referenceAt(0, x, y, c); });
    QVERIFY2(luma.isEmpty(), qPrintable(QStringLiteral("Y plane %1 (%2)").arg(luma, negotiated.get())));
    const QString chroma = firstMismatch(readBack(*rhi, textures->texture(1)), kVaWidth / 2, kVaHeight / 2, 2,
                                         [&](int x, int y, int c) { return referenceAt(1, x, y, c); });
    QVERIFY2(chroma.isEmpty(), qPrintable(QStringLiteral("UV plane %1 (%2)").arg(chroma, negotiated.get())));
#endif
}

void GStreamerGlReadbackTest::_testGlMemoryFrameReadsBack()
{
#if !defined(QGC_TEST_GLMEMORY_READBACK)
    QSKIP("GLMemory GPU path on EGL not compiled in this build");
#else
    QRhi* rhi = _glRhi();
    if (!rhi) {
        QSKIP(qPrintable(_rhiUnavailable));
    }
    const QString missing = GstTest::firstMissingFactory({"videotestsrc", "glupload", "glcolorconvert", "fakesink"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 factory unavailable").arg(missing)));
    }
#if defined(QGC_GST_BIN_HAS_GLUPLOAD)
    // The direct-DMABuf bin doesn't offer GLMemory.
    const GstQgc::ScopedGpuBinModeForTest glBin(GstQgc::GpuBinMode::GlUpload);
#endif
    const GstQgc::ScopedRenderTargetForTest target({.backend = GstQgc::RenderBackend::OpenGL, .egl = true});

    constexpr int kFrameWidth = 64;
    constexpr int kFrameHeight = 48;
    const QString rawCaps =
        QStringLiteral("video/x-raw,format=RGBA,width=%1,height=%2,framerate=30/1").arg(kFrameWidth).arg(kFrameHeight);

    // The same videotestsrc frame in system memory is the reference.
    GstTest::TestPipeline reference;
    QVERIFY2(reference.launch(
                 QStringLiteral("videotestsrc num-buffers=1 pattern=smpte ! %1 ! fakesink name=cpu").arg(rawCaps)),
             qPrintable(reference.error()));
    QVERIFY(reference.play());
    const QString referenceFailure = reference.runToEos();
    QVERIFY2(referenceFailure.isEmpty(), qPrintable(referenceFailure));
    GstSample* cpuSample = nullptr;
    g_object_get(reference.element("cpu").get(), "last-sample", &cpuSample, nullptr);
    QVERIFY(cpuSample);
    const auto unrefSample = qScopeGuard([cpuSample] { gst_sample_unref(cpuSample); });
    GstVideoInfo cpuInfo;
    QVERIFY(gst_video_info_from_caps(&cpuInfo, gst_sample_get_caps(cpuSample)));
    GstVideoFrame cpuFrame;
    QVERIFY(gst_video_frame_map(&cpuFrame, &cpuInfo, gst_sample_get_buffer(cpuSample), GST_MAP_READ));
    const auto unmapCpuFrame = qScopeGuard([&cpuFrame] { gst_video_frame_unmap(&cpuFrame); });

    // Uploaded by glupload in a gst-gl context the GL bridge shares with Qt's, delivered as the receiver delivers it.
    QVideoSink videoSink;
    QVideoFrame frame;
    QObject owner;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &owner, [&frame](const QVideoFrame& delivered) {
        if (delivered.isValid()) {
            frame = delivered;
        }
    });
    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("videotestsrc num-buffers=1 pattern=smpte ! %1 ! glupload ! "
                                            "video/x-raw(memory:GLMemory),format=RGBA ! "
                                            "qgcvideosinkbin name=sink gpu-zerocopy=true")
                                 .arg(rawCaps)),
             qPrintable(pipeline.error()));
    QVERIFY(GStreamer::setupQVideoSinkElement(pipeline.element("sink").get(), &videoSink, &owner));
    QVERIFY(pipeline.play());
    const QString failure = pipeline.runToEos();
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
    QVERIFY_TRUE_WAIT(frame.isValid(), TestTimeout::mediumMs());

    QHwVideoBuffer* const buffer = QVideoFramePrivate::hwBuffer(frame);
    QVERIFY2(dynamic_cast<GstGlVideoBuffer*>(buffer), "the frame did not take the GLMemory path");
    QVideoFrameTexturesUPtr none;
    const QVideoFrameTexturesUPtr textures = buffer->mapTextures(*rhi, none);
    QVERIFY2(textures, "GstGlVideoBuffer did not map into the GL QRhi");

    const auto* const pixels = static_cast<const quint8*>(GST_VIDEO_FRAME_PLANE_DATA(&cpuFrame, 0));
    const int stride = GST_VIDEO_FRAME_PLANE_STRIDE(&cpuFrame, 0);
    const QString mismatch =
        firstMismatch(readBack(*rhi, textures->texture(0)), kFrameWidth, kFrameHeight, 4,
                      [pixels, stride](int x, int y, int c) { return pixels[(y * stride) + (x * 4) + c]; });
    QVERIFY2(mismatch.isEmpty(), qPrintable(QStringLiteral("RGBA texture %1").arg(mismatch)));
#endif
}

void GStreamerGlReadbackTest::_testGlMemoryFrameToImage()
{
#if !defined(QGC_TEST_GLMEMORY_READBACK)
    QSKIP("GLMemory GPU path on EGL not compiled in this build");
#else
    QRhi* rhi = _glRhi();
    if (!rhi) {
        QSKIP(qPrintable(_rhiUnavailable));
    }
    const QString missing = GstTest::firstMissingFactory({"videotestsrc", "glupload", "appsink"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 factory unavailable").arg(missing)));
    }

    // One frame in GLMemory from a gst-gl context the GL bridge shares with Qt's, and the same frame in system memory.
    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("videotestsrc num-buffers=1 pattern=smpte ! "
                                            "video/x-raw,format=RGBA,width=64,height=48,framerate=30/1 ! tee name=t "
                                            "t. ! queue ! glupload ! video/x-raw(memory:GLMemory),format=RGBA ! "
                                            "appsink name=gpu sync=false "
                                            "t. ! queue ! appsink name=cpu sync=false")),
             qPrintable(pipeline.error()));
    const GStreamer::GstBusPtr bus = GStreamer::adoptBus(gst_element_get_bus(pipeline.get()));
    gst_bus_set_sync_handler(
        bus.get(),
        [](GstBus*, GstMessage* message, gpointer) {
            return GstGlContextBridge::instance().handleSyncMessage(message);
        },
        nullptr, nullptr);
    QVERIFY(pipeline.play());
    const auto pull = [&pipeline](const char* name) {
        return gst_app_sink_try_pull_sample(GST_APP_SINK(pipeline.element(name).get()),
                                            GstTest::clockTime(TestTimeout::mediumDuration()));
    };
    GstSample* const gpuSample = pull("gpu");
    GstSample* const cpuSample = pull("cpu");
    const auto unref = qScopeGuard([gpuSample, cpuSample] {
        if (gpuSample) {
            gst_sample_unref(gpuSample);
        }
        if (cpuSample) {
            gst_sample_unref(cpuSample);
        }
    });
    QVERIFY2(gpuSample && cpuSample, "the pipeline produced no frame");

    GstVideoInfo info;
    QVERIFY(gst_video_info_from_caps(&info, gst_sample_get_caps(cpuSample)));
    const QVideoFrameFormat format(QSize(GST_VIDEO_INFO_WIDTH(&info), GST_VIDEO_INFO_HEIGHT(&info)),
                                   QVideoFrameFormat::Format_RGBA8888);
    HwVideoBufferContext context;
    context.gpuEnabled = true;
    HwVideoBufferPath path = HwVideoBufferPath::None;
    std::unique_ptr<QHwVideoBuffer> buffer = makeHwVideoBuffer(
        gst_sample_get_buffer(gpuSample), gst_sample_get_caps(gpuSample), info, format, context, path);
    QVERIFY2(buffer, "the GLMemory sample did not create a hardware video buffer");
    QCOMPARE(path, HwVideoBufferPath::GlMemory);

    const QImage actual = QVideoFrame(std::make_unique<GstTest::RhiBoundVideoBuffer>(std::move(buffer), rhi)).toImage();
    const QImage expected = CpuVideoFramePool::copyFromBuffer(gst_sample_get_buffer(cpuSample), info, format).toImage();
    const QString mismatch = GstTest::firstImageMismatch(actual, expected, /*tolerance=*/2);
    QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
#endif
}

#endif  // QGC_GST_STREAMING

UT_REGISTER_TEST(GStreamerGlReadbackTest, TestLabel::Integration)
