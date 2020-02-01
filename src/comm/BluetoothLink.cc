/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Definition of Bluetooth connection for unmanned vehicles
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#include <QtGlobal>
#include <QTimer>
#include <QList>
#include <QDebug>
#include <iostream>

#include <QtBluetooth/QBluetoothDeviceDiscoveryAgent>
#include <QtBluetooth/QBluetoothLocalDevice>
#include <QtBluetooth/QBluetoothUuid>
#include <QtBluetooth/QBluetoothSocket>

#include "QGCApplication.h"
#include "BluetoothLink.h"
#include "QGC.h"

BluetoothLink::BluetoothLink(SharedLinkConfigurationPointer& config)
    : LinkInterface(config)
    , _config(qobject_cast<BluetoothConfiguration*>(config.data()))
    , _connectState(false)
    , _targetSocket(nullptr)
#ifdef __ios__
    , _discoveryAgent(nullptr)
#endif
    , _shutDown(false)
{

}

BluetoothLink::~BluetoothLink()
{
    _disconnect();
#ifdef __ios__
    if(_discoveryAgent) {
        _shutDown = true;
        _discoveryAgent->stop();
        _discoveryAgent->deleteLater();
        _discoveryAgent = nullptr;
    }
#endif
}

void BluetoothLink::run()
{
}

void BluetoothLink::_restartConnection()
{
    if(this->isConnected())
    {
        _disconnect();
        _connect();
    }
}

QString BluetoothLink::getName() const
{
    return _config->name();
}

void BluetoothLink::_writeBytes(const QByteArray bytes)
{
    if (_targetSocket) {
        if(_targetSocket->write(bytes) > 0) {
            emit bytesSent(this, bytes);
            _logOutputDataRate(bytes.size(), QDateTime::currentMSecsSinceEpoch());
        } else {
            qWarning() << "Bluetooth write error";
        }
    }
}

void BluetoothLink::readBytes()
{
    if (_targetSocket) {
        while (_targetSocket->bytesAvailable() > 0) {
            QByteArray datagram;
            datagram.resize(_targetSocket->bytesAvailable());
            _targetSocket->read(datagram.data(), datagram.size());
            emit bytesReceived(this, datagram);
            _logInputDataRate(datagram.length(), QDateTime::currentMSecsSinceEpoch());
        }
    }
}

void BluetoothLink::_disconnect(void)
{
#ifdef __ios__
    if(_discoveryAgent) {
        _shutDown = true;
        _discoveryAgent->stop();
        _discoveryAgent->deleteLater();
        _discoveryAgent = nullptr;
    }
#endif
    if(_targetSocket)
    {
        _targetSocket->deleteLater();
        _targetSocket = nullptr;
        emit disconnected();
    }
    _connectState = false;
}

bool BluetoothLink::_connect(void)
{
    _hardwareConnect();
    return true;
}

bool BluetoothLink::_hardwareConnect()
{
#ifdef __ios__
    if(_discoveryAgent) {
        _shutDown = true;
        _discoveryAgent->stop();
        _discoveryAgent->deleteLater();
        _discoveryAgent = nullptr;
    }
    _discoveryAgent = new QBluetoothServiceDiscoveryAgent(this);
    QObject::connect(_discoveryAgent, &QBluetoothServiceDiscoveryAgent::serviceDiscovered, this, &BluetoothLink::serviceDiscovered);
    QObject::connect(_discoveryAgent, &QBluetoothServiceDiscoveryAgent::finished, this, &BluetoothLink::discoveryFinished);
    QObject::connect(_discoveryAgent, &QBluetoothServiceDiscoveryAgent::canceled, this, &BluetoothLink::discoveryFinished);
    _shutDown = false;
    _discoveryAgent->start();
#else
    _createSocket();
    _targetSocket->connectToService(QBluetoothAddress(_config->device().address), QBluetoothUuid(QBluetoothUuid::SerialPort));
#endif
    return true;
}

void BluetoothLink::_createSocket()
{
    if(_targetSocket)
    {
        delete _targetSocket;
        _targetSocket = nullptr;
    }
    _targetSocket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);
    QObject::connect(_targetSocket, &QBluetoothSocket::connected, this, &BluetoothLink::deviceConnected);

    QObject::connect(_targetSocket, &QBluetoothSocket::readyRead, this, &BluetoothLink::readBytes);
    QObject::connect(_targetSocket, &QBluetoothSocket::disconnected, this, &BluetoothLink::deviceDisconnected);

    QObject::connect(_targetSocket, static_cast<void (QBluetoothSocket::*)(QBluetoothSocket::SocketError)>(&QBluetoothSocket::error),
            this, &BluetoothLink::deviceError);
}

#ifdef __ios__
void BluetoothLink::serviceDiscovered(const QBluetoothServiceInfo& info)
{
    if(!info.device().name().isEmpty() && !_targetSocket)
    {
        if(_config->device().uuid == info.device().deviceUuid() && _config->device().name == info.device().name())
        {
            _createSocket();
            _targetSocket->connectToService(info);
        }
    }
}
#endif

#ifdef __ios__
void BluetoothLink::discoveryFinished()
{
    if(_discoveryAgent && !_shutDown)
    {
        _shutDown = true;
        _discoveryAgent->deleteLater();
        _discoveryAgent = nullptr;
        if(!_targetSocket)
        {
            _connectState = false;
            emit communicationError("Could not locate Bluetooth device:", _config->device().name);
        }
    }
}
#endif

