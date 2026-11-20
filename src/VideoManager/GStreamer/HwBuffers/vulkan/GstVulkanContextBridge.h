#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)

#include "GstContextBridgeBase.h"

/// Answers gst.vulkan.instance/device/queue so gst-vulkan's elements (vulkanh26{4,5}dec, vulkanupload) share one
/// GstVulkanDevice on the physical device Qt renders on, when Qt renders with Vulkan; it never primes otherwise. That
/// device is not Qt's VkDevice: gst-vulkan can only create devices, not wrap one (gst_vulkan_device_new() takes a
/// physical device), so the images on it can't be sampled by QRhi and GstVulkanVideoBuffer imports none.
/// gst-vulkan asks for the instance on the bus but for the device on its pads only. An element that finds none there
/// asks its instance to create one, and gets the bridge's through the instance's create-device signal.
namespace GstVulkanContextBridge {

/// The process-wide Vulkan bridge.
GstContextBridgeBase& instance();

/// Has @p instance answer its create-device signal with @p device, a GstVulkanDevice the handler keeps a reference to,
/// through the handler that fits the signal's parameters: none before gst-vulkan 1.26, the index of the device asked
/// for from then on. The handler's id; 0, with nothing connected, when @p instance has no such signal or one with
/// other parameters.
gulong connectCreateDevice(GObject* instance, GstObject* device);

}  // namespace GstVulkanContextBridge

#endif  // QGC_HAS_GST_VULKAN_GPU_PATH
