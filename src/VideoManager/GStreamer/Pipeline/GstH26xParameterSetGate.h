#pragma once

#include <gst/gstelement.h>

/// The H.264/H.265 setup the source factory gives the depayloaders and parsers it builds or parsebin plugs.
/// gateOnParameterSets() (GstSourceFactory.h) is defined here too.
namespace GStreamer::SourceFactory {

/// When @p element is h264parse or h265parse, repeats the codec parameter sets at every IDR, so a recording started
/// mid-stream can build its codec_data.
void configureH26xParser(GstElement* element);

/// When @p element is rtph264depay or rtph265depay, gates its output on the parameter sets (gateOnParameterSets()) and,
/// if @p requestKeyframes (the source has a return channel, RTSP's RTCP, that carries a keyframe request upstream),
/// asks for a keyframe after packet loss.
void configureH26xRtpDepayloader(GstElement* element, bool requestKeyframes);

}  // namespace GStreamer::SourceFactory
