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
#ifdef QGC_ENABLE_PAIRING
#include "PairingManager.h"
#endif
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
    if (_statusTimer) {
        _statusTimer->stop();
        _statusTimer->deleteLater();
        _statusTimer = nullptr;
    }
    if (_configureTimer) {
        _configureTimer->stop();
        _configureTimer ->deleteLater();
        _configureTimer = nullptr;
    }
    _close();
}

//-----------------------------------------------------------------------------
void
MicrohardSettings::run()
{
    _loggedIn = false;
    _statusTimer = new QTimer();
    connect(_statusTimer, SIGNAL(timeout()), this, SLOT(_getStatus()));
    _configureTimer = new QTimer();
    connect(_configureTimer, SIGNAL(timeout()), this, SLOT(_configureTimeout()));
    _start();
    _statusTimer->start(2500);
}

//-----------------------------------------------------------------------------
void
MicrohardSettings::_getStatus()
{
    if (!_tcpSocket || !_loggedIn || _configurationRunning) {
        return;
    }
    _tcpSocket->write("AT+MWSTATUS\n");
}

//-----------------------------------------------------------------------------
void
MicrohardSettings::_configureTimeout()
{
    qCDebug(MicrohardLog) << "Microhard configuration timeout.";
    _configureAfterConnect = true;
    _start();
}

//-----------------------------------------------------------------------------
void
MicrohardSettings::configure(QString key, int power, int channel, int bandwidth, QString networkId)
{
    if (!_tcpSocket || !_loggedIn) {
        _configureAfterConnect = true;
        return;
    }

    if (power > 0) {
        _writeList.append("AT+MWTXPOWER=" + QString::number(power) + "\n");
    }
    // Some modems fail to set bandwith if current channel is out of range. Channel 15 is available on all models.
    _writeList.append("AT+MWFREQ=15\n");
    _writeList.append("AT+MWBAND=" + QString::number(bandwidth) + "\n");
    _writeList.append("AT+MWFREQ=" + QString::number(channel) + "\n");
    _writeList.append(key.isEmpty() ? "AT+MWVENCRYPT=0\n" : "AT+MWVENCRYPT=" + _encryptionType + "," + key + "\n");
    if (!networkId.isEmpty()) {
        _writeList.append("AT+MWNETWORKID=" + networkId + "\n");
    }
    _writeList.append("AT&W\n");

    _configurationRunning = true;
    _tcpSocket->write(_writeList.first().toStdString().c_str());
    _writeList.removeFirst();
    _configureTimer->start(10000);

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
    bool clear = true;
    bool runConfig = false;
    _readData.append(_tcpSocket->readAll());

    if (_readData.contains("login:")) {
        std::string userName = qgcApp()->toolbox()->microhardManager()->configUserName().toStdString() + "\n";
        _tcpSocket->write(userName.c_str());
    } else if (_readData.contains("Password:") && !_readData.contains("Encryption")) {
        std::string pwd = qgcApp()->toolbox()->microhardManager()->configPassword().toStdString() + "\n";
        _tcpSocket->write(pwd.c_str());
    } else if (_readData.contains("Login incorrect")) {
        emit connected(tr("Login Error"));
    } else if (_readData.contains("Entering")) {
        if (!_configure) {
            _loggedIn = true;
            emit connected(tr("Connected"));
        } else {
            _tcpSocket->write("at+mssysi\n");
        }
    } else if (_readData.contains("OK")) {
        if ((j = _readData.indexOf("Product")) > 0) {
            int i = _readData.indexOf(": ", j);
            if (i > 0) {
                QString product = _readData.mid(i + 2, _readData.indexOf("\r", i + 3) - (i + 2));
                qgcApp()->toolbox()->microhardManager()->setProductName(product);
            }
            _tcpSocket->write("at+mwvencrypt\n");
        }
        // +MWVENCRYPT: Virtual Interface:
        //Encryption Type: 1 - AES-128
        if ((j = _readData.indexOf("Encryption Type:")) > 0) {
            int i = _readData.indexOf(": ", j);
            if (i > 0) {
                _encryptionType = _readData.mid(i + 2, _readData.indexOf(" - ", i + 3) - (i + 2));
                if (_encryptionType == "0") {
                    _encryptionType = "1";
                }
                qCDebug(MicrohardLog) << "Encryption type: " << _encryptionType;
            }
            if (!_loggedIn && _configure && _configureAfterConnect) {
                runConfig = true;
            }
            _loggedIn = true;
            emit connected(tr("Connected"));
        }
        if ((j = _readData.indexOf("RSSI (dBm)")) > 0) {
            int i2 = _readData.indexOf(": ", j);
            if (i2 > 0) {
                i2 += 2;
                int i3 = _readData.indexOf(" ", i2);
                int val = _readData.mid(i2, i3 - i2).toInt();
                if (val < 0 && _rssiVal != val) {
                    _rssiVal = val;
                    emit rssiUpdated(_rssiVal);
                }
            }
            _readData.clear();
            clear = false;
        }
        if (_readData.contains("Restarting the services")) {
            _configureTimer->stop();
            _configurationRunning = false;
            qCDebug(MicrohardLog) << "Microhard configuration succeeded.";
        }

        if (!_writeList.empty()) {
            _tcpSocket->write(_writeList.first().toStdString().c_str());
            _writeList.removeFirst();
        }
    } else {
        clear = false;
    }

    if (clear) {
//        auto list = _readData.replace("\r", "").split('\n');
//        for (auto i = list.begin(); i != list.end(); i++)
//            qCDebug(MicrohardLog) << "MH: " << *i;

        _readData.clear();
    }

    if (runConfig) {
        _configureAfterConnect = false;
        qgcApp()->toolbox()->microhardManager()->configure();
    }
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
    if (_configureTimer) {
        _configureTimer->stop();
    }
    _loggedIn = false;
    _configurationRunning = false;
    _writeList.clear();
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
    _configureAfterConnect = _configurationRunning;
    _start();
}

//-----------------------------------------------------------------------------
