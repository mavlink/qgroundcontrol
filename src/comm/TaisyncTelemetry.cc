/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TaisyncTelemetry.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#include "VideoManager.h"


QGC_LOGGING_CATEGORY(TaisyncTelemetryLog, "TaisyncTelemetryLog")

//-----------------------------------------------------------------------------
TaisyncTelemetry::TaisyncTelemetry(QObject* parent)
    : QObject (parent)
{
}

//-----------------------------------------------------------------------------
TaisyncTelemetry::~TaisyncTelemetry()
{
    close();
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::close()
{
    qCDebug(TaisyncTelemetryLog) << "Close Taisync Telemetry";
    if(_tcpTelemetrySocket) {
        _tcpTelemetrySocket->close();
        _tcpTelemetrySocket->deleteLater();
        _tcpTelemetrySocket = nullptr;
    }
    if(_udpTelemetrySocket) {
        _udpTelemetrySocket->close();
        _udpTelemetrySocket->deleteLater();
        _udpTelemetrySocket = nullptr;
    }
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::startTelemetry()
{
    qCDebug(TaisyncTelemetryLog) << "Start Taisync Telemetry";
    _udpTelemetrySocket = new QUdpSocket(this);
    _udpTelemetrySocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,    64 * 1024);
    _udpTelemetrySocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 64 * 1024);
    QObject::connect(_udpTelemetrySocket, &QUdpSocket::readyRead, this, &TaisyncTelemetry::_readBytes);
    _udpTelemetrySocket->bind(QHostAddress::LocalHost, 0, QUdpSocket::ShareAddress);
    _tcpTelemetryServer = new QTcpServer(this);
    QObject::connect(_tcpTelemetryServer, &QTcpServer::newConnection, this, &TaisyncTelemetry::_newTelemetryConnection);
    _tcpTelemetryServer->listen(QHostAddress::Any, TAISYNC_USB_TELEM_PORT);
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::_newTelemetryConnection()
{
    qCDebug(TaisyncTelemetryLog) << "New Taisync Temeletry Connection";
    if(_tcpTelemetrySocket) {
        _tcpTelemetrySocket->close();
        _tcpTelemetrySocket->deleteLater();
    }
    _tcpTelemetrySocket = _tcpTelemetryServer->nextPendingConnection();
    QObject::connect(_tcpTelemetrySocket, &QIODevice::readyRead, this, &TaisyncTelemetry::_readTelemetryBytes);
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::_telemetrySocketDisconnected()
{
    qCDebug(TaisyncTelemetryLog) << "Taisync Telemetry Connection Closed";
    if(_tcpTelemetrySocket) {
        _tcpTelemetrySocket->close();
        _tcpTelemetrySocket->deleteLater();
        _tcpTelemetrySocket = nullptr;
    }
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::_readTelemetryBytes()
{
    QByteArray bytesIn = _tcpTelemetrySocket->read(_tcpTelemetrySocket->bytesAvailable());
    _udpTelemetrySocket->writeDatagram(bytesIn, QHostAddress::LocalHost, 14550);
    qCDebug(TaisyncTelemetryLog) << "Taisync telemetry data:" << bytesIn.size();
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::_readBytes()
{
    if (!_udpTelemetrySocket) {
        return;
    }
    while (_udpTelemetrySocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(_udpTelemetrySocket->pendingDatagramSize()));
        _udpTelemetrySocket->readDatagram(datagram.data(), datagram.size());
        if(_tcpTelemetrySocket) {
            _tcpTelemetrySocket->write(datagram);
        }
    }
}
