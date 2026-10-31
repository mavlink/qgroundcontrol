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

/// As disposeChild(), but sends a FLUSH_START into @p element's sink pads before setting it to NULL. The flush wakes
/// streaming threads parked inside @p element, which going NULL would otherwise wait on.
void flushAndDisposeChild(GstElement* element);

/// Adds @p element, which must not be floating (see makeElement()), to @p bin, which takes its own ref; ours is
/// dropped. Returns the borrowed (bin-owned) element, or null if the bin rejected it; @p what names it in the warning.
GstElement* addToBin(GstElement* bin, GstElementPtr element, const char* what);

/// The head of a branch behind @p tee: a queue and a closed valve (drop=TRUE), added to @p pipeline and linked
/// tee ! queue ! valve. Both are null on failure, which is logged with @p what naming the branch.
struct TeeBranch
{
    GstElement* queue = nullptr;
    GstElement* valve = nullptr;
};

TeeBranch addTeeBranch(GstElement* pipeline, GstElement* tee, const char* what);

/// Asks the encoder upstream of @p sinkPad for a keyframe with its parameter sets (on RTSP, rtpbin turns this into an
/// RTCP PLI/FIR). Doesn't block: the event is not serialized.
void requestKeyframe(GstPad* sinkPad);

/// Sends @p event into @p pad from GStreamer's thread pool, so a serialized event waiting for a busy streaming thread
/// never blocks the caller. Takes @p event; the pad is kept alive until the send returns.
void sendEventAsync(GstPad* pad, GstEvent* event);

/// @p element's first src pad, or null when it has none (yet).
GstPadPtr firstSrcPad(GstElement* element);

/// For a pad a streaming thread just added: blocks its data, since a push on an unlinked pad would fail the stream,
/// and returns the task for the thread that changes the pipeline, which calls @p link with the pad and unblocks it.
std::function<void()> holdPadUntilLinked(GstPad* pad, std::function<void(GstPad* pad)> link);

bool isHardwareDecoderFactory(GstElementFactory* factory);

/// "[HW|SW] <plugin>/<factory> rank=<rank> (<klass>)" for decoder logs.
QString describeDecoderFactory(GstElementFactory* factory);

/// ForceVideoDecoder* family that claims decoder factory @p factoryName by name prefix, or
/// ForceVideoDecoderDefault when it belongs to no user-selectable hardware family.
VideoDecoderOptions decoderFamilyForFactory(const char* factoryName);

/// Look up @p featureName in @p registry and set its autoplug rank to @p rank, releasing the
/// transfer-full feature ref. Returns false when the registry/name is null or the feature is absent.
/// A feature named in GST_PLUGIN_FEATURE_RANK keeps the user's rank, here and in setCodecPriorities().
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
