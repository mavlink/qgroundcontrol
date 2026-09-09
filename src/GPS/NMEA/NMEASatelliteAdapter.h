#pragma once

#include <QtCore/QIODevice>
#include <QtCore/QMap>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoSatelliteInfo>

#include "GPSReadTimestamp.h"

/// Normalizes constellation and signal-specific NMEA satellite reports for Qt's decoder.
class NMEASatelliteAdapter : public QIODevice, public GPSReadTimestamp
{
    Q_OBJECT

public:
    explicit NMEASatelliteAdapter(QIODevice* source, QObject* parent = nullptr);
    ~NMEASatelliteAdapter() override;

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override;
    bool canReadLine() const override;
    void close() override;

    quint64 lastReadTimestampUs() const override { return _lastReadTimestampUs; }

    struct Snapshot
    {
        QList<QGeoSatelliteInfo> satellites;
        quint64 receivedAtUs = 0;
        QMap<QByteArray, quint64> constellationReceipts;
        QMap<QByteArray, QSet<int>> usedIds;
    };

    quint64 satelliteTimestampUs(bool inUse) const;
    Snapshot satelliteSnapshot(const QList<QGeoSatelliteInfo>& satellites, bool inUse) const;

protected:
    qint64 readData(char* data, qint64 maxSize) override;
    qint64 readLineData(char* data, qint64 maxSize) override;

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    struct SignalReport
    {
        int messageCount = 0;
        int satelliteCount = 0;
        int nextMessage = 1;
        quint64 receivedAtUs = 0;
        QList<QList<QByteArray>> satellites;

        bool complete() const { return messageCount > 0 && nextMessage == messageCount + 1; }
    };

    void _readAvailable();
    void _parseSentence(const QByteArray& sentence, quint64 receivedAtUs);
    void _flush();

    struct TimedSentence
    {
        QByteArray bytes;
        QByteArray talker;
        quint64 receivedAtUs = 0;
        bool inUse = false;
        QSet<int> usedIds = {};
    };

    QPointer<QIODevice> _source;
    QTimer _idleTimer;
    QTimer _batchTimer;
    QMap<QByteArray, QMap<int, SignalReport>> _reports;
    QMap<QByteArray, QList<QByteArray>> _inUse;
    QMap<QByteArray, quint64> _inUseReceivedAtUs;
    QMap<QByteArray, quint64> _consumedViewTimestamps;
    QMap<QByteArray, quint64> _consumedUseTimestamps;
    QMap<QByteArray, QSet<int>> _consumedUseIds;
    QByteArray _epochTime;
    QList<TimedSentence> _output;
    qsizetype _bufferSize = 0;
    quint64 _lastReadTimestampUs = 0;
};
