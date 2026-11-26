#pragma once

#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <memory>

#include <QtCore/qglobal.h>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstHwTypes.h"

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <initializer_list>
#include <span>
#include <vector>

class GstContextBridgeBase;
class GstHwVideoBuffer;
class QRhi;

/// Everything the factory and the HwBuffers facade know about one compiled GPU path. Each backend defines its
/// descriptor in its own translation unit (GstHwPaths below); GstHwPathRegistry lists the compiled ones.
struct HwPathDescriptor
{
    /// Path the imported buffers report.
    HwVideoBufferPath path = HwVideoBufferPath::None;

    /// True when this path imports @p mem's kind of memory; plane 0's memory is the dispatch key. Decided by the memory
    /// alone, so a miss holds for every frame of that allocator. Streaming thread.
    bool (*matches)(GstMemory* mem) = nullptr;

    /// Optional. False when the path can't import @p mem now (no display yet, a latched failure, a foreign context):
    /// the matched frame goes to the CPU without trying later paths, and a later frame is asked again. Records why.
    bool (*admits)(GstMemory* mem, const HwVideoBufferContext& context) = nullptr;

    /// Wraps @p sample for import; nullptr when it can't be imported.
    std::unique_ptr<GstHwVideoBuffer> (*construct)(GstSample* sample, const GstVideoInfo& info,
                                                   QVideoFrameFormat format,
                                                   const HwVideoBufferContext& context) = nullptr;

    /// Optional. Runs for each GPU-enabled HwVideoBufferContext a sink builds at start: fills this path's fields, or
    /// only logs once whether the path can import (DMABuf, whose display the sink sets per frame).
    void (*prepareContext)(HwVideoBufferContext& context) = nullptr;

    /// Optional. Runs on the render thread when the scene graph's QRhi is published, before the streaming threads
    /// see its backend: learns what its device imports.
    void (*probeDevice)(QRhi& rhi) = nullptr;

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

extern const HwPathDescriptor kDmaBuf;
extern const HwPathDescriptor kGlMemory;
extern const HwPathDescriptor kD3D11;
extern const HwPathDescriptor kD3D12;
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

class QHwVideoBuffer;

/// Per-stream GPU path cache, keyed to a caps epoch. Once a sample validated on a path, makeHwVideoBuffer() tries that
/// path's descriptors first; an allocator no path imports skips GPU selection altogether. Reset on set_caps.
struct HwResolvedPathCache
{
    HwVideoBufferPath path = HwVideoBufferPath::None;
    bool validated = false;                     // path resolved by a prior validated sample this epoch
    GType unmatchedAllocator = G_TYPE_INVALID;  // allocator no compiled path imports; its frames go straight to the CPU
    bool validationFailureLogged = false;       // one warning per epoch for buffers that fail plane-handle validation
};

/// Selects the zero-copy QHwVideoBuffer for @p buffer; returns nullptr when no GPU path matches (CPU fallback). The
/// GstSample the buffer holds is only created once a path matches.
std::unique_ptr<QHwVideoBuffer> makeHwVideoBuffer(GstBuffer* buffer, GstCaps* caps, const GstVideoInfo& info,
                                                  QVideoFrameFormat format, const HwVideoBufferContext& context,
                                                  HwVideoBufferPath& matchedPath, HwResolvedPathCache* cache = nullptr);
