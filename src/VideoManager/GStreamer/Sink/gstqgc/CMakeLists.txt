# gstqgc: QGC's video sink (qgcqvideosink, created from its GType, not registered as a plugin), the sink bin built
# around it and the zero-copy frame mapping. Sources compile into QGCGStreamer.
target_sources(
    QGCGStreamer
    PRIVATE
        gstqgcqvideosink.cc
        gstqgcqvideosink.h
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
