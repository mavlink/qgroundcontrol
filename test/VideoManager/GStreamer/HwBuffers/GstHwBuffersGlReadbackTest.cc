#include "GstHwBuffersGlReadbackTest.h"

#include <array>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <memory>
#include <rhi/qrhi.h>

#include <QtCore/QByteArray>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtGui/QGuiApplication>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/qopenglcontext_platform.h>
#include <QtMultimedia/QVideoFrameFormat>

#include "GStreamerTestHelpers.h"
#include "GstHwVideoInfo.h"
#include "GstQgcFrameMap.h"
#include "HwBuffers/GstHwImageReadback.h"
#include "HwBuffers/dmabuf/GstDmaDrmCaps.h"
#include "QGCQVideoSinkController.h"
#include "gstqgc/GstQgcSinkPolicy.h"
#include "gstqgc/GstQgcTestSupport.h"
#include "gstqgc/GstQgcVideoFormats.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include <fcntl.h>
#include <gst/allocators/gstdmabuf.h>
#include <sys/ioctl.h>
#include <unistd.h>
#if __has_include(<linux/udmabuf.h>)
#include <linux/udmabuf.h>
#endif

#include "GstDmaBufVideoBuffer.h"
#include "GstQtDmaBufImport.h"
#include "QGCRhiCapture.h"
#endif

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
#include <atomic>
#include <gst/gl/gl.h>

#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>

#include "GstGlContextBridge.h"
#include "GstGlVideoBuffer.h"
#include "GstHwPathRegistry.h"
#include "HwBuffers/HwBuffers.h"
#endif

GstHwBuffersGlReadbackTest::GstHwBuffersGlReadbackTest() = default;
GstHwBuffersGlReadbackTest::~GstHwBuffersGlReadbackTest() = default;

QRhi* GstHwBuffersGlReadbackTest::_glRhi()
{
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
}

void GstHwBuffersGlReadbackTest::init()
{
    GStreamerTestBase::init();
    // The xcb platform this class runs on registers with the desktop portal when one is around.
    ignoreLogMessage("qt.qpa.services", QtWarningMsg, QRegularExpression(QStringLiteral("host portal")));
}

void GstHwBuffersGlReadbackTest::cleanupTestCase()
{
    _rhi.reset();
    _surface.reset();
    GStreamerTestBase::cleanupTestCase();
}

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
    const int memfd = GstTest::makeMemfd(bytes);
    if (memfd < 0) {
        return -1;
    }
