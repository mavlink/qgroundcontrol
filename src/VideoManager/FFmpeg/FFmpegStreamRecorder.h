#pragma once

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QtTypes>

#include "FFmpegPointers.h"
#include "VideoReceiver.h"

/// Records an FFmpegVideoReceiver's stream as it arrives: its video packets, copied without re-encoding into Matroska,
/// MP4 or MOV with libavformat's muxers, one file at a time, each from a keyframe on, with the parameter sets the
/// container needs taken from the stream. MP4 and MOV are fragmented at each keyframe (movflags frag_keyframe and
/// empty_moov): a file cut short by a crash keeps every fragment but the last. A container that can't hold the
/// stream's codec is reported, for the receiver to ask for Matroska.
///
/// The receiver says what to record (record(), stop()); it follows its RecordingPlan. A file of the same session is
/// replaced at the session's next keyframe, so a recording continues in its next part without a gap; a file of another
/// session is finished at once.
///
/// Threading: record() and stop() on the receiver's thread, write() on a session's thread; a thread of its own writes
/// the files and calls the Listener. The destructor finishes the open file and waits for that thread.
class FFmpegStreamRecorder
{
public:
    /// The stream a session's packets belong to: its codec parameters and time base. Shared by the session that
    /// created it and the packets it hands over; never changed.
    struct Stream
    {
        FFmpegPointers::CodecParameters parameters;
        int timeBaseNum = 1;
        int timeBaseDen = 90000;
    };

    /// Called on the recorder's thread.
    class Listener
    {
    public:
        virtual ~Listener() = default;
        /// @p file got its first packet.
        virtual void onRecordingFileOpened(const QString& file) = 0;
        /// @p file was finished; @p error is empty when it was written to its end.
        virtual void onRecordingFileClosed(const QString& file, const QString& error) = 0;
        /// @p file wasn't opened: @p format is a container that holds the stream's codec.
        virtual void onRecordingNeedsFormat(const QString& file, VideoReceiver::FileFormat format) = 0;
        /// @p file couldn't be opened; nothing more is recorded until record() is called again.
        virtual void onRecordingFailed(const QString& file, const QString& error) = 0;
    };

    /// How many packets wait for the disk at most: 30 s of a 30 fps stream. Beyond, they are dropped up to the
    /// next keyframe.
    static constexpr qsizetype kMaxQueuedPackets = 900;

    explicit FFmpegStreamRecorder(Listener& listener);
    ~FFmpegStreamRecorder();

    FFmpegStreamRecorder(const FFmpegStreamRecorder&) = delete;
    FFmpegStreamRecorder& operator=(const FFmpegStreamRecorder&) = delete;

    /// Records the packets of session @p session to @p file in @p format's container, from the session's next
    /// keyframe; 0 for none yet. Finishes the file recorded so far: at that keyframe for one of the same session, at
    /// once for another's.
    void record(const QString& file, VideoReceiver::FileFormat format, quint64 session);
    /// Finishes the open file at once, and records nothing more.
    void stop();
    /// A video packet of session @p session's @p stream, which it copies when it is recording that session.
    void write(quint64 session, const std::shared_ptr<const Stream>& stream, const AVPacket& packet);

private:
    struct Target
    {
        QString file;
        VideoReceiver::FileFormat format = VideoReceiver::FileFormat::Mkv;
        quint64 session = 0;
    };

    struct Queued
    {
        quint64 session = 0;
        std::shared_ptr<const Stream> stream;
        FFmpegPointers::Packet packet;
    };

    void _run();
    /// Follows a new target: finishes the open file at once unless it continues in the same session.
    void _retarget(const std::optional<Target>& target);
    void _writeQueued(Queued& queued);
    bool _open(const Target& target, const Stream& stream, const AVPacket& keyframe);
    /// Finishes the open file and reports it with @p error.
    void _close(const QString& error = {});

    Listener& _listener;
    const QByteArray _comment;
    std::thread _thread;

    std::mutex _mutex;
    std::condition_variable _wake;
    // Under _mutex.
    std::optional<Target> _target;
    quint64 _targetVersion = 0;
    std::deque<Queued> _queue;
    /// The queue overflowed: packets are dropped up to the next keyframe.
    bool _awaitingKeyframe = false;
    bool _quit = false;

    // The recorder's thread only.
    std::optional<Target> _currentTarget;
    quint64 _currentTargetVersion = 0;
    /// The target's file was refused (wrong container, or it failed): nothing is recorded until the next target.
    bool _targetRefused = false;
    FFmpegPointers::Output _output;
    QString _openFile;
    quint64 _openSession = 0;
    int _inTimeBaseNum = 1;
    int _inTimeBaseDen = 90000;
    int64_t _firstDts = 0;
    int64_t _lastDts = 0;
    bool _anyWritten = false;
};
