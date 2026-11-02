#pragma once

#include <memory>
#include <optional>

#include <QtCore/QMetaType>
#include <QtCore/QString>

#include "VideoReceiver.h"
#include "VideoStreamStats.h"

typedef struct _GstElement GstElement;

// What GstVideoReceiver and its GstReceiverWorker pass between their threads: the receiver hands over the whole
// GstReceiverRequest after each of its requests, and the worker reports its whole GstReceiverStatus after each change.

struct GstRecordingRequest
{
    QString file;
    VideoReceiver::FileFormat format = VideoReceiver::FileFormat::Mkv;
    /// Differs for each startRecording(), so asking for the same file again starts a new recording.
    quint64 id = 0;
};

/// Everything the receiver asks for, as it stands after each of its requests.
struct GstReceiverRequest
{
    bool active = false;
    QString name;
    VideoReceiver::Config config;
    std::shared_ptr<GstElement> videoSink;
    /// Counts restart() calls: a new count restarts the stream, with videoSink.
    quint64 restarts = 0;
    std::optional<GstRecordingRequest> recording;
};

/// What the worker reports, as it stands after each change.
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