#if __has_include(<linux/udmabuf.h>)
    const int device = ::open("/dev/udmabuf", O_RDWR | O_CLOEXEC);
    if (device >= 0) {
        udmabuf_create create = {};
        create.memfd = static_cast<__u32>(memfd);
        create.flags = UDMABUF_FLAGS_CLOEXEC;
        create.size = static_cast<__u64>(lseek(memfd, 0, SEEK_END));
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

void GstHwBuffersGlReadbackTest::_testNv12PlanesReadBack_data()
{
    QTest::addColumn<bool>("dmaBufPerPlane");
    QTest::addColumn<QString>("caps");

    const QString legacy = QStringLiteral("video/x-raw(memory:DMABuf),format=NV12,width=%1,height=%2");
    QTest::newRow("one dma-buf, implicit modifier") << false << legacy.arg(kWidth).arg(kHeight);
    QTest::newRow("dma-buf per plane, implicit modifier") << true << legacy.arg(kWidth).arg(kHeight);
    // The buffer's GstVideoMeta keeps the padded size: the import takes the whole frame, and the viewport the picture.
    QTest::newRow("one dma-buf, padded beyond the caps") << false << legacy.arg(kWidth).arg(kHeight - 8);
#if GST_CHECK_VERSION(1, 24, 0)
    const QString linearDrm =
        QStringLiteral("video/x-raw(memory:DMABuf),format=DMA_DRM,drm-format=NV12,width=%1,height=%2");
    QTest::newRow("one dma-buf, LINEAR modifier") << false << linearDrm.arg(kWidth).arg(kHeight);
#endif
}

void GstHwBuffersGlReadbackTest::_testNv12PlanesReadBack()
{
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
    GstTest::SamplePtr sample(gst_sample_new(buffer, sampleCaps.get(), nullptr, nullptr));
    // The format the sink gives the frame.
    const QSize capsSize(GST_VIDEO_INFO_WIDTH(&info), GST_VIDEO_INFO_HEIGHT(&info));
    GstDmaBufVideoBuffer frame(
        sample.get(), info,
        withFrameGeometry(QVideoFrameFormat(capsSize, QVideoFrameFormat::Format_NV12), frameGeometry(buffer, info)),
        static_cast<EGLDisplay>(QGCRhiCapture::qtEglDisplay()));
    sample.reset();
    QVERIFY(frame.validatePlaneHandles());
    QCOMPARE(frame.format().viewport(), QRect(QPoint(0, 0), capsSize));

    QVideoFrameTexturesUPtr none;
    const QVideoFrameTexturesUPtr textures = frame.mapTextures(*rhi, none);
    QVERIFY2(textures, "Qt's DMABuf importer rejected the frame");

    const QString luma = firstMismatch(readBack(*rhi, textures->texture(0)), kWidth, kHeight, 1,
                                       [](int x, int y, int) { return lumaAt(x, y); });
    QVERIFY2(luma.isEmpty(), qPrintable(QStringLiteral("Y plane %1").arg(luma)));
    const QString chroma = firstMismatch(readBack(*rhi, textures->texture(1)), kWidth / 2, kHeight / 2, 2,
                                         [](int x, int y, int c) { return (c == 0) ? cbAt(x, y) : crAt(x, y); });
    QVERIFY2(chroma.isEmpty(), qPrintable(QStringLiteral("UV plane %1").arg(chroma)));
}

void GstHwBuffersGlReadbackTest::_testVaDecodedFrameReadBack()
{
#if !GST_CHECK_VERSION(1, 24, 0)
    QSKIP("DMA_DRM caps need GStreamer 1.24");
#else
    QRhi* rhi = _glRhi();
    if (!rhi) {
        QSKIP(qPrintable(_rhiUnavailable));
    }
    if (!QGCRhiCapture::qtEglDisplay() || (GstQtDmaBufImport::importDisplay(*rhi) == EGL_NO_DISPLAY)) {
        QSKIP("Qt's GL context can't bind EGLImages");
    }
    GSTTEST_SKIP_UNLESS_FACTORIES("vah264enc", "vah264dec");
    constexpr std::array kNv12{GST_VIDEO_FORMAT_NV12};
    std::string drmCaps = GstHw::buildSupportedDmaDrmCaps(kNv12, &toQtPixelFormat, QGCRhiCapture::qtEglDisplay());
    if (drmCaps.empty()) {
        QSKIP("Qt's EGL display offers no importable NV12 modifier");
    }
    GstDmaBufVideoBuffer::resetCachedState();

    // One bitstream decoded twice: into the driver's own (usually tiled) DMABuf layout, and downloaded to system
    // memory as the reference.
    constexpr int kVaWidth = 320;
    constexpr int kVaHeight = 240;
    GstTest::VaFramePair frames;
    GstTest::pullVaFramePair(
        frames,
        QStringLiteral("video/x-raw,format=NV12,width=%1,height=%2,framerate=30/1 ! vah264enc ! h264parse")
            .arg(kVaWidth)
            .arg(kVaHeight),
        QStringLiteral("vah264dec"), drmCaps, GST_VIDEO_FORMAT_NV12);
    if (!frames.gpu) {
        return;
    }
    GstSample* const gpuSample = frames.gpu.get();
    GstSample* const cpuSample = frames.cpu.get();

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

void GstHwBuffersGlReadbackTest::_testDmaBufBufferReturnsAfterGpuReads()
{
    QRhi* rhi = _glRhi();
    if (!rhi) {
        QSKIP(qPrintable(_rhiUnavailable));
    }
    if (!QGCRhiCapture::qtEglDisplay() || (GstQtDmaBufImport::importDisplay(*rhi) == EGL_NO_DISPLAY)) {
        QSKIP("Qt's GL context can't bind EGLImages");
    }
    GstDmaBufVideoBuffer::resetCachedState();

    constexpr int kSide = 16;
    GstAllocator* allocator = gst_dmabuf_allocator_new();
    const auto allocatorGuard = qScopeGuard([allocator] { gst_object_unref(allocator); });
    GstBuffer* buffer = gst_buffer_new();
    const auto bufferGuard = qScopeGuard([buffer] { gst_buffer_unref(buffer); });
    bool realDmaBuf = false;
    GstMemory* memory = wrapDmaBuf(allocator, QByteArray(kSide * kSide * 4, char(0x7f)), realDmaBuf);
    QVERIFY(memory);
    gst_buffer_append_memory(buffer, memory);
    if (!realDmaBuf && !rhi->driverInfo().deviceName.contains("llvmpipe")) {
        QSKIP("No /dev/udmabuf access, and only Mesa's software rasterizer imports a plain memfd");
    }
    QVERIFY(gst_buffer_add_video_meta(buffer, GST_VIDEO_FRAME_FLAG_NONE, GST_VIDEO_FORMAT_RGBA, kSide, kSide));
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_caps_from_string(
        qPrintable(QStringLiteral("video/x-raw(memory:DMABuf),format=RGBA,width=%1,height=%1").arg(kSide))));
    QVERIFY(caps);
    GstVideoInfo info;
    QVERIFY(GstHw::dmaDrmAwareVideoInfo(caps.get(), &info));
    GstTest::SamplePtr sample(gst_sample_new(buffer, caps.get(), nullptr, nullptr));
    GstDmaBufVideoBuffer frame(sample.get(), info,
                               QVideoFrameFormat(QSize(kSide, kSide), QVideoFrameFormat::Format_RGBA8888),
                               static_cast<EGLDisplay>(QGCRhiCapture::qtEglDisplay()));
    sample.reset();

    // A pipe the test writes to stands in for the fence of the GPU's work.
    int fence[2] = {-1, -1};
    QCOMPARE(::pipe(fence), 0);
    const auto fenceGuard = qScopeGuard([&fence] {
        GstDmaBufVideoBuffer::setGpuFenceForTest(-1);
        ::close(fence[0]);
        ::close(fence[1]);
    });
    GstDmaBufVideoBuffer::setGpuFenceForTest(fence[0]);

    const int idleRefs = GST_MINI_OBJECT_REFCOUNT_VALUE(buffer);
    QVideoFrameTexturesUPtr none;
    QVideoFrameTexturesUPtr textures = frame.mapTextures(*rhi, none);
    QVERIFY2(textures, "Qt's DMABuf importer rejected the frame");
    QCOMPARE(GST_MINI_OBJECT_REFCOUNT_VALUE(buffer), idleRefs + 1);

    // Qt deletes the textures while the GPU still draws with them: the decoder doesn't get its buffer back yet.
    textures.reset();
    QCOMPARE(GST_MINI_OBJECT_REFCOUNT_VALUE(buffer), idleRefs + 1);

    QCOMPARE(::write(fence[1], "", 1), ssize_t(1));
    QVERIFY_TRUE_WAIT(GST_MINI_OBJECT_REFCOUNT_VALUE(buffer) == idleRefs, TestTimeout::shortMs());
}
#endif

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
void GstHwBuffersGlReadbackTest::_testGlMemoryFrameReadsBack_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<QString>("format");

    const int glUpload = int(GstQgc::GpuBinMode::GlUpload);
    QTest::newRow("RGBA, glupload bin") << glUpload << QStringLiteral("RGBA");
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    // The glupload ahead of its switchbin stands for a decoder with GL output, which needs Qt's context as it starts.
    const int dmaBufOrGlUpload = int(GstQgc::GpuBinMode::DmaBufOrGlUpload);
    QTest::newRow("RGBA, DMABuf-or-glupload bin") << dmaBufOrGlUpload << QStringLiteral("RGBA");
    // Two textures a frame, as NVDEC hands them on.
    QTest::newRow("NV12, DMABuf-or-glupload bin") << dmaBufOrGlUpload << QStringLiteral("NV12");
#endif
}

