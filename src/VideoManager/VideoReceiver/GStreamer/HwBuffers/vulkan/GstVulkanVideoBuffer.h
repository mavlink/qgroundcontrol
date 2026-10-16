#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)

#include <QtMultimedia/QVideoFrameFormat>
#include <gst/gst.h>
#include <gst/video/video-info.h>

#include "GstHwVideoBuffer.h"

class QRhi;

/// Wrapper for vulkanh264dec/vulkanh265dec output (GstVulkanImageMemory). mapTextures() sends every frame to the CPU
/// path: QRhi can't sample an image from another VkDevice, and no decoder->QRhi sync exists for a same-device one.
class GstVulkanVideoBuffer final : public GstHwVideoBuffer
{
public:
    GstVulkanVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format);

    bool validatePlaneHandles() const override;
    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& old) override;
};

#endif  // QGC_HAS_GST_VULKAN_GPU_PATH
