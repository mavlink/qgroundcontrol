#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)

#include <gst/gst.h>

/// Answers gst.vulkan.instance/device/queue NEED_CONTEXT so vulkanh26{4,5}dec allocates its GstVulkanImageMemory on the
/// VkDevice that QRhi (Vulkan backend) renders with — the precondition for zero-copy import. No-ops when the active RHI
/// isn't Vulkan, so the importer's per-frame device-match guard is the hard safety net.
namespace GstVulkanContextBridge {

/// Idempotent; builds a GstVulkanInstance/Device/Queue from QRhi's native Vulkan handles. True on success.
bool prime();

/// Inspect a NEED_CONTEXT and respond with the shared instance/device/queue; GST_BUS_DROP when consumed, else
/// GST_BUS_PASS. Thread-safe.
GstBusSyncReply handleSyncMessage(GstMessage* message);

/// Answer a downstream GST_QUERY_CONTEXT for gst.vulkan.{instance,device,queue}; true -> GST_PAD_PROBE_HANDLED.
bool answerContextQuery(GstQuery* query);

/// Drop the cached instance/device so the next prime() rebuilds; call from receiver teardown.
void reset();

/// Clear exhausted-retry latch so a later NEED_CONTEXT can prime; no-op if already primed.
void rearm();

}  // namespace GstVulkanContextBridge

#endif  // QGC_HAS_GST_VULKAN_GPU_PATH
