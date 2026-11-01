#include "GstHwPathRegistry.h"

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <algorithm>
#include <atomic>

namespace GstHwPathRegistry {
namespace {

// CUDA precedes DMABuf because it exports CUDA memory to the DMABuf importer.
constexpr const HwPathDescriptor* kCompiledPaths[] = {
#if defined(QGC_HAS_GST_CUDA_GPU_PATH)
    &GstHwPaths::kCuda,
#endif
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    &GstHwPaths::kDmaBuf,
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    &GstHwPaths::kGlMemory,
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    &GstHwPaths::kD3D11,
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    &GstHwPaths::kD3D12,
#endif
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    &GstHwPaths::kVulkan,
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
    &GstHwPaths::kIOSurface,
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    &GstHwPaths::kAHardwareBuffer,
#endif
};

#ifdef QGC_UNITTEST_BUILD
constinit std::atomic<const std::vector<const HwPathDescriptor*>*> s_testPaths{nullptr};
#endif

}  // namespace

std::span<const HwPathDescriptor* const> paths() noexcept
{
#ifdef QGC_UNITTEST_BUILD
    if (const auto* testPaths = s_testPaths.load(std::memory_order_acquire)) {
        return *testPaths;
    }
#endif
    return kCompiledPaths;
}

bool has(HwVideoBufferPath path) noexcept
{
    const auto compiled = paths();
    return std::any_of(compiled.begin(), compiled.end(),
                       [path](const HwPathDescriptor* descriptor) { return descriptor->path == path; });
}

#ifdef QGC_UNITTEST_BUILD
ScopedPathsForTest::ScopedPathsForTest(std::initializer_list<const HwPathDescriptor*> paths)
    : _paths(paths)
    , _previous(s_testPaths.exchange(&_paths, std::memory_order_acq_rel))
{}

ScopedPathsForTest::~ScopedPathsForTest()
{
    s_testPaths.store(_previous, std::memory_order_release);
}
#endif

}  // namespace GstHwPathRegistry

#endif  // QGC_HAS_ANY_GPU_PATH
