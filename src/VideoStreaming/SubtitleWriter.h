/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Video Subtitle Writer
 *   @author Willian Galvani <williangalvani@gmail.com>
 */

#pragma once

#include "QGCLoggingCategory.h"
#include "VideoReceiver.h"
#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QFile>

Q_DECLARE_LOGGING_CATEGORY(SubtitleWriterLog)

class SubtitleWriter : public QObject
{
    Q_OBJECT

public:
    explicit SubtitleWriter(QObject* parent = nullptr);
    ~SubtitleWriter() = default;

    void setVideoReceiver(VideoReceiver* videoReceiver);

private slots:
    // Fires with every "videoRecordingChanged() signal, stops capturing telemetry if video stopped."
    void _onVideoRecordingChanged();

    // Captures a snapshot of telemetry data from vehicle into the subtitles file.
    void _captureTelemetry();

    // starts capturing vehicle telemetry.
    void _startCapturingTelemetry();

private:
    QTimer _timer;
    QStringList _values;
    QDateTime _startTime;
    QFile _file;

    VideoReceiver* _videoReceiver;

    static const int _sampleRate;
};
