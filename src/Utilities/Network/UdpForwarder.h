#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtNetwork/QHostAddress>

Q_DECLARE_LOGGING_CATEGORY(UdpForwarderLog)

class QUdpSocket;

class UdpForwarder : public QObject
{
    Q_OBJECT

public:
    explicit UdpForwarder(QObject* parent = nullptr);
    ~UdpForwarder() override;

    bool configure(const QString& address, quint16 port);
    void forward(const QByteArray& data);
    void stop();

    bool isEnabled() const { return _enabled; }

    QString address() const { return _address.toString(); }

    quint16 port() const { return _port; }

private:
    QUdpSocket* _socket = nullptr;
    QHostAddress _address;
    quint16 _port = 0;
    bool _enabled = false;
};