void GstHwBuffersGlReadbackTest::_testGlMemoryFrameReadsBack()
{
    QFETCH(int, mode);
    QFETCH(QString, format);
    QRhi* rhi = _glRhi();
    if (!rhi) {
        QSKIP(qPrintable(_rhiUnavailable));
    }
    GSTTEST_SKIP_UNLESS_FACTORIES("videotestsrc", "glupload", "glcolorconvert", "videoconvert");
    const GstQgc::SinkEnvironment environment{.target = {.backend = GstQgc::RenderBackend::OpenGL, .egl = true},
                                              .gpuBin = {.mode = static_cast<GstQgc::GpuBinMode>(mode)}};

    constexpr int kFrameWidth = 64;
    constexpr int kFrameHeight = 48;
    const QString rawCaps = QStringLiteral("video/x-raw,format=%1,width=%2,height=%3,framerate=30/1")
                                .arg(format)
                                .arg(kFrameWidth)
                                .arg(kFrameHeight);

    // The same videotestsrc frame in system memory, as RGBA, is the reference.
    GstTest::TestPipeline reference;
    QVERIFY2(reference.launch(QStringLiteral("videotestsrc num-buffers=1 pattern=smpte ! %1 ! videoconvert ! "
                                             "video/x-raw,format=RGBA ! fakesink name=cpu")
                                  .arg(rawCaps)),
             qPrintable(reference.error()));
    QVERIFY(reference.play());
    const QString referenceFailure = reference.runToEos();
    QVERIFY2(referenceFailure.isEmpty(), qPrintable(referenceFailure));
    const GstTest::SamplePtr cpuSample = reference.lastSample("cpu");
    QVERIFY(cpuSample);
    GstVideoInfo cpuInfo;
    QVERIFY(gst_video_info_from_caps(&cpuInfo, gst_sample_get_caps(cpuSample.get())));
    GstVideoFrame cpuFrame;
    QVERIFY(gst_video_frame_map(&cpuFrame, &cpuInfo, gst_sample_get_buffer(cpuSample.get()), GST_MAP_READ));
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
    const GstQgc::SinkBin sinkBin =
        GstQgcTest::launchIntoSinkBin(pipeline,
                                      QStringLiteral("videotestsrc num-buffers=1 pattern=smpte ! %1 ! glupload ! "
                                                     "video/x-raw(memory:GLMemory),format=%2")
                                          .arg(rawCaps, format),
                                      {.gpuZeroCopy = true, .environment = environment});
    QVERIFY2(sinkBin, qPrintable(pipeline.error()));
    QVERIFY(GStreamer::setupQVideoSinkElement(sinkBin.qvideosink.get(), &videoSink, &owner));
    QVERIFY(pipeline.play());
    const QString failure = pipeline.runToEos();
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
    QVERIFY_TRUE_WAIT(frame.isValid(), TestTimeout::mediumMs());

    QHwVideoBuffer* const buffer = QVideoFramePrivate::hwBuffer(frame);
    QVERIFY2(dynamic_cast<GstGlVideoBuffer*>(buffer), "the frame did not take the GLMemory path");
    QVideoFrameTexturesUPtr none;
    const QVideoFrameTexturesUPtr textures = buffer->mapTextures(*rhi, none);
    QVERIFY2(textures, "GstGlVideoBuffer did not map into the GL QRhi");

    const QByteArray texture = readBack(*rhi, textures->texture(0));
    const auto* const pixels = static_cast<const quint8*>(GST_VIDEO_FRAME_PLANE_DATA(&cpuFrame, 0));
    const int stride = GST_VIDEO_FRAME_PLANE_STRIDE(&cpuFrame, 0);
    if (format == QStringLiteral("RGBA")) {
        const QString mismatch =
            firstMismatch(texture, kFrameWidth, kFrameHeight, 4,
                          [pixels, stride](int x, int y, int c) { return pixels[(y * stride) + (x * 4) + c]; });
        QVERIFY2(mismatch.isEmpty(), qPrintable(QStringLiteral("RGBA texture %1").arg(mismatch)));
        return;
    }
    // glcolorconvert made the texture of the YUV planes. videoconvert's fixed-point matrix lands a few levels below its
    // shader on saturated colours, and the shader filters the subsampled chroma across a bar's edge.
    QCOMPARE(texture.size(), qsizetype(kFrameWidth) * kFrameHeight * 4);
    const QImage actual(reinterpret_cast<const uchar*>(texture.constData()), kFrameWidth, kFrameHeight, kFrameWidth * 4,
                        QImage::Format_RGBA8888);
    const QImage expected(pixels, kFrameWidth, kFrameHeight, stride, QImage::Format_RGBA8888);
    const QString mismatch = GstTest::firstImageMismatch(actual, expected, /*tolerance=*/6, /*edgeRadius=*/2);
    QVERIFY2(mismatch.isEmpty(), qPrintable(QStringLiteral("texture converted from %1: %2").arg(format, mismatch)));
}

