#pragma once

#include <cstdint>
#include <functional>
#include <glib.h>
#include <gst/gst.h>

#include <QtCore/QString>

#include "GStreamer.h"  // VideoDecoderOptions
#include "GstScoped.h"

namespace GStreamer {
bool isValidRtspUri(const gchar* uri_str);

/// Takes @p element out of its parent bin, then sets it to NULL. Removed first, so the bin can't bring it back up and
/// nothing it posts while stopping reaches the bin's bus. No-op when @p element has no parent. The caller's refs on
/// @p element, if any, stay valid.
void disposeChild(GstElement* element);

/// Adds @p element to @p bin. gst_bin_add consumes a floating element even when it fails, so ownership always moves.
/// Returns the borrowed (bin-owned) element, or null if the bin rejected it; @p what names it in the warning.
GstElement* addToBin(GstElement* bin, GstElementPtr element, const char* what);

/// The head of a branch behind @p tee: a queue and a closed valve (drop=TRUE), added to @p pipeline and linked
/// tee ! queue ! valve. Both are null on failure, which is logged with @p what naming the branch.
struct TeeBranch
{
    GstElement* queue = nullptr;
    GstElement* valve = nullptr;
};

TeeBranch addTeeBranch(GstElement* pipeline, GstElement* tee, const char* what);

/// @p element's first src pad, or null when it has none (yet).
GstPadPtr firstSrcPad(GstElement* element);

bool isHardwareDecoderFactory(GstElementFactory* factory);

/// "[HW|SW] <plugin>/<factory> rank=<rank> (<klass>)" for decoder logs.
QString describeDecoderFactory(GstElementFactory* factory);

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

/// Invokes @p visitor for every video decoder factory of at least @p minRank in the default registry. The factory is
/// valid only for the duration of the call.
void forEachVideoDecoderFactory(GstRank minRank, const std::function<void(GstElementFactory*)>& visitor);
}  // namespace GStreamer
