#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>

class NTRIPConnectionStats : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint64 bytesReceived READ bytesReceived NOTIFY bytesReceivedChanged)
    Q_PROPERTY(quint32 messagesReceived READ messagesReceived NOTIFY messagesReceivedChanged)
    Q_PROPERTY(double dataRateBytesPerSec READ dataRateBytesPerSec NOTIFY dataRateChanged)

public:
    explicit NTRIPConnectionStats(QObject* parent = nullptr);

    void start();
    void stop();
    void recordMessage(int bytes);
    void reset();

    quint64 bytesReceived() const { return _bytesReceived; }
    quint32 messagesReceived() const { return _messagesReceived; }
    double dataRateBytesPerSec() const { return _dataRateBytesPerSec; }

signals:
    void bytesReceivedChanged();
    void messagesReceivedChanged();
    void dataRateChanged();

private:
    quint64 _bytesReceived = 0;
    quint32 _messagesReceived = 0;
    double _dataRateBytesPerSec = 0.0;
    quint64 _dataRatePrevBytes = 0;
    QTimer _rateTimer;
};