void GstHwBuffersGlReadbackTest::_testGlMemoryFrameToImage_data()
{
    QTest::addColumn<int>("capsHeight");

    QTest::newRow("caps of the texture's size") << 48;
    // As glupload hands on a dma-buf padded beyond the caps: the texture and its GstVideoMeta keep the padded size.
    QTest::newRow("texture padded beyond the caps") << 40;
}

void GstHwBuffersGlReadbackTest::_testGlMemoryFrameToImage()
{
    QFETCH(int, capsHeight);
    QRhi* rhi = _glRhi();
    if (!rhi) {
        QSKIP(qPrintable(_rhiUnavailable));
    }
    GSTTEST_SKIP_UNLESS_FACTORIES("videotestsrc", "glupload", "appsink");

    // One frame in GLMemory from a gst-gl context the GL bridge shares with Qt's, and the same frame in system memory.
    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("videotestsrc num-buffers=1 pattern=smpte ! "
                                            "video/x-raw,format=RGBA,width=64,height=48,framerate=30/1 ! tee name=t "
                                            "t. ! queue ! glupload ! video/x-raw(memory:GLMemory),format=RGBA ! "
                                            "appsink name=gpu sync=false "
                                            "t. ! queue ! appsink name=cpu sync=false")),
             qPrintable(pipeline.error()));
    GstGlContextBridge::instance().provideContexts(pipeline.get());
    QVERIFY(pipeline.play());
    const GstTest::SamplePtr gpuSample = pipeline.pullSample("gpu");
    const GstTest::SamplePtr cpuSample = pipeline.pullSample("cpu");
    QVERIFY2(gpuSample && cpuSample, "the pipeline produced no frame");

    GstVideoInfo info;
    QVERIFY(gst_video_info_from_caps(&info, gst_sample_get_caps(cpuSample.get())));
    const QVideoFrameFormat format(QSize(GST_VIDEO_INFO_WIDTH(&info), GST_VIDEO_INFO_HEIGHT(&info)),
                                   QVideoFrameFormat::Format_RGBA8888);
    // The frame as the sink makes it under caps of capsHeight rows.
    GstVideoInfo capsInfo;
    QVERIFY(
        gst_video_info_set_format(&capsInfo, GST_VIDEO_INFO_FORMAT(&info), GST_VIDEO_INFO_WIDTH(&info), capsHeight));
    const QSize capsSize(GST_VIDEO_INFO_WIDTH(&capsInfo), capsHeight);
    const QVideoFrameFormat gpuFormat =
        withFrameGeometry(QVideoFrameFormat(capsSize, QVideoFrameFormat::Format_RGBA8888),
                          frameGeometry(gst_sample_get_buffer(gpuSample.get()), capsInfo));
    HwVideoBufferContext context;
    context.gpuEnabled = true;
    HwVideoBufferPath path = HwVideoBufferPath::None;
    std::unique_ptr<QHwVideoBuffer> buffer =
        makeHwVideoBuffer(gst_sample_get_buffer(gpuSample.get()), gst_sample_get_caps(gpuSample.get()), capsInfo,
                          gpuFormat, context, path);
    QVERIFY2(buffer, "the GLMemory sample did not create a hardware video buffer");
    QCOMPARE(path, HwVideoBufferPath::GlMemory);

    // toImage() converts the whole frame; the viewport is the picture.
    const QImage actual = QVideoFrame(std::make_unique<GstTest::RhiBoundVideoBuffer>(std::move(buffer), rhi))
                              .toImage()
                              .copy(gpuFormat.viewport());
    const QImage expected =
        mapCpuFrame(gst_sample_get_buffer(cpuSample.get()), info, format).toImage().copy(QRect(QPoint(0, 0), capsSize));
    const QString mismatch = GstTest::firstImageMismatch(actual, expected, /*tolerance=*/2);
    QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
}

