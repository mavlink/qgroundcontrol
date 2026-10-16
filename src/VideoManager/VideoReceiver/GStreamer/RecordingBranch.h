#pragma once

#include <chrono>

#include <QtCore/QString>

#include "GstScoped.h"
#include "RecordingFinalizer.h"
#include "VideoReceiver.h"

class GstVideoReceiver;
class QObject;
class QTimer;

/// \brief The recording branch behind GstVideoReceiver's tee: queue ! valve [! parser ! splitmuxsink].
///
/// Builds a file sink per recording, starts the file at the first keyframe, and finalizes it on stop within
/// kFinalizeTimeout. A failing recorder ends the recording, not the stream. Reports through the receiver's signals.
///
/// Threading: the onBus*() methods run on whichever thread posts, the keyframe probe on the streaming thread, and
/// everything else on the receiver's worker.
class RecordingBranch
{
public:
    /// How long a recording may take to finalize before it is closed anyway; covers slow storage.
    static constexpr std::chrono::seconds kFinalizeTimeout{3};

    RecordingBranch(GstVideoReceiver& receiver, QObject* workerContext);

    /// The sink bin a recording links to the valve: an optional parser for @p inputCaps, then splitmuxsink writing
    /// @p videoFile. Returns an owned, non-floating ref (adding the bin to a pipeline takes another), or null on
    /// failure.
    static GStreamer::GstElementPtr makeFileSink(const QString& videoFile, VideoReceiver::FILE_FORMAT format,
                                                 const GstCaps* inputCaps);

    /// Adds the branch's queue and closed valve to @p pipeline behind @p tee.
    [[nodiscard]] bool build(GstElement* pipeline, GstElement* tee);
    /// Drops the recording, if any, and forgets the pipeline, which must be in NULL with its callbacks stopped.
    void reset();

    void start(const QString& videoFile, VideoReceiver::FILE_FORMAT format);
    void stop();
    /// Before the pipeline is torn down: drains an active recording into its file, then waits up to
    /// kFinalizeTimeout for the file (or one stop() is finalizing) to be closed.
    void finishForTeardown();

    /// Pipeline EOS: every branch drained, the recorder included.
    void onBusEos();
    void onBusElement(GstMessage* message);
    /// Returns whether the error came from the recording.
    bool onBusError(GstMessage* message, const QString& text);

private:
    enum class State
    {
        Idle,
        Recording,
        Stopping,  ///< stop() sent EOS into the branch; waiting for the file to be finalized
    };

    /// Unlinks the branch from the valve and sends EOS into it.
    bool _unlinkBranch();
    void _shutdown();
    void _onFinalized();
    void _onRecorderError();
    void _onStopTimeout();

    GstVideoReceiver& _receiver;
    /// Bounds stop(): completes it when the file isn't reported finalized in time.
    QTimer* _stopTimer = nullptr;
    // Borrowed: the receiver owns the pipeline, the pipeline the tee and valve.
    GstElement* _pipeline = nullptr;
    GstElement* _tee = nullptr;
    /// Closed by the bus handler on a recorder error; fixed while the handler is attached.
    GstElement* _valve = nullptr;
    /// Ours, besides the pipeline's while it is inside.
    GStreamer::GstElementPtr _fileSink;
    State _state = State::Idle;
    GStreamer::PadProbe _keyframeWatch;
    RecordingFinalizer _finalizer;
};
