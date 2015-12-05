/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Definition of Bluetooth connection for unmanned vehicles
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include <QtGlobal>
#include <QTimer>
#include <QList>
#include <QDebug>
#include <iostream>

#include <QtBluetooth/QBluetoothSocket>
#include <QtBluetooth/QBluetoothDeviceDiscoveryAgent>
#include <QtBluetooth/QBluetoothLocalDevice>
#include <QtBluetooth/QBluetoothUuid>

#include "BluetoothLink.h"
#include "QGC.h"

/*
static void print_device_info(QBluetoothDeviceInfo info)
{
    qDebug() << "Bluetooth:      " << info.name();
    qDebug() << "       Services:" << info.serviceClasses();
    qDebug() << "   Major Classs:" << info.majorDeviceClass();
    qDebug() << "   Minor Classs:" << info.minorDeviceClass();
    qDebug() << "           RSSI:" << info.rssi();
    qDebug() << "           UUID:" << info.deviceUuid();
    qDebug() << "   Service UUID:" << info.serviceUuids();
    qDebug() << "    Core Config:" << info.coreConfigurations();
    qDebug() << "          Valid:" << info.isValid();
    qDebug() << "        Address:" << info.address().toString();
}
*/

BluetoothLink::BluetoothLink(BluetoothConfiguration* config)
    : _connectState(false)
    , _targetSocket(NULL)
    , _targetDevice(NULL)
    , _running(false)
{
    Q_ASSERT(config != NULL);
    _config = config;
    _config->setLink(this);
    // We're doing it wrong - because the Qt folks got the API wrong:
    // http://blog.qt.digia.com/blog/2010/06/17/youre-doing-it-wrong/
    moveToThread(this);
}

BluetoothLink::~BluetoothLink()
{
    // Disconnect link from configuration
    _config->setLink(NULL);
    _disconnect();
    // Tell the thread to exit
    _running = false;
    quit();
    // Wait for it to exit
    wait();
    this->deleteLater();
}

void BluetoothLink::run()
{
    if(_running && _hardwareConnect()) {
        exec();
    }
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

void BluetoothLink::writeBytes(const char* data, qint64 size)
{
    _sendBytes(data, size);
}

void BluetoothLink::_sendBytes(const char* data, qint64 size)
{
    if(_targetSocket)
    {
        if(_targetSocket->isWritable())
        {
            if(_targetSocket->write(data, size) > 0) {
                _logOutputDataRate(size, QDateTime::currentMSecsSinceEpoch());
            }
            else
                qWarning() << "Bluetooth write error";
        }
        else
            qWarning() << "Bluetooth not writable error";
    }
}

void BluetoothLink::readBytes()
{
    while (_targetSocket->bytesAvailable() > 0)
    {
        QByteArray datagram;
        datagram.resize(_targetSocket->bytesAvailable());
        _targetSocket->read(datagram.data(), datagram.size());
        emit bytesReceived(this, datagram);
        _logInputDataRate(datagram.length(), QDateTime::currentMSecsSinceEpoch());
    }
}

void BluetoothLink::_disconnect(void)
{
    _running = false;
    quit();
    wait();
    if(_targetDevice)
    {
        delete _targetDevice;
        _targetDevice = NULL;
    }
    if(_targetSocket)
    {
        _targetSocket->deleteLater();
        _targetSocket = NULL;
        emit disconnected();
    }
    _connectState = false;
}

bool BluetoothLink::_connect(void)
{
    if(this->isRunning() || _running)
    {
        _running = false;
        quit();
        wait();
    }
    if(_targetDevice)
    {
        delete _targetDevice;
        _targetDevice = NULL;
    }
    //-- Start Thread
    _running = true;
    start(NormalPriority);
    return true;
}

bool BluetoothLink::_hardwareConnect()
{
    if(_targetSocket)
    {
        delete _targetSocket;
        _targetSocket = NULL;
    }
    _targetDevice = new QBluetoothDeviceInfo(QBluetoothAddress(_config->device().address), _config->device().name, _config->device().bits);
    _targetDevice->setCoreConfigurations(QBluetoothDeviceInfo::BaseRateCoreConfiguration);
    _targetSocket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);
    _targetSocket->moveToThread(this);
    //print_device_info(*_targetDevice);
    QObject::connect(_targetSocket, SIGNAL(connected()), this, SLOT(deviceConnected()));
    QObject::connect(_targetSocket, SIGNAL(error(QBluetoothSocket::SocketError)), this, SLOT(deviceError(QBluetoothSocket::SocketError)));
    QObject::connect(_targetSocket, SIGNAL(readyRead()), this, SLOT(readBytes()));
    QObject::connect(_targetSocket, SIGNAL(disconnected()), this, SLOT(deviceDisconnected()));
    _targetSocket->connectToService(_targetDevice->address(), QBluetoothUuid(QBluetoothUuid::Rfcomm));
    return true;
}

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
    emit communicationError("Bluetooth Link Error", _targetSocket->errorString());
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
    , _deviceDiscover(NULL)
{

}

BluetoothConfiguration::BluetoothConfiguration(BluetoothConfiguration* source)
    : LinkConfiguration(source)
    , _deviceDiscover(NULL)
{
    _device = source->device();
}

BluetoothConfiguration::~BluetoothConfiguration()
{
    if(_deviceDiscover)
    {
        _deviceDiscover->stop();
        delete _deviceDiscover;
    }
}

void BluetoothConfiguration::copyFrom(LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);
    BluetoothConfiguration* usource = dynamic_cast<BluetoothConfiguration*>(source);
    Q_ASSERT(usource != NULL);
    _device = usource->device();
}

void BluetoothConfiguration::saveSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    settings.setValue("name",    _device.name);
    settings.setValue("address", _device.address);
    settings.setValue("bits",    _device.bits);
    settings.endGroup();
}

void BluetoothConfiguration::loadSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    _device.name    = settings.value("name",    _device.name).toString();
    _device.address = settings.value("address", _device.address).toString();
    _device.bits    = settings.value("bits",    _device.bits).toUInt();
    settings.endGroup();
}

void BluetoothConfiguration::updateSettings()
{
    if(_link) {
        BluetoothLink* ulink = dynamic_cast<BluetoothLink*>(_link);
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
        _deviceDiscover = NULL;
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
    //print_device_info(info);
    if(!info.name().isEmpty() && info.isValid())
    {
        BluetoothData data;
        data.name    = info.name();
        data.address = info.address().toString();
        data.bits |= ((qint32)info.serviceClasses()   << 13);   // Service Class
        data.bits |= ((qint32)info.majorDeviceClass() << 8);    // CLASS MAJOR
        data.bits |= ((qint32)info.minorDeviceClass() << 2);    // CLASS MINOR
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
        _deviceDiscover = NULL;
        emit scanningChanged();
    }
}

void BluetoothConfiguration::setDevName(const QString &name)
{
    foreach(const BluetoothData& data, _deviceList)
    {
        if(data.name == name)
        {
            _device = data;
            emit devNameChanged();
            emit addressChanged();
            return;
        }
    }
}

