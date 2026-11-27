#include "GstHwBuffersVulkanReadbackTest.h"

#include <array>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <memory>
#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>
#include <string>

#include <QtCore/QRegularExpression>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/private/qvulkandefaultinstance_p.h>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtMultimedia/QVideoSink>
#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>

#include "GStreamerTestHelpers.h"
#include "GstDmaBufVideoBuffer.h"
#include "GstDmaBufVulkanImport.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoInfo.h"
#include "HwBuffers/GstHwImageReadback.h"
#include "HwBuffers/dmabuf/GstDmaDrmCaps.h"
#include "QGCQVideoSinkController.h"
#include "QGCRhiCapture.h"
#include "gstqgc/GstQgcFrameMap.h"
#include "gstqgc/GstQgcSinkPolicy.h"
#include "gstqgc/GstQgcSinkStats.h"
#include "gstqgc/GstQgcTestSupport.h"
#include "gstqgc/GstQgcVideoFormats.h"

namespace {

/// A Vulkan QRhi on the instance Qt Quick renders with; null with the reason in @p unavailable.
std::unique_ptr<QRhi> createGstVulkanTestRhi(QString& unavailable)
{
    QRhiVulkanInitParams params;
    params.inst = QVulkanDefaultInstance::instance();
    if (!params.inst) {
        unavailable = QStringLiteral("No Vulkan instance on the %1 platform").arg(QGuiApplication::platformName());
        return {};
    }
    std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::Vulkan, &params));
    if (!rhi) {
        unavailable = QStringLiteral("QRhi::create(Vulkan) failed");
    }
    return rhi;
}

}  // namespace

GstHwBuffersVulkanReadbackTest::GstHwBuffersVulkanReadbackTest() = default;
GstHwBuffersVulkanReadbackTest::~GstHwBuffersVulkanReadbackTest() = default;

QRhi* GstHwBuffersVulkanReadbackTest::_vulkanRhi()
{
    if (_rhiProbed) {
        return _rhi.get();
    }
    _rhiProbed = true;
    // As HwBuffers::configureQtGraphics() does before the application's QRhi exists.
    GstDmaBufVulkan::requestDeviceExtensions();
    _rhi = createGstVulkanTestRhi(_rhiUnavailable);
    if (!_rhi) {
        return nullptr;
    }
    GstDmaBufVulkan::probeDevice(*_rhi);
    if (!GstDmaBufVulkan::importDevice()) {
        _rhiUnavailable =
            QStringLiteral("The Vulkan device (%1) doesn't import dma-bufs").arg(_rhi->driverInfo().deviceName);
        _rhi.reset();
    }
    return _rhi.get();
}

void GstHwBuffersVulkanReadbackTest::init()
{
    GStreamerTestBase::init();
    // The xcb platform this class runs on registers with the desktop portal when one is around.
    ignoreLogMessage("qt.qpa.services", QtWarningMsg, QRegularExpression(QStringLiteral("host portal")));
    // A platform or a device without Vulkan, or without the import's extensions, says so before the tests skip.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("[Vv]ulkan|Device extension")));
}

void GstHwBuffersVulkanReadbackTest::cleanupTestCase()
{
    _rhi.reset();
    GStreamerTestBase::cleanupTestCase();
}

namespace {

/// The renderer the streaming-thread checks see while alive: a Vulkan scene graph.
class ScopedVulkanSnapshot
{
public:
    ScopedVulkanSnapshot()
        : _previous(QGCRhiCapture::deviceSnapshot().backend.exchange(static_cast<int>(QRhi::Vulkan)))
    {}

    ~ScopedVulkanSnapshot() { QGCRhiCapture::deviceSnapshot().backend.store(_previous); }

