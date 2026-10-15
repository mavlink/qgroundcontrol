#pragma once

#include <gst/gst.h>

namespace GstQgc {

/// Populate an ALLOCATION query with qgcqvideosink's consumed metas and, when needed, a min-buffer pool hint.
/// Called from the sink's propose_allocation vmethod.
void populateAllocationQuery(GstQuery* query);

}  // namespace GstQgc
