#pragma once

#include "GStreamerTestBase.h"

/// The Direct3D 11 and Direct3D 12 paths fed by pipelines that upload into GPU memory, on the QRhi each can render
/// with. Built on Windows with either path; each test skips where the host can't create the device.
class GstHwBuffersD3DTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void _testD3D11MemoryDispatch();
    void _testD3D12MemoryDispatch();
    void _testD3DMapTexturesWithQRhi_data();
    void _testD3DMapTexturesWithQRhi();
    void _testD3DFrameToImage_data();
    void _testD3DFrameToImage();
    void _testD3DBridgeDropsRemovedDevice_data();
    void _testD3DBridgeDropsRemovedDevice();
    void _testWindowsGpuSinkPreservesSoftwarePixels();
    void _testWindowsGpuSinkPreservesDirectD3D11Memory();
};
