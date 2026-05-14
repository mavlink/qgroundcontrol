#pragma once

#include <QtCore/qglobal.h>

#if (defined(Q_OS_MACOS) || defined(Q_OS_IOS)) && defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)

#include "GstHwVideoBuffer.h"

class QRhi;

/// \brief Wraps a GstAppleCoreVideoMemory/IOSurface-backed sample as a QHwVideoBuffer; samples natively on QRhi::Metal.
///
class GstIOSurfaceVideoBuffer final : public GstHwVideoBuffer
{
public:
    GstIOSurfaceVideoBuffer(GstSample *sample,
                            const GstVideoInfo &videoInfo,
                            const QVideoFrameFormat &format);
    ~GstIOSurfaceVideoBuffer() override;

    MapData map(QVideoFrame::MapMode mode) override;
    QVideoFrameTexturesUPtr mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr &oldTextures) override;
    bool validatePlaneHandles() const override;

    static quint64 takeMapFailureCount();
    static quint64 peekMapFailureCount();
};

#endif // (Q_OS_MACOS || Q_OS_IOS) && QGC_HAS_GST_IOSURFACE_GPU_PATH
