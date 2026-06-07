#include "LocalHttpTestServer.h"

#include <QtCore/QLoggingCategory>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QHostAddress>

#include "UnitTest.h"

namespace TestFixtures {

LocalHttpTestServer::~LocalHttpTestServer()
{
    close();
}

bool LocalHttpTestServer::listen()
{
    if (_server.listen(QHostAddress::LocalHost, 0) || _server.listen(QHostAddress::LocalHostIPv6, 0) ||
        _server.listen(QHostAddress::Any, 0)) {
        return true;
    }
    qCWarning(UnitTestLog) << "LocalHttpTestServer: could not bind:" << _server.errorString();
    return false;
}

bool LocalHttpTestServer::isListening() const
{
    return _server.isListening();
}

quint16 LocalHttpTestServer::port() const
{
    return _server.isListening() ? _server.serverPort() : 0;
}

QString LocalHttpTestServer::url(const QString& path) const
{
    const QString host = (_server.serverAddress().protocol() == QAbstractSocket::IPv6Protocol)
                             ? QStringLiteral("[::1]")
                             : QStringLiteral("127.0.0.1");
    return QStringLiteral("http://%1:%2%3").arg(host).arg(port()).arg(path);
}

namespace {
QByteArray httpReasonPhrase(int statusCode)
{
    switch (statusCode) {
    case 200: return QByteArrayLiteral("OK");
    case 204: return QByteArrayLiteral("No Content");
    case 206: return QByteArrayLiteral("Partial Content");
    case 304: return QByteArrayLiteral("Not Modified");
    case 400: return QByteArrayLiteral("Bad Request");
    case 404: return QByteArrayLiteral("Not Found");
    case 500: return QByteArrayLiteral("Internal Server Error");
    default: return QByteArrayLiteral("Status");
    }
}
} // namespace

void LocalHttpTestServer::installHttpResponder(const QByteArray& body, int statusCode, const QByteArray& contentType,
                                               int cacheMaxAge)
{
    QByteArray header = QStringLiteral("HTTP/1.1 %1 %2\r\n"
                                       "Content-Type: %3\r\n"
                                       "Connection: close\r\n")
                            .arg(statusCode)
                            .arg(QString::fromLatin1(httpReasonPhrase(statusCode)))
                            .arg(QString::fromLatin1(contentType))
                            .toUtf8();
    if (cacheMaxAge >= 0) {
        header += QStringLiteral("Cache-Control: max-age=%1\r\n").arg(cacheMaxAge).toUtf8();
    }
    header += QByteArrayLiteral("Content-Length: ") + QByteArray::number(body.size()) + QByteArrayLiteral("\r\n\r\n");
    installRawResponder(header + body);
}

void LocalHttpTestServer::installRawResponder(const QByteArray& rawResponse)
{
    QObject::disconnect(&_server, &QTcpServer::newConnection, nullptr, nullptr);
    (void) QObject::connect(&_server, &QTcpServer::newConnection, &_server, [this, rawResponse]() {
        while (_server.hasPendingConnections()) {
            QTcpSocket* const socket = _server.nextPendingConnection();
            (void) QObject::connect(
                socket, &QTcpSocket::readyRead, socket,
                [socket, rawResponse]() {
                    socket->readAll();
                    socket->write(rawResponse);
                    socket->flush();
                    socket->disconnectFromHost();
                },
                Qt::SingleShotConnection);
            (void) QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    });
}

QTcpSocket* LocalHttpTestServer::waitForConnection(int timeoutMs)
{
    if (!_server.isListening()) {
        return nullptr;
    }
    bool timedOut = false;
    if (!_server.waitForNewConnection(timeoutMs, &timedOut)) {
        return nullptr;
    }
    return _server.nextPendingConnection();
}

void LocalHttpTestServer::close()
{
    _server.close();
}

}  // namespace TestFixtures
