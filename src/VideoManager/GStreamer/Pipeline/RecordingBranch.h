#pragma once

#include <atomic>
#include <chrono>
#include <optional>

#include <QtCore/QElapsedTimer>
#include <QtCore/QString>
#include <QtCore/QTimer>

#include "BranchContext.h"
#include "GstScoped.h"
#include "RecordingSink.h"
#include "TeeBranchHead.h"
#include "VideoReceiver.h"

/// \brief The recording branch behind the session's tee: queue ! valve [! parser ! muxer ! filesink].
///
/// Records the requested file from the next keyframe once the source streams, asking upstream for that keyframe, and
/// finalizes it within kFinalizeTimeout when the request goes away or the pipeline is torn down. The file starts with
/// the first keyframe the recorder's parser passes on, which is one that comes with its parameter sets; the recording
/// fails when none does within its start timeout of the stream's first keyframe. A request for another file while one
/// records ends that one before the stream's next keyframe, asked for upstream too, which starts the other: no frame
/// between them is lost. MP4/MOV are muxed so a crash leaves the file playable up to the last header update. The
/// branch asks the listener for another file before the header space they reserve runs out, and when the stream's caps
/// change (a new resolution or profile), where the running file ends and the other begins. A failing recorder (disk
/// full, storage too slow to keep up) ends the recording, not the stream: its queue holds kQueueMaxBytes of backlog
/// and then stops taking frames rather than holding up the tee and with it the display. A finalized file is synced
/// to the disk before it is reported closed. A recording that is stopped ends behind the frames its queue still
/// holds, which it has kFinalizeTimeout more to take. A request whose container can't hold the stream's codec (MP4
/// takes neither MJPEG, VP8 nor MPEG-2) isn't started: the branch asks the listener for a file in the container
/// that does (GStreamer::RecordingSink::containerFor()), and the recording fails, naming the codec, when none does.
/// The start timeout, and how soon a muxer's estimate of its header space counts, are GstReceiverTimings's. The bin
/// behind the valve is GStreamer::RecordingSink::create()'s.
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
        /// @p file isn't to take what follows: an MP4 or MOV is about to run out of the header space it reserved, or
        /// the stream's caps changed, and a file holds one format. The recording goes on, without losing a frame, in
        /// the file the listener requests in answer.
        virtual void onRecordingNeedsNewFile(const QString& file) = 0;
        /// @p file wasn't started: its container can't hold the stream's codec, and @p format's can. The recording
        /// goes into the file the listener requests in answer, which is to be in that format, and fails without one.
        virtual void onRecordingNeedsFormat(const QString& file, VideoReceiver::FileFormat format) = 0;

    protected:
        ~Listener() = default;
    };

    struct Request
    {
        QString file;
        VideoReceiver::FileFormat format = VideoReceiver::FileFormat::Mkv;

        friend bool operator==(const Request&, const Request&) = default;
    };

    /// How long a recording may take to finalize, its last sync to the disk included, before it is closed anyway;
    /// covers slow storage. A stopped recording first has as long to take the frames its queue still holds.
    static constexpr std::chrono::seconds kFinalizeTimeout{3};
    /// Backlog the recording queue holds for slow storage before the recording gives up.
    static constexpr guint64 kQueueMaxBytes = guint64{64} * 1024 * 1024;
    /// The recording time an MP4 or MOV file's header must have room for; with less left, the recording asks for
    /// another file. Covers the wait for the keyframe the next file starts with.
    static constexpr std::chrono::seconds kHeaderSpaceMargin{60};

    RecordingBranch(const BranchContext& context, Listener& listener);
    ~RecordingBranch();

    RecordingBranch(const RecordingBranch&) = delete;
    RecordingBranch& operator=(const RecordingBranch&) = delete;

    /// Adds the branch's queue and closed valve behind the tee.
    [[nodiscard]] bool build();
    /// Drops the recording, if any, without reports; the pipeline must be in NULL.
    void reset();

    /// Records @p request from now on, finishing a different one first, at the next keyframe if it has frames; nullopt
    /// stops recording.
    void setRequest(const std::optional<Request>& request);
    /// The first frame reached the tee, so the input caps are known: a waiting request starts.
    void onSourceStreaming();

    /// Before a teardown: drains an open file through EOS. True while its finalization is outstanding, which the caller
    /// waits for by handling the bus until completeTeardown().
    bool beginTeardown();

    bool finalizing() const { return _state == State::Finalizing; }

    /// Closes a file whose finalization is still outstanding by flushing the recorder, and reports it.
    void completeTeardown();

    /// A bus ERROR from the recording branch (GStreamer::PipelineBranch::Recording): the current recorder's fails the
    /// recording, with @p text unless the error is a full disk, which is named instead.
    void onBusError(GstMessage* message, const GError* error, const QString& text, const char* debug);
    /// A bus ELEMENT message: the current recorder's EOS, as the pipeline forwards it, means its file is closed.
    void onBusElement(GstMessage* message);

    GstElement* valve() const { return _head.valve(); }

