#pragma once

#include <QtCore/QString>
#include <cstdint>
#include <functional>
#include <glib.h>
#include <gst/gst.h>

#include "GStreamer.h"  // VideoDecoderOptions

namespace GStreamer {
/// Every pipeline graph QGC writes omits element properties, because source properties can contain credentials.
inline constexpr GstDebugGraphDetails kDiagnosticDotGraphDetails = static_cast<GstDebugGraphDetails>(
    GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS | GST_DEBUG_GRAPH_SHOW_STATES);

bool isValidRtspUri(const gchar* uri_str);

/// Returns @p tag as a file-name part: letters, digits, '-' and '_' only (anything else becomes '_'), at most 40
/// characters, never empty. A dump tag can then neither leave its directory nor make the file name too long.
QString dotFileTag(QStringView tag);

/// Writes @p pipeline's graph (kDiagnosticDotGraphDetails) into @p directory as "<dotFileTag(tag)>-<UTC time>.dot",
/// keeping at most 10 .dot files there. Returns the file's path, or empty on failure.
QString writePipelineDot(GstElement* pipeline, QStringView tag, const QString& directory);

bool isHardwareDecoderFactory(GstElementFactory* factory);

/// ForceVideoDecoder* family that claims decoder factory @p factoryName by name prefix, or
/// ForceVideoDecoderDefault when it belongs to no user-selectable hardware family.
VideoDecoderOptions decoderFamilyForFactory(const char* factoryName);

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