void GstHwBuffersGlReadbackTest::_testTenBitPlanarFrameToImage()
{
    // Qt converts the frame with shaders on a QRhi of its own; it has no CPU conversion for this format.
    if (!_glRhi()) {
        QSKIP(qPrintable(_rhiUnavailable));
    }
    GSTTEST_SKIP_UNLESS_FACTORIES("videotestsrc", "videoconvert", "appsink");

    // A frame as a software Main 10 decoder sends it, through the sink, and the same frame converted by GStreamer.
    QVideoSink videoSink;
    QVideoFrame frame;
    QObject owner;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &owner, [&frame](const QVideoFrame& delivered) {
        if (delivered.isValid()) {
            frame = delivered;
        }
    });
    GstTest::TestPipeline pipeline;
    const GstQgc::SinkBin sinkBin = GstQgcTest::launchIntoSinkBin(
        pipeline, QStringLiteral("videotestsrc num-buffers=1 pattern=smpte ! "
                                 "video/x-raw,format=I420_10LE,width=64,height=48,framerate=30/1 ! "
                                 "tee name=t t. ! queue ! videoconvert ! video/x-raw,format=BGRA ! "
                                 "appsink name=reference sync=false t. ! queue"));
    QVERIFY2(sinkBin, qPrintable(pipeline.error()));
    QVERIFY(GStreamer::setupQVideoSinkElement(sinkBin.qvideosink.get(), &videoSink, &owner));
    QVERIFY(pipeline.play());
    const GstTest::SamplePtr reference = pipeline.pullSample("reference");
    QVERIFY2(reference, "the pipeline produced no frame");
    QVERIFY_TRUE_WAIT(frame.isValid(), TestTimeout::mediumMs());

    // The decoder's own format, with nothing converted ahead of the sink.
    QCOMPARE(frame.pixelFormat(), QVideoFrameFormat::Format_YUV420P10);

    GstVideoInfo info;
    QVERIFY(gst_video_info_from_caps(&info, gst_sample_get_caps(reference.get())));
    GstVideoFrame converted;
    QVERIFY(gst_video_frame_map(&converted, &info, gst_sample_get_buffer(reference.get()), GST_MAP_READ));
    const auto unmapConverted = qScopeGuard([&converted] { gst_video_frame_unmap(&converted); });
    // BGRA bytes are QImage's ARGB32 on a little-endian host.
    const QImage expected(static_cast<const uchar*>(GST_VIDEO_FRAME_PLANE_DATA(&converted, 0)),
                          GST_VIDEO_FRAME_WIDTH(&converted), GST_VIDEO_FRAME_HEIGHT(&converted),
                          GST_VIDEO_FRAME_PLANE_STRIDE(&converted, 0), QImage::Format_ARGB32);
    const QString mismatch = GstTest::firstImageMismatch(frame.toImage(), expected, /*tolerance=*/4, /*edgeRadius=*/2);
    QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
}