private:
    enum class State
    {
        Idle,
        Recording,
        Draining,    ///< Stopped: the frames its queue holds go into the file, up to the mark sent in behind them.
        Finalizing,  ///< EOS sent into the recorder, or its start failed; waiting for its EOS or ERROR.
    };

    void _reconcile();
    /// Starts @p request, or asks the listener for a file in the container that holds the stream. By value: the
    /// listener's answer replaces the request this one was read from.
    void _start(Request request);
    /// Asks upstream for a keyframe and holds the stream's next one back until the current file is finished and the
    /// requested one linked, so that it starts the requested one.
    void _switchAtKeyframe();
    /// The keyframe to switch files at is held by hold @p hold: the current file ends before it.
    void _onSwitchKeyframe(quint64 hold);
    /// Lets a held keyframe or change of caps go and stops waiting for a keyframe.
    void _releaseSwitchHold();
    /// Ends a stopped recording behind the frames its queue holds: marks the end there, and lets the file take what
    /// is ahead of the mark before _end().
    void _drain();
    /// The end mark of recording @p recording came out of the queue: its file has every frame queued before the stop.
    void _onDrained(quint64 recording);
    /// Finishes the current recording: drains its file through EOS, or drops it without a frame. @p failure ends the
    /// request with that error once the file is closed.
    void _end(const QString& failure = QString());
    /// Takes the recorder out, reports the file and goes idle. @p error: why the file may be incomplete. @p fileClosed:
    /// the recorder's EOS told that it closed the file, so none of its streaming threads is left inside it.
    void _finish(const QString& error, bool fileClosed = false);
    /// Whether @p source is, or is inside, the current recording's file sink. A message about an earlier recording
    /// can still be queued when the next one runs, and must not end it.
    bool _fromRecorder(GstObject* source) const;
    void _onFileOpened(quint64 recording);
    void _onOverrun(quint64 recording);
    /// The stream's caps changed after recording @p recording's file started, and the valve's output is held at the
    /// change: the file ends before it, and the one the listener requests begins with it.
    void _onCapsChanged(quint64 recording);
    /// Fails a recording whose recorder has had the stream's keyframes for its start timeout without a frame to write.
    void _checkStarted();
    /// Asks the listener for another file once the current one's muxer has little header space left.
    void _checkHeaderSpace();
    void _reportOpened();

    static GstPadProbeReturn _keyframeProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data);
    static GstPadProbeReturn _startProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data);
    static GstPadProbeReturn _capsProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data);
    static GstPadProbeReturn _switchProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data);
    static GstPadProbeReturn _drainProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data);
    static void _onQueueOverrun(GstElement* queue, gpointer data);

    const BranchContext _context;
    Listener& _listener;
    QTimer _finalizeTimer;
    /// Ticks while a file records, to check on its recorder.
    QTimer _watchTimer;

    TeeBranchHead _head;

    State _state = State::Idle;
    bool _streaming = false;
    std::optional<Request> _requested;
    Request _current;
    /// Identifies _current, so reports from an earlier recording's probes are ignored.
    std::atomic<quint64> _recording = 0;
    /// The current recording's sink bin and its parts, from its start until _finish().
    GStreamer::RecordingSink::SinkBin _fileSink;
    /// On the valve's output: lets the stream into the recorder from its first keyframe on.
    GStreamer::PadProbe _keyframeWatch;
    /// On the output of the recorder's parser, or the valve's without one: starts the file with the first keyframe
    /// that gets there.
    GStreamer::PadProbe _startWatch;
    /// On the valve's input: has the valve's output held at a change of the stream's caps once the file started.
    GStreamer::PadProbe _capsWatch;
    /// On the valve's input while Draining: closes the valve at the end mark.
    GStreamer::PadProbe _drainWatch;
    /// The probe that blocks the valve's output at a change of caps, 0 without one; set by the caps probe, and
    /// removed by _releaseSwitchHold() once what follows the file is started, or nothing does.
    std::atomic<gulong> _capsHold = 0;
    /// Blocks the valve's output at a keyframe while _switching, until _finish() has started what follows.
    GStreamer::PadProbe _switchHold;
    bool _switching = false;
    /// Identifies the hold, so a keyframe reported by an earlier one is ignored.
    quint64 _switchHoldId = 0;
    /// When the recorder got the stream's first keyframe (g_get_monotonic_time()), 0 before; set by the keyframe probe.
    std::atomic<gint64> _fedAt = 0;
    /// Set by the start probe once the file got its first frame.
    std::atomic<bool> _fileStarted = false;
    bool _openedReported = false;
    /// Since the current file was reported opened.
    QElapsedTimer _openedFor;
    /// Why the recorder failed, once it did; the request ends with it.
    QString _failure;
};
