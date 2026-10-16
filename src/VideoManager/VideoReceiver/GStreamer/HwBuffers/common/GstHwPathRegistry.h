#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <initializer_list>
#include <memory>
#include <span>
#include <vector>

#include <QtMultimedia/QVideoFrameFormat>

#include "GstHwTypes.h"

class GstContextBridgeBase;
class GstHwVideoBuffer;

/// Everything the factory and the HwBuffers facade know about one compiled GPU path. Each backend defines its
/// descriptor in its own translation unit (GstHwPaths below); GstHwPathRegistry lists the compiled ones.
struct HwPathDescriptor
{
    /// Path the imported buffers report. CUDA reports DmaBuf: it imports through the DMABuf importer.
    HwVideoBufferPath path = HwVideoBufferPath::None;

    /// True when this path imports @p mem's memory; plane 0's memory is the dispatch key. Streaming thread.
    bool (*matches)(GstMemory* mem, const HwVideoBufferContext& context) = nullptr;

    /// Optional. False sends a matched frame to the CPU without trying later paths; records why.
    bool (*admits)() = nullptr;

    /// Wraps @p sample for import; nullptr when it can't be imported.
    std::unique_ptr<GstHwVideoBuffer> (*construct)(GstSample* sample, const GstVideoInfo& info,
                                                   QVideoFrameFormat format,
                                                   const HwVideoBufferContext& context) = nullptr;

    /// Optional. Fills this path's fields of a GPU-enabled HwVideoBufferContext.
    void (*prepareContext)(HwVideoBufferContext& context) = nullptr;

    /// Optional. Drops cached native imports (scene-graph teardown, window swap, device loss).
    void (*resetCaches)() = nullptr;

    /// Optional. Adds a pool on Qt's device to an ALLOCATION query whose caps carry this path's memory; false when it
    /// didn't.
    bool (*proposePool)(GstQuery* query, GstCaps* caps, const GstVideoInfo* info, gsize size,
                        guint minBuffers) = nullptr;

    /// Optional. Bridge answering this path's GstContext requests.
    GstContextBridgeBase& (*bridge)() = nullptr;
};

/// Descriptors of the GPU paths, each defined by its backend's translation unit when CMake compiles it in.
namespace GstHwPaths {

/// HwPathDescriptor::construct for buffers that need nothing from the context.
template <class Buffer>
std::unique_ptr<GstHwVideoBuffer> construct(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                                            const HwVideoBufferContext& /*context*/)
{
    return std::make_unique<Buffer>(sample, info, format);
}

extern const HwPathDescriptor kCuda;
extern const HwPathDescriptor kDmaBuf;
extern const HwPathDescriptor kGlMemory;
extern const HwPathDescriptor kD3D11;
extern const HwPathDescriptor kD3D12;
extern const HwPathDescriptor kVulkan;
extern const HwPathDescriptor kIOSurface;
extern const HwPathDescriptor kAHardwareBuffer;

}  // namespace GstHwPaths

/// The compiled GPU paths, a constant list any thread can read.
namespace GstHwPathRegistry {

/// Compiled paths in selection order: a frame takes the first one whose matches() accepts its memory.
std::span<const HwPathDescriptor* const> paths() noexcept;

/// True when a compiled path imports frames as @p path.
bool has(HwVideoBufferPath path) noexcept;

#ifdef QGC_UNITTEST_BUILD
/// paths() returns only @p paths while alive, then what it returned before.
class ScopedPathsForTest
{
public:
    explicit ScopedPathsForTest(std::initializer_list<const HwPathDescriptor*> paths);
    ~ScopedPathsForTest();
    ScopedPathsForTest(const ScopedPathsForTest&) = delete;
    ScopedPathsForTest& operator=(const ScopedPathsForTest&) = delete;

private:
    std::vector<const HwPathDescriptor*> _paths;
    const std::vector<const HwPathDescriptor*>* _previous = nullptr;
};
#endif

}  // namespace GstHwPathRegistry

#endif  // QGC_HAS_ANY_GPU_PATH
