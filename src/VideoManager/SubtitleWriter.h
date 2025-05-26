/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QTime>
#include <QtCore/QTimer>

class Fact;

Q_DECLARE_LOGGING_CATEGORY(SubtitleWriterLog)

class SubtitleWriter : public QObject
{
    Q_OBJECT

public:
    explicit SubtitleWriter(QObject *parent = nullptr);
    ~SubtitleWriter();

    void startCapturingTelemetry(const QString &videoFile, QSize size);
    void stopCapturingTelemetry();

private slots:
    void _captureTelemetry();

private:
    QFile _file;
    QList<Fact*> _facts;
    QSize _size;
    QTime _lastEndTime;
    QTimer _timer;

    static constexpr int _kSampleRate = 1; ///< Sample rate in Hz for getting telemetry data, most players do weird stuff when > 1Hz
};
