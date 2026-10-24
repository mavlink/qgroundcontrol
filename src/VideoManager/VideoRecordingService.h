#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "VideoReceiver.h"

class QDateTime;
class SubtitleWriter;
class VideoStreamController;

/// Records the video streams: names the files, keeps the recordings within the storage limit, writes the primary
/// stream's telemetry subtitles and reports recording failures to the user.
class VideoRecordingService : public QObject
{
    Q_OBJECT

    friend class VideoRecordingServiceTest;

public:
    VideoRecordingService(VideoStreamController* primaryStream, VideoStreamController* thermalStream,
                          QObject* parent = nullptr);
    ~VideoRecordingService() override;

    /// True while any stream records.
    bool recording() const { return _recording; }

    /// Records every active stream into the video save directory as "<baseName>.<ext>", the thermal stream as
    /// "<baseName>.thermalVideo.<ext>". An empty @p baseName picks an unused timestamped name.
    void startRecording(const QString& baseName = QString());
    void stopRecording();

    static QString recordingFile(const QString& directory, const QString& baseName, const VideoStreamController* stream,
                                 VideoReceiver::FileFormat format);
    /// @p time as "yyyy-MM-dd_hh.mm.ss", suffixed "_2", "_3"... while files of that name exist in @p directory.
    static QString uniqueBaseName(const QString& directory, const QDateTime& time);
    /// Deletes the oldest recordings and subtitle files in @p directory until they use less than @p maxBytes. Returns
    /// the deleted files.
    static QStringList pruneStorage(const QString& directory, quint64 maxBytes);

signals:
    void recordingChanged(bool recording);
    void recordingStarted(const QString& file);

private:
    void _connectStream(VideoStreamController* stream);
    void _updateRecording();

    VideoStreamController* const _primaryStream;
    VideoStreamController* const _thermalStream;
    SubtitleWriter* const _subtitleWriter;
    bool _recording = false;
};
