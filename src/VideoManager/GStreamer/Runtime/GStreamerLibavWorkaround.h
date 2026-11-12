#pragma once

#include <gst/gst.h>

#include <QtCore/QLatin1StringView>

namespace GStreamer {

/// The version of the libavcodec this build's libav plugin decodes with, from its pkg-config file at configure time;
/// empty when the file was not there.
QLatin1StringView libavcodecVersion();

/// FFmpeg 7.1.0 (libavcodec 61.19.100, which GStreamer 1.28 packages for Android, iOS, macOS and Windows) decodes the
/// wavefront rows of an HEVC picture on slice threads that wait for one another through condition variables indexed
/// by thread, not by row. With more rows than threads a row can wait forever, and avdec_h265's streaming thread with
/// it, so that the decoder can be neither stopped nor replaced. FFmpeg 7.1.1 has the fix. libav decodes on slice
/// threads behind a live source, and where decodeWithoutDelay() asks for them. With that @p version this has
/// @p decoder, when it is avdec_h265, decode on one thread, which takes the rows in order. Returns whether it changed
/// the decoder.
bool avoidHevcWavefrontDeadlock(GstElement* decoder, QLatin1StringView version = libavcodecVersion());

}  // namespace GStreamer
