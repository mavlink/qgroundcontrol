#include "GstHwBuffersVulkanReadbackTest.h"

#include <rhi/qrhi.h>

#ifdef QGC_GST_STREAMING

#include <array>
#include <atomic>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <memory>
#include <string>

#include <QtCore/QByteArray>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtMultimedia/QVideoSink>

#include "GStreamer.h"
#include "GStreamerTestHelpers.h"

#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT) || defined(QGC_HAS_GST_VULKAN_GPU_PATH)
#include <rhi/qrhi_platform.h>

#include <QtGui/private/qvulkandefaultinstance_p.h>

#include "QGCRhiCapture.h"
#endif

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
#include <QtMultimedia/private/qhwvideobuffer_p.h>

// gst-vulkan's gstvkdecoder.h declares a struct member named `slots`, which collides with Qt's `slots` keyword macro.
#pragma push_macro("slots")
#undef slots
#include <gst/vulkan/gstvkdevice.h>
#include <gst/vulkan/gstvkimagememory.h>
#pragma pop_macro("slots")

#include "GstHwPathTelemetry.h"
#include "GstScoped.h"
#include "GstVulkanContextBridge.h"
#include "GstVulkanVideoBuffer.h"
#endif

#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>

#include "CpuVideoFramePool.h"
#include "GstDmaBufVideoBuffer.h"
#include "GstDmaBufVulkanImport.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoInfo.h"
#include "HwBuffers/GstHwImageReadback.h"
#include "HwBuffers/dmabuf/GstDmaDrmCaps.h"
#include "gstqgc/GstQgcCaps.h"
#include "gstqgc/GstQgcSinkStats.h"
#include "gstqgc/GstQgcTestSupport.h"
#endif

#endif  // QGC_GST_STREAMING

GstHwBuffersVulkanReadbackTest::GstHwBuffersVulkanReadbackTest() = default;
GstHwBuffersVulkanReadbackTest::~GstHwBuffersVulkanReadbackTest() = default;

QRhi* GstHwBuffersVulkanReadbackTest::_vulkanRhi()
{
#if defined(QGC_GST_STREAMING) && defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    if (_rhiProbed) {
        return _rhi.get();
    }
    _rhiProbed = true;
    // As HwBuffers::configureQtGraphics() does before the application's QRhi exists.
    GstDmaBufVulkan::requestDeviceExtensions();
    // The instance Qt Quick renders with.
    QRhiVulkanInitParams params;
    params.inst = QVulkanDefaultInstance::instance();
    if (!params.inst) {
        _rhiUnavailable = QStringLiteral("No Vulkan instance on the %1 platform").arg(QGuiApplication::platformName());
        return nullptr;
    }
    _rhi.reset(QRhi::create(QRhi::Vulkan, &params));
    if (!_rhi) {
        _rhiUnavailable = QStringLiteral("QRhi::create(Vulkan) failed");
        return nullptr;
    }
    GstDmaBufVulkan::probeDevice(*_rhi);
    if (!GstDmaBufVulkan::importDevice()) {
        _rhiUnavailable =
            QStringLiteral("The Vulkan device (%1) doesn't import dma-bufs").arg(_rhi->driverInfo().deviceName);
        _rhi.reset();
    }
    return _rhi.get();
#else
    _rhiUnavailable = QStringLiteral("DMABuf import under Vulkan not compiled in this build");
    return nullptr;
#endif
}

#ifdef QGC_GST_STREAMING

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

