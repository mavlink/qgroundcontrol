/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ADSBVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "ADSBVehicleManagerSettings.h"

#include <QDebug>

ADSBVehicleManager::ADSBVehicleManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
}

void ADSBVehicleManager::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);

    connect(&_adsbVehicleCleanupTimer, &QTimer::timeout, this, &ADSBVehicleManager::_cleanupStaleVehicles);
    _adsbVehicleCleanupTimer.setSingleShot(false);
    _adsbVehicleCleanupTimer.start(1000);

    ADSBVehicleManagerSettings* settings = qgcApp()->toolbox()->settingsManager()->adsbVehicleManagerSettings();
    if (settings->adsbServerConnectEnabled()->rawValue().toBool()) {
        _tcpLink = new ADSBTCPLink(settings->adsbServerHostAddress()->rawValue().toString(), settings->adsbServerPort()->rawValue().toInt(), this);
        connect(_tcpLink, &ADSBTCPLink::adsbVehicleUpdate,  this, &ADSBVehicleManager::adsbVehicleUpdate,   Qt::QueuedConnection);
        connect(_tcpLink, &ADSBTCPLink::error,              this, &ADSBVehicleManager::_tcpError,           Qt::QueuedConnection);
    }
}

void ADSBVehicleManager::_cleanupStaleVehicles()
{
    // Remove all expired ADSB vehicles
    for (int i=_adsbVehicles.count()-1; i>=0; i--) {
        ADSBVehicle* adsbVehicle = _adsbVehicles.value<ADSBVehicle*>(i);
        if (adsbVehicle->expired()) {
            qCDebug(ADSBVehicleManagerLog) << "Expired" << QStringLiteral("%1").arg(adsbVehicle->icaoAddress(), 0, 16);
            _adsbVehicles.removeAt(i);
            adsbVehicle->deleteLater();
        }
    }
}

void ADSBVehicleManager::adsbVehicleUpdate(const ADSBVehicle::VehicleInfo_t vehicleInfo)
{
    uint32_t icaoAddress = vehicleInfo.icaoAddress;

    if (_adsbICAOMap.contains(icaoAddress)) {
        _adsbICAOMap[icaoAddress]->update(vehicleInfo);
    } else {
        if (vehicleInfo.availableFlags & ADSBVehicle::LocationAvailable) {
            ADSBVehicle* adsbVehicle = new ADSBVehicle(vehicleInfo, this);
            _adsbICAOMap[icaoAddress] = adsbVehicle;
            _adsbVehicles.append(adsbVehicle);
        }
    }
}

void ADSBVehicleManager::_tcpError(const QString errorMsg)
{
    qgcApp()->showMessage(tr("ADSB Server Error: %1").arg(errorMsg));
}


ADSBTCPLink::ADSBTCPLink(const QString& hostAddress, int port, QObject* parent)
    : QThread       (parent)
    , _hostAddress  (hostAddress)
    , _port         (port)
{
    moveToThread(this);
    start();
}

ADSBTCPLink::~ADSBTCPLink(void)
{
    if (_socket) {
        _socket->disconnectFromHost();
        _socket->deleteLater();
        _socket = nullptr;
    }
    quit();
    wait();
}

void ADSBTCPLink::run(void)
{
    _hardwareConnect();
    exec();
}

void ADSBTCPLink::_hardwareConnect()
{
    _socket = new QTcpSocket();

    QObject::connect(_socket, &QTcpSocket::readyRead, this, &ADSBTCPLink::_readBytes);

    _socket->connectToHost(_hostAddress, static_cast<quint16>(_port));

    // Give the socket a second to connect to the other side otherwise error out
    if (!_socket->waitForConnected(1000)) {
        qCDebug(ADSBVehicleManagerLog) << "ADSB Socket failed to connect";
        emit error(_socket->errorString());
        delete _socket;
        _socket = nullptr;
        return;
    }

    qCDebug(ADSBVehicleManagerLog) << "ADSB Socket connected";
}

void ADSBTCPLink::_readBytes(void)
{
    if (_socket) {
        QByteArray bytes = _socket->readLine();
        _parseLine(QString::fromLocal8Bit(bytes));
    }
}

void ADSBTCPLink::_parseLine(const QString& line)
{
    if (line.startsWith(QStringLiteral("MSG"))) {
        qCDebug(ADSBVehicleManagerLog) << "ADSB SBS-1" << line;

        QStringList values = line.split(QStringLiteral(","));

        if (values[1] == QStringLiteral("3")) {
            bool icaoOk, altOk, latOk, lonOk;

            uint32_t    icaoAddress =   values[4].toUInt(&icaoOk, 16);
            int         modeCAltitude = values[11].toInt(&altOk);
            double      lat =           values[14].toDouble(&latOk);
            double      lon =           values[15].toDouble(&lonOk);
            QString     callsign =      values[10];

            if (!icaoOk || !altOk || !latOk || !lonOk) {
                return;
            }
            if (lat == 0 && lon == 0) {
                return;
            }

            double altitude = modeCAltitude / 3.048;
            QGeoCoordinate location(lat, lon);

            ADSBVehicle::VehicleInfo_t adsbInfo;
            adsbInfo.icaoAddress = icaoAddress;
            adsbInfo.callsign = callsign;
            adsbInfo.location = location;
            adsbInfo.altitude = altitude;
            adsbInfo.availableFlags = ADSBVehicle::CallsignAvailable | ADSBVehicle::LocationAvailable | ADSBVehicle::AltitudeAvailable;
            emit adsbVehicleUpdate(adsbInfo);
        } else if (values[1] == QStringLiteral("4")) {
            bool icaoOk, headingOk;

            uint32_t    icaoAddress =   values[4].toUInt(&icaoOk, 16);
            double      heading =       values[13].toDouble(&headingOk);

            if (!icaoOk || !headingOk) {
                return;
            }

            ADSBVehicle::VehicleInfo_t adsbInfo;
            adsbInfo.icaoAddress = icaoAddress;
            adsbInfo.heading = heading;
            adsbInfo.availableFlags = ADSBVehicle::HeadingAvailable;
            emit adsbVehicleUpdate(adsbInfo);
        } else if (values[1] == QStringLiteral("1")) {
            bool icaoOk;

            uint32_t icaoAddress = values[4].toUInt(&icaoOk, 16);
            if (!icaoOk) {
                return;
            }

            ADSBVehicle::VehicleInfo_t adsbInfo;
            adsbInfo.icaoAddress = icaoAddress;
            adsbInfo.callsign = values[10];
            adsbInfo.availableFlags = ADSBVehicle::CallsignAvailable;
            emit adsbVehicleUpdate(adsbInfo);
        }
    }
}
