/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ADSBTCPLink.h"
// #include "DeviceInfo.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QTimer>
#include <QtNetwork/QTcpSocket>

QGC_LOGGING_CATEGORY(ADSBTCPLinkLog, "qgc.adsb.adsbtcplink")

ADSBTCPLink::ADSBTCPLink(const QHostAddress &hostAddress, quint16 port, QObject *parent)
    : QObject(parent)
    , _hostAddress(hostAddress)
    , _port(port)
    , _socket(new QTcpSocket(this))
    , _processTimer(new QTimer(this))
{
#ifdef QT_DEBUG
    (void) connect(_socket, &QTcpSocket::stateChanged, this, [](QTcpSocket::SocketState state) {
        switch (state) {
        case QTcpSocket::UnconnectedState:
            qCDebug(ADSBTCPLinkLog) << "ADSB Socket disconnected";
            break;
        case QTcpSocket::SocketState::ConnectingState:
            qCDebug(ADSBTCPLinkLog) << "ADSB Socket connecting...";
            break;
        case QTcpSocket::SocketState::ConnectedState:
            qCDebug(ADSBTCPLinkLog) << "ADSB Socket connected";
            break;
        case QTcpSocket::SocketState::ClosingState:
            qCDebug(ADSBTCPLinkLog) << "ADSB Socket closing...";
            break;
        default:
            break;
        }
    }, Qt::AutoConnection);
#endif

    (void) QObject::connect(_socket, &QTcpSocket::errorOccurred, this, [this](QTcpSocket::SocketError error) {
        qCDebug(ADSBTCPLinkLog) << error << _socket->errorString();
        // TODO: Check if it is a critical error or not and send if the socket is stopped/recoverable
        emit errorOccurred(_socket->errorString(), false);
    }, Qt::AutoConnection);

    (void) connect(_socket, &QTcpSocket::readyRead, this, &ADSBTCPLink::_readBytes);

    _processTimer->setInterval(_processInterval); // Set an interval for processing lines
    (void) connect(_processTimer, &QTimer::timeout, this, &ADSBTCPLink::_processLines);

    init();

    // qCDebug(ADSBTCPLinkLog) << Q_FUNC_INFO << this;
}

ADSBTCPLink::~ADSBTCPLink()
{
    // qCDebug(ADSBTCPLinkLog) << Q_FUNC_INFO << this;
}

bool ADSBTCPLink::init()
{
    // TODO: Check Address Target & Internet Availability
    // QGCDeviceInfo::isInternetAvailable()

    if (_hostAddress.isNull()) {
        return false;
    }

    _socket->connectToHost(_hostAddress, _port);

    return true;
}

void ADSBTCPLink::_readBytes()
{
    while (_socket && _socket->canReadLine()) {
        const QByteArray bytes = _socket->readLine();
        (void) _lineBuffer.append(QString::fromLocal8Bit(bytes));
    }

    // Start or restart the timer to process lines
    if (!_processTimer->isActive()) {
        _processTimer->start();
    }
}

void ADSBTCPLink::_processLines()
{
    int linesProcessed = 0;
    while (!_lineBuffer.isEmpty() && (linesProcessed < _maxLinesToProcess)) {
        const QString line = _lineBuffer.takeFirst();
        _parseLine(line);
        ++linesProcessed;
    }

    // Stop the timer if there are no more lines to process
    if (_lineBuffer.isEmpty()) {
        _processTimer->stop();
    }
}

void ADSBTCPLink::_parseLine(const QString &line)
{
    if (line.size() <= 4) {
        return;
    }

    if (!line.startsWith(QStringLiteral("MSG"))) {
        return;
    }

    const int msgType = line.at(4).digitValue();
    if (msgType == ADSB::Unsupported) {
        qCDebug(ADSBTCPLinkLog) << "ADSB Invalid message type" << msgType;
        return;
    }

    // Skip unsupported mesg types to avoid parsing
    if ((msgType == ADSB::SurfacePosition) || (msgType > ADSB::SurveillanceId)) {
        return;
    }

    qCDebug(ADSBTCPLinkLog) << "ADSB SBS-1" << line;

    const QStringList values = line.split(QChar(','));
    if (values.size() <= 4) {
        return;
    }

    bool icaoOk;
    const uint32_t icaoAddress = values.at(4).toUInt(&icaoOk, 16);
    if (!icaoOk) {
        return;
    }

    ADSB::VehicleInfo_t adsbInfo;
    adsbInfo.icaoAddress = icaoAddress;

    switch (msgType) {
    case ADSB::IdentificationAndCategory:
    case ADSB::SurveillanceAltitude:
    case ADSB::SurveillanceId:
        _parseAndEmitCallsign(adsbInfo, values);
        break;
    case ADSB::AirbornePosition:
        _parseAndEmitLocation(adsbInfo, values);
        break;
    case ADSB::AirborneVelocity:
        _parseAndEmitHeading(adsbInfo, values);
        break;
    default:
        break;
    }
}

void ADSBTCPLink::_parseAndEmitCallsign(ADSB::VehicleInfo_t &adsbInfo, const QStringList &values)
{
    if (values.size() <= 10) {
        return;
    }

    const QString callsign = values.at(10).trimmed();
    if (callsign.isEmpty()) {
        return;
    }

    adsbInfo.callsign = callsign;
    adsbInfo.availableFlags = ADSB::CallsignAvailable;

    emit adsbVehicleUpdate(adsbInfo);
}

void ADSBTCPLink::_parseAndEmitLocation(ADSB::VehicleInfo_t &adsbInfo, const QStringList &values)
{
    if (values.size() <= 19) {
        return;
    }

    // Altitude is either Barometric - based on pressure, in ft
    // or HAE - as reported by GPS - based on WGS84 Ellipsoid, in ft
    // If altitude ends with H, we have HAE
    // There's a slight difference between Barometric alt and HAE, but it would require
    // knowledge about Geoid shape in particular Lat, Lon. It's not worth complicating the code
    QString altitudeStr = values.at(11);
    if (altitudeStr.endsWith('H')) {
        (void) altitudeStr.chop(1);
    }

    bool altOk, latOk, lonOk, alertOk;
    const int modeCAltitude = altitudeStr.toInt(&altOk);
    const double lat = values.at(14).toDouble(&latOk);
    const double lon = values.at(15).toDouble(&lonOk);
    const int alert = values.at(19).toInt(&alertOk);

    if (!altOk || !latOk || !lonOk || !alertOk) {
        return;
    }

    if (qFuzzyIsNull(lat) && qFuzzyIsNull(lon)) {
        return;
    }

    const double altitude = modeCAltitude * 0.3048;
    const QGeoCoordinate location(lat, lon);

    adsbInfo.location = location;
    adsbInfo.altitude = altitude;
    adsbInfo.alert = (alert == 1);
    adsbInfo.availableFlags = ADSB::LocationAvailable | ADSB::AltitudeAvailable | ADSB::AlertAvailable;

    emit adsbVehicleUpdate(adsbInfo);
}

void ADSBTCPLink::_parseAndEmitHeading(ADSB::VehicleInfo_t &adsbInfo, const QStringList &values)
{
    if (values.size() <= 13) {
        return;
    }

    bool headingOk;
    const double heading = values.at(13).toDouble(&headingOk);
    if (!headingOk) {
        return;
    }

    adsbInfo.heading = heading;
    adsbInfo.availableFlags = ADSB::HeadingAvailable;

    emit adsbVehicleUpdate(adsbInfo);
}
