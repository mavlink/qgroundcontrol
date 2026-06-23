#pragma once

#include <QtCore/qglobal.h>

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)

#include "GstHwVideoBuffer.h"

class QRhi;

/// \brief Wraps a D3D11Memory-backed GstSample as a QHwVideoBuffer; samples natively on QRhi::D3D11.
///
class GstD3D11VideoBuffer final : public GstHwVideoBuffer
{
public:
    GstD3D11VideoBuffer(GstSample *sample,
                        const GstVideoInfo &videoInfo,
                        const QVideoFrameFormat &format);
    ~GstD3D11VideoBuffer() override;

    MapData map(QVideoFrame::MapMode mode) override;
    QVideoFrameTexturesUPtr mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr &oldTextures) override;
    bool validatePlaneHandles() const override;

    static quint64 takeMapFailureCount();
    static quint64 peekMapFailureCount();
};

#endif // Q_OS_WIN && QGC_HAS_GST_D3D11_GPU_PATH
