#pragma once

#include <gst/gst.h>

namespace GstQgc {

/// Downstream query probe for qgcqvideosink's sink pad. Answers CONTEXT queries synchronously,
/// so negotiation terminates at the sink instead of racing NEED_CONTEXT bus fallback.
GstPadProbeReturn videosinkQueryProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);

/// Populate an ALLOCATION query with qgcqvideosink's consumed metas and, when needed, a min-buffer pool hint.
/// Called from the sink's propose_allocation vmethod.
void populateAllocationQuery(GstQuery* query);

}  // namespace GstQgc
