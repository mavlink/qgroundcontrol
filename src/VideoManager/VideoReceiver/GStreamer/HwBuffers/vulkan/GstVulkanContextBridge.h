#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)

#include "GstContextBridgeBase.h"

/// Answers gst.vulkan.instance/device/queue so vulkanh26{4,5}dec allocates its GstVulkanImageMemory on QRhi's physical
/// device (Vulkan backend) — the precondition for zero-copy import. Never primes when the active RHI isn't Vulkan, so
/// the importer's per-frame device-match guard is the hard safety net.
namespace GstVulkanContextBridge {

/// The process-wide Vulkan bridge.
GstContextBridgeBase& instance();

}  // namespace GstVulkanContextBridge

#endif  // QGC_HAS_GST_VULKAN_GPU_PATH
