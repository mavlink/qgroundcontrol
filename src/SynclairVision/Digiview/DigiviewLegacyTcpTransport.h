#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QTcpSocket>
#include <QtCore/QTimer>

#include "DigiviewLegacyTcpAdapter.h"

class DigiviewLegacyTcpTransport : public QObject
{
    Q_OBJECT

public:
    explicit DigiviewLegacyTcpTransport(QObject* parent = nullptr);

    [[nodiscard]] bool connectToEndpoint(const QString& host, quint16 port);
    void disconnectFromEndpoint();
    void parkConnection();
    [[nodiscard]] bool connected() const;
    [[nodiscard]] bool connecting() const;
    [[nodiscard]] bool sendMessage(const mavlink_message_t& message);
    [[nodiscard]] bool restartDigiView(const QString& host, quint16 port, quint64 generation = 0);
    void cancelRestartDigiView();

signals:
    void connectedToEndpoint();
    void disconnectedFromEndpoint();
    void messageReceived(const mavlink_message_t& message);
    void errorOccurred(const QString& error);
    void restartQuitSent(quint64 generation);
    void restartFailed(quint64 generation, const QString& error);

private slots:
    void _readAvailableRecords();
    void _socketErrorOccurred(QAbstractSocket::SocketError socketError);
    void _restartSocketConnected();
    void _restartSocketDisconnected();
    void _restartSocketErrorOccurred(QAbstractSocket::SocketError socketError);
    void _restartSocketBytesWritten(qint64 bytes);
    void _restartRetry();

private:
    QTcpSocket _socket;
    QByteArray _receiveBuffer;
    DigiviewLegacyTcpAdapter _adapter;
    bool _disconnectRequested = false;
    bool _parked = false;
    QTcpSocket _restartSocket;
    QTimer _restartTimer;
    QString _restartHost;
    quint16 _restartPort = 0;
    int _restartAttempts = 0;
    bool _restartInProgress = false;
    bool _restartQuitSent = false;
    bool _restartConnectingAttempt = false;
    QByteArray _restartRecord;
    qsizetype _restartWriteOffset = 0;
    quint64 _restartGeneration = 0;
    QString _restartLastError;
};
