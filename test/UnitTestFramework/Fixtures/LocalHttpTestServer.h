#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

/// @file
/// @brief Composable RAII fixture for a local HTTP test server

namespace TestFixtures {

/// RAII fixture owning a QTcpServer bound on an ephemeral loopback port.
///
/// Usage:
/// @code
///   LocalHttpTestServer server;
///   QVERIFY(server.listen());
///   server.installHttpResponder("hello");
///   QString u = server.url("/test.txt");
/// @endcode
///
/// The server closes automatically in the destructor — no teardown needed.
class LocalHttpTestServer
{
public:
    LocalHttpTestServer() = default;
    ~LocalHttpTestServer();

    LocalHttpTestServer(const LocalHttpTestServer&) = delete;
    LocalHttpTestServer& operator=(const LocalHttpTestServer&) = delete;
    LocalHttpTestServer(LocalHttpTestServer&&) = delete;
    LocalHttpTestServer& operator=(LocalHttpTestServer&&) = delete;

    /// Binds on an ephemeral loopback port (tries LocalHost, LocalHostIPv6, Any).
    /// @return true on success; false if no address could be bound.
    bool listen();

    /// @return true once listen() succeeded.
    bool isListening() const;

    /// @return The bound port, or 0 if not listening.
    quint16 port() const;

    /// @return "http://127.0.0.1:{port}{path}"
    QString url(const QString& path = QStringLiteral("/")) const;

    /// Installs a responder that auto-replies to every incoming request.
    ///
    /// @param body           Response body bytes.
    /// @param statusCode     HTTP status code (default 200).
    /// @param contentType    Content-Type header value.
    /// @param cacheMaxAge    If >= 0, adds Cache-Control and Connection: close headers.
    void installHttpResponder(const QByteArray& body, int statusCode = 200,
                              const QByteArray& contentType = "application/octet-stream",
                              int cacheMaxAge = -1);

    /// Installs a responder that sends a fully pre-built response for every request.
    void installRawResponder(const QByteArray& rawResponse);

    /// Waits for a client to connect and returns the socket (not owned by the caller).
    /// @return The connected socket, or nullptr on timeout.
    QTcpSocket* waitForConnection(int timeoutMs);

    /// Closes the server immediately (also called by the destructor).
    void close();

private:
    QTcpServer _server;
};

}  // namespace TestFixtures
