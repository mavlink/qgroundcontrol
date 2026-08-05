#include "GstCudaVideoBuffer.h"

#if defined(QGC_HAS_GST_CUDA_GPU_PATH)

#include <gst/allocators/gstdmabuf.h>
#include <gst/cuda/gstcuda.h>
#include <gst/video/video.h>
#include <memory>
#include <unistd.h>

#include "GstDmaBufVideoBuffer.h"

namespace GstCudaVideoBuffer {
namespace {

// gst_cuda_memory_export only succeeds for cuMemCreate/cuMemMap-backed allocations (ALLOC_MMAP); the common
// cuMemAlloc/pitch decoder default cannot be shared and returns FALSE -> nullptr -> factory CPU latch.
int exportCudaMemoryToFd(GstMemory* mem)
{
    if (!gst_is_cuda_memory(mem)) {
        return -1;
    }
    auto* cudaMem = reinterpret_cast<GstCudaMemory*>(mem);

    // Linux: os_handle is an int* receiving a POSIX file descriptor (CU_MEM_HANDLE_TYPE_POSIX_FILE_DESCRIPTOR).
    // gst_cuda_memory_export hands us an owned fd; we transfer it to the dmabuf allocator below.
    int fd = -1;
    if (!gst_cuda_memory_export(cudaMem, &fd) || fd < 0) {
        return -1;
    }
    return fd;
}

}  // namespace

std::unique_ptr<GstHwVideoBuffer> exportToDmaBuf(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
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

    GstAllocator* allocator = gst_dmabuf_allocator_new();
    if (!allocator) {
        close(fd);
        return nullptr;
    }

    // Allocator takes ownership of fd; on success it is closed when dmaMem (and the GstBuffer wrapping it) is unreffed.
    GstMemory* dmaMem = gst_dmabuf_allocator_alloc(allocator, fd, gst_memory_get_sizes(mem0, nullptr, nullptr));
    if (!dmaMem) {
        close(fd);
        gst_object_unref(allocator);
        return nullptr;
    }

    GstBuffer* dmaBuffer = gst_buffer_new();
    if (!dmaBuffer) {
        gst_memory_unref(dmaMem);
        gst_object_unref(allocator);
        return nullptr;
    }
    gst_buffer_append_memory(dmaBuffer, dmaMem);

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
        gst_object_unref(allocator);
        return nullptr;
    }

    // GstHwVideoBuffer refs the sample; our local refs (sample + allocator) drop here.
    auto buf = std::make_unique<GstDmaBufVideoBuffer>(dmaSample, info, format, context.dmaBufEglDisplay);
    gst_sample_unref(dmaSample);
    gst_object_unref(allocator);
    return buf;
}

}  // namespace GstCudaVideoBuffer

#endif  // QGC_HAS_GST_CUDA_GPU_PATH
