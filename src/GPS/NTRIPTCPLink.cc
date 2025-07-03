/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "NTRIPTCPLink.h"
#include "NMEA.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"

#include <QtNetwork/QTcpSocket>

QGC_LOGGING_CATEGORY(NTRIPTCPLinkLog, "qgc.gps.ntriptcplink")

NTRIPTCPLink::NTRIPTCPLink(const QString &hostAddress, int port,
                           const QString &username, const QString &password,
                           const QString &mountpoint, const QString &whitelist,
                           const bool &enableVRS, QObject *parent)
    : QThread(parent)
    , _hostAddress(hostAddress)
    , _port(port)
    , _username(username)
    , _password(password)
    , _mountpoint(mountpoint)
    , _isVRSEnable(enableVRS)
{
    qCDebug(NTRIPTCPLinkLog) << this;

    for (const QString &msg : whitelist.split(',')) {
        const int msg_int = msg.toInt();
        if (msg_int) {
            _whitelist.insert(msg_int);
        }
    }
    qCDebug(NTRIPTCPLinkLog) << "whitelist:" << _whitelist;

    if (!_rtcm_parsing) {
        _rtcm_parsing = new RTCMParsing();
    }
    _rtcm_parsing->reset();
    _state = NTRIPState::uninitialised;

    moveToThread(this);
    start();
}

NTRIPTCPLink::~NTRIPTCPLink()
{
    if (_socket) {
        if (_isVRSEnable) {
            _vrsSendTimer->stop();
            (void) QObject::disconnect(_vrsSendTimer, &QTimer::timeout, this, &NTRIPTCPLink::_sendNmeaGga);
            delete _vrsSendTimer;
            _vrsSendTimer = nullptr;
        }

        (void) QObject::disconnect(_socket, &QTcpSocket::readyRead, this, &NTRIPTCPLink::_readBytes);
        _socket->disconnectFromHost();
        _socket->deleteLater();
        _socket = nullptr;

        delete _rtcm_parsing;
        _rtcm_parsing = nullptr;
    }

    quit();
    wait();

    qCDebug(NTRIPTCPLinkLog) << this;
}

void NTRIPTCPLink::run()
{
    _hardwareConnect();

    if (_isVRSEnable) {
        _vrsSendTimer = new QTimer();
        _vrsSendTimer->setInterval(_vrsSendRateMSecs);
        _vrsSendTimer->setSingleShot(false);
        QObject::connect(_vrsSendTimer, &QTimer::timeout, this, &NTRIPTCPLink::_sendNmeaGga);
        _vrsSendTimer->start();
    }

    exec();
}

void NTRIPTCPLink::_hardwareConnect()
{
    qCDebug(NTRIPTCPLinkLog) << "Connecting to NTRIP Server:" << _hostAddress << ":" << _port;
    _socket = new QTcpSocket();
    QObject::connect(_socket, &QTcpSocket::readyRead, this, &NTRIPTCPLink::_readBytes);
    _socket->connectToHost(_hostAddress, static_cast<quint16>(_port));

    // Give the socket a second to connect to the other side otherwise error out
    if (!_socket->waitForConnected(1000)) {
        qCDebug(NTRIPTCPLinkLog) << "NTRIP Socket failed to connect";
        emit error(_socket->errorString());
        delete _socket;
        _socket = nullptr;
        return;
    }

    // If mountpoint is specified, send an http get request for data
    if (!_mountpoint.isEmpty()) {
        qCDebug(NTRIPTCPLinkLog) << "Sending HTTP request, using mount point:" << _mountpoint;

        QString digest = QString(_username + ":" + _password).toUtf8().toBase64();
        QString auth = QStringLiteral("Authorization: Basic %1\r\n").arg(digest);
        QString query;
        if (_ntripForceV1) {
            query = "GET /%1 HTTP/1.0\r\n"
                    "User-Agent: NTRIP QGroundControl\r\n"
                    "%2"
                    "Connection: close\r\n\r\n";
        } else {
            query = "GET /%1 HTTP/1.1\r\n"
                    "User-Agent: NTRIP QGroundControl\r\n"
                    "Ntrip-Version: Ntrip/2.0\r\n"
                    "%2"
                    "Connection: close\r\n\r\n";
        }

        _socket->write(query.arg(_mountpoint).arg(auth).toUtf8());
        _state = NTRIPState::waiting_for_http_response;
    } else {
        // If no mountpoint is set, assume we will just get data from the tcp stream
        _state = NTRIPState::waiting_for_rtcm_header;
    }

    qCDebug(NTRIPTCPLinkLog) << "NTRIP Socket connected";
}

