#pragma once

#include <memory>

#include <QtCore/QString>

#include "GStreamerTestBase.h"

class QRhi;

/// DMABuf frames imported into a real Vulkan QRhi, as Qt Quick uses with QSG_RHI_BACKEND=vulkan, and converted with
/// QVideoFrame::toImage(); and, in builds with the gst-vulkan path, the device its bridge hands to GStreamer and what
/// becomes of a VulkanImage frame. Built with either. Needs a platform plugin with Vulkan support, which the offscreen
/// QPA lacks: CTest runs this class on xcb, and every test skips without the Vulkan device it needs.
class GstHwBuffersVulkanReadbackTest : public GStreamerTestBase
{
    Q_OBJECT

public:
    GstHwBuffersVulkanReadbackTest();
    ~GstHwBuffersVulkanReadbackTest() override;

private slots:
    void init() override;
    void cleanupTestCase() override;

#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    void _testVaFrameToImage_data();
    void _testVaFrameToImage();
    void _testVaStreamTakesDmaBufPath();
#endif
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    void _testGstVulkanBridgeSharesQtPhysicalDevice_data();
    void _testGstVulkanBridgeSharesQtPhysicalDevice();
    void _testGstVulkanInstanceCreatesBridgeDevice();
    void _testGstVulkanCreateDeviceHandlerFitsSignal_data();
    void _testGstVulkanCreateDeviceHandlerFitsSignal();
    void _testVulkanImageFrameIsNotImported();
#endif

private:
    /// A Vulkan QRhi whose device was asked for the import's extensions and probed, created once; null with the reason
    /// in _rhiUnavailable.
    QRhi* _vulkanRhi();
    /// Undoes a test's publishing of its own QRhi as the scene graph's, for the gst-vulkan bridge.
    void _withdrawPublishedRhi();

    std::unique_ptr<QRhi> _rhi;
    QString _rhiUnavailable;
    bool _rhiProbed = false;
};
