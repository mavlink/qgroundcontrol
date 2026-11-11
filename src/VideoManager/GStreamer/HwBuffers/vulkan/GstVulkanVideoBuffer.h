#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)

#include <gst/gst.h>
#include <gst/video/video-info.h>

#include <QtMultimedia/QVideoFrameFormat>

#include "GstHwVideoBuffer.h"

class QRhi;

/// Wrapper for GstVulkanImageMemory (vulkanh264dec/vulkanh265dec, vulkanupload). Not a zero-copy path: mapTextures()
/// refuses every frame, since QRhi can't sample an image of another VkDevice and gst-vulkan (through 1.28) only
/// creates its own. Qt then asks for the CPU map, which gst-vulkan grants for host-visible memory only, and its pools
/// allocate device-local memory: such a frame isn't shown at all. No stream gets here: the sink offers no
/// memory:VulkanImage caps, so the frames are downloaded upstream of it.
class GstVulkanVideoBuffer final : public GstHwVideoBuffer
{
public:
    GstVulkanVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format);

    bool validatePlaneHandles() const override;
    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& old) override;
};

#endif  // QGC_HAS_GST_VULKAN_GPU_PATH
