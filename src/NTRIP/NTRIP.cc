/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "NTRIP.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "NTRIPSettings.h"

#include <QDebug>

NTRIP::NTRIP(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
}

void NTRIP::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);

    NTRIPSettings* settings = qgcApp()->toolbox()->settingsManager()->ntripSettings();
    if (settings->ntripServerConnectEnabled()->rawValue().toBool()) {
        _tcpLink = new NTRIPTCPLink(settings->ntripServerHostAddress()->rawValue().toString(), settings->ntripServerPort()->rawValue().toInt(), this);
        connect(_tcpLink, &NTRIPTCPLink::error,              this, &NTRIP::_tcpError,           Qt::QueuedConnection);
    }
}


void NTRIP::_tcpError(const QString errorMsg)
{
    qgcApp()->showAppMessage(tr("NTRIP Server Error: %1").arg(errorMsg));
}


NTRIPTCPLink::NTRIPTCPLink(const QString& hostAddress, int port, QObject* parent)
    : QThread       (parent)
    , _hostAddress  (hostAddress)
    , _port         (port)
{
    moveToThread(this);
    start();
}

NTRIPTCPLink::~NTRIPTCPLink(void)
{
    if (_socket) {
        QObject::disconnect(_socket, &QTcpSocket::readyRead, this, &NTRIPTCPLink::_readBytes);
        _socket->disconnectFromHost();
        _socket->deleteLater();
        _socket = nullptr;
    }
    quit();
    wait();
}

void NTRIPTCPLink::run(void)
{
    _hardwareConnect();
    exec();
}

void NTRIPTCPLink::_hardwareConnect()
{
    _socket = new QTcpSocket();

    QObject::connect(_socket, &QTcpSocket::readyRead, this, &NTRIPTCPLink::_readBytes);

    _socket->connectToHost(_hostAddress, static_cast<quint16>(_port));

    // Give the socket a second to connect to the other side otherwise error out
    if (!_socket->waitForConnected(1000)) {
        qCDebug(NTRIPLog) << "NTRIP Socket failed to connect";
        emit error(_socket->errorString());
        delete _socket;
        _socket = nullptr;
        return;
    }

    qCDebug(NTRIPLog) << "NTRIP Socket connected";
}

void NTRIPTCPLink::_parseLine(const QString &line)
{
    
}

void NTRIPTCPLink::_readBytes(void)
{
    if (_socket) {
        QByteArray bytes = _socket->readLine();
        _parseLine(QString::fromLocal8Bit(bytes));
    }
}

