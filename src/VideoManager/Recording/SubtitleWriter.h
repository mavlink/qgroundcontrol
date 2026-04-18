#pragma once

#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSize>
#include <QtCore/QTime>
#include <QtCore/QTimer>

class Fact;
class QVideoSink;

class SubtitleWriter : public QObject
{
    Q_OBJECT

public:
    explicit SubtitleWriter(QObject* parent = nullptr);
    ~SubtitleWriter();

    void startCapturingTelemetry(const QString& videoFile, QSize size);
    void stopCapturingTelemetry();

    /// Set a live sink for real-time OSD via QVideoSink::setSubtitleText().
    /// Starts a 1 Hz timer if not already capturing to file.
    void setLiveVideoSink(QVideoSink* sink);

private slots:
    void _captureTelemetry();

private:
    /// Snapshots Facts from the currently displayed telemetry bar grid for
    /// the active vehicle. Two callers:
    ///  - `startCapturingTelemetry` pins the result into `_recordingFacts`
    ///    so the file-write path stays bound to the vehicle that started the
    ///    recording, even if the user switches active mid-recording.
    ///  - `setLiveVideoSink` refreshes `_liveFacts` so the OSD overlay tracks
    ///    whichever vehicle's video is currently on screen.
    QList<Fact*> _gatherFacts() const;

    /// Build a plain-text summary of `facts` for live OSD.
    QString _buildTelemetrySummary(const QList<Fact*>& facts) const;

    QFile _file;
    /// Pinned at `startCapturingTelemetry`. File-write path uses these so
    /// the .ass sidecar reflects the recording vehicle's telemetry only.
    QList<Fact*> _recordingFacts;
    /// Refreshed at `setLiveVideoSink`. OSD overlay uses these so it tracks
    /// the active vehicle (whose video is being displayed).
    QList<Fact*> _liveFacts;
    QSize _size;
    QTime _lastEndTime;
    QTimer _timer;
    QPointer<QVideoSink> _liveSink;

    static constexpr int _kSampleRate =
        1;  ///< Sample rate in Hz for getting telemetry data, most players do weird stuff when > 1Hz
};
