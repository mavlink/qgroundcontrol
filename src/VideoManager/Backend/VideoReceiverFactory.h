#pragma once

class QObject;
class VideoReceiver;

namespace VideoSourceResolver {
struct VideoSource;
}

/// Backend-dispatch factories for producing a VideoReceiver suited to the
/// given source. `gstOrQtMultimedia()` picks GStreamer when the backend is
/// registered and the URI's scheme prefers it, otherwise falls through to the
/// Qt Multimedia receiver. `uvc()` unconditionally returns a UVCReceiver.
///
/// Both functions match `VideoStream::ReceiverFactory`'s signature and are
/// intended to be plugged directly into `VideoStream` construction.
namespace VideoReceiverFactory {

VideoReceiver* forSource(const VideoSourceResolver::VideoSource& source, bool thermal, QObject* parent);

VideoReceiver* gstOrQtMultimedia(const VideoSourceResolver::VideoSource& source, bool thermal, QObject* parent);

VideoReceiver* uvc(const VideoSourceResolver::VideoSource& source, bool thermal, QObject* parent);

}  // namespace VideoReceiverFactory
