#pragma once

#include "GStreamerTestBase.h"

/// The HwBuffers GPU paths fed by pipelines that upload into or decode to GPU memory (DMABuf, GLMemory, Vulkan,
/// D3D11/D3D12, IOSurface) when the host has the device; each test skips without it.
class GstHwBuffersPipelineTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testGlMemoryDispatch();
    void _testD3D11MemoryDispatch();
    void _testD3D12MemoryDispatch();
    void _testD3DMapTexturesWithQRhi_data();
    void _testD3DMapTexturesWithQRhi();
    void _testD3DFrameToImage_data();
    void _testD3DFrameToImage();
    void _testDmaBufDispatch();
    void _testVulkanDispatchDemotesToCpu();
    void _testHwBufferFactoryDispatchSystemMemory();
    void _testIOSurfaceFrameToImage();
#endif
};
