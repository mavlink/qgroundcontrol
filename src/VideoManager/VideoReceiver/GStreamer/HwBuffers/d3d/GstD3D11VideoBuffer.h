#pragma once

#include <QtCore/qglobal.h>

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)

#include <array>

#include "GstHwVideoBuffer.h"

class QRhi;
struct ID3D11Texture2D;

/// Wraps a D3D11Memory-backed GstSample as a QHwVideoBuffer; device guard and slice copy run at construction on the
/// streaming thread so mapTextures only imports resolved textures.
class GstD3D11VideoBuffer final : public GstHwVideoBuffer
{
public:
    GstD3D11VideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format);
    ~GstD3D11VideoBuffer() override;

    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& oldTextures) override;
    bool validatePlaneHandles() const override;

    const char* storageTag() const override { return "D3D11"; }

    /// Release pooled staging textures on device-loss/TDR; wired into the facade reset path.
    static void resetCachedState() noexcept;

private:
    /// Streaming-thread resolve: device guard and slice copy; sets _resolved false on any failure.
    void resolvePlaneResources();

    std::array<ID3D11Texture2D*, GstHw::kMaxPlanes> _textures{};
    int _resolvedCount = 0;
    bool _resolved = false;
};

#endif  // Q_OS_WIN && QGC_HAS_GST_D3D11_GPU_PATH
