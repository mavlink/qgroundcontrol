#pragma once

#include <memory>

#include <QtCore/QString>

#include "GStreamerTestBase.h"

class QRhi;

/// DMABuf frames imported into a real Vulkan QRhi, as Qt Quick uses with QSG_RHI_BACKEND=vulkan, and converted with
/// QVideoFrame::toImage(). Needs a platform plugin with Vulkan support, which the offscreen QPA lacks: CTest runs this
/// class on xcb, and every test skips without a Vulkan device that imports dma-bufs.
class GstHwBuffersVulkanReadbackTest : public GStreamerTestBase
{
    Q_OBJECT

public:
    GstHwBuffersVulkanReadbackTest();
    ~GstHwBuffersVulkanReadbackTest() override;

private slots:
#ifdef QGC_GST_STREAMING
    void init() override;
    void cleanupTestCase() override;

    void _testVaFrameToImage_data();
    void _testVaFrameToImage();
    void _testVaStreamTakesDmaBufPath();
#endif

private:
    /// A Vulkan QRhi whose device was asked for the import's extensions and probed, created once; null with the reason
    /// in _rhiUnavailable.
    QRhi* _vulkanRhi();

    std::unique_ptr<QRhi> _rhi;
    QString _rhiUnavailable;
    bool _rhiProbed = false;
};
