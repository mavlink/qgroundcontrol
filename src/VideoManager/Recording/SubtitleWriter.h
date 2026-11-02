#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#include "TelemetryBarValues.h"

/// Writes the telemetry bar's values next to a video recording, as an Advanced SubStation Alpha (.ass) subtitle file
/// that players overlay on the video.
class SubtitleWriter : public QObject
{
    Q_OBJECT

    friend class SubtitleWriterTest;

public:
    explicit SubtitleWriter(QObject* parent = nullptr);
    ~SubtitleWriter() override;

    /// Starts "<videoFile without extension>.ass", replacing any existing file, timed from now. Call when the video
    /// file starts. Stops any capture in progress first.
    void startCapturingTelemetry(const QString& videoFile, QSize videoSize);
    void stopCapturingTelemetry();

    bool capturing() const { return _file.isOpen(); }

    /// The video file the current capture belongs to.
    QString videoFile() const { return _videoFile; }

private:
    struct Sample
    {
        qint64 startMs = -1;
        QStringList names;
        QStringList values;
        QString date;
    };

    void _captureTelemetry();
    Sample _sample(qint64 startMs) const;
    void _writeSample(const Sample& sample, qint64 endMs);

    QFile _file;
    QString _videoFile;
    TelemetryBarValues _telemetry;
    QSize _size;
    int _rowWidth = 0;
    QElapsedTimer _elapsed;
    QTimer _timer;
    /// The latest sample, written once the next one (or the end of the capture) sets its end time.
    Sample _pending;

    static constexpr int _kSampleRate = 1;  ///< Hz; most players misbehave above 1 Hz
};
