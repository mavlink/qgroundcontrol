#pragma once

#include <gst/gst.h>

namespace GstQgc {

/// Populate an ALLOCATION query with qgcqvideosink's consumed metas and its pool entry: the buffers the sink holds at
/// once, with a pool when the query needs one. False, with the query untouched, when it has no caps that describe
/// video. Called from the sink's propose_allocation vmethod.
[[nodiscard]] bool populateAllocationQuery(GstQuery* query);

}  // namespace GstQgc
