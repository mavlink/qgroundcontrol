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

#include "NTRIPConfiguration.h"
#include "NTRIPTlsTestFixtures.h"
#include "UnitTest.h"

class ScriptedNtripCaster
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

        void writeChunks(std::initializer_list<QByteArray> chunks) const
        {
            for (const QByteArray& chunk : chunks) {
                write(chunk);
            }
        }

        void disconnectFromHost() const
        {
            if (peer) {
                peer->disconnectFromHost();
            }
        }

        QPointer<QTcpSocket> peer;
        QByteArray request;
    };

    explicit ScriptedNtripCaster(Transport transport = Transport::Tcp, Certificate certificate = Certificate::Loopback)
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

    QTcpServer* server() const { return _server.get(); }

    void setCertificate(Certificate certificate)
    {
        if (auto* server = qobject_cast<QSslServer*>(_server.get())) {
            server->setSslConfiguration(_tlsConfiguration(certificate));
        }
    }

    NTRIPConnectionConfig connectionConfig(const QString& mountpoint = QStringLiteral("TEST")) const
    {
        NTRIPConnectionConfig config;
        config.host = QStringLiteral("127.0.0.1");
        config.port = port();
        config.mountpoint = mountpoint;
        config.useTls = qobject_cast<QSslServer*>(_server.get()) != nullptr;
        return config;
    }

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

    QByteArray waitForRequest(int timeoutMs = TestTimeout::mediumMs())
    {
        Connection* connection = waitForConnection(timeoutMs);
        return connection ? connection->waitForRequest(timeoutMs) : QByteArray();
    }

    Connection* latestConnection() const { return _connections.empty() ? nullptr : _connections.back().get(); }

    void write(const QByteArray& bytes) const
    {
        if (Connection* connection = latestConnection()) {
            connection->write(bytes);
        }
    }

    void writeChunks(std::initializer_list<QByteArray> chunks) const
    {
        if (Connection* connection = latestConnection()) {
            connection->writeChunks(chunks);
        }
    }

    void disconnectPeer() const
    {
        if (Connection* connection = latestConnection()) {
            connection->disconnectFromHost();
        }
    }

    void close()
    {
        if (_server) {
            _server->close();
        }
    }

private:
    static QSslConfiguration _tlsConfiguration(Certificate certificate)
    {
        QSslConfiguration configuration = QSslConfiguration::defaultConfiguration();
        const QByteArray& certificatePem = certificate == Certificate::Mismatched
                                               ? NTRIPTlsTestFixtures::MISMATCHED_CERT_PEM
                                               : NTRIPTlsTestFixtures::SERVER_CERT_PEM;
        configuration.setLocalCertificate(QSslCertificate(certificatePem, QSsl::Pem));
        configuration.setPrivateKey(QSslKey(NTRIPTlsTestFixtures::PRIVATE_KEY_PEM, QSsl::Rsa, QSsl::Pem));
        configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
        return configuration;
    }

    std::unique_ptr<QTcpServer> _server;
    std::vector<std::unique_ptr<Connection>> _connections;
    bool _listening = false;
};
