#pragma once

#include <cstdint>
#include <functional>
#include <gst/gst.h>

#include <QtCore/QList>
#include <QtCore/QString>

#include "GStreamer.h"  // VideoDecoderOptions

// Video decoder classification and the autoplug rank overrides behind the forceVideoDecoder setting. Every rank change
// goes through here, so a feature named in GST_PLUGIN_FEATURE_RANK keeps the user's rank.
namespace GStreamer {

/// Invokes @p visitor for every video decoder factory of at least @p minRank in the default registry. The factory is
/// valid only for the duration of the call.
void forEachVideoDecoderFactory(GstRank minRank, const std::function<void(GstElementFactory*)>& visitor);

bool isHardwareDecoderFactory(GstElementFactory* factory);

/// "[HW|SW] <plugin>/<factory> rank=<rank> (<klass>)" for decoder logs.
QString describeDecoderFactory(GstElementFactory* factory);

/// ForceVideoDecoder* family that claims decoder factory @p factoryName by name prefix, or
/// ForceVideoDecoderDefault when it belongs to no user-selectable hardware family.
VideoDecoderOptions decoderFamilyForFactory(const char* factoryName);

/// Hardware decoder families currently present in the GStreamer registry, as VideoDecoderOptions
/// values (always omits ForceVideoDecoderDefault/Software). Lets the settings layer validate a
/// persisted ForceVideoDecoder* choice against what the running build can actually provide. Vulkan is
/// only offered with the opt-in Vulkan GPU path.
QList<VideoDecoderOptions> availableDecoderFamilies();

/// Look up @p featureName in @p registry and set its autoplug rank to @p rank, releasing the
/// transfer-full feature ref. Returns false when the registry/name is null or the feature is absent.
/// A feature named in GST_PLUGIN_FEATURE_RANK keeps the user's rank, here and in setCodecPriorities().
bool changeFeatureRank(GstRegistry* registry, const char* featureName, uint16_t rank);

/// Apply decoder rank overrides for @p option to the global GStreamer registry, starting from the ranks the features
/// had before QGC first changed them, so a new option replaces the previous one rather than adding to it. GUI thread.
void setCodecPriorities(VideoDecoderOptions option);
/// Overload taking the raw forceVideoDecoder setting value; the cast/range-check lives in the impl.
void setCodecPriorities(int rawOption);

// Diagnostic dump of installed video-decoder factories, sorted by rank, with
// HW/SW classification. No-op once the registry is empty.
void logDecoderRanks();

}  // namespace GStreamer
