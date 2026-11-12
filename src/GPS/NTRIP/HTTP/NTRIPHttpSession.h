#pragma once

#include <chrono>
#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "NTRIPConfiguration.h"
#include "NTRIPError.h"
#include "NTRIPHttpCodec.h"
#include "ScheduledTask.h"

class QSslError;
class QSslSocket;
class QTcpSocket;
class RuntimeScheduler;

/// The casters found to take only NTRIP version 1 requests, by host, port and TLS. The sessions that share one ask
/// such a caster in that form at once, instead of finding it out again on every connection. A caster is remembered
/// once it accepts a version 1 request. It is forgotten when it does not accept one, answering it with anything else
/// or closing it unanswered, and when it accepts a version 2 request: the next exchange then starts in version 2 form
/// again. A failure that says nothing of the caster, such as a lost network or a timeout, leaves it remembered.
class NTRIPCasterVersions : public QObject
{
    Q_OBJECT

public:
    /// The most casters remembered. For another one, the caster that accepted a version 1 request longest ago is
    /// forgotten.
    static constexpr qsizetype MAX_CASTERS = 16;

    [[nodiscard]] NTRIPVersion versionFor(const NTRIPConnectionConfig& caster) const;
    /// Remembers that @a caster accepted a version 1 request, or forgets it for Version2.
    void setVersion(const NTRIPConnectionConfig& caster, NTRIPVersion version);

    /// The remembered casters as one line of text, which restore() reads: their URLs, the latest last, with a space
    /// between them.
    [[nodiscard]] QString saved() const;
    /// Remembers the casters of @a saved in place of the present ones: the last MAX_CASTERS of them, each once.
    void restore(const QString& saved);

signals:
    /// saved() changed.
    void changed();

private:
    /// Keeps the last MAX_CASTERS of @a casters and announces a change.
    void _remember(const QStringList& casters);

    /// The latest last.
    QStringList _version1Casters;
};

/// One HTTP exchange with a caster over TCP or TLS, shared by the correction stream and source-table fetches: it sends
/// the request once connected and decodes the response. responseStarted() precedes the body, which arrives through
/// bodyReceived(); the exchange ends with at most one failed() or finished(), and abort() and retire() end it silently.
/// An error status waits up to ERROR_BODY_TIMEOUT for the caster's explanation in its body.
/// Observers may abort or retire the session from any of its signals, but must not delete it there: the signal
/// is delivered from inside the session's socket, which deleting the session would destroy mid-emission.
///
/// With the self-signed opt-in, the first self-signed certificate accepted from a host:port is pinned; later
/// connections accept a self-signed certificate only if it matches the configured pin.
///
/// A caster that speaks only NTRIP version 1 closes a version 2 request without answering it. The session then sends
/// the request once more, in version 1 form over a new connection, as part of the same exchange: nothing is reported
/// for the unanswered request, and the owner's timeout covers both. A caster that sent anything is not asked again.
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
    /// With @a casterVersions, a caster that accepted a version 1 request is asked in that form first, until it does
    /// not accept such a request.
    QString open(const NTRIPConnectionConfig& config, std::shared_ptr<NTRIPCasterVersions> casterVersions = {});
    /// Sends @a bytes after the request. False unless isConnected() and the socket accepted every byte; the owner
    /// decides how to report it.
    bool write(const QByteArray& bytes);
    /// The connection is open and the request has been sent on it, so the caster reads what is written from now on
    /// as following the request.
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
    /// @a bytes were read from the connection, whatever they are part of: the response's header, its framing or its
    /// body. Precedes what decoding them publishes.
    void dataReceived(qint64 bytes);
    /// Body bytes, with the scheduler time at which the bytes carrying them arrived.
    void bodyReceived(const QByteArray& body, qint64 receivedAtMs);
    /// A socket, TLS or HTTP failure, including an error status with the caster's explanation.
    void failed(const NTRIPFailure& failure);
    /// The response ended where its HTTP framing allows.
    void finished();

private:
    /// Opens a connection to the caster, which sends _request once established.
    void _connect();
    /// Also the test seam for sockets created elsewhere.
    void _attach(QTcpSocket* socket);
    void _verifyCertificate(QSslSocket* socket, const QList<QSslError>& errors);
    void _establish();
    void _read();
    /// The caster closed the connection: repeats a version 2 request it left unanswered, else ends the response.
    void _closed();
    void _repeatInVersion1();
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
    NTRIPConnectionConfig _config;
    std::shared_ptr<NTRIPCasterVersions> _casterVersions;
    /// The form of _request.
    NTRIPVersion _version = NTRIPVersion::Version2;
    QByteArray _request;
    QTcpSocket* _socket = nullptr;
    QString _endpoint;
    QString _newPin;
    bool _requestSent = false;
    /// The caster sent something in answer to _request.
    bool _answered = false;
    /// The caster accepted _request, so what follows is its body.
    bool _accepted = false;
    bool _reading = false;
    bool _ended = false;
};