namespace {

/// Sync points set in the frame's own sync meta in _testGlMemoryMapLeavesFrameSyncMeta.
std::atomic<int> s_frameSyncPointsSet{0};

}  // namespace

void GstHwBuffersGlReadbackTest::_testGlMemoryMapLeavesFrameSyncMeta()
{
    QRhi* rhi = _glRhi();
    if (!rhi) {
        QSKIP(qPrintable(_rhiUnavailable));
    }
    GSTTEST_SKIP_UNLESS_FACTORIES("videotestsrc", "glupload", "appsink");

    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("videotestsrc num-buffers=1 ! "
                                            "video/x-raw,format=RGBA,width=64,height=48,framerate=30/1 ! glupload ! "
                                            "video/x-raw(memory:GLMemory),format=RGBA ! appsink name=gpu sync=false")),
             qPrintable(pipeline.error()));
    GstGlContextBridge::instance().provideContexts(pipeline.get());
    QVERIFY(pipeline.play());
    const GstTest::SamplePtr sample = pipeline.pullSample("gpu");
    QVERIFY2(sample, "the pipeline produced no frame");

    // The sync meta a producer attaches for the consumers of its buffer, which glupload does for the buffers it
    // uploads into. The buffer is shared with them, so the meta's fence is theirs to wait on.
    GstBuffer* const buffer = gst_sample_get_buffer(sample.get());
    GstGLSyncMeta* frameSync = gst_buffer_get_gl_sync_meta(buffer);
    if (!frameSync) {
        frameSync =
            gst_buffer_add_gl_sync_meta(GST_GL_BASE_MEMORY_CAST(gst_buffer_peek_memory(buffer, 0))->context, buffer);
    }
    QVERIFY(frameSync);
    frameSync->set_sync = [](GstGLSyncMeta*, GstGLContext*) { ++s_frameSyncPointsSet; };
    s_frameSyncPointsSet = 0;

    GstVideoInfo info;
    QVERIFY(gst_video_info_from_caps(&info, gst_sample_get_caps(sample.get())));
    HwVideoBufferContext hwContext;
    hwContext.gpuEnabled = true;
    HwVideoBufferPath path = HwVideoBufferPath::None;
    const std::unique_ptr<QHwVideoBuffer> frame =
        makeHwVideoBuffer(buffer, gst_sample_get_caps(sample.get()), info,
                          QVideoFrameFormat(QSize(64, 48), QVideoFrameFormat::Format_RGBA8888), hwContext, path);
    QVERIFY2(frame, "the GLMemory sample did not create a hardware video buffer");
    QVideoFrameTexturesUPtr none;
    QVERIFY2(frame->mapTextures(*rhi, none), "GstGlVideoBuffer did not map into the GL QRhi");

    QCOMPARE(s_frameSyncPointsSet.load(), 0);
}

