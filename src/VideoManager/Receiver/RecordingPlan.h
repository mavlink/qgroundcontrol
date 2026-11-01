#pragma once

#include <chrono>
#include <optional>

#include <QtCore/QString>

#include "VideoReceiver.h"

/// \brief What a receiver was asked to record, across the sessions reconnects start.
///
/// Owns VideoReceiver::startRecording()'s file contract for a receiver that records the stream as it arrives: the first
/// session records to the file asked for, and each later one, once the part before got frames, to
/// "<base>_part<N>.<ext>", N counting from 2. A receiver that encodes the frames anew keeps one part. Also tells the
/// recording's time and size for VideoStreamStats.
class RecordingPlan
{
public:
    using Clock = std::chrono::steady_clock;

    RecordingPlan(const QString& file, VideoReceiver::FileFormat format);

    /// "<base>_part<part>.<ext>" for @p file "<base>.<ext>"; @p file itself for part 1.
    static QString partFile(const QString& file, int part);

    const QString& file() const { return _file; }

    VideoReceiver::FileFormat format() const { return _format; }

    int part() const { return _part; }

    /// The file the current part records to.
    QString currentFile() const { return partFile(_file, _part); }

    /// The current part got a frame.
    bool opened() const { return _opened; }

    /// @p file got its first frame at @p now. False, changing nothing, unless it is the current part's file.
    bool onFileOpened(const QString& file, Clock::time_point now = Clock::now());
    /// The session ended: the next one records the next part, if the current one got frames.
    void onSessionEnded();

    /// For VideoStreamStats::recordingMs: since the first part got its first frame; -1 before.
    qint64 recordingMs(Clock::time_point now = Clock::now()) const;
    /// For VideoStreamStats::recordingBytes: the size of the parts' files so far.
    qint64 bytesWritten() const;

private:
    QString _file;
    VideoReceiver::FileFormat _format;
    int _part = 1;
    bool _opened = false;
    std::optional<Clock::time_point> _startedAt;
};
