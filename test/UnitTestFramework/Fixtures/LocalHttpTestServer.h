#pragma once

#include <QtCore/QByteArray>
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
    QTcpSocket* waitForConnection(int timeoutMs);
    void close();

private:
    QTcpServer _server;
};

}  // namespace TestFixtures
