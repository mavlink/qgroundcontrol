#pragma once

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QTest>

#include <memory>

#include "UnitTest.h"

/// Test fixture providing a local TCP server for network-related tests.
///
/// Handles server creation on ephemeral ports, connection waiting, and
/// simple HTTP response serving. Automatically cleans up in cleanup().
///
/// Example:
/// @code
/// class MyNetworkTest : public TcpServerTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testDownload() {
///         createLocalServer();
///         installHttpResponder("hello world");
///         QString url = serverUrl("/test.txt");
///         // ... use url to trigger download, verify result ...
///     }
/// };
/// @endcode
class TcpServerTest : public UnitTest
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TcpServerTest)

public:
    explicit TcpServerTest(QObject* parent = nullptr) : UnitTest(parent)
    {
    }

protected:
    /// Creates a local TCP server on an ephemeral port.
    /// Tries LocalHost, then LocalHostIPv6, then Any.
    ///
    /// On failure (e.g. CI without loopback) the current test is marked as
    /// skipped via QTest::qSkip() and this function returns nullptr. Callers
    /// MUST check the result and return from the test slot, because QSKIP
    /// invoked from a helper only returns from the helper — the test body
    /// continues executing otherwise.
    ///
    /// @return The server (owned by the test, cleaned up automatically), or
    ///         nullptr if no address could be bound.
    QTcpServer* createLocalServer()
    {
        // No Qt parent — the unique_ptr is the sole owner. A parent here would
        // risk a double-delete if cleanup() runs before the test object is
        // destroyed (unique_ptr deletes, then Qt's parent-child tree deletes).
        auto server = std::make_unique<QTcpServer>();

        if (!server->listen(QHostAddress::LocalHost, 0) &&
            !server->listen(QHostAddress::LocalHostIPv6, 0) &&
            !server->listen(QHostAddress::Any, 0)) {
            const QString reason = QStringLiteral("Could not start local TCP server: %1").arg(server->errorString());
            qCWarning(UnitTestLog) << reason;
            QTest::qSkip(qPrintable(reason), __FILE__, __LINE__);
            return nullptr;
        }

        _server = server.get();
        _servers.push_back(std::move(server));
        return _server;
    }

    /// Returns the port of the most recently created server.
    quint16 serverPort() const
    {
        return _server ? _server->serverPort() : 0;
    }

    /// Returns an HTTP URL for the server: "http://127.0.0.1:{port}{path}"
    QString serverUrl(const QString& path = QStringLiteral("/")) const
    {
        return QStringLiteral("http://127.0.0.1:%1%2").arg(serverPort()).arg(path);
    }

    /// Installs a simple HTTP responder that sends a fixed body for every request.
    /// @param body Response body bytes
    /// @param statusCode HTTP status code (default 200)
    /// @param contentType Content-Type header value
    void installHttpResponder(const QByteArray& body, int statusCode = 200,
                              const QString& contentType = QStringLiteral("application/octet-stream"))
    {
        if (!_server) {
            // createLocalServer() was never called or failed; be defensive so
            // we don't crash on Q_ASSERT in release-mode test runs.
            QTest::qSkip("installHttpResponder called without a valid server", __FILE__, __LINE__);
            return;
        }
        const QByteArray header = QStringLiteral("HTTP/1.1 %1 OK\r\n"
                                                 "Content-Type: %2\r\n"
                                                 "Content-Length: %3\r\n"
                                                 "\r\n")
                                      .arg(statusCode)
                                      .arg(contentType)
                                      .arg(body.size())
                                      .toUtf8();

        (void)connect(_server, &QTcpServer::newConnection, _server, [this, header, body]() {
            while (_server->hasPendingConnections()) {
                QTcpSocket* socket = _server->nextPendingConnection();
                (void)connect(socket, &QTcpSocket::readyRead, socket, [socket, header, body]() {
                    socket->readAll();  // consume the request
                    socket->write(header + body);
                    socket->flush();
                    socket->disconnectFromHost();
                });
                (void)connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
    }

    /// Waits for a client to connect and returns the socket.
    /// @return The connected socket, or nullptr on timeout
    QTcpSocket* waitForConnection(int timeoutMs = TestTimeout::mediumMs())
    {
        if (!_server) {
            return nullptr;
        }
        bool timedOut = false;
        if (!_server->waitForNewConnection(timeoutMs, &timedOut)) {
            return nullptr;
        }
        return _server->nextPendingConnection();
    }

protected slots:
    void cleanup() override
    {
        _server = nullptr;
        _servers.clear();
        UnitTest::cleanup();
    }

private:
    QTcpServer* _server = nullptr;
    std::vector<std::unique_ptr<QTcpServer>> _servers;
};
