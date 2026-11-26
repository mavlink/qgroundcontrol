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
        GstQgcFrameMap.cc
        GstQgcFrameMap.h
        GstQgcAllocation.cc
        GstQgcAllocation.h
        GstQgcSinkBin.cc
        GstQgcSinkBin.h
        GstQgcCaps.cc
        GstQgcCaps.h
        GstQgcFrameDelivery.cc
        GstQgcFrameDelivery.h
        GstQgcGpuBin.cc
        GstQgcGpuBin.h
        GstQgcPathSession.cc
        GstQgcPathSession.h
        GstQgcRenderTarget.cc
        GstQgcRenderTarget.h
        GstQgcSinkCounters.cc
        GstQgcSinkCounters.h
        GstQgcSinkStats.cc
        GstQgcSinkStats.h
        GstQgcVideoFormats.h
)
