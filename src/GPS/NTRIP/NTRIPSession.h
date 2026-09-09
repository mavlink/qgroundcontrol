#pragma once

#include <QtCore/QChronoTimer>
#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <functional>

#include "NTRIPStream.h"
#include "NTRIPTransportConfig.h"

/// Settings-independent owner of one caster session and its replacement attempts.
class NTRIPSession : public QObject
{
    Q_OBJECT
    friend class NTRIPSessionTest;
    friend class NTRIPManagerTest;

public:
    enum class State
    {
        Disconnected,
        Connecting,
        Connected,
        Reconnecting,
        Error
    };
    Q_ENUM(State)
    using StreamFactory = std::function<NTRIPStream*(const NTRIPTransportConfig&, QObject*)>;
    using Clock = std::function<qint64()>;

    explicit NTRIPSession(StreamFactory factory, QObject* parent = nullptr, Clock clock = {});
    ~NTRIPSession() override;

    void start(const NTRIPTransportConfig& config, bool reconnect = true);
    void stop();
    void setWhitelist(const QString& whitelist);
    void sendNMEA(const QByteArray& sentence);

    void setStreamForTest(NTRIPStream* stream) { _injectedStream = stream; }

    State state() const { return _state; }

    QString statusMessage() const { return _message; }

    const NTRIPTransportConfig& config() const { return _config; }

    QString sourceId() const;

    quint64 activeAttemptId() const { return _stream ? _activeAttemptId : 0; }

    int failedAttempts() const { return _failedAttempts; }

    std::chrono::milliseconds nextRetryDelay() const;

signals:
    void stateChanged(NTRIPSession::State state, const QString& message);
    void streamStarted(quint64 attemptId, const QString& sourceId);
    void streamEnded(quint64 attemptId);
    void streamConnected(NTRIPStream* stream);
    void correctionReceived(const QByteArray& data, int messageId, bool filtered, qint64 receivedAtMs,
                            quint64 attemptId);
    void correctionRejected(const QByteArray& data, int messageId, qint64 receivedAtMs, quint64 attemptId);
    void bytesReceived(qint64 count);
    void failureOccurred(const NTRIPFailure& failure);
    void plaintextCredentialsWarning();

private:
    void _beginAttempt(quint64 generation);
    bool _retireStream(quint64 generation);
    bool _setState(State state, const QString& message, quint64 generation);
    void _onFailure(const NTRIPFailure& failure, NTRIPStream* stream);
    void _onCorrection(const QByteArray& data, int messageId, bool filtered, qint64 receivedAtMs, NTRIPStream* stream,
                       quint64 attemptId);
    void _onConnected(NTRIPStream* stream);

    StreamFactory _factory;
    Clock _clock;
    NTRIPTransportConfig _config;
    QPointer<NTRIPStream> _stream;
    QPointer<NTRIPStream> _injectedStream;
    QChronoTimer _retryTimer;
    State _state = State::Disconnected;
    QString _message;
    quint64 _generation = 0;
    quint64 _nextAttemptId = 0;
    quint64 _activeAttemptId = 0;
    int _failedAttempts = 0;
    qint64 _healthySince = -1;
    qint64 _lastValid = -1;
    bool _reconnect = true;
    static constexpr int MAX_FAILED_ATTEMPTS = 100;
    static constexpr qint64 HEALTHY_PERIOD_MS = 10000;
    static constexpr qint64 HEALTHY_GAP_MS = 5000;
};
