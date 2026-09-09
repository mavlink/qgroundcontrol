#pragma once

#include <QtCore/QIODevice>
#include <QtCore/QMap>
#include <QtCore/QPointer>
#include <QtCore/QTimer>

/// Normalizes constellation and signal-specific NMEA satellite reports for Qt's decoder.
class NMEASatelliteAdapter : public QIODevice
{
    Q_OBJECT

public:
    explicit NMEASatelliteAdapter(QIODevice* source, QObject* parent = nullptr);
    ~NMEASatelliteAdapter() override;

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override;
    bool canReadLine() const override;
    void close() override;

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
        QList<QList<QByteArray>> satellites;

        bool complete() const { return messageCount > 0 && nextMessage == messageCount + 1; }
    };

    void _readAvailable();
    void _parseSentence(const QByteArray& sentence);
    void _flush();

    QPointer<QIODevice> _source;
    QTimer _idleTimer;
    QTimer _batchTimer;
    QMap<QByteArray, QMap<int, SignalReport>> _reports;
    QMap<QByteArray, QList<QByteArray>> _inUse;
    QByteArray _epochTime;
    QByteArray _buffer;
};