void GstHwBuffersGlReadbackTest::_testUnsharedGlMemoryTakesCpuPath()
{
    if (!_glRhi()) {
        QSKIP(qPrintable(_rhiUnavailable));
    }
    GSTTEST_SKIP_UNLESS_FACTORIES("videotestsrc", "glupload", "appsink");

    // Nothing answers this pipeline's context requests, so glupload uploads in a GL context of gst-gl's own.
    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("videotestsrc num-buffers=1 ! "
                                            "video/x-raw,format=RGBA,width=64,height=48,framerate=30/1 ! glupload ! "
                                            "video/x-raw(memory:GLMemory),format=RGBA ! appsink name=gpu sync=false")),
             qPrintable(pipeline.error()));
    if (!pipeline.play()) {
        QSKIP("gst-gl can't create a GL context of its own here");
    }
    const GstTest::SamplePtr sample = pipeline.pullSample("gpu");
    if (!sample) {
        QSKIP("gst-gl can't create a GL context of its own here");
    }

    GstVideoInfo info;
    QVERIFY(gst_video_info_from_caps(&info, gst_sample_get_caps(sample.get())));
    HwVideoBufferContext context;
    context.gpuEnabled = true;
    HwVideoBufferPath path = HwVideoBufferPath::GlMemory;
    // Warned once per process.
    ignoreLogMessage("Video.GStreamer.HwBuffers.GstGlBuf", QtWarningMsg,
                     QRegularExpression(QStringLiteral("doesn't share with Qt's")));
    const std::unique_ptr<QHwVideoBuffer> buffer =
        makeHwVideoBuffer(gst_sample_get_buffer(sample.get()), gst_sample_get_caps(sample.get()), info,
                          QVideoFrameFormat(QSize(64, 48), QVideoFrameFormat::Format_RGBA8888), context, path);
    QVERIFY2(!buffer, "Qt's context can't sample a texture of a GL context outside its share group");
    QCOMPARE(path, HwVideoBufferPath::None);
}

