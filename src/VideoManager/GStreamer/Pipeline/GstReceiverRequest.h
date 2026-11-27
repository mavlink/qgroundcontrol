#pragma once

#include <memory>
#include <optional>

#include <QtCore/QMetaType>
#include <QtCore/QString>

#include "VideoReceiver.h"
#include "VideoStreamStats.h"

namespace GstQgc {
struct SinkBin;
}

struct GstRecordingRequest
{
    QString file;
    VideoReceiver::FileFormat format = VideoReceiver::FileFormat::Mkv;
    /// Differs for each startRecording(), so asking for the same file again starts a new recording.
    quint64 id = 0;
};

/// Everything GstVideoReceiver asks of its GstReceiverWorker, handed whole to the worker's thread after each request.
struct GstReceiverRequest
{
    bool active = false;
    QString name;
    VideoReceiver::Config config;
    std::shared_ptr<const GstQgc::SinkBin> videoSink;
    /// Counts restart() calls: a new count restarts the stream, with videoSink.
    quint64 restarts = 0;
    /// Counts reconnectNow() calls: a new count cuts a reconnect's backoff short.
    quint64 reconnects = 0;
    std::optional<GstRecordingRequest> recording;
};

/// What GstReceiverWorker reports, whole, to the receiver's GUI thread after each change.
struct GstReceiverStatus
{
    VideoReceiver::State state = VideoReceiver::State::Stopped;
    bool decoding = false;
    QString decoderName;
    bool recording = false;
    /// About once a second while streaming, and invalid once the session stops.
    VideoStreamStats stats;
};

Q_DECLARE_METATYPE(GstReceiverStatus)