#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT) || defined(QGC_HAS_GST_VULKAN_GPU_PATH)
namespace {

using VulkanTestSample = std::unique_ptr<GstSample, decltype(&gst_sample_unref)>;

}  // namespace
#endif

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
namespace {

/// A Vulkan QRhi on the instance Qt Quick renders with, on the software device when @p software; null with the
/// reason in @p unavailable.
std::unique_ptr<QRhi> createGstVulkanTestRhi(bool software, QString& unavailable)
{
    QRhiVulkanInitParams params;
    params.inst = QVulkanDefaultInstance::instance();
    if (!params.inst) {
        unavailable = QStringLiteral("No Vulkan instance on the %1 platform").arg(QGuiApplication::platformName());
        return {};
    }
    std::unique_ptr<QRhi> rhi(
        QRhi::create(QRhi::Vulkan, &params, software ? QRhi::Flags(QRhi::PreferSoftwareRenderer) : QRhi::Flags()));
    if (!rhi) {
        unavailable = QStringLiteral("QRhi::create(Vulkan) failed");
    } else if (software && (rhi->driverInfo().deviceType != QRhiDriverInfo::CpuDevice)) {
        unavailable = QStringLiteral("No software Vulkan device");
        rhi.reset();
    }
    return rhi;
}

/// Publishes @p rhi as the render thread publishes the scene graph's, primes the gst-vulkan bridge on it and returns
/// the GstVulkanDevice the bridge then answers context queries with; null when it doesn't prime.
GStreamer::GstObjectPtr primeVulkanBridgeOn(QRhi& rhi)
{
    GstContextBridgeBase& bridge = GstVulkanContextBridge::instance();
    QGCRhiCapture::publishDeviceSnapshotForTest(&rhi);
    bridge.reset();
    const GStreamer::GstQueryPtr query = GStreamer::adoptQuery(gst_query_new_context("gst.vulkan.device"));
    if (!bridge.prime() || !bridge.answerContextQuery(query.get())) {
        return {};
    }
    GstContext* context = nullptr;
    gst_query_parse_context(query.get(), &context);
    const GValue* const device =
        context ? gst_structure_get_value(gst_context_get_structure(context), "gst.vulkan.device") : nullptr;
    return (device && G_VALUE_HOLDS_OBJECT(device)) ? GStreamer::adoptObject(GST_OBJECT(g_value_dup_object(device)))
                                                    : GStreamer::GstObjectPtr();
}

}  // namespace
#endif

#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
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

VulkanTestSample lastSampleOf(const GstTest::TestPipeline& pipeline, const char* sink)
{
    GstSample* sample = nullptr;
    g_object_get(pipeline.element(sink).get(), "last-sample", &sample, nullptr);
    return VulkanTestSample(sample, &gst_sample_unref);
}

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
    const QImage expected =
        GstTest::cpuImage(CpuVideoFramePool::copyFromBuffer(gst_sample_get_buffer(reference), referenceInfo, format));
    // Subsampled chroma is interpolated by the GPU up to two pixels into a neighbor bar.
    return GstTest::firstImageMismatch(actual, expected, /*tolerance=*/8, subsampled ? 2 : 0);
}

}  // namespace
#endif

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
#if !defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT) || !GST_CHECK_VERSION(1, 24, 0)
    QSKIP("DMABuf DMA_DRM import under Vulkan not compiled in this build");
#else
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
    std::string drmCaps = GstHw::buildVulkanDmaDrmCaps(formats);
    if (drmCaps.empty()) {
        QSKIP("The Vulkan device imports no layout of this format");
    }
    drmCaps.resize(drmCaps.find_last_not_of("; ") + 1);
    GstDmaBufVideoBuffer::resetCachedState();

    // One SMPTE frame through the VA element twice: into the layout the driver picks among those the device imports
    // (usually a tiled one), and downloaded to system memory as the reference. A frame sampled with the wrong pitch,
    // plane offset, plane format or tiling shows in the bars.
    GstTest::TestPipeline pipeline;
    if (!pipeline.launch(QStringLiteral("videotestsrc num-buffers=1 pattern=smpte ! %1 ! tee name=t "
                                        "t. ! queue ! %2 ! capsfilter name=gpucaps ! fakesink name=gpu "
                                        "t. ! queue ! %2 ! video/x-raw,format=%3 ! fakesink name=cpu")
                             .arg(source, producer, QLatin1String(gst_video_format_to_string(format))))) {
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
        QSKIP(qPrintable(QStringLiteral("VA driver can't produce the offered layouts (%1): %2")
                             .arg(QString::fromStdString(drmCaps), failure)));
    }
    const VulkanTestSample gpuSample = lastSampleOf(pipeline, "gpu");
    const VulkanTestSample cpuSample = lastSampleOf(pipeline, "cpu");
    QVERIFY(gpuSample && cpuSample);

    GstVideoInfo gpuInfo;
    QVERIFY(GstHw::dmaDrmAwareVideoInfo(gst_sample_get_caps(gpuSample.get()), &gpuInfo));
    GstVideoInfo cpuInfo;
    QVERIFY(gst_video_info_from_caps(&cpuInfo, gst_sample_get_caps(cpuSample.get())));
    auto frame = std::make_unique<GstDmaBufVideoBuffer>(
        gpuSample.get(), gpuInfo,
        QVideoFrameFormat(QSize(320, 240), static_cast<QVideoFrameFormat::PixelFormat>(qtFormat)), EGL_NO_DISPLAY);
    const GStreamer::GCharPtr negotiated =
        GStreamer::adoptGChar(gst_caps_to_string(gst_sample_get_caps(gpuSample.get())));
    QVERIFY2(frame->validatePlaneHandles(), negotiated.get());

    const QString mismatch =
        vulkanImportMismatch(std::move(frame), *rhi, cpuSample.get(), cpuInfo, GST_VIDEO_INFO_IS_YUV(&cpuInfo));
    QVERIFY2(mismatch.isEmpty(), qPrintable(QStringLiteral("%1 (%2)").arg(mismatch, QLatin1String(negotiated.get()))));
#endif
}

void GstHwBuffersVulkanReadbackTest::_testVaStreamTakesDmaBufPath()
{
#if !defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT) || !GST_CHECK_VERSION(1, 24, 0)
    QSKIP("DMABuf DMA_DRM import under Vulkan not compiled in this build");
#else
    QRhi* rhi = _vulkanRhi();
    if (!rhi) {
        QSKIP(qPrintable(_rhiUnavailable));
    }
    GSTTEST_SKIP_UNLESS_FACTORIES("vah264enc", "vah264dec");
    const ScopedVulkanSnapshot vulkanSceneGraph;
    // What the sink reads once Qt Quick's Vulkan scene graph runs on this device.
    const GstQgc::ScopedSinkEnvironmentForTest environment(
        {.target = {.backend = GstQgc::RenderBackend::Vulkan}, .dmaBufDisplay = GstDmaBufVulkan::importDevice()});
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
    if (!pipeline.launch(QStringLiteral("videotestsrc num-buffers=10 ! "
                                        "video/x-raw,format=NV12,width=320,height=240,framerate=30/1 ! "
                                        "vah264enc ! h264parse ! vah264dec ! "
                                        "qgcvideosinkbin name=sink gpu-zerocopy=true"))) {
        QSKIP(qPrintable(QStringLiteral("VA pipeline parse skipped: %1").arg(pipeline.error())));
    }
    const GStreamer::GstElementPtr sinkBin = pipeline.element("sink");
    QVERIFY(GStreamer::setupQVideoSinkElement(sinkBin.get(), &videoSink, &owner));
    const GStreamer::GstElementPtr qvideosink = GstQgcTest::qvideosink(sinkBin.get());
    QVERIFY(qvideosink);
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

    QVERIFY_TRUE_WAIT(GstQgc::frameStatsField(GstQgc::frameStats(qvideosink.get()).get(),
                                              GstQgc::sinkFramesField(HwVideoBufferPath::DmaBuf)) > 0,
                      TestTimeout::shortMs());
    QVERIFY_TRUE_WAIT(frame.isValid(), TestTimeout::shortMs());
    QHwVideoBuffer* const buffer = QVideoFramePrivate::hwBuffer(frame);
    QVERIFY2(dynamic_cast<GstDmaBufVideoBuffer*>(buffer), "the frame did not take the DMABuf path");
    QVideoFrameTexturesUPtr none;
    const QVideoFrameTexturesUPtr textures = buffer->mapTextures(*rhi, none);
    QVERIFY2(textures, qPrintable(QStringLiteral("the delivered frame did not import (%1)").arg(negotiated)));
#endif
}

void GstHwBuffersVulkanReadbackTest::_testGstVulkanBridgeSharesQtPhysicalDevice_data()
{
    QTest::addColumn<bool>("software");

    QTest::newRow("default device") << false;
    // Next to a GPU, not the first device either instance enumerates.
    QTest::newRow("software device") << true;
}

