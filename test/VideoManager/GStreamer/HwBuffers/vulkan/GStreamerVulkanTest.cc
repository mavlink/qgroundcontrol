#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <gst/gst.h>
#include <memory>

#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>

#include "GStreamer.h"
#include "GStreamerTestHelpers.h"
#include "GstHwPathTelemetry.h"
#include "gstqgc/gstqgcqvideosink.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) && defined(QGC_HAS_GST_VULKAN_GPU_PATH)
#include <rhi/qrhi.h>

#include "GstDmaBufVulkanImport.h"
#endif

void GStreamerTest::_testVulkanDispatchDemotesToCpu()
{
    if (!GstTest::firstMissingFactory({"vulkanupload"}).isEmpty()) {
        QSKIP("vulkanupload factory unavailable — gst-vulkan not registered in this build");
    }

    QVideoSink videoSink;
    int frameCount = 0;

    GstTest::TestPipeline pipeline;
    if (!pipeline.launch(QStringLiteral("videotestsrc num-buffers=10 ! "
                                        "video/x-raw,format=RGBA,width=320,height=240,framerate=30/1 ! "
                                        "vulkanupload ! "
                                        "video/x-raw(memory:VulkanImage) ! "
                                        "qgcvideosinkbin name=sink gpu-zerocopy=true"))) {
        QSKIP(qPrintable(QStringLiteral("Vulkan pipeline parse skipped: %1").arg(pipeline.error())));
    }

    const GStreamer::GstElementPtr sinkBin = pipeline.element("sink");
    QVERIFY2(sinkBin, "Could not find 'sink' element");

    auto owner = std::make_unique<QObject>();
    if (!GStreamer::setupQVideoSinkElement(sinkBin.get(), &videoSink, owner.get())) {
        QSKIP("setupQVideoSinkElement() failed (no GPU context for Vulkan)");
    }

    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, owner.get(),
                     [&frameCount](const QVideoFrame&) { ++frameCount; });

    const GStreamer::GstElementPtr qvideosink =
        GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(sinkBin.get()), "qgcqvideosink"));
    QVERIFY2(qvideosink, "Could not find 'qgcqvideosink' inside sink bin");
    const GStreamer::GstPadPtr appsinkPad = GStreamer::adoptPad(gst_element_get_static_pad(qvideosink.get(), "sink"));
    QVERIFY2(appsinkPad, "qgcqvideosink has no sink pad");

    std::atomic<int> bufferCount{0};
    GStreamer::PadProbe bufferProbe;
    QVERIFY(bufferProbe.add(appsinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &bufferCount));

    using GstHwPathTelemetry::HwFallbackReason;
    const quint64 noSyncBefore =
        GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::Vulkan, HwFallbackReason::VulkanNoSync);
    const quint64 noExtBefore =
        GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::Vulkan, HwFallbackReason::NoExt);

    if (!pipeline.play()) {
        QSKIP("Vulkan pipeline failed to PLAY (no Vulkan device in headless env?)");
    }

    const GStreamer::GstMessagePtr msg = pipeline.waitForEosOrError();
    const bool sawError = msg && (GST_MESSAGE_TYPE(msg.get()) == GST_MESSAGE_ERROR);
    const QString errMsg = sawError ? GstTest::errorText(msg.get()) : QString();

    // Demotion telemetry is recorded on the QRhi render path, which can land after the bus EOS. Headless runs never
    // record it and skip below, so this waits without UnitTest::waitForCondition()'s timeout warning.
    const bool demotionSignalled = QTest::qWaitFor(
        [&] {
            return (GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::Vulkan, HwFallbackReason::VulkanNoSync) >
                    noSyncBefore) ||
                   (GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::Vulkan, HwFallbackReason::NoExt) >
                    noExtBefore);
        },
        TestTimeout::shortDuration());

    const QString negotiatedStr = GstTest::currentCapsString(appsinkPad.get());
    const int probedBuffers = bufferCount.load();
    guint64 vulkanFrames = 0;
    if (const GStreamer::GstStructurePtr stats = GstQgc::frameStats(qvideosink.get())) {
        (void) gst_structure_get_uint64(stats.get(), GstQgc::sinkFramesField(HwVideoBufferPath::Vulkan), &vulkanFrames);
    }

    if (sawError) {
        QSKIP(qPrintable(QStringLiteral("Vulkan pipeline error (likely no Vulkan device): %1").arg(errMsg)));
    }
    if (probedBuffers == 0 || !negotiatedStr.contains(QStringLiteral("memory:VulkanImage"))) {
        QSKIP(qPrintable(QStringLiteral("Vulkan memory not negotiated headless. Buffers: %1, caps: %2")
                             .arg(probedBuffers)
                             .arg(negotiatedStr)));
    }

    // Vulkan is compiled but runtime-dormant: QGC pins the GL RHI, so the per-frame VkDevice-match
    // guard demotes every foreign-device frame to CPU. Assert delivery still works, never zero-copy.
    QVERIFY2(frameCount > 0 || probedBuffers > 0, "No frames delivered through the sink under Vulkan caps");
    QVERIFY2(vulkanFrames == 0,
             qPrintable(QStringLiteral("Vulkan zero-copy delivery was recorded but the device-match guard "
                                       "should demote (frames=%1)")
                            .arg(vulkanFrames)));

    if (!demotionSignalled) {
        QSKIP(qPrintable(QStringLiteral("Vulkan frames negotiated VulkanImage but the device-match demotion "
                                        "did not fire headless (mapTextures() needs a QRhi render thread). "
                                        "caps: %1")
                             .arg(negotiatedStr)));
    }
}

void GStreamerTest::_testVulkanOwnedFrameTexturesReleaseOnDestroy()
{
#if !defined(QGC_HAS_GST_DMABUF_GPU_PATH) || !defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    QSKIP("DMABuf->Vulkan frame bundles not compiled in this build");
#else
    const std::unique_ptr<QRhi> rhi = GstTest::makeNullRhi();
    QVERIFY(rhi);

    // Any non-null handle: the Null QRhi wraps it without touching Vulkan, while a null one is left unwrapped.
    const auto image = VkImage(quintptr(1));
    auto bundle = std::make_unique<GstVulkanOwnedFrameTextures>(rhi.get(), QSize(64, 64),
                                                                QVideoFrameFormat::Format_RGBA8888, image);
    QVERIFY(bundle->allPlanesValid());
    int releases = 0;
    bundle->adoptVulkanResources([&releases] { ++releases; });
    QRhiTexture* wrapped = bundle->texture(0);

    // Qt keeps sampling the current bundle across window frames; frame end must not release the imported image.
    for (int frame = 0; frame < 3; ++frame) {
        bundle->onFrameEndInvoked();
    }
    QCOMPARE(bundle->texture(0), wrapped);
    QCOMPARE(releases, 0);

    bundle.reset();
    QCOMPARE(releases, 1);
#endif
}

#endif
