#pragma once

// CUDA memory handed to the DMABuf path through the fd gst_cuda_memory_export() returns (GStreamer 1.24+), instead of
// CUDA-GL interop. Never run on NVIDIA hardware, and not expected to import there: the fd is CUDA's shareable handle
// (cuMemExportToShareableHandle), which the desktop driver's EGL refuses as a dma-buf, so the path checks what it was
// given and turns itself off when that is no dma-buf. Only memory from a cuMemCreate pool (GST_CUDA_MEMORY_ALLOC_MMAP)
// can be exported at all, and nothing here asks a decoder for one. Built when HwBuffers/CMakeLists.txt finds gst-cuda
// and cuda.h.

#if defined(QGC_HAS_GST_CUDA_GPU_PATH)

#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <memory>

#include "GstHwVideoBufferFactory.h"

class GstHwVideoBuffer;

namespace GstCudaVideoBuffer {

/// False once the driver failed to export exportable memory, or exported something else than a dma-buf: neither
/// changes while the process runs, so GstHwPaths::kCuda then sends every CUDA frame to the CPU.
bool exportSupported() noexcept;

/// Wraps the fd gst_cuda_memory_export() returns for @p sample's CUDA memory in a DMABuf-backed GstHwVideoBuffer.
/// Returns nullptr when there is none to wrap, and turns exportSupported() off when the driver is why.
std::unique_ptr<GstHwVideoBuffer> exportToDmaBuf(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                                                 const HwVideoBufferContext& context);

}  // namespace GstCudaVideoBuffer

#endif  // QGC_HAS_GST_CUDA_GPU_PATH
