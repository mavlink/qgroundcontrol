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
            qCDebug(ADSBVehicleManagerLog) << "Expired " << QStringLiteral("%1").arg(adsbVehicle->icaoAddress(), 0, 16);
            _adsbVehicles.removeAt(i);
            _adsbICAOMap.remove(adsbVehicle->icaoAddress());
            adsbVehicle->deleteLater();
        }
    }
}

void ADSBVehicleManager::adsbVehicleUpdate(const ADSBVehicle::ADSBVehicleInfo_t vehicleInfo)
{
    uint32_t icaoAddress = vehicleInfo.icaoAddress;

    if (_adsbICAOMap.contains(icaoAddress)) {
        _adsbICAOMap[icaoAddress]->update(vehicleInfo);
    } else {
        if (vehicleInfo.availableFlags & ADSBVehicle::LocationAvailable) {
            ADSBVehicle* adsbVehicle = new ADSBVehicle(vehicleInfo, this);
            _adsbICAOMap[icaoAddress] = adsbVehicle;
            _adsbVehicles.append(adsbVehicle);
            qCDebug(ADSBVehicleManagerLog) << "Added " << QStringLiteral("%1").arg(adsbVehicle->icaoAddress(), 0, 16);
        }
    }
}

void ADSBVehicleManager::_tcpError(const QString errorMsg)
{
    qgcApp()->showAppMessage(tr("ADSB Server Error: %1").arg(errorMsg));
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
        QObject::disconnect(_socket, &QTcpSocket::readyRead, this, &ADSBTCPLink::_readBytes);
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
        while(_socket->canReadLine()) {
            QByteArray bytes = _socket->readLine();
            _parseLine(QString::fromLocal8Bit(bytes));
        }
    }
}

void ADSBTCPLink::_parseLine(const QString &line)
{
    if (line.startsWith(QStringLiteral("MSG"))) {
        bool icaoOk;
        int msgType = line.at(4).digitValue();
        if (msgType == -1) {
            qCDebug(ADSBVehicleManagerLog) << "ADSB Invalid message type " << line.at(4);
            return;
        }
        // Skip unsupported mesg types to avoid parsing
        if (msgType == 2 || msgType > 6) {
            return;
        }
        qCDebug(ADSBVehicleManagerLog) << " ADSB SBS-1 " << line;
        QStringList values = line.split(QChar(','));
        uint32_t icaoAddress = values[4].toUInt(&icaoOk, 16);

        if (!icaoOk) {
            return;
        }

        ADSBVehicle::ADSBVehicleInfo_t adsbInfo;
        adsbInfo.icaoAddress = icaoAddress;

        switch (msgType) {
        case 1:
        case 5:
        case 6:
            _parseAndEmitCallsign(adsbInfo, values);
            break;
        case 3:
            _parseAndEmitLocation(adsbInfo, values);
            break;
        case 4:
            _parseAndEmitHeading(adsbInfo, values);
            break;
        }
    }
}

void ADSBTCPLink::_parseAndEmitCallsign(ADSBVehicle::ADSBVehicleInfo_t &adsbInfo, QStringList values)
{
    QString callsign = values[10].trimmed();
    if (callsign.isEmpty()) {
        return;
    }

    adsbInfo.callsign = callsign;
    adsbInfo.availableFlags = ADSBVehicle::CallsignAvailable;
    emit adsbVehicleUpdate(adsbInfo);
}

void ADSBTCPLink::_parseAndEmitLocation(ADSBVehicle::ADSBVehicleInfo_t &adsbInfo, QStringList values)
{
    bool altOk, latOk, lonOk;
    int modeCAltitude;

    QString altitudeStr = values[11];
    // Altitude is either Barometric - based on pressure, in ft
    // or HAE - as reported by GPS - based on WGS84 Ellipsoid, in ft
    // If altitude ends with H, we have HAE
    // There's a slight difference between Barometric alt and HAE, but it would require
    // knowledge about Geoid shape in particular Lat, Lon. It's not worth complicating the code
    if (altitudeStr.endsWith('H')) {
        altitudeStr.chop(1);
    }
    modeCAltitude = altitudeStr.toInt(&altOk);

    double lat = values[14].toDouble(&latOk);
    double lon = values[15].toDouble(&lonOk);
    int alert = values[19].toInt();

    if (!altOk || !latOk || !lonOk) {
        return;
    }
    if (lat == 0 && lon == 0) {
        return;
    }

    double altitude = modeCAltitude * 0.3048;
    QGeoCoordinate location(lat, lon);
    adsbInfo.location = location;
    adsbInfo.altitude = altitude;
    adsbInfo.alert = alert == 1;
    adsbInfo.availableFlags = ADSBVehicle::LocationAvailable | ADSBVehicle::AltitudeAvailable | ADSBVehicle::AlertAvailable;
    emit adsbVehicleUpdate(adsbInfo);
}

void ADSBTCPLink::_parseAndEmitHeading(ADSBVehicle::ADSBVehicleInfo_t &adsbInfo, QStringList values)
{
    bool headingOk;
    double heading = values[13].toDouble(&headingOk);
    if (!headingOk) {
        return;
    }

    adsbInfo.heading = heading;
    adsbInfo.availableFlags = ADSBVehicle::HeadingAvailable;
    emit adsbVehicleUpdate(adsbInfo);
}