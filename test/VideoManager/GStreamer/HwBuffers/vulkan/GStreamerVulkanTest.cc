#include "HwBuffers/GstHwBuffersTest.h"

#ifdef QGC_GST_STREAMING

#include <memory>

#include "GStreamerTestHelpers.h"

#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
#include <rhi/qrhi.h>
#include <span>

#include "GstDmaBufVulkanImport.h"
#endif

void GstHwBuffersTest::_testVulkanOwnedFrameTexturesReleaseOnDestroy()
{
#if !defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    QSKIP("DMABuf->Vulkan frame bundles not compiled in this build");
#else
    const std::unique_ptr<QRhi> rhi = GstTest::makeNullRhi();
    QVERIFY(rhi);

    // Any non-null handle: the Null QRhi wraps it without touching Vulkan, while a null one is left unwrapped.
    const auto image = VkImage(quintptr(1));
    auto bundle = std::make_unique<GstVulkanOwnedFrameTextures>(
        rhi.get(), QSize(64, 64), QVideoFrameFormat::Format_RGBA8888, std::span<const VkImage>(&image, 1));
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
