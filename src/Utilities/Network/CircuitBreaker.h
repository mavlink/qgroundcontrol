/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QObject>

Q_DECLARE_LOGGING_CATEGORY(CircuitBreakerLog)

class CircuitBreaker : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Closed,      // Normal operation
        Open,        // Failing, reject requests
        HalfOpen     // Testing if service recovered
    };

    explicit CircuitBreaker(QObject *parent = nullptr);

    void setFailureThreshold(int threshold) { _failureThreshold = threshold; }
    void setTimeout(int timeoutMs) { _timeoutMs = timeoutMs; }
    void setSuccessThreshold(int threshold) { _successThreshold = threshold; }

    // Call before making a request
    bool allowRequest();

    // Record request results
    void recordSuccess();
    void recordFailure();

    // State queries
    State state() const { return _state; }
    bool isOpen() const { return _state == State::Open; }
    QString stateString() const;

    void reset();

signals:
    void stateChanged(State newState);
    void circuitOpened();
    void circuitClosed();

private:
    void setState(State newState);
    void attemptReset();

    State _state = State::Closed;
    int _failureCount = 0;
    int _successCount = 0;
    int _failureThreshold = 5;
    int _successThreshold = 2;
    int _timeoutMs = 60000;  // 1 minute
    QDateTime _openedTime;
    mutable QMutex _mutex;
};
