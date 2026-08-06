#pragma once

#include <QtCore/qglobal.h>

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include <array>

#include "GstHwVideoBuffer.h"

class QRhi;
struct ID3D12Resource;

/// \brief Wraps a D3D12Memory-backed GstSample as a QHwVideoBuffer; samples natively on QRhi::D3D12.
/// Decoder sync + slice copy (GPU-fence blocking) run at construction on the streaming thread, never in mapTextures
/// (QSGRenderThread).
class GstD3D12VideoBuffer final : public GstHwVideoBuffer
{
public:
    GstD3D12VideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format);
    ~GstD3D12VideoBuffer() override;

    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& oldTextures) override;
    bool validatePlaneHandles() const override;

    const char* storageTag() const override { return "D3D12"; }

    /// Release pooled staging resources on device-loss/TDR; wired into the facade reset path.
    static void resetCachedState() noexcept;

private:
    /// Streaming-thread resolve: per-plane decoder sync, device guard, slice copy; fills _resources, leaves _resolved
    /// false on failure (mapTextures then uses CPU fallback).
    void resolvePlaneResources();

    std::array<ID3D12Resource*, GstHw::kMaxPlanes> _resources{};
    int _resolvedCount = 0;
    bool _resolved = false;
};

#endif  // Q_OS_WIN && QGC_HAS_GST_D3D12_GPU_PATH
