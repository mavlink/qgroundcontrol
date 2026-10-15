#pragma once

// CUDA memory is routed by exporting it to a DMABuf fd (gst_cuda_memory_export, GStreamer 1.24+) and reusing the DMABuf
// EGLImage path rather than CUDA-GL interop. Built when HwBuffers/CMakeLists.txt finds gst-cuda and cuda.h; not yet
// validated on NVIDIA hardware.

#if defined(QGC_HAS_GST_CUDA_GPU_PATH)

#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <memory>

#include "GstHwVideoBufferFactory.h"

class GstHwVideoBuffer;

namespace GstCudaVideoBuffer {

/// False once an export failed: GstHwPaths::kCuda then sends CUDA frames to the CPU for the rest of the process.
bool exportSupported() noexcept;

/// Export @p sample's CUDA memory to a DMABuf-backed GstHwVideoBuffer via gst_cuda_memory_export. Returns nullptr, and
/// turns exportSupported() off, when the running driver can't export it or the DMABuf importer would reject the result.
std::unique_ptr<GstHwVideoBuffer> exportToDmaBuf(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                                                 const HwVideoBufferContext& context);

}  // namespace GstCudaVideoBuffer

#endif  // QGC_HAS_GST_CUDA_GPU_PATH
