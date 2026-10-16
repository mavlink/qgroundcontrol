# gstqgc: the QGC GStreamer plugin (qgcvideosinkbin + qvideosink + zero-copy frame mapping).
# Sources compile into QGCGStreamer; GStreamer.cc strong-refs gst_plugin_qgc_register via
# GST_PLUGIN_STATIC_DECLARE/REGISTER(qgc), pulling the member from the same archive.
target_sources(
    QGCGStreamer
    PRIVATE gstqgc.cc
            gstqgcelements.cc
            gstqgcelements.h
            gstqgcvideosinkbin.cc
            gstqgcvideosinkbin.h
            gstqgcqvideosink.cc
            gstqgcqvideosink.h
            GstQgcFrameMap.cc
            GstQgcFrameMap.h
            GstQgcAllocation.cc
            GstQgcAllocation.h
            GstQgcBinChain.h
            GstQgcCaps.cc
            GstQgcCaps.h
            GstQgcVideoFormats.h
)
