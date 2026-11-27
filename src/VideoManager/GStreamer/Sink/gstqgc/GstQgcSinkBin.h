#pragma once

#include <gst/gst.h>
#include <optional>

#include <QtCore/QByteArray>

#include "GstQgcSinkPolicy.h"
#include "GstScoped.h"

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
    /// Stands in for the process's environment (currentSinkEnvironment()), which the bin reads as it is built and its
    /// sink follows from then on. For tests, which have no scene graph to render with.
    std::optional<SinkEnvironment> environment = {};
};

/// A sink bin makeSinkBin() built, and the qgcqvideosink in it: a reference to each, neither floating.
struct SinkBin
{
    GStreamer::GstElementPtr bin;
    GStreamer::GstElementPtr qvideosink;

    explicit operator bool() const { return bin != nullptr; }
};

/// A new bin of [converter ! (GPU upload !) qgcqvideosink] behind a "sink" ghost pad, with its qgcqvideosink.
/// qgcqvideosink's get_caps decides what negotiates: the formats Qt renders, the GPU memory the render target imports,
/// and square pixels unless disablePar. Empty, with the reason logged, when an element is missing or the chain doesn't
/// link.
SinkBin makeSinkBin(const SinkBinConfig& config);

}  // namespace GstQgc
