#pragma once

#include <QtCore/QString>
#include <cstdint>
#include <functional>
#include <glib.h>
#include <gst/gst.h>

#include "GStreamer.h"  // VideoDecoderOptions

namespace GStreamer {
bool isValidRtspUri(const gchar* uri_str);

/// Dump @p pipeline's graph as a rotating .dot under CacheLocation/qgc-pipeline-dot/ for field reports.
/// Returns empty (no-op) when GST_DEBUG_DUMP_DOT_DIR is set or on I/O failure.
QString writePipelineDot(GstElement* pipeline, const char* tag);

bool isHardwareDecoderFactory(GstElementFactory* factory);

/// Look up @p featureName in @p registry and set its autoplug rank to @p rank, releasing the
/// transfer-full feature ref. Returns false when the registry/name is null or the feature is absent.
bool changeFeatureRank(GstRegistry* registry, const char* featureName, uint16_t rank);

/// Apply decoder rank overrides for @p option to the global GStreamer registry. Defined here
/// alongside the decoder-rank table it drives; re-exported through the GStreamer facade.
void setCodecPriorities(VideoDecoderOptions option);
/// Overload taking the raw forceVideoDecoder setting value; the cast/range-check lives in the impl.
void setCodecPriorities(int rawOption);

/// Walk every plugin in the GStreamer registry, invoking @p visitor for each; frees the GList
/// internally. The plugin pointer is valid only for the duration of the call (don't ref it).
void forEachPlugin(GstRegistry* registry, const std::function<void(GstPlugin*)>& visitor);
}  // namespace GStreamer
