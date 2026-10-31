#include "GstCudaVideoBuffer.h"

#if defined(QGC_HAS_GST_CUDA_GPU_PATH)

#include <atomic>
#include <gst/allocators/gstdmabuf.h>
#include <gst/cuda/gstcuda.h>
#include <gst/video/video.h>
#include <memory>
#include <utility>

#include <QtCore/QLoggingCategory>

#include "GstDmaBufVideoBuffer.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstCudaLog, "Video.GStreamer.HwBuffers.GstCuda")

namespace GstCudaVideoBuffer {
namespace {

// Desktop dGPU drivers can reject the export on every frame, so the first failure turns the path off for the process.
std::atomic<bool> s_exportUnsupported{false};

void disableAfterFailedExport()
{
    if (!s_exportUnsupported.exchange(true, std::memory_order_relaxed)) {
        GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf,
                                                 GstHwPathTelemetry::HwFallbackReason::NoExt);
        qCWarning(GstCudaLog) << "CUDA->DMABuf export unsupported on this driver — CPU fallback for the"
                              << "remainder of the process";
    }
}

// gst_cuda_memory_export only succeeds for cuMemCreate/cuMemMap-backed allocations (ALLOC_MMAP); the common
// cuMemAlloc/pitch decoder default cannot be shared and returns FALSE -> nullptr -> factory CPU latch.
int exportCudaMemoryToFd(GstMemory* mem)
{
    if (!gst_is_cuda_memory(mem)) {
        return -1;
    }
    auto* cudaMem = reinterpret_cast<GstCudaMemory*>(mem);

    // The decoder's CUDA stream may still be writing; the EGL import has no ordering against it.
    gst_cuda_memory_sync(cudaMem);

    // Linux: os_handle is an int* receiving a POSIX file descriptor (CU_MEM_HANDLE_TYPE_POSIX_FILE_DESCRIPTOR). The fd
    // stays owned by @p mem (closed when the CUDA memory is freed) and is reused across exports.
    int fd = -1;
    if (!gst_cuda_memory_export(cudaMem, &fd) || fd < 0) {
        return -1;
    }
    return fd;
}

std::unique_ptr<GstHwVideoBuffer> exportSample(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                                               const HwVideoBufferContext& context)
{
    if (context.dmaBufEglDisplay == EGL_NO_DISPLAY) {
        return nullptr;  // No EGLImage display to import the exported fd; fall back to CPU.
    }

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        return nullptr;
    }

    // Single contiguous CUDA allocation per frame (NV12/P010 planes share one device buffer); multi-memory CUDA
    // layouts are out of scope, so only memory 0 is exported.
    GstMemory* mem0 = gst_buffer_peek_memory(buffer, 0);
    const int fd = exportCudaMemoryToFd(mem0);
    if (fd < 0) {
        return nullptr;
    }

    // Process-lifetime allocator; memories hold their own ref on it.
    static GstAllocator* const s_dmabufAllocator = gst_dmabuf_allocator_new();
    if (!s_dmabufAllocator) {
        return nullptr;
    }

    // DONT_CLOSE: the fd belongs to the CUDA memory, which the parent-buffer meta below keeps alive.
    GstMemory* dmaMem = gst_dmabuf_allocator_alloc_with_flags(
        s_dmabufAllocator, fd, gst_memory_get_sizes(mem0, nullptr, nullptr), GST_FD_MEMORY_FLAG_DONT_CLOSE);
    if (!dmaMem) {
        return nullptr;
    }

    GstBuffer* dmaBuffer = gst_buffer_new();
    if (!dmaBuffer) {
        gst_memory_unref(dmaMem);
        return nullptr;
    }
    gst_buffer_append_memory(dmaBuffer, dmaMem);
    // Pin the CUDA buffer until the render thread drops this frame, else it returns to the decoder pool and is
    // rewritten while still being sampled.
    gst_buffer_add_parent_buffer_meta(dmaBuffer, buffer);

    // Carry the source GstVideoMeta so GstDmaBufVideoBuffer reads correct per-plane offsets/strides without mmapping.
    if (GstVideoMeta* srcMeta = gst_buffer_get_video_meta(buffer)) {
        gst_buffer_add_video_meta_full(dmaBuffer, GST_VIDEO_FRAME_FLAG_NONE, srcMeta->format, srcMeta->width,
                                       srcMeta->height, srcMeta->n_planes, srcMeta->offset, srcMeta->stride);
    } else {
        gst_buffer_add_video_meta(dmaBuffer, GST_VIDEO_FRAME_FLAG_NONE, GST_VIDEO_INFO_FORMAT(&info),
                                  GST_VIDEO_INFO_WIDTH(&info), GST_VIDEO_INFO_HEIGHT(&info));
    }

    // Reuse the source caps: the exported fd is LINEAR device memory, so the original (non-DRM) caps drive a
    // modifier-0 import in GstDmaBufVideoBuffer.
    GstCaps* caps = gst_sample_get_caps(sample);
    GstSample* dmaSample = gst_sample_new(dmaBuffer, caps, nullptr, nullptr);
    gst_buffer_unref(dmaBuffer);
    if (!dmaSample) {
        return nullptr;
    }

    // GstHwVideoBuffer refs the sample; our local ref drops here.
    auto buf = std::make_unique<GstDmaBufVideoBuffer>(dmaSample, info, format, context.dmaBufEglDisplay);
    gst_sample_unref(dmaSample);
    return buf;
}

}  // namespace

bool exportSupported() noexcept
{
    return !s_exportUnsupported.load(std::memory_order_relaxed);
}

std::unique_ptr<GstHwVideoBuffer> exportToDmaBuf(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                                                 const HwVideoBufferContext& context)
{
    // A frame the DMABuf importer would reject latches the path off too (the factory then re-runs this cheap check).
    auto buf = exportSample(sample, info, std::move(format), context);
    if (!buf || !buf->validatePlaneHandles()) {
        disableAfterFailedExport();
        return nullptr;
    }
    return buf;
}

}  // namespace GstCudaVideoBuffer

constinit const HwPathDescriptor GstHwPaths::kCuda = {
    .path = HwVideoBufferPath::DmaBuf,
    .matches = [](GstMemory* mem, const HwVideoBufferContext&) -> bool { return gst_is_cuda_memory(mem); },
    .admits = &GstCudaVideoBuffer::exportSupported,
    .construct = &GstCudaVideoBuffer::exportToDmaBuf,
};

#endif  // QGC_HAS_GST_CUDA_GPU_PATH
