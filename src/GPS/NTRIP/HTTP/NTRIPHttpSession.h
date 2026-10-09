#pragma once

#include <chrono>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>

#include "NTRIPError.h"
#include "NTRIPHttpCodec.h"
#include "ScheduledTask.h"

struct NTRIPConnectionConfig;
class QSslError;
class QSslSocket;
class QTcpSocket;
class RuntimeScheduler;

/// One HTTP exchange with a caster over TCP or TLS, shared by the correction stream and source-table fetches: it sends
/// the request once connected and decodes the response. responseStarted() precedes the body, which arrives through
/// bodyReceived(); the exchange ends with at most one failed() or finished(), and abort() and retire() end it silently.
/// An error status waits up to ERROR_BODY_TIMEOUT for the caster's explanation in its body.
/// Observers may abort or retire the session from any of its signals, but must not delete it there: the signal
/// is delivered from inside the session's socket, which deleting the session would destroy mid-emission.
///
/// With the self-signed opt-in, the first self-signed certificate accepted from a host:port is pinned; later
/// connections accept a self-signed certificate only if it matches the configured pin.
class NTRIPHttpSession : public QObject
{
    Q_OBJECT
    friend class NTRIPHttpSessionTest;
    friend class NTRIPHttpTransportTest;

public:
    static constexpr qint64 READ_BUFFER_BYTES = 64 * 1024;
    static constexpr qint64 READ_CHUNK_BYTES = 16 * 1024;
    static constexpr std::chrono::milliseconds ERROR_BODY_TIMEOUT{250};

    explicit NTRIPHttpSession(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr,
                              NTRIPHttpPurpose purpose = NTRIPHttpPurpose::Corrections);

    /// Connects once and sends the request of the session's purpose when connected; later calls are ignored. Returns
    /// why @a config cannot build a request, without connecting, or empty.
    QString open(const NTRIPConnectionConfig& config);
    /// False unless the socket accepted every byte; the owner decides how to report it.
    bool write(const QByteArray& bytes);
    bool isConnected() const;

    /// An error status arrived and its body is still being read.
    bool awaitingErrorBody() const { return _decoder.awaitingErrorBody(); }

    /// Ends the connection without further signals.
    void abort();
    /// Detaches from its owning parent, aborts, and deletes the session later, so abort observers cannot reach
    /// the owner.
    void retire();

signals:
    /// A newly trusted self-signed certificate, as NTRIPConnectionConfig::pinnedCertificate; precedes the request.
    void certificatePinned(const QString& pin);
    /// The caster accepted the request; its body follows.
    void responseStarted();
    /// Body bytes, with the scheduler time at which the bytes carrying them arrived.
    void bodyReceived(const QByteArray& body, qint64 receivedAtMs);
    /// A socket, TLS or HTTP failure, including an error status with the caster's explanation.
    void failed(const NTRIPFailure& failure);
    /// The response ended where its HTTP framing allows.
    void finished();

private:
    /// Also the test seam for sockets created elsewhere.
    void _attach(QTcpSocket* socket);
    void _verifyCertificate(QSslSocket* socket, const QList<QSslError>& errors);
    void _establish();
    void _read();
    /// Decodes bytes that arrived at @a receivedAtMs; also the test seam for responses without a socket.
    void _receive(QByteArrayView bytes, qint64 receivedAtMs, const QDateTime& utcNow = QDateTime::currentDateTimeUtc());
    /// Emits what @a result decoded; a failure or a complete response ends the exchange.
    void _publish(const NTRIPHttpDecoder::Result& result, qint64 receivedAtMs);
    /// Publishes the end of the response as the bytes so far frame it.
    void _finishResponse();
    void _fail(NTRIPError code, const QString& message);
    void _end();

    RuntimeScheduler* const _scheduler;
    const NTRIPHttpPurpose _purpose;
    ScheduledTask _errorBodyTask;
    NTRIPHttpDecoder _decoder;
    QByteArray _request;
    QTcpSocket* _socket = nullptr;
    QString _endpoint;
    QString _pinnedCertificate;
    QString _newPin;
    bool _allowSelfSignedCerts = false;
    bool _reading = false;
    bool _ended = false;
};
