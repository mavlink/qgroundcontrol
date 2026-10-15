#pragma once

#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <memory>

#include <QtCore/qglobal.h>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstHwTypes.h"

class QHwVideoBuffer;

/// Per-stream GPU path cache, keyed to a caps epoch. Lets makeHwVideoBuffer() skip the predicate ladder once a path has
/// been resolved by a validated sample, and skip GPU selection for an allocator no path imports. Reset on set_caps.
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

/// As above, for a buffer already wrapped in @p sample.
std::unique_ptr<QHwVideoBuffer> makeHwVideoBuffer(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                                                  const HwVideoBufferContext& context, HwVideoBufferPath& matchedPath,
                                                  HwResolvedPathCache* cache = nullptr);
