#pragma once

// SCAFFOLD — requires NVIDIA hardware to validate; gated behind QGC_HAS_GST_CUDA_GPU_PATH (never defined by CMake yet).
// CUDA/NVMM buffers are routed by exporting the device memory to a DMABuf fd and reusing the DMABuf EGLImage path
// rather than CUDA-GL interop. Enabling this requires CMake gating for gst/cuda/gstcuda.h (gst-plugins-bad CUDA) and a
// Jetson/desktop-NVIDIA test pass.

#if defined(QGC_HAS_GST_CUDA_GPU_PATH)

#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <memory>

#include "GstHwVideoBufferFactory.h"

class GstHwVideoBuffer;

namespace GstCudaVideoBuffer {

/// Export @p sample's CUDA/NVMM memory to a DMABuf-backed GstHwVideoBuffer (gst_cuda_memory_export on desktop,
/// NvBufSurfaceMapEglImage on Jetson). Returns nullptr to trigger the factory's CPU fallback when export is
/// unsupported on the running driver.
std::unique_ptr<GstHwVideoBuffer> exportToDmaBuf(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                                                 const HwVideoBufferContext& context);

}  // namespace GstCudaVideoBuffer

#endif  // QGC_HAS_GST_CUDA_GPU_PATH
