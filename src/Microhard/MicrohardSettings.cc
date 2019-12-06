/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MicrohardSettings.h"
#include "MicrohardManager.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#include "VideoManager.h"

QGC_LOGGING_CATEGORY(MicrohardLog, "MicrohardLog")

//-----------------------------------------------------------------------------
MicrohardSettings::MicrohardSettings(QString address_, bool configure)
    : QObject()
{
    _address = address_;
    _configure = configure;
}

//-----------------------------------------------------------------------------
MicrohardSettings::~MicrohardSettings()
{
    _statusTimer->stop();
    _statusTimer->deleteLater();
    _statusTimer = nullptr;
    _close();
}

//-----------------------------------------------------------------------------
void
MicrohardSettings::run()
{
    _loggedIn = false;
    _statusTimer = new QTimer();
    connect(_statusTimer, SIGNAL(timeout()), this, SLOT(_getStatus()));
    _statusTimer->start(2500);
    _start();
}

//-----------------------------------------------------------------------------
void
MicrohardSettings::_getStatus()
{
    if (_loggedIn && _tcpSocket) {
        _tcpSocket->write("AT+MWSTATUS\n");
    }
}

//-----------------------------------------------------------------------------
void
MicrohardSettings::configure(QString key, int power, int channel, int bandwidth, QString networkId)
{
    if (!_tcpSocket) {
        _configureAfterConnect = true;
        return;
    }
    QString cmd;
    if (power > 0) {
        cmd += "AT+MWTXPOWER=" + QString::number(power) + "\n";
    }
    cmd += "AT+MWFREQ=" + QString::number(channel) + "\n";
    cmd += "AT+MWBAND=" + QString::number(bandwidth) + "\n";
    cmd += key.isEmpty() ? "AT+MWVENCRYPT=0\n" : "AT+MWVENCRYPT=1," + key + "\n";
    if (!networkId.isEmpty()) {
        cmd +="AT+MWNETWORKID=" + networkId + "\n";
    }
    cmd += "AT&W\n";
    _tcpSocket->write(cmd.toStdString().c_str());

    qCDebug(MicrohardLog) << "Configure key: " << key << " power: " << power << " channel: "
                          << channel << " bandwidth: " << bandwidth
                          << (!networkId.isEmpty() ? " network ID: " + networkId : "");
}

//-----------------------------------------------------------------------------
void
MicrohardSettings::_readBytes()
{
    if (!_tcpSocket) {
        return;
    }
    int j;
    QByteArray bytesIn = _tcpSocket->read(_tcpSocket->bytesAvailable());

    //qCDebug(MicrohardLog) << "Read bytes: " << bytesIn;

    if (_loggedIn) {
        int i1 = bytesIn.indexOf("RSSI (dBm)");
        if (i1 > 0) {
            int i2 = bytesIn.indexOf(": ", i1);
            if (i2 > 0) {
                i2 += 2;
                int i3 = bytesIn.indexOf(" ", i2);
                int val = bytesIn.mid(i2, i3 - i2).toInt();
                if (val < 0) {
                    _rssiVal = val;
                }
            }
        }
    } else if (bytesIn.contains("login:")) {
        std::string userName = qgcApp()->toolbox()->microhardManager()->configUserName().toStdString() + "\n";
        _tcpSocket->write(userName.c_str());
    } else if (bytesIn.contains("Password:")) {
        std::string pwd = qgcApp()->toolbox()->microhardManager()->configPassword().toStdString() + "\n";
        _tcpSocket->write(pwd.c_str());
    } else if (bytesIn.contains("Login incorrect")) {
        emit connected(tr("Login Error"));
    } else if (bytesIn.contains("Entering")) {
        if (!_configure) {
            _loggedIn = true;
            emit connected(tr("Connected"));
        } else {
            _tcpSocket->write("at+mssysi\n");
        }
    } else if ((j = bytesIn.indexOf("Product")) > 0) {
        int i = bytesIn.indexOf(": ", j);
        if (i > 0) {
            QString product = bytesIn.mid(i + 2, bytesIn.indexOf("\r", i + 3) - (i + 2));
            qgcApp()->toolbox()->microhardManager()->setProductName(product);
        }
        if (!_loggedIn && (_configure || _configureAfterConnect)) {
            _configureAfterConnect = false;
            qgcApp()->toolbox()->microhardManager()->configure();
        }
        _loggedIn = true;
        emit connected(tr("Connected"));
    }

    emit rssiUpdated(_rssiVal);
}

//-----------------------------------------------------------------------------
void
MicrohardSettings::_start()
{
    _close();
    _tcpSocket = new QTcpSocket();
    connect(_tcpSocket, &QIODevice::readyRead, this, &MicrohardSettings::_readBytes, Qt::QueuedConnection);
    connect(_tcpSocket, &QTcpSocket::stateChanged, this, &MicrohardSettings::_stateChanged, Qt::QueuedConnection);
    connect(_tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &MicrohardSettings::_socketError, Qt::QueuedConnection);
    _tcpSocket->connectToHost(QHostAddress(_address), MICROHARD_SETTINGS_PORT);
}

//-----------------------------------------------------------------------------
void
MicrohardSettings::_close()
{
    if (_tcpSocket) {
        disconnect(_tcpSocket, &QIODevice::readyRead, this, &MicrohardSettings::_readBytes);
        disconnect(_tcpSocket, &QTcpSocket::stateChanged, this, &MicrohardSettings::_stateChanged);
        disconnect(_tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &MicrohardSettings::_socketError);
        _tcpSocket->close();
        _tcpSocket->deleteLater();
        _tcpSocket = nullptr;
    }
}

//-----------------------------------------------------------------------------
void
MicrohardSettings::_stateChanged(QAbstractSocket::SocketState socketState)
{
    if (socketState != QAbstractSocket::ConnectedState) {
        _loggedIn = false;
        emit connected(tr("Not Connected"));
    }
}

//-----------------------------------------------------------------------------
void
MicrohardSettings::_socketError(QAbstractSocket::SocketError socketError)
{
    qCDebug(MicrohardLog) << "Socket error: " << socketError;
    emit connected(tr("Not Connected"));
    _start();
}

//-----------------------------------------------------------------------------
