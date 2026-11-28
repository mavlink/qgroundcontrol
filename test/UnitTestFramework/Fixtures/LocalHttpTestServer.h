#pragma once

#include <functional>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

namespace TestFixtures {

class LocalHttpTestServer
{
public:
    LocalHttpTestServer() = default;
    ~LocalHttpTestServer();

    LocalHttpTestServer(const LocalHttpTestServer&) = delete;
    LocalHttpTestServer& operator=(const LocalHttpTestServer&) = delete;
    LocalHttpTestServer(LocalHttpTestServer&&) = delete;
    LocalHttpTestServer& operator=(LocalHttpTestServer&&) = delete;

    bool listen();
    bool isListening() const;
    quint16 port() const;
    QString url(const QString& path = QStringLiteral("/")) const;

    void installHttpResponder(const QByteArray& body, int statusCode = 200,
                              const QByteArray& contentType = "application/octet-stream",
                              int cacheMaxAge = -1);

    void installRawResponder(const QByteArray& rawResponse);
    /// Answers a request for one of the paths in @a files with its content, and any other with 404
    void installFileResponder(const QHash<QString, QByteArray>& files);

    /// Connections that the responder has accepted
    int connectionCount() const { return _connectionCount; }
    QTcpSocket* waitForConnection(int timeoutMs);
    void close();

private:
    /// Answers each request with what @a respond returns for it
    void _installResponder(std::function<QByteArray(const QByteArray& request)> respond);

    QTcpServer _server;
    int _connectionCount = 0;
};

}  // namespace TestFixtures
