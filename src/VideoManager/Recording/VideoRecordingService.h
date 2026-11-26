#pragma once

#include <functional>
#include <optional>

#include <QtCore/QFuture>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#include "VideoReceiver.h"

class QDateTime;
class SubtitleWriter;
class VideoStreamController;

/// Records the video sources of the main picture: the primary stream or the USB camera, and the thermal stream. Names
/// the files, keeps the recordings within the storage limit as one starts and while it runs, keeps from recording
/// once the storage is nearly full (kFreeSpaceFloor), writes the telemetry subtitles of the video on display (the
/// primary stream or the USB camera) and reports recording failures to the user. The sources in tiles aren't
/// recorded; a recording goes on for a source that moves to a tile meanwhile.
class VideoRecordingService : public QObject
{
    Q_OBJECT

    friend class VideoRecordingServiceTest;

public:
    explicit VideoRecordingService(const QList<VideoStreamController*>& sources, QObject* parent = nullptr);
    ~VideoRecordingService() override;

    /// With less than this free on the volume of the video save directory nothing is recorded: a start is refused,
    /// and a recording that runs is stopped at the next check, which comes every 10 s. Ten seconds of even a
    /// 100 Mbit/s recording are 125 MB; the rest is room to finish the files and for QGC's telemetry logs, which go to
    /// the same storage. Free space that isn't known keeps nothing from being recorded.
    static constexpr qint64 kFreeSpaceFloor = 256LL * 1024 * 1024;

    /// Follows @p source as well, for as long as it exists.
    void addSource(VideoStreamController* source);

    /// True while any source records.
    bool recording() const { return _recording; }

    /// Records every source that plays and isn't in a tile into the video save directory as "<baseName>.<ext>", the
    /// thermal stream as "<baseName>.thermalVideo.<ext>": also while its view is off, when it connects for the
    /// recording. An empty @p baseName picks an unused timestamped name. Refused, with a warning logged, while a
    /// recording runs: that one goes on as it is. Refused, and the user told, with less than kFreeSpaceFloor free.
    /// The storage limit, when it is on, makes room first: a start short of space waits until it has, and another
    /// start is refused meanwhile.
    void startRecording(const QString& baseName = QString());
    /// Stops the recordings; the sources finish their files. Gives up a start that waits for the storage limit.
    void stopRecording();

    static QString recordingFile(const QString& directory, const QString& baseName, const VideoStreamController* source,
                                 VideoReceiver::FileFormat format);
    /// @p time as "yyyy-MM-dd_hh.mm.ss", suffixed "_2", "_3"... while files of that name exist in @p directory.
    static QString uniqueBaseName(const QString& directory, const QDateTime& time);
    /// Deletes the oldest recordings and subtitle files in @p directory until they use less than @p maxBytes. Leaves
    /// the files being written: @p openFiles, and any written within the last minute, as a recording's file is before
    /// it reports its first frame. Returns the deleted files.
    static QStringList pruneStorage(const QString& directory, quint64 maxBytes, const QStringList& openFiles = {});

signals:
    void recordingChanged(bool recording);

private:
    /// Logs @p source's recordings and reports their failures.
    void _connectRecordings(VideoStreamController* source);
    /// Writes @p source's telemetry subtitles beside each file it records.
    void _connectSubtitles(VideoStreamController* source);
    void _updateRecording();
    /// startRecording(); @p pruned once the storage limit's pruning that the start waited for is done.
    void _startRecording(const QString& baseName, bool pruned);
    /// Prunes the save directory on the thread pool, when the storage limit is on, and returns that pruning.
    std::optional<QFuture<QStringList>> _applyStorageLimit();
    /// Less than kFreeSpaceFloor is known to be free on the volume of @p directory.
    bool _freeSpaceShort(const QString& directory) const;
    /// Stops the recordings, and tells the user, once the free space is short.
    void _checkFreeSpace();
    /// The files being recorded, each with the subtitle file that may go with it.
    QStringList _openFiles() const;

    QList<VideoStreamController*> _sources;
    SubtitleWriter* const _subtitleWriter;
    bool _recording = false;
    QSet<QString> _recordingFiles;
    /// While recording: the files grow, and a long recording's closed parts can make room for its next ones.
    QTimer _storageLimitTimer;
    /// The bytes free on the volume of a directory; negative when that isn't known.
    std::function<qint64(const QString& directory)> _freeSpace;
    /// While recording: _checkFreeSpace().
    QTimer _freeSpaceTimer;
    /// The base name of a start that waits for the storage limit's pruning to make room.
    std::optional<QString> _pendingStart;
};
