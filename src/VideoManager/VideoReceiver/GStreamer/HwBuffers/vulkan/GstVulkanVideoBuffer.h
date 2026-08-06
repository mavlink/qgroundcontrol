#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)

#include <QtMultimedia/QVideoFrameFormat>
#include <gst/gst.h>
#include <gst/video/video-info.h>

#include "GstHwVideoBuffer.h"

class QRhi;

/// Zero-copy wrapper for vulkanh264dec/vulkanh265dec output: imports the decoder's GstVulkanImageMemory VkImage as a
/// non-owning QRhiTexture. The VkImage stays owned by GstVulkanImageMemory and is kept alive via the held GstSample;
/// only valid when QRhi's VkDevice matches the gst-vulkan device (else mapTextures fails -> CPU fallback).
class GstVulkanVideoBuffer final : public GstHwVideoBuffer
{
public:
    GstVulkanVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format);

    const char* storageTag() const override { return "Vulkan"; }
    bool validatePlaneHandles() const override;
    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& old) override;
};

#endif  // QGC_HAS_GST_VULKAN_GPU_PATH
