/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ADSBTCPLink.h"
#include <QGCLoggingCategory.h>

#include <QtNetwork/QTcpSocket>
#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(ADSBTCPLinkLog, "qgc.adsb.adsbtcplink")

ADSBTCPLink::ADSBTCPLink(const QString &hostAddress, quint16 port, QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_processTimer(new QTimer(this))
{
    // qCDebug(ADSBTCPLinkLog) << Q_FUNC_INFO << this;

    (void) connect(m_socket, &QTcpSocket::stateChanged, this, [this](QTcpSocket::SocketState state) {
        switch (state) {
            case QTcpSocket::SocketState::UnconnectedState:
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

            default:
                break;
        }
    }, Qt::AutoConnection);

    (void) QObject::connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QTcpSocket::SocketError error) {
        qCDebug(ADSBTCPLinkLog) << error << m_socket->errorString();
        emit errorOccurred(m_socket->errorString());
    }, Qt::AutoConnection);

    (void) connect(m_socket, &QTcpSocket::readyRead, this, &ADSBTCPLink::_readBytes);

    m_processTimer->setInterval(s_processInterval); // Set an interval for processing lines
    (void) connect(m_processTimer, &QTimer::timeout, this, &ADSBTCPLink::_processLines);

    m_socket->connectToHost(hostAddress, port);
}

ADSBTCPLink::~ADSBTCPLink()
{
    // qCDebug(ADSBTCPLinkLog) << Q_FUNC_INFO << this;
}

void ADSBTCPLink::_readBytes()
{
    while (m_socket && m_socket->canReadLine()) {
        const QByteArray bytes = m_socket->readLine();
        m_lineBuffer.append(QString::fromLocal8Bit(bytes));
    }

    // Start or restart the timer to process lines
    if (!m_processTimer->isActive()) {
        m_processTimer->start();
    }
}

void ADSBTCPLink::_processLines()
{
    int linesProcessed = 0;
    while (!m_lineBuffer.isEmpty() && (linesProcessed < s_maxLinesToProcess)) {
        const QString line = m_lineBuffer.takeFirst();
        _parseLine(line);
        ++linesProcessed;
    }

    // Stop the timer if there are no more lines to process
    if (m_lineBuffer.isEmpty()) {
        m_processTimer->stop();
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
    if (msgType == ADSB::ADSBMessageType::Unsupported) {
        qCDebug(ADSBTCPLinkLog) << "ADSB Invalid message type " << msgType;
        return;
    }

    // Skip unsupported mesg types to avoid parsing
    if ((msgType == ADSB::ADSBMessageType::SurfacePosition) || (msgType > ADSB::ADSBMessageType::SurveillanceId)) {
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

    ADSBVehicle::ADSBVehicleInfo_t adsbInfo;
    adsbInfo.icaoAddress = icaoAddress;

    switch (msgType) {
        case ADSB::ADSBMessageType::IdentificationAndCategory:
        case ADSB::ADSBMessageType::SurveillanceAltitude:
        case ADSB::ADSBMessageType::SurveillanceId:
            _parseAndEmitCallsign(adsbInfo, values);
            break;

        case ADSB::ADSBMessageType::AirbornePosition:
            _parseAndEmitLocation(adsbInfo, values);
            break;

        case ADSB::ADSBMessageType::AirborneVelocity:
            _parseAndEmitHeading(adsbInfo, values);
            break;

        default:
            break;
    }
}

void ADSBTCPLink::_parseAndEmitCallsign(ADSBVehicle::ADSBVehicleInfo_t &adsbInfo, const QStringList &values)
{
    if (values.size() <= 10) {
        return;
    }

    const QString callsign = values.at(10).trimmed();
    if (callsign.isEmpty()) {
        return;
    }

    adsbInfo.callsign = callsign;
    adsbInfo.availableFlags = ADSBVehicle::CallsignAvailable;

    emit adsbVehicleUpdate(adsbInfo);
}

void ADSBTCPLink::_parseAndEmitLocation(ADSBVehicle::ADSBVehicleInfo_t &adsbInfo, const QStringList &values)
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
        altitudeStr.chop(1);
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
    adsbInfo.availableFlags = ADSBVehicle::LocationAvailable | ADSBVehicle::AltitudeAvailable | ADSBVehicle::AlertAvailable;

    emit adsbVehicleUpdate(adsbInfo);
}

void ADSBTCPLink::_parseAndEmitHeading(ADSBVehicle::ADSBVehicleInfo_t &adsbInfo, const QStringList &values)
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
    adsbInfo.availableFlags = ADSBVehicle::HeadingAvailable;

    emit adsbVehicleUpdate(adsbInfo);
}
