#pragma once

#include <QtCore/qglobal.h>
#include <QtMultimedia/QVideoFrameFormat>
#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <memory>

class QHwVideoBuffer;

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) || defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
#include <EGL/egl.h>
#endif

/// Identifies which GPU path was chosen; used by the adapter to increment the right counter.
enum class HwVideoBufferPath
{
    None,
    DmaBuf,
    GlMemory,
    D3D11,
    D3D12,
    IOSurface,
    AHardwareBuffer,
    Vulkan,
};

/// Per-stream GPU path cache, keyed to a caps epoch. Lets makeHwVideoBuffer() skip the predicate ladder once a path has
/// been resolved by a validated sample. Reset on set_caps.
struct HwResolvedPathCache
{
    HwVideoBufferPath path = HwVideoBufferPath::None;
    bool validated = false;        // path resolved by a prior validated sample this epoch
    bool demotionRecorded = false; // one-shot guard for recordStreamDemotion() per epoch
};

/// Platform context for the factory; encapsulates EGL handles so callers don't need path-specific ifdefs.
struct HwVideoBufferContext
{
    bool gpuEnabled = false;
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    EGLDisplay dmaBufEglDisplay = EGL_NO_DISPLAY;
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    EGLDisplay ahbEglDisplay = EGL_NO_DISPLAY;
#endif
};

/// Selects the zero-copy QHwVideoBuffer for a GstSample; returns nullptr when no GPU path matches (CPU memcpy
/// fallback).
std::unique_ptr<QHwVideoBuffer> makeHwVideoBuffer(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                                                  const HwVideoBufferContext& context, HwVideoBufferPath& matchedPath,
                                                  HwResolvedPathCache* cache = nullptr);
