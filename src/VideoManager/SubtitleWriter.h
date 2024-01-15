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
#include "Fact.h"
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

    // starts capturing vehicle telemetry.
    void startCapturingTelemetry(const QString& videoFile);
    void stopCapturingTelemetry();

private slots:
    // Captures a snapshot of telemetry data from vehicle into the subtitles file.
    void _captureTelemetry();

private:
    QTimer _timer;
    QList<Fact*> _facts;
    QTime _lastEndTime;
    QFile _file;

    static const int _sampleRate; // Sample rate in Hz for getting telemetry data, most players do weird stuff when > 1Hz
};
