#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

Q_DECLARE_LOGGING_CATEGORY(UdpForwarderLog)

class UdpForwarder : public QObject
{
    Q_OBJECT

public:
    explicit UdpForwarder(QObject* parent = nullptr);

    bool configure(const QString& address, quint16 port);
    /// Returns the bytes sent. A send that fails is logged once until one succeeds again.
    qint64 forward(const QByteArray& data);
    void stop();

    bool isEnabled() const { return _port != 0; }

    QString address() const { return _address.toString(); }

    quint16 port() const { return _port; }

    /// The socket's reason the last datagram was not sent; empty once one is sent, and after configure() and stop().
    QString errorString() const { return _errorString; }

private:
    QUdpSocket _socket{this};
    QHostAddress _address;
    quint16 _port = 0;
    QString _errorString;
};
