#pragma once

#include "DataRateTracker.h"

#include <QtCore/QChronoTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

#include <chrono>

class NTRIPConnectionStats : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(quint64 bytesReceived READ bytesReceived NOTIFY bytesReceivedChanged)
    Q_PROPERTY(quint32 messagesReceived READ messagesReceived NOTIFY messagesReceivedChanged)
    Q_PROPERTY(double dataRateBytesPerSec READ dataRateBytesPerSec NOTIFY dataRateChanged)
    Q_PROPERTY(double correctionAgeSec READ correctionAgeSec NOTIFY correctionAgeChanged)
    Q_PROPERTY(bool   dataStale       READ dataStale       NOTIFY dataStaleChanged)

public:
    explicit NTRIPConnectionStats(QObject* parent = nullptr);

    void start();
    void stop();
    void recordMessage(int bytes);
    void reset();

    quint64 bytesReceived() const { return _rateTracker.totalBytes(); }
    quint32 messagesReceived() const { return _messagesReceived; }
    double dataRateBytesPerSec() const { return _rateTracker.bytesPerSec(); }
    double correctionAgeSec() const { return _lastMessageTime.isValid() ? _lastMessageTime.elapsed() / 1000.0 : -1.0; }
    bool dataStale() const { return _dataStale; }

signals:
    void bytesReceivedChanged();
    void messagesReceivedChanged();
    void dataRateChanged();
    void correctionAgeChanged();
    void dataStaleChanged();

private:
    static constexpr std::chrono::milliseconds kStaleThreshold{5000};

    DataRateTracker _rateTracker;
    quint32 _messagesReceived = 0;
    quint32 _prevMessagesReceived = 0;
    bool _dataStale = false;
    QElapsedTimer _lastMessageTime;
    QChronoTimer _rateTimer;
};