void BluetoothLink::deviceConnected()
{
    _connectState = true;
    emit connected();
}

void BluetoothLink::deviceDisconnected()
{
    _connectState = false;
    qWarning() << "Bluetooth disconnected";
}

void BluetoothLink::deviceError(QBluetoothSocket::SocketError error)
{
    _connectState = false;
    qWarning() << "Bluetooth error" << error;
    emit communicationError(tr("Bluetooth Link Error"), _targetSocket->errorString());
}

bool BluetoothLink::isConnected() const
{
    return _connectState;
}

qint64 BluetoothLink::getConnectionSpeed() const
{
    return 1000000; // 1 Mbit
}

qint64 BluetoothLink::getCurrentInDataRate() const
{
    return 0;
}

qint64 BluetoothLink::getCurrentOutDataRate() const
{
    return 0;
}

//--------------------------------------------------------------------------
//-- BluetoothConfiguration

BluetoothConfiguration::BluetoothConfiguration(const QString& name)
    : LinkConfiguration(name)
    , _deviceDiscover(nullptr)
{

}

BluetoothConfiguration::BluetoothConfiguration(BluetoothConfiguration* source)
    : LinkConfiguration(source)
    , _deviceDiscover(nullptr)
    , _device(source->device())
{
}

BluetoothConfiguration::~BluetoothConfiguration()
{
    if(_deviceDiscover)
    {
        _deviceDiscover->stop();
        delete _deviceDiscover;
    }
}

QString BluetoothConfiguration::settingsTitle()
{
    if(qgcApp()->toolbox()->linkManager()->isBluetoothAvailable()) {
        return tr("Bluetooth Link Settings");
    } else {
        return tr("Bluetooth Not Available");
    }
}

void BluetoothConfiguration::copyFrom(LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);
    auto* usource = qobject_cast<BluetoothConfiguration*>(source);
    Q_ASSERT(usource != nullptr);
    _device = usource->device();
}

void BluetoothConfiguration::saveSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    settings.setValue("deviceName", _device.name);
#ifdef __ios__
    settings.setValue("uuid", _device.uuid.toString());
#else
    settings.setValue("address",_device.address);
#endif
    settings.endGroup();
}

void BluetoothConfiguration::loadSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    _device.name    = settings.value("deviceName", _device.name).toString();
#ifdef __ios__
    QString suuid   = settings.value("uuid", _device.uuid.toString()).toString();
    _device.uuid    = QUuid(suuid);
#else
    _device.address = settings.value("address", _device.address).toString();
#endif
    settings.endGroup();
}

void BluetoothConfiguration::updateSettings()
{
    if(_link) {
        auto* ulink = qobject_cast<BluetoothLink*>(_link);
        if(ulink) {
            ulink->_restartConnection();
        }
    }
}

void BluetoothConfiguration::stopScan()
{
    if(_deviceDiscover)
    {
        _deviceDiscover->stop();
        _deviceDiscover->deleteLater();
        _deviceDiscover = nullptr;
        emit scanningChanged();
    }
}

void BluetoothConfiguration::startScan()
{
    if(!_deviceDiscover)
    {
        _deviceDiscover = new QBluetoothDeviceDiscoveryAgent(this);
        connect(_deviceDiscover, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,  this, &BluetoothConfiguration::deviceDiscovered);
        connect(_deviceDiscover, &QBluetoothDeviceDiscoveryAgent::finished,          this, &BluetoothConfiguration::doneScanning);
        emit scanningChanged();
    }
    else
    {
        _deviceDiscover->stop();
    }
    _nameList.clear();
    _deviceList.clear();
    emit nameListChanged();
    _deviceDiscover->setInquiryType(QBluetoothDeviceDiscoveryAgent::GeneralUnlimitedInquiry);
    _deviceDiscover->start();
}

void BluetoothConfiguration::deviceDiscovered(QBluetoothDeviceInfo info)
{
    if(!info.name().isEmpty() && info.isValid())
    {
#if 0
        qDebug() << "Name:           " << info.name();
        qDebug() << "Address:        " << info.address().toString();
        qDebug() << "Service Classes:" << info.serviceClasses();
        QList<QBluetoothUuid> uuids = info.serviceUuids();
        for (QBluetoothUuid uuid: uuids) {
            qDebug() << "Service UUID:   " << uuid.toString();
        }
#endif
        BluetoothData data;
        data.name    = info.name();
#ifdef __ios__
        data.uuid    = info.deviceUuid();
#else
        data.address = info.address().toString();
#endif
        if(!_deviceList.contains(data))
        {
            _deviceList += data;
            _nameList   += data.name;
            emit nameListChanged();
            return;
        }
    }
}

void BluetoothConfiguration::doneScanning()
{
    if(_deviceDiscover)
    {
        _deviceDiscover->deleteLater();
        _deviceDiscover = nullptr;
        emit scanningChanged();
    }
}

void BluetoothConfiguration::setDevName(const QString &name)
{
    for(const BluetoothData& data: _deviceList)
    {
        if(data.name == name)
        {
            _device = data;
            emit devNameChanged();
#ifndef __ios__
            emit addressChanged();
#endif
            return;
        }
    }
}

QString BluetoothConfiguration::address()
{
#ifdef __ios__
    return {};
#else
    return _device.address;
#endif
}
