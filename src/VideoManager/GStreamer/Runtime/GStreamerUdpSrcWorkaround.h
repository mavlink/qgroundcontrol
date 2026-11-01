#pragma once

namespace GStreamer {

/// Before gst-plugins-good 1.24.10, udpsrc's unlock_stop() replaces its GCancellable on the streaming thread (basesrc
/// defers it to the first create() after PLAYING) while unlock() cancels it from the thread leaving PLAYING. A pipeline
/// stopped right after it started then cancels a freed GCancellable and corrupts the heap. On those versions this wraps
/// the two udpsrc class methods to serialize them, as the upstream fix does (gstreamer!7897); on newer ones, or
/// without udpsrc, it does nothing. Runs once per process, after gst_init().
void serializeUdpSrcUnlock();

/// Whether serializeUdpSrcUnlock() patched udpsrc.
bool udpSrcUnlockSerialized();

}  // namespace GStreamer
