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

//-----------------------------------------------------------------------------
MicrohardSettings::MicrohardSettings(QString address_, QObject* parent, bool configure)
    : MicrohardHandler(parent)
{
    _address = address_;
    _configure = configure;
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::start()
{
    _loggedIn = false;
    _start(MICROHARD_SETTINGS_PORT, QHostAddress(_address));
    return true;
}

//-----------------------------------------------------------------------------
void
MicrohardSettings::getStatus()
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

    QString cmd = "AT+MWTXPOWER=" + QString::number(power) + "\n";
    cmd += "AT+MWFREQ=" + QString::number(channel) + "\n";
    cmd += "AT+MWBAND=" + QString::number(bandwidth) + "\n";
    cmd += key.isEmpty() ? "AT+MWVENCRYPT=0\n" : "AT+MWVENCRYPT=1," + key + "\n";
    if (!networkId.isEmpty()) {
        cmd +="AT+MWNETWORKID=" + networkId.toUpper() + "\n";
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
        emit connected(-1);
    } else if (bytesIn.contains("Entering")) {
        if (!_configure) {
            _loggedIn = true;
            emit connected(1);
        } else {
            _tcpSocket->write("at+mssysi\n");
        }
    } else if ((j = bytesIn.indexOf("Product")) > 0) {
        int i = bytesIn.indexOf(": ", j);
        if (i > 0) {
            QString product = bytesIn.mid(i + 2, bytesIn.indexOf("\r", i + 3) - (i + 2));
            qgcApp()->toolbox()->microhardManager()->setProductName(product);
        }
        if (!loggedIn() && (_configure || _configureAfterConnect)) {
            _configureAfterConnect = false;
            qgcApp()->toolbox()->microhardManager()->configure();
        }
        _loggedIn = true;
        emit connected(1);
    }

    emit rssiUpdated(_rssiVal);
}

