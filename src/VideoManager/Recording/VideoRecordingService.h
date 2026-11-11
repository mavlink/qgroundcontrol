#pragma once

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#include "VideoReceiver.h"

class QDateTime;
class SubtitleWriter;
class UVCCameraController;
class VideoStreamController;

/// Records the video streams, or the USB camera: names the files, keeps the recordings within the storage limit as
/// one starts and while it runs, writes the telemetry subtitles of the primary stream or the USB camera and reports
/// recording failures to the user.
class VideoRecordingService : public QObject
{
    Q_OBJECT

    friend class VideoRecordingServiceTest;

public:
    VideoRecordingService(VideoStreamController* primaryStream, VideoStreamController* thermalStream,
                          UVCCameraController* uvcCamera, QObject* parent = nullptr);
    ~VideoRecordingService() override;

    /// True while any stream, or the USB camera, records.
    bool recording() const { return _recording; }

    /// Records every active stream into the video save directory as "<baseName>.<ext>", the thermal stream as
    /// "<baseName>.thermalVideo.<ext>", and the USB camera while it runs as "<baseName>.<ext>". An empty @p baseName
    /// picks an unused timestamped name.
    void startRecording(const QString& baseName = QString());
    void stopRecording();

    static QString recordingFile(const QString& directory, const QString& baseName, const VideoStreamController* stream,
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
    /// Logs @p source's recordings and reports their failures; a VideoStreamController or the UVCCameraController.
    template <class Source>
    void _connectRecordings(Source* source);
    /// Writes @p source's telemetry subtitles beside each file it records; @p videoSize gives their resolution.
    template <class Source, class VideoSize>
    void _connectSubtitles(Source* source, VideoSize videoSize);
    void _updateRecording();
    /// Prunes the save directory on the thread pool, when the storage limit is on.
    void _applyStorageLimit();
    /// The files being recorded, each with the subtitle file that may go with it.
    QStringList _openFiles() const;

    VideoStreamController* const _primaryStream;
    VideoStreamController* const _thermalStream;
    UVCCameraController* const _uvcCamera;
    SubtitleWriter* const _subtitleWriter;
    bool _recording = false;
    QSet<QString> _recordingFiles;
    /// While recording: the files grow, and a long recording's closed parts can make room for its next ones.
    QTimer _storageLimitTimer;
};
