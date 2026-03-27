#pragma once

#include "LineBuffer.h"

#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

Q_DECLARE_LOGGING_CATEGORY(NmeaStreamSplitterLog)

/// A simple sequential QIODevice that acts as a writable pipe.
/// Data written via feedData() becomes available for reading.
/// Used to duplicate an NMEA stream to multiple consumers.
class NmeaPipeDevice : public QIODevice
{
    Q_OBJECT

public:
    explicit NmeaPipeDevice(QObject *parent = nullptr);

    bool isSequential() const override { return true; }
    bool canReadLine() const override;
    qint64 bytesAvailable() const override;

    void feedData(const QByteArray &data);

    static constexpr qsizetype kMaxBufferSize = 65536;

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 readLineData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    LineBuffer _buffer;
};

/// Reads from a source QIODevice and duplicates the data to two NmeaPipeDevice outputs.
/// This allows a single NMEA serial/UDP stream to feed both a QNmeaPositionInfoSource
/// and a QNmeaSatelliteInfoSource simultaneously.
///
/// Audit (2026-04-17): QNmeaPositionInfoSource is only constructed in
/// QGCPositionManager::_setNmeaSource, and no other site reads NMEA from the
/// serial/UDP devices directly. All NMEA consumers (position, satellites,
/// NTRIP GGA, Follow-me) reach the stream through this splitter and
/// QGCPositionManager's signals. Re-run the audit before adding any new
/// NMEA-dependent subsystem to preserve the single-parse-pass guarantee.
class NmeaStreamSplitter : public QObject
{
    Q_OBJECT

public:
    explicit NmeaStreamSplitter(QIODevice *source, QObject *parent = nullptr);
    ~NmeaStreamSplitter();

    NmeaPipeDevice *positionPipe() const { return _positionPipe; }
    NmeaPipeDevice *satellitePipe() const { return _satellitePipe; }

private slots:
    void _onSourceReadyRead();

private:
    QIODevice *_source = nullptr;
    NmeaPipeDevice *_positionPipe = nullptr;
    NmeaPipeDevice *_satellitePipe = nullptr;
};
