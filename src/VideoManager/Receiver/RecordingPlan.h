#pragma once

#include <chrono>
#include <optional>

#include <QtCore/QString>

#include "VideoReceiver.h"

/// \brief What a receiver was asked to record, across the sessions reconnects start and the hours it runs for.
///
/// Owns VideoReceiver::startRecording()'s file contract for a receiver that records the stream as it arrives: it starts
/// in the file asked for and continues in "<base>_part<N>.<ext>", N counting from 2, after each session that got
/// frames and whenever a part has recorded for kMaxPartDuration. A receiver that encodes the frames anew keeps one
/// part. Where the container asked for can't hold the stream, the files from then on are named for the one that
/// does (setFormat()). Also tells the recording's time and size for VideoStreamStats.
class RecordingPlan
{
public:
    using Clock = std::chrono::steady_clock;

    /// How long a part records before the next one takes over. It bounds what a file holds, for the storage limit to
    /// remove a long recording's oldest parts, and what an MP4 or MOV muxer must reserve header space for.
    static constexpr std::chrono::minutes kMaxPartDuration{60};

    RecordingPlan(const QString& file, VideoReceiver::FileFormat format,
                  std::chrono::milliseconds maxPartDuration = kMaxPartDuration);

    /// "<base><tag>.<ext>" for @p file "<base>.<ext>", and "<file><tag>" for one without an extension.
    static QString insertBeforeExtension(const QString& file, const QString& tag);
    /// "<base>_part<part>.<ext>" for @p file "<base>.<ext>"; @p file itself for part 1.
    static QString partFile(const QString& file, int part);

    VideoReceiver::FileFormat format() const { return _format; }

    /// What is recorded next goes into @p format's container, in files named for it: "<base>.<its extension>", parts
    /// numbered on. A part that got frames keeps its file, and the next one begins.
    void setFormat(VideoReceiver::FileFormat format);

    int part() const { return _part; }

    /// The file the current part records to.
    QString currentFile() const { return partFile(_file, _part); }

    /// The current part got a frame.
    bool opened() const { return _opened; }

    /// @p file got its first frame at @p now. False, changing nothing, unless it is the current part's file.
    bool onFileOpened(const QString& file, Clock::time_point now = Clock::now());
    /// @p file is closed. Counts for the one that opened last only: a file of an earlier recording can close late.
    void onFileClosed(const QString& file);
    /// The current part has recorded for its maximum duration by @p now.
    bool partDue(Clock::time_point now = Clock::now()) const;
    /// What is recorded next goes to the next part, if the current one got frames: after partDue(), and once the
    /// session ended.
    void nextPart();

    /// For VideoStreamStats::recordingMs: since the first part got its first frame; -1 before.
    qint64 recordingMs(Clock::time_point now = Clock::now()) const;
    /// For VideoStreamStats::recordingBytes: what the parts' files held when they closed, and the open one holds now.
    qint64 bytesWritten() const;

private:
    QString _file;
    VideoReceiver::FileFormat _format;
    std::chrono::milliseconds _maxPartDuration;
    int _part = 1;
    bool _opened = false;
    std::optional<Clock::time_point> _startedAt;
    /// When the current part got its first frame; meaningful while _opened.
    Clock::time_point _partStartedAt;
    /// The file that opened last, until it closes: the only one whose size still changes.
    QString _openFile;
    qint64 _closedBytes = 0;
};
