#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include "NTRIPError.h"

Q_DECLARE_LOGGING_CATEGORY(NTRIPHttpSessionLog)

struct NTRIPConnectionConfig;
class QTcpSocket;

/// One caster connection over TCP or TLS, shared by the correction stream and source-table fetches.
/// established() asks the owner to send its request, and the response arrives through bytesReceived().
/// The connection ends with at most one failed() or closed(); abort() and retire() end it silently.
/// Observers may abort, retire, or delete the session from any of its signals.
class NTRIPHttpSession : public QObject
{
    Q_OBJECT
    friend class NTRIPHttpTransportTest;

public:
    static constexpr qint64 kReadBufferBytes = 64 * 1024;
    static constexpr qint64 kReadChunkBytes = 16 * 1024;

    explicit NTRIPHttpSession(QObject* parent = nullptr);

    /// Connects once; later calls are ignored.
    void open(const NTRIPConnectionConfig& config);
    /// False unless the socket accepted every byte; the owner decides how to report it.
    bool write(const QByteArray& bytes);
    bool isConnected() const;

    /// A bytesReceived() delivery is in progress.
    bool reading() const { return _reading; }

    /// Ends the connection without further signals.
    void abort();
    /// Detaches from its owning parent, aborts, and deletes the session later, so abort observers cannot reach
    /// the owner.
    void retire();

signals:
    void established();
    void bytesReceived(const QByteArray& bytes, qint64 receivedAtMs);
    /// Socket or certificate failure.
    void failed(NTRIPError code, const QString& message);
    /// The caster closed the connection after every received byte was delivered.
    void closed();

private:
    /// Also the test seam for sockets created elsewhere.
    void _attach(QTcpSocket* socket, bool allowSelfSignedCerts = false);
    void _establish();
    void _read();
    void _fail(NTRIPError code, const QString& message);

    QPointer<QTcpSocket> _socket;
    bool _reading = false;
    bool _ended = false;
};
