#pragma once

#include <memory>
#include <vector>

#include <QtCore/QPointer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslServer>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QTest>

#include "NTRIP/Support/NTRIPTestHelpers.h"
#include "NTRIP/Support/NTRIPTlsTestFixtures.h"
#include "NTRIPConfiguration.h"
#include "UnitTest.h"

namespace GPSTest {

/// A loopback NTRIP caster, over TCP or TLS, whose responses each test writes by hand.
class ScriptedNTRIPCaster
{
public:
    enum class Transport
    {
        Tcp,
        Tls,
    };

    enum class Certificate
    {
        Loopback,
        Mismatched,
        Rotated,
    };

    class Connection
    {
    public:
        explicit Connection(QTcpSocket* acceptedPeer)
            : peer(acceptedPeer)
        {}

        QByteArray waitForRequest(int timeoutMs = TestTimeout::mediumMs())
        {
            if (!peer) {
                return request;
            }
            const bool complete = QTest::qWaitFor(
                [&]() {
                    request += peer->readAll();
                    return request.endsWith("\r\n\r\n");
                },
                timeoutMs);
            Q_UNUSED(complete);
            return request;
        }

        qint64 write(const QByteArray& bytes) const { return peer ? peer->write(bytes) : -1; }

        void disconnectFromHost() const
        {
            if (peer) {
                peer->disconnectFromHost();
            }
        }

        QPointer<QTcpSocket> peer;
        QByteArray request;
    };

    explicit ScriptedNTRIPCaster(Transport transport = Transport::Tcp, Certificate certificate = Certificate::Loopback)
    {
        if (transport == Transport::Tls) {
            auto server = std::make_unique<QSslServer>();
            server->setSslConfiguration(_tlsConfiguration(certificate));
            _server = std::move(server);
        } else {
            _server = std::make_unique<QTcpServer>();
        }
        _listening = _server->listen(QHostAddress::LocalHost);
    }

    bool isListening() const { return _listening; }

    quint16 port() const { return _server ? _server->serverPort() : 0; }

    /// The TLS listener, or null for a plain-TCP caster.
    QSslServer* tlsServer() const { return qobject_cast<QSslServer*>(_server.get()); }

    void setCertificate(Certificate certificate)
    {
        if (auto* server = tlsServer()) {
            server->setSslConfiguration(_tlsConfiguration(certificate));
        }
    }

    NTRIPConnectionConfig connectionConfig(const QString& mountpoint = QStringLiteral("TEST")) const
    {
        NTRIPConnectionConfig config = GPSTest::connectionConfig(port(), mountpoint);
        config.useTls = tlsServer() != nullptr;
        return config;
    }

    /// Accepts the next connection; over TLS, once its handshake has completed.
    Connection* waitForConnection(int timeoutMs = TestTimeout::mediumMs())
    {
        if (!_server) {
            return nullptr;
        }
        const bool connected = QTest::qWaitFor([&]() { return _server->hasPendingConnections(); }, timeoutMs);
        if (!connected || !_server->hasPendingConnections()) {
            return nullptr;
        }
        _connections.push_back(std::make_unique<Connection>(_server->nextPendingConnection()));
        return _connections.back().get();
    }

    /// Answers the next source-table request with @a table. @return whether a request arrived and was answered.
    bool serveSourceTable(const QString& table, int timeoutMs = TestTimeout::mediumMs())
    {
        Connection* connection = waitForConnection(timeoutMs);
        if (!connection || connection->waitForRequest(timeoutMs).isEmpty()) {
            return false;
        }
        const QByteArray response = okResponse(table.toUtf8());
        return connection->write(response) == response.size();
    }

    /// Accepts the next request and drops the connection without an answer.
    bool dropNextRequest(int timeoutMs = TestTimeout::mediumMs())
    {
        Connection* connection = waitForConnection(timeoutMs);
        if (!connection || connection->waitForRequest(timeoutMs).isEmpty()) {
            return false;
        }
        connection->disconnectFromHost();
        return true;
    }

    void close()
    {
        if (_server) {
            _server->close();
        }
    }

    static const QByteArray& certificatePem(Certificate certificate)
    {
        switch (certificate) {
            case Certificate::Mismatched:
                return GPSTest::MISMATCHED_CERT_PEM;
            case Certificate::Rotated:
                return GPSTest::ROTATED_CERT_PEM;
            case Certificate::Loopback:
                break;
        }
        return GPSTest::SERVER_CERT_PEM;
    }

private:
    static QSslConfiguration _tlsConfiguration(Certificate certificate)
    {
        QSslConfiguration configuration = QSslConfiguration::defaultConfiguration();
        configuration.setLocalCertificate(QSslCertificate(certificatePem(certificate), QSsl::Pem));
        configuration.setPrivateKey(QSslKey(GPSTest::PRIVATE_KEY_PEM, QSsl::Rsa, QSsl::Pem));
        configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
        return configuration;
    }

    std::unique_ptr<QTcpServer> _server;
    std::vector<std::unique_ptr<Connection>> _connections;
    bool _listening = false;
};

}  // namespace GPSTest
