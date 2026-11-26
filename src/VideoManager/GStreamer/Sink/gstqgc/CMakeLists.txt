# gstqgc: the QGC GStreamer plugin (qgcqvideosink), the sink bin built around it and the zero-copy frame mapping.
# Sources compile into QGCGStreamer; GStreamer.cc strong-refs gst_plugin_qgc_register via
# GST_PLUGIN_STATIC_DECLARE/REGISTER(qgc), pulling the member from the same archive.
target_sources(
    QGCGStreamer
    PRIVATE
        gstqgc.cc
        gstqgcelements.h
        gstqgcqvideosink.cc
        gstqgcqvideosink.h
        CpuVideoFramePool.cc
        CpuVideoFramePool.h
        GstQgcFrameMap.cc
        GstQgcFrameMap.h
        GstQgcSinkBin.cc
        GstQgcSinkBin.h
        GstQgcFrameDelivery.cc
        GstQgcFrameDelivery.h
        GstQgcSinkPolicy.cc
        GstQgcSinkPolicy.h
        GstQgcSinkStats.cc
        GstQgcSinkStats.h
        GstQgcVideoFormats.cc
        GstQgcVideoFormats.h
)
