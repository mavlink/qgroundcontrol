#include "GstCudaVideoBuffer.h"

#if defined(QGC_HAS_GST_CUDA_GPU_PATH)

#include <atomic>
#include <gst/allocators/gstdmabuf.h>
#include <gst/cuda/gstcuda.h>
#include <gst/video/video.h>
#include <memory>

#include <QtCore/QLoggingCategory>

#include "GstDmaBufVideoBuffer.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstCudaLog, "Video.GStreamer.HwBuffers.GstCuda")

namespace GstCudaVideoBuffer {
namespace {

// What the driver exports doesn't change while the process runs, so its first refusal turns the path off for good.
std::atomic<bool> s_exportUnsupported{false};

void disableExport(const char* why)
{
    if (!s_exportUnsupported.exchange(true, std::memory_order_relaxed)) {
        GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf,
                                                 GstHwPathTelemetry::HwFallbackReason::ImportUnsupported);
        qCWarning(GstCudaLog) << "CUDA frames take the CPU path for the rest of the process:" << why;
    }
}

/// Only memory created with cuMemCreate/cuMemMap has a shareable handle. Decoders allocate with cuMemAlloc unless a
/// pool configured for GST_CUDA_MEMORY_ALLOC_MMAP is proposed to them, which QGC doesn't do.
bool isExportable(GstMemory* mem)
{
    return gst_is_cuda_memory(mem) &&
           (gst_cuda_memory_get_alloc_method(reinterpret_cast<GstCudaMemory*>(mem)) == GST_CUDA_MEMORY_ALLOC_MMAP);
}

/// The fd of @p mem's shareable handle, which stays owned by @p mem (closed when the CUDA memory is freed) and is the
/// same for every export. -1, with the path turned off, when the driver exports none or one that is no dma-buf.
int exportDmaBufFd(GstMemory* mem)
{
    auto* cudaMem = reinterpret_cast<GstCudaMemory*>(mem);

    // The decoder's CUDA stream may still be writing; the EGL import has no ordering against it.
    gst_cuda_memory_sync(cudaMem);

    // Linux: os_handle is an int* receiving a POSIX file descriptor (CU_MEM_HANDLE_TYPE_POSIX_FILE_DESCRIPTOR).
    int fd = -1;
    if (!gst_cuda_memory_export(cudaMem, &fd) || (fd < 0)) {
        disableExport("the driver doesn't export CUDA memory");
        return -1;
    }
    // Wrapped as a dma-buf without being one, the fd would fail every EGL import, and those failures would count
    // against the DMABuf path's own frames.
    if (!GstDmaBufVideoBuffer::isDmaBufFd(fd)) {
        disableExport("the handle the driver exports is not a dma-buf");
        return -1;
    }
    return fd;
}

// Asked for every frame: the memory, Qt's EGLDisplay and the DMABuf importer's state all change within a stream, and
// none of them says anything about what the driver exports.
bool admitCuda(GstMemory* mem, const HwVideoBufferContext& context)
{
    // The exported fd is imported as an EGLImage, with OpenGL only: under Vulkan the importer is no EGLDisplay.
    if (exportSupported() && isExportable(mem) && !GstDmaBufVideoBuffer::importDisabled() &&
        (GstDmaBufVideoBuffer::eglDisplayFor(context) != EGL_NO_DISPLAY)) {
        return true;
    }
    GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::HwFallbackReason::NoExt);
    return false;
}

}  // namespace

bool exportSupported() noexcept
{
    return !s_exportUnsupported.load(std::memory_order_relaxed);
}

std::unique_ptr<GstHwVideoBuffer> exportToDmaBuf(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                                                 const HwVideoBufferContext& context)
{
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        return nullptr;
    }

    // Single contiguous CUDA allocation per frame (NV12/P010 planes share one device buffer); multi-memory CUDA
    // layouts are out of scope, so only memory 0 is exported.
    GstMemory* mem0 = gst_buffer_peek_memory(buffer, 0);
    if (!isExportable(mem0)) {
        return nullptr;
    }
    const int fd = exportDmaBufFd(mem0);
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
    auto buf =
        std::make_unique<GstDmaBufVideoBuffer>(dmaSample, info, format, GstDmaBufVideoBuffer::eglDisplayFor(context));
    gst_sample_unref(dmaSample);
    return buf;
}

}  // namespace GstCudaVideoBuffer

constinit const HwPathDescriptor GstHwPaths::kCuda = {
    .path = HwVideoBufferPath::DmaBuf,
    .matches = [](GstMemory* mem) -> bool { return gst_is_cuda_memory(mem); },
    .admits = &GstCudaVideoBuffer::admitCuda,
    .construct = &GstCudaVideoBuffer::exportToDmaBuf,
};

#endif  // QGC_HAS_GST_CUDA_GPU_PATH
