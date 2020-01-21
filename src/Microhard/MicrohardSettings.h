/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCLoggingCategory.h"
#include <QHostAddress>
#include <QTcpSocket>
#include <QTimer>

#define MICROHARD_SETTINGS_PORT   23

Q_DECLARE_LOGGING_CATEGORY(MicrohardLog)

class MicrohardSettings : public QObject
{
    Q_OBJECT
public:
    explicit MicrohardSettings(QString address, bool configure = false);
    ~MicrohardSettings();

public slots:
    void run();
    void configure(QString key, int power, int channel, int bandwidth, QString networkId);

private slots:
    void _readBytes();
    void _stateChanged(QAbstractSocket::SocketState socketState);
    void _socketError(QAbstractSocket::SocketError socketError);
    void _getStatus();
    void _configureTimeout();

signals:
    void connected(QString status);
    void rssiUpdated(int rssi);

private:
    bool _loggedIn;
    bool _configurationRunning = false;
    int _rssiVal;
    QString _address;
    bool _configure;
    bool _configureAfterConnect = false;
    QTcpSocket* _tcpSocket = nullptr;
    QTimer* _statusTimer = nullptr;
    QTimer* _configureTimer = nullptr;
    QByteArray _readData;
    QList<QString> _writeList;
    QString _encryptionType = "1";

    void _start();
    void _close();
};