    ScopedVulkanSnapshot(const ScopedVulkanSnapshot&) = delete;
    ScopedVulkanSnapshot& operator=(const ScopedVulkanSnapshot&) = delete;

private:
    int _previous;
};

/// Where @p frame, imported into @p rhi and converted there, differs from the CPU conversion of @p reference; empty
/// when they match. Also a mismatch when the import fails: toImage() would then quietly convert the CPU map.
QString vulkanImportMismatch(std::unique_ptr<GstDmaBufVideoBuffer> frame, QRhi& rhi, GstSample* reference,
                             const GstVideoInfo& referenceInfo, bool subsampled)
{
    const QVideoFrameFormat format = frame->format();
    {
        QVideoFrameTexturesUPtr none;
        const QVideoFrameTexturesUPtr textures = frame->mapTextures(rhi, none);
        if (!textures || !textures->texture(0)) {
            return QStringLiteral("the frame did not import into the Vulkan QRhi");
        }
    }
    const quint64 failuresBefore =
        GstHwPathTelemetry::snapshot().count(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::MapFailures);
    QImage actual = QVideoFrame(std::make_unique<GstTest::RhiBoundVideoBuffer>(std::move(frame), &rhi)).toImage();
    // Qt's converter draws its quad without QRhi::clipSpaceCorrMatrix(), so under Vulkan, whose clip space points
    // down, the image comes out upside down.
    if (!rhi.isYUpInNDC()) {
        actual.flip(Qt::Vertical);
    }
    if (GstHwPathTelemetry::snapshot().count(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::MapFailures) !=
        failuresBefore) {
        return QStringLiteral("the frame was converted from its CPU map, not from the imported textures");
    }
    const QImage expected = GstTest::cpuImage(mapCpuFrame(gst_sample_get_buffer(reference), referenceInfo, format));
    // Subsampled chroma is interpolated by the GPU up to two pixels into a neighbor bar.
    return GstTest::firstImageMismatch(actual, expected, /*tolerance=*/8, subsampled ? 2 : 0);
}

}  // namespace

void GstHwBuffersVulkanReadbackTest::_testVaFrameToImage_data()
{
    QTest::addColumn<int>("gstFormat");
    QTest::addColumn<int>("qtFormat");
    QTest::addColumn<QString>("source");    ///< What the two branches are fed.
    QTest::addColumn<QString>("producer");  ///< The VA element whose output is imported.

    // A decoder's frames, as a stream delivers them, and the other plane layouts through the VA postprocessor.
    const QString raw = QStringLiteral("video/x-raw,format=%1,width=320,height=240,framerate=30/1");
    const QString decoder = QStringLiteral("vah264dec");
    const QString postprocessor = QStringLiteral("vapostproc");
    QTest::newRow("NV12, decoded") << int(GST_VIDEO_FORMAT_NV12) << int(QVideoFrameFormat::Format_NV12)
                                   << (raw.arg(QStringLiteral("NV12")) + QStringLiteral(" ! vah264enc ! h264parse"))
                                   << decoder;
    QTest::newRow("I420") << int(GST_VIDEO_FORMAT_I420) << int(QVideoFrameFormat::Format_YUV420P)
                          << raw.arg(QStringLiteral("I420")) << postprocessor;
    QTest::newRow("P010") << int(GST_VIDEO_FORMAT_P010_10LE) << int(QVideoFrameFormat::Format_P010)
                          << raw.arg(QStringLiteral("P010_10LE")) << postprocessor;
    QTest::newRow("RGBA") << int(GST_VIDEO_FORMAT_RGBA) << int(QVideoFrameFormat::Format_RGBA8888)
                          << raw.arg(QStringLiteral("RGBA")) << postprocessor;
    QTest::newRow("BGRA") << int(GST_VIDEO_FORMAT_BGRA) << int(QVideoFrameFormat::Format_BGRA8888)
                          << raw.arg(QStringLiteral("BGRA")) << postprocessor;
}

