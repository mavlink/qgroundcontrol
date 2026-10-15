#pragma once

#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <memory>

#include <QtCore/qglobal.h>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstHwTypes.h"

class QHwVideoBuffer;

/// Per-stream GPU path cache, keyed to a caps epoch. Lets makeHwVideoBuffer() skip the predicate ladder once a path has
/// been resolved by a validated sample. Reset on set_caps.
struct HwResolvedPathCache
{
    HwVideoBufferPath path = HwVideoBufferPath::None;
    bool validated = false;        // path resolved by a prior validated sample this epoch
};

/// Selects the zero-copy QHwVideoBuffer for a GstSample; returns nullptr when no GPU path matches (CPU memcpy
/// fallback).
std::unique_ptr<QHwVideoBuffer> makeHwVideoBuffer(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                                                  const HwVideoBufferContext& context, HwVideoBufferPath& matchedPath,
                                                  HwResolvedPathCache* cache = nullptr);
