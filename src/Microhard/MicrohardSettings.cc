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
MicrohardSettings::MicrohardSettings(QString address_, QObject* parent, bool setEncryptionKey)
    : MicrohardHandler(parent)
{
    _address = address_;
    _setEncryptionKey = setEncryptionKey;
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::start()
{
    qCDebug(MicrohardLog) << "Start Microhard Settings";
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
MicrohardSettings::setEncryptionKey(QString key)
{
    if (!_tcpSocket) {
        return;
    }
    QString cmd = "AT+MWVENCRYPT=1," + key + "\n";
    _tcpSocket->write(cmd.toStdString().c_str());
    cmd = "AT&W\n";
    _tcpSocket->write(cmd.toStdString().c_str());

    qCDebug(MicrohardLog) << "Set encryption key: " << key;
}

//-----------------------------------------------------------------------------
void
MicrohardSettings::_readBytes()
{
    if (!_tcpSocket) {
        return;
    }
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
        if (!loggedIn() && _setEncryptionKey) {
            qgcApp()->toolbox()->microhardManager()->setEncryptionKey();
        }
        _loggedIn = true;
        emit connected(1);
    }

    emit rssiUpdated(_rssiVal);
}