void NTRIPTCPLink::_parse(const QByteArray &buffer)
{
    qCDebug(NTRIPTCPLinkLog) << "Parsing" << buffer.size() << "bytes";
    qCDebug(NTRIPTCPLinkLog) << "Buffer:" << QString(buffer);

    for (const uint8_t byte : buffer) {
        if (_state == NTRIPState::waiting_for_rtcm_header) {
            if (byte != RTCM3_PREAMBLE && byte != RTCM2_PREAMBLE) {
                qCDebug(NTRIPTCPLinkLog) << "NOT RTCM 2/3 preamble, ignoring byte " << byte;
                continue;
            }
            _state = NTRIPState::accumulating_rtcm_packet;
        }

        if (_rtcm_parsing->addByte(byte)) {
            _state = NTRIPState::waiting_for_rtcm_header;
            QByteArray message((char *)_rtcm_parsing->message(), static_cast<int>(_rtcm_parsing->messageLength()));
            uint16_t id = _rtcm_parsing->messageId();
            uint8_t version = _rtcm_parsing->rtcmVersion();
            qCDebug(NTRIPTCPLinkLog) << "RTCM version" << version;
            qCDebug(NTRIPTCPLinkLog) << "RTCM message ID" << id;
            qCDebug(NTRIPTCPLinkLog) << "RTCM message size" << message.size();

            if (version == 2) {
                qCWarning(NTRIPTCPLinkLog) << "RTCM 2 not supported";
                emit error("Server sent RTCM 2 message. Not supported!");
                continue;
            } else if (version != 3) {
                qCWarning(NTRIPTCPLinkLog) << "Unknown RTCM version" << version;
                emit error("Server sent unknown RTCM version");
                continue;
            }

            if (_whitelist.empty() || _whitelist.contains(id)) {
                qCDebug(NTRIPTCPLinkLog) << "Sending message ID [" << id << "] of size" << message.length();
                emit RTCMDataUpdate(message);
            } else {
                qCDebug(NTRIPTCPLinkLog) << "Ignoring" << id;
            }

            _rtcm_parsing->reset();
        }
    }
}

void NTRIPTCPLink::_readBytes()
{
    if (!_socket) {
        return;
    }

    if (_state == NTRIPState::waiting_for_http_response) {
        QString line = _socket->readLine();
        qCDebug(NTRIPTCPLinkLog) << "Server responded with" << line;
        if (line.contains("200")) {
            qCDebug(NTRIPTCPLinkLog) << "Server responded with" << line;
            if (line.contains("SOURCETABLE")) {
                qCDebug(NTRIPTCPLinkLog) << "Server responded with SOURCETABLE, not supported";
                emit error("NTRIP Server responded with SOURCETABLE. Bad mountpoint?");
                _state = NTRIPState::uninitialised;
            } else {
                _state = NTRIPState::waiting_for_rtcm_header;
            }
        } else if (line.contains("401")) {
            qCWarning(NTRIPTCPLinkLog) << "Server responded with" << line;
            emit error("NTRIP Server responded with 401 Unauthorized");
            _state = NTRIPState::uninitialised;
        } else {
            qCWarning(NTRIPTCPLinkLog) << "Server responded with" << line;
            // TODO: Handle failure. Reconnect?
            // Just move into parsing mode and hope for now.
            _state = NTRIPState::waiting_for_rtcm_header;
        }
    }

    if (_state == NTRIPState::uninitialised) {
        qCDebug(NTRIPTCPLinkLog) << "NTRIP State is uninitialised. Discarding bytes";
        _socket->readAll();
        return;
    }

    const QByteArray bytes = _socket->readAll();
    _parse(bytes);
}

void NTRIPTCPLink::_sendNmeaGga()
{
    if (!MultiVehicleManager::instance()->activeVehicleAvailable()) {
        qCDebug(NTRIPTCPLinkLog) << "No active vehicle";
        return;
    }
    qCDebug(NTRIPTCPLinkLog) << "Active vehicle found. Using vehicle position";

    if (MultiVehicleManager::instance()->vehicles()->count() > 1) {
        // TODO(bzd): Consider how to handle multiple vehicles - best option would be
        // send separate RTCM messages for each vehicle or at least
        // calculate the average position of all vehicles and use one RTCM for all
        qCDebug(NTRIPTCPLinkLog) << "More than one vehicle found. Using active vehicle for correction";
    }

    const Vehicle *vehicle = MultiVehicleManager::instance()->activeVehicle();
    const NMEAMessage nmeaMessage(vehicle->coordinate());
    // Write NMEA message
    if (_socket) {
        const QString &gga = nmeaMessage.getGGA();
        _socket->write(gga.toUtf8());
        qCDebug(NTRIPTCPLinkLog) << "NMEA GGA Message:" << gga;
    }
}
