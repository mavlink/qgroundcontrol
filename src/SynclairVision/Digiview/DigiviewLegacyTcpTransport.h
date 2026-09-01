#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QTcpSocket>

#include "DigiviewLegacyTcpAdapter.h"

class DigiviewLegacyTcpTransport : public QObject
{
    Q_OBJECT

public:
    explicit DigiviewLegacyTcpTransport(QObject* parent = nullptr);

    [[nodiscard]] bool connectToEndpoint(const QString& host, quint16 port);
    void disconnectFromEndpoint();
    [[nodiscard]] bool connected() const;
    [[nodiscard]] bool connecting() const;
    [[nodiscard]] bool sendMessage(const mavlink_message_t& message);

signals:
    void connectedToEndpoint();
    void disconnectedFromEndpoint();
    void messageReceived(const mavlink_message_t& message);
    void errorOccurred(const QString& error);

private slots:
    void _readAvailableRecords();
    void _socketErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket _socket;
    QByteArray _receiveBuffer;
    DigiviewLegacyTcpAdapter _adapter;
    bool _disconnectRequested = false;
};
