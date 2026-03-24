#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslKey>

class QSslSocket;
class QUdpSocket;

class RemoteTransport : public QObject
{
    Q_OBJECT

public:
    explicit RemoteTransport(QObject *parent = nullptr) : QObject(parent) {}

    virtual bool send(const QByteArray &data) = 0;
    virtual void close() = 0;
    virtual bool isConnected() const = 0;

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &message);
};

class UdpTransport : public RemoteTransport
{
    Q_OBJECT

public:
    explicit UdpTransport(QObject *parent = nullptr);

    void setTarget(const QString &host, quint16 port);

    bool send(const QByteArray &data) override;
    void close() override;
    bool isConnected() const override { return true; }

    int failureCount() const { return _failureCount; }
    void resetFailureCount() { _failureCount = 0; }

private:
    QUdpSocket *_socket = nullptr;
    QString _host;
    quint16 _port = 0;
    int _failureCount = 0;

    static constexpr int kMaxDatagramSize = 8192;
};

class TcpTransport : public RemoteTransport
{
    Q_OBJECT

public:
    explicit TcpTransport(QObject *parent = nullptr);

    void setTarget(const QString &host, quint16 port);
    void setTlsEnabled(bool enabled);
    void setTlsVerifyPeer(bool verify);
    void setTlsCaCertificates(const QList<QSslCertificate> &certs);
    void setTlsClientCertificate(const QSslCertificate &cert, const QSslKey &key);

    bool send(const QByteArray &data) override;
    void close() override;
    bool isConnected() const override { return _connected; }

    void connectToHost();

private slots:
    void _onConnected();
    void _onDisconnected();
    void _onError(QAbstractSocket::SocketError error);
    void _onSslErrors(const QList<QSslError> &errors);

private:
    void _createSocket();

    QSslSocket *_socket = nullptr;
    QString _host;
    quint16 _port = 0;
    bool _connected = false;
    bool _tlsEnabled = false;
    bool _tlsVerifyPeer = true;

    QList<QSslCertificate> _caCertificates;
    QSslCertificate _clientCert;
    QSslKey _clientKey;
};
