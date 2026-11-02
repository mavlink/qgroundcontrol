#pragma once

#include <atomic>
#include <chrono>
#include <optional>

#include <QtCore/QString>
#include <QtCore/QTimer>

#include "BranchContext.h"
#include "GstScoped.h"
#include "RecordingFinalizer.h"
#include "TeeBranchHead.h"
#include "VideoReceiver.h"

/// \brief The recording branch behind the session's tee: queue ! valve [! parser ! muxer ! filesink].
///
/// Records the requested file from the next keyframe once the source streams, asking upstream for that keyframe, and
/// finalizes it within kFinalizeTimeout when the request goes away or the pipeline is torn down. MP4/MOV are muxed so a
/// crash leaves the file playable up to the last header update. A failing recorder (disk full, storage too slow to keep
/// up) ends the recording, not the stream: its queue holds kQueueMaxBytes of backlog and then stops taking frames
/// rather than holding up the tee and with it the display.
///
/// Threading: the receiver's worker, except the probes and the queue's signal handler (streaming threads), which hand
/// their work to the worker through BranchContext::post().
class RecordingBranch
{
public:
    class Listener
    {
    public:
        /// @p file got its first frame.
        virtual void onRecordingFileOpened(const QString& file) = 0;
        /// @p file is closed; @p error is empty when it was finalized cleanly.
        virtual void onRecordingFileClosed(const QString& file, const QString& error) = 0;
        /// The recorder failed, so recording to @p file is over; the branch drops the request. @p fileOpened: the file
        /// got frames, and onRecordingFileClosed() already reported it with @p error.
        virtual void onRecordingFailed(const QString& file, const QString& error, bool fileOpened) = 0;

    protected:
        ~Listener() = default;
    };

    struct Request
    {
        QString file;
        VideoReceiver::FileFormat format = VideoReceiver::FileFormat::Mkv;

        friend bool operator==(const Request&, const Request&) = default;
    };

    /// How long a recording may take to finalize before it is closed anyway; covers slow storage.
    static constexpr std::chrono::seconds kFinalizeTimeout{3};
    /// Backlog the recording queue holds for slow storage before the recording gives up.
    static constexpr guint64 kQueueMaxBytes = 64 * 1024 * 1024;

    /// The sink bin a recording links to the valve: an optional parser for @p inputCaps, the muxer for @p format and a
    /// filesink writing @p file. Returns an owned, non-floating ref, or null on failure.
    static GStreamer::GstElementPtr makeFileSink(const QString& file, VideoReceiver::FileFormat format,
                                                 const GstCaps* inputCaps);

    RecordingBranch(const BranchContext& context, Listener& listener);
    ~RecordingBranch();

    RecordingBranch(const RecordingBranch&) = delete;
    RecordingBranch& operator=(const RecordingBranch&) = delete;

    /// Adds the branch's queue and closed valve behind the tee.
    [[nodiscard]] bool build();
    /// Drops the recording, if any, without reports; the pipeline must be in NULL.
    void reset();

    /// Records @p request from now on, finishing a different one first; nullopt stops recording.
    void setRequest(const std::optional<Request>& request);
    /// The first frame reached the tee, so the input caps are known: a waiting request starts.
    void onSourceStreaming();

    /// Before a teardown: drains an open file through EOS. True while its finalization is outstanding, which the caller
    /// waits for by handling the bus until completeTeardown().
    bool beginTeardown();

    bool finalizing() const { return _state == State::Finalizing; }

    /// Closes a file whose finalization is still outstanding by flushing the recorder, and reports it.
    void completeTeardown();

    /// A bus ERROR from the recording branch (GStreamer::PipelineBranch::Recording).
    void onBusError(GstMessage* message, const QString& text, const char* debug);
    /// A bus ELEMENT message: the recorder's EOS, as the pipeline forwards it, means its file is closed.
    void onBusElement(GstMessage* message);

    GstElement* valve() const { return _head.valve(); }

private:
    enum class State
    {
        Idle,
        Recording,
        Finalizing,  ///< EOS sent into the recorder, or its start failed; waiting for its EOS or ERROR.
    };

    void _reconcile();
    void _start(const Request& request);
    /// Finishes the current recording: drains its file through EOS, or drops it without a frame. @p failure ends the
    /// request with that error once the file is closed.
    void _end(const QString& failure = QString());
    /// Takes the recorder out, reports the file and goes idle. @p error: why the file may be incomplete.
    void _finish(const QString& error);
    void _onFileOpened(quint64 recording);
    void _onOverrun(quint64 recording);
    void _reportOpened();

    static GstPadProbeReturn _keyframeProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data);
    static void _onQueueOverrun(GstElement* queue, gpointer data);

    const BranchContext _context;
    Listener& _listener;
    QTimer _finalizeTimer;

    TeeBranchHead _head;

    State _state = State::Idle;
    bool _streaming = false;
    std::optional<Request> _requested;
    Request _current;
    /// Identifies _current, so reports from an earlier recording's probes are ignored.
    std::atomic<quint64> _recording = 0;
    GStreamer::GstElementPtr _fileSink;
    RecordingFinalizer _finalizer;
    GStreamer::PadProbe _keyframeWatch;
    /// Set by the keyframe probe once the file got its first frame.
    std::atomic<bool> _fileStarted = false;
    bool _openedReported = false;
    /// Why the recorder failed, once it did; the request ends with it.
    QString _failure;
};
