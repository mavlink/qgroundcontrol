#pragma once

#include <QtMultimedia/QMediaMetaData>
#include <gst/gststructure.h>

/// Utility: build a QMediaMetaData from a GstStructure* (from negotiated caps).
/// Extracts Resolution, VideoFrameRate, and VideoCodec where available.
/// Returns a default-constructed (empty) QMediaMetaData on null/empty input.
QMediaMetaData gstStructureToMediaMetaData(const GstStructure* structure);
