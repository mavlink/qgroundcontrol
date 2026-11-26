#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)

#include "GstHwVideoBuffer.h"

class QRhi;

/// Zero-copy QVideoFrame backing for GstGLMemory samples; QRhi GL context must share with GstGLContext (see
/// GstGlContextBridge).
class GstGlVideoBuffer final : public GstHwVideoBuffer
{
public:
    /// @p sample is ref'd; the buffer keeps it alive until destruction.
    using GstHwVideoBuffer::GstHwVideoBuffer;

    QVideoFrameTexturesUPtr importTextures(QRhi& rhi, QVideoFrameTexturesUPtr& oldTextures) override;
    bool validatePlaneHandles() const override;
};

#endif  // QGC_HAS_GST_GLMEMORY_GPU_PATH