void GstHwBuffersVulkanReadbackTest::_testGstVulkanBridgeSharesQtPhysicalDevice()
{
#if !defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    QSKIP("gst-vulkan path not compiled in this build (QGC_GST_ENABLE_VULKAN)");
#else
    QFETCH(bool, software);

    QString unavailable;
    const std::unique_ptr<QRhi> rhi = createGstVulkanTestRhi(software, unavailable);
    if (!rhi) {
        QSKIP(qPrintable(unavailable));
    }

    const auto withdraw = qScopeGuard([this] { _withdrawPublishedRhi(); });
    const GStreamer::GstObjectPtr device = primeVulkanBridgeOn(*rhi);
    QVERIFY2(device, "GStreamer's Vulkan instance has no physical device matching the QRhi's");

    GstObject* physicalDevice = nullptr;
    g_object_get(device.get(), "physical-device", &physicalDevice, nullptr);
    const GStreamer::GstObjectPtr physicalDeviceOwner = GStreamer::adoptObject(physicalDevice);
    QVERIFY(physicalDevice);
    gchar* name = nullptr;
    g_object_get(physicalDevice, "name", &name, nullptr);
    const GStreamer::GCharPtr nameOwner(name);
    QCOMPARE(QByteArray(name), rhi->driverInfo().deviceName);
#endif
}

void GstHwBuffersVulkanReadbackTest::_testVulkanImageFrameIsNotImported()
{
#if !defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    QSKIP("gst-vulkan path not compiled in this build (QGC_GST_ENABLE_VULKAN)");
#else
    QString unavailable;
    const std::unique_ptr<QRhi> rhi = createGstVulkanTestRhi(/*software=*/false, unavailable);
    if (!rhi) {
        QSKIP(qPrintable(unavailable));
    }
    const auto withdraw = qScopeGuard([this] { _withdrawPublishedRhi(); });
    const GStreamer::GstObjectPtr device = primeVulkanBridgeOn(*rhi);
    QVERIFY2(device, "GStreamer's Vulkan instance has no physical device matching the QRhi's");

    // A frame as a gst-vulkan element on the bridge's device allocates it, in device-local memory. Allocated here
    // rather than by vulkanupload, whose upload hangs the GPU on some drivers (Mesa ANV with gst-vulkan 1.24).
    gst_vulkan_image_memory_init_once();
    GstMemory* const memory = gst_vulkan_image_memory_alloc(
        GST_VULKAN_DEVICE(device.get()), VK_FORMAT_R8G8B8A8_UNORM, 320, 240, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    QVERIFY(memory);
    GstBuffer* const buffer = gst_buffer_new();
    gst_buffer_append_memory(buffer, memory);
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(
        gst_caps_from_string("video/x-raw(memory:VulkanImage),format=RGBA,width=320,height=240,framerate=30/1"));
    const VulkanTestSample sample(gst_sample_new(buffer, caps.get(), nullptr, nullptr), &gst_sample_unref);
    gst_buffer_unref(buffer);
    GstVideoInfo info;
    QVERIFY(gst_video_info_from_caps(&info, caps.get()));
    GstVulkanVideoBuffer frame(sample.get(), info,
                               QVideoFrameFormat(QSize(320, 240), QVideoFrameFormat::Format_RGBA8888));
    QVERIFY(frame.validatePlaneHandles());

    // The image is on the same physical device as the QRhi but on GStreamer's VkDevice, so it can't be wrapped.
    using GstHwPathTelemetry::HwFallbackReason;
    const quint64 refusedBefore =
        GstHwPathTelemetry::snapshot().count(HwVideoBufferPath::Vulkan, HwFallbackReason::NoExt);
    // Warned about once per device.
    ignoreLogMessage("Video.GStreamer.HwBuffers.GstVulkanBuf", QtWarningMsg,
                     QRegularExpression(QStringLiteral("VkDevice != QRhi VkDevice")));
    QVideoFrameTexturesUPtr none;
    QVERIFY(!frame.mapTextures(*rhi, none));
    QCOMPARE(GstHwPathTelemetry::snapshot().count(HwVideoBufferPath::Vulkan, HwFallbackReason::NoExt),
             refusedBefore + 1);
    // Nor has the frame a CPU map for Qt to show it from: gst-vulkan maps host-visible memory only.
    QCOMPARE(frame.map(QVideoFrame::ReadOnly).planeCount, 0);
#endif
}

void GstHwBuffersVulkanReadbackTest::_withdrawPublishedRhi()
{
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    QGCRhiCapture::publishDeviceSnapshotForTest(nullptr);
    GstVulkanContextBridge::instance().reset();
    // Publishing probed the GPU paths with a test's own QRhi; the other tests probe theirs again.
    _rhi.reset();
    _rhiProbed = false;
#endif
}

#endif  // QGC_GST_STREAMING

UT_REGISTER_TEST(GstHwBuffersVulkanReadbackTest, TestLabel::Integration)