void GstHwBuffersGlReadbackTest::_testGlBridgeKeepsGstGlInQtsApi()
{
    if (!_glRhi()) {
        QSKIP(qPrintable(_rhiUnavailable));
    }

    // The display and the wrap of Qt's context, as a pipeline gets them.
    const GStreamer::GstElementPtr pipeline =
        GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(gst_pipeline_new(nullptr))));
    GstGlContextBridge::instance().provideContexts(pipeline.get());
    const GStreamer::GstContextPtr displayContext =
        GStreamer::adoptContext(gst_element_get_context(pipeline.get(), GST_GL_DISPLAY_CONTEXT_TYPE));
    GstGLDisplay* display = nullptr;
    QVERIFY(displayContext && gst_context_get_gl_display(displayContext.get(), &display));
    const auto unrefDisplay = qScopeGuard([display] { gst_object_unref(display); });

    const GStreamer::GstContextPtr wrapContext =
        GStreamer::adoptContext(gst_element_get_context(pipeline.get(), "gst.gl.app_context"));
    GstGLContext* qtContext = nullptr;
    QVERIFY(wrapContext && gst_structure_get(gst_context_get_structure(wrapContext.get()), "context",
                                             GST_TYPE_GL_CONTEXT, &qtContext, nullptr));
    const auto unrefQtContext = qScopeGuard([qtContext] { gst_object_unref(qtContext); });

    // gst-gl creates its contexts in an API of the display's, whatever the context it is told to share with.
    const GstGLAPI qtApi = gst_gl_context_get_gl_api(qtContext);
    QCOMPARE(gst_gl_display_get_gl_api(display), qtApi);

    GstGLContext* created = nullptr;
    GError* error = nullptr;
    const bool ensured = gst_gl_display_ensure_context(display, qtContext, &created, &error);
    const GStreamer::GErrorPtr errorGuard = GStreamer::adoptError(error);
    QVERIFY2(ensured, error ? error->message : "gst-gl created no context sharing with Qt's");
    const auto unrefCreated = qScopeGuard([created] { gst_object_unref(created); });
    const GStreamer::GCharPtr createdApi =
        GStreamer::adoptGChar(gst_gl_api_to_string(gst_gl_context_get_gl_api(created)));
    QVERIFY2(gst_gl_context_get_gl_api(created) & qtApi, createdApi.get());
    QVERIFY(GstGlContextBridge::sharesQtContext(created));
}
#endif

UT_REGISTER_TEST(GstHwBuffersGlReadbackTest, TestLabel::Integration)
