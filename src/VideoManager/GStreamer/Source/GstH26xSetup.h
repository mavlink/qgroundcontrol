#pragma once

#include <gst/gst.h>

/// The H.264/H.265 setup GStreamer::SourceFactory::create() gives the depayloaders and parsers it builds or parsebin
/// plugs.
namespace GStreamer::SourceFactory {

/// When @p element is h264parse or h265parse, repeats the codec parameter sets at every IDR, so a recording started
/// mid-stream can build its codec_data.
void configureH26xParser(GstElement* element);

/// When @p element is rtph264depay or rtph265depay, gates its output on the parameter sets (gateOnParameterSets()) and,
/// if @p requestKeyframes (the source has a return channel, RTSP's RTCP, that carries a keyframe request upstream),
/// asks for a keyframe after packet loss.
void configureH26xRtpDepayloader(GstElement* element, bool requestKeyframes);

/// Whether @p buffer of byte-stream H.264 or H.265 begins an access unit: with a delimiter, a parameter set, an SEI
/// message or the first slice of a picture. False for one that goes on within a frame.
bool startsAccessUnit(GstBuffer* buffer, bool h265);

/// Holds an RTP H.264/H.265 depayloader's byte-stream output back until a full parameter set (SPS and PPS, plus VPS
/// for H.265) has arrived, then emits the parameter sets together with the first slice and removes itself. create()
/// applies it to every depayloader it builds or parsebin plugs; length-prefixed output is left alone.
void gateOnParameterSets(GstPad* depayloaderSrc, bool h265);

}  // namespace GStreamer::SourceFactory