void GstHwBuffersVulkanReadbackTest::_testVaFrameToImage()
{
    QFETCH(int, gstFormat);
    QFETCH(int, qtFormat);
    QFETCH(QString, source);
    QFETCH(QString, producer);

    QRhi* rhi = _vulkanRhi();
    if (!rhi) {
        QSKIP(qPrintable(_rhiUnavailable));
    }
    GSTTEST_SKIP_UNLESS_FACTORIES("vah264enc", "vah264dec", "vapostproc");
    const ScopedVulkanSnapshot vulkanSceneGraph;
    const auto format = static_cast<GstVideoFormat>(gstFormat);
    const std::array formats{format};
    std::string drmCaps = GstHw::buildVulkanDmaDrmCaps(formats, &toQtPixelFormat);
    if (drmCaps.empty()) {
        QSKIP("The Vulkan device imports no layout of this format");
    }
    GstDmaBufVideoBuffer::resetCachedState();

    GstTest::VaFramePair frames;
    GstTest::pullVaFramePair(frames, source, producer, drmCaps, format);
    if (!frames.gpu) {
        return;
    }

    GstVideoInfo gpuInfo;
    QVERIFY(GstHw::dmaDrmAwareVideoInfo(gst_sample_get_caps(frames.gpu.get()), &gpuInfo));
    GstVideoInfo cpuInfo;
    QVERIFY(gst_video_info_from_caps(&cpuInfo, gst_sample_get_caps(frames.cpu.get())));
    auto frame = std::make_unique<GstDmaBufVideoBuffer>(
        frames.gpu.get(), gpuInfo,
        QVideoFrameFormat(QSize(320, 240), static_cast<QVideoFrameFormat::PixelFormat>(qtFormat)), EGL_NO_DISPLAY);
    const GStreamer::GCharPtr negotiated =
        GStreamer::adoptGChar(gst_caps_to_string(gst_sample_get_caps(frames.gpu.get())));
    QVERIFY2(frame->validatePlaneHandles(), negotiated.get());

    const QString mismatch =
        vulkanImportMismatch(std::move(frame), *rhi, frames.cpu.get(), cpuInfo, GST_VIDEO_INFO_IS_YUV(&cpuInfo));
    QVERIFY2(mismatch.isEmpty(), qPrintable(QStringLiteral("%1 (%2)").arg(mismatch, QLatin1String(negotiated.get()))));
}

void GstHwBuffersVulkanReadbackTest::_testVaStreamTakesDmaBufPath()
{
    QRhi* rhi = _vulkanRhi();
    if (!rhi) {
        QSKIP(qPrintable(_rhiUnavailable));
    }
    GSTTEST_SKIP_UNLESS_FACTORIES("vah264enc", "vah264dec");
    const ScopedVulkanSnapshot vulkanSceneGraph;
    // What the sink reads once Qt Quick's Vulkan scene graph runs on this device.
    const GstQgc::SinkEnvironment environment{.target = {.backend = GstQgc::RenderBackend::Vulkan},
                                              .dmaBufDisplay = GstDmaBufVulkan::importDevice()};
    GstDmaBufVideoBuffer::resetCachedState();

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
                                      QStringLiteral("videotestsrc num-buffers=10 ! "
                                                     "video/x-raw,format=NV12,width=320,height=240,framerate=30/1 ! "
                                                     "vah264enc ! h264parse ! vah264dec"),
                                      {.gpuZeroCopy = true, .environment = environment});
    if (!sinkBin) {
        QSKIP(qPrintable(QStringLiteral("VA pipeline parse skipped: %1").arg(pipeline.error())));
    }
    const GStreamer::GstElementPtr& qvideosink = sinkBin.qvideosink;
    QVERIFY(GStreamer::setupQVideoSinkElement(qvideosink.get(), &videoSink, &owner));
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(qvideosink.get(), "sink"));
    QVERIFY(sinkPad);
    if (!pipeline.play()) {
        QSKIP("VA pipeline failed to PLAY (no VA driver / DRI device?)");
    }
    const QString failure = pipeline.runToEos();
    const QString negotiated = GstTest::currentCapsString(sinkPad.get());
    if (!failure.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("VA pipeline into the sink failed: %1 (%2)").arg(failure, negotiated)));
    }
    if (!negotiated.contains(QStringLiteral("memory:DMABuf"))) {
        QSKIP(qPrintable(QStringLiteral("VA driver exports no layout the Vulkan device imports: %1").arg(negotiated)));
    }

    QVERIFY_TRUE_WAIT(GstQgc::sinkStats(qvideosink.get()).frames(HwVideoBufferPath::DmaBuf) > 0,
                      TestTimeout::shortMs());
    QVERIFY_TRUE_WAIT(frame.isValid(), TestTimeout::shortMs());
    QHwVideoBuffer* const buffer = QVideoFramePrivate::hwBuffer(frame);
    QVERIFY2(dynamic_cast<GstDmaBufVideoBuffer*>(buffer), "the frame did not take the DMABuf path");
    QVideoFrameTexturesUPtr none;
    const QVideoFrameTexturesUPtr textures = buffer->mapTextures(*rhi, none);
    QVERIFY2(textures, qPrintable(QStringLiteral("the delivered frame did not import (%1)").arg(negotiated)));
}

UT_REGISTER_TEST(GstHwBuffersVulkanReadbackTest, TestLabel::Integration)
