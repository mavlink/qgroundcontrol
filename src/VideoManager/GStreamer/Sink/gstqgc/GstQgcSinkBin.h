#pragma once

#include <gst/gst.h>

#include <QtCore/QByteArray>

namespace GstQgc {

/// What makeSinkBin() builds a sink bin with: fixed once the bin is built.
struct SinkBinConfig
{
    /// Takes the GPU memory Qt's renderer imports, through the GPU variant that suits the renderer, beside system
    /// memory.
    bool gpuZeroCopy = false;
    /// Factory of the converter at the head of the bin. Empty: a SoC's own converter where there is one
    /// (imxvideoconvert_g2d, nvvidconv), else videoconvertscale.
    QByteArray conversionElement = {};
    /// Lets the sink take non-square pixels instead of having the converter scale them to pixel-aspect-ratio=1/1: for
    /// a picture shown stretched to a forced ratio, and for v4l2 drivers without VIDIOC_CROPCAP, which deadlock when
    /// the ratio is forced.
    bool disablePar = false;
};

/// A new bin, floating, of [converter ! (GPU upload !) qgcqvideosink] behind a "sink" ghost pad. qgcqvideosink's
/// get_caps decides what negotiates: the formats Qt renders, the GPU memory the render target imports, and square
/// pixels unless disablePar. Null, with the reason logged, when an element is missing or the chain doesn't link.
GstElement* makeSinkBin(const SinkBinConfig& config);

/// The qgcqvideosink of @p sinkBin, a bin makeSinkBin() built: a new reference, or null for anything else.
GstElement* sinkBinVideoSink(GstElement* sinkBin);

}  // namespace GstQgc
