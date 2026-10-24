#pragma once

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>

#include <QtCore/QtTypes>

#include "gstqgc/gstqgcqvideosink.h"

/// Readers of qgcqvideosink's counters shared by the gstqgc test classes.
namespace GstQgcTest {

inline guint64 framesDelivered(GstElement* qvideosink)
{
    guint64 delivered = 0;
    g_object_get(qvideosink, "frames-delivered", &delivered, nullptr);
    return delivered;
}

/// A guint64 field of the sink's "frame-stats" structure; -1 if the field is missing.
inline qint64 sinkStat(GstElement* qvideosink, const char* field)
{
    const GStreamer::GstStructurePtr stats = GstQgc::frameStats(qvideosink);
    guint64 value = 0;
    const bool found = stats && gst_structure_get_uint64(stats.get(), field, &value);
    return found ? static_cast<qint64>(value) : -1;
}

}  // namespace GstQgcTest

#endif
