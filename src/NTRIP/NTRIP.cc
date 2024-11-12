/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "NTRIP.h"
#include "QGCLoggingCategory.h"
#include "QGCToolbox.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "PositionManager.h"
#include "NTRIPSettings.h"

QGC_LOGGING_CATEGORY(NTRIPLog, "NTRIP")

NTRIP::NTRIP(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
}

void NTRIP::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);

    NTRIPSettings* settings = qgcApp()->toolbox()->settingsManager()->ntripSettings();
    if (settings->ntripServerConnectEnabled()->rawValue().toBool()) {
        qCDebug(NTRIPLog) << settings->ntripEnableVRS()->rawValue().toBool();
        _rtcmMavlink = new RTCMMavlink(*toolbox);

        _tcpLink = new NTRIPTCPLink(settings->ntripServerHostAddress()->rawValue().toString(),
                                    settings->ntripServerPort()->rawValue().toInt(),
                                    settings->ntripUsername()->rawValue().toString(),
                                    settings->ntripPassword()->rawValue().toString(),
                                    settings->ntripMountpoint()->rawValue().toString(),
                                    settings->ntripWhitelist()->rawValue().toString(),
                                    settings->ntripEnableVRS()->rawValue().toBool());
        connect(_tcpLink, &NTRIPTCPLink::error,              this, &NTRIP::_tcpError,           Qt::QueuedConnection);
        connect(_tcpLink, &NTRIPTCPLink::RTCMDataUpdate,   _rtcmMavlink, &RTCMMavlink::RTCMDataUpdate);
    } else {
        qCDebug(NTRIPLog) << "NTRIP Server is not enabled";
    }
}


void NTRIP::_tcpError(const QString errorMsg)
{
    qgcApp()->showAppMessage(tr("NTRIP Server Error: %1").arg(errorMsg));
}


NTRIPTCPLink::NTRIPTCPLink(const QString& hostAddress,
                           int port,
                           const QString &username,
                           const QString &password,
                           const QString &mountpoint,
                           const QString &whitelist,
                           const bool    &enableVRS)
    : QThread       ()
    , _hostAddress  (hostAddress)
    , _port         (port)
    , _username     (username)
    , _password     (password)
    , _mountpoint   (mountpoint)
    , _isVRSEnable  (enableVRS)
    , _toolbox      (qgcApp()->toolbox())
{
    for(const auto& msg: whitelist.split(',')) {
        int msg_int = msg.toInt();
        if(msg_int) {
            _whitelist.insert(msg_int);
        }
    }
    qCDebug(NTRIPLog) << "whitelist: " << _whitelist;
    if (!_rtcm_parsing) {
        _rtcm_parsing = new RTCMParsing();
    }
    _rtcm_parsing->reset();
    _state = NTRIPState::uninitialised;

    // Start TCP Socket
    moveToThread(this);
    start();
}

NTRIPTCPLink::~NTRIPTCPLink(void)
{
  qCDebug(NTRIPLog) << "NTRIP Thread stopped";
    if (_socket) {
        if(_isVRSEnable) {
            _vrsSendTimer->stop();
            QObject::disconnect(_vrsSendTimer, &QTimer::timeout, this, &NTRIPTCPLink::_sendNMEA);
            delete _vrsSendTimer;
            _vrsSendTimer = nullptr;
        }
        QObject::disconnect(_socket, &QTcpSocket::readyRead, this, &NTRIPTCPLink::_readBytes);
        _socket->disconnectFromHost();
        _socket->deleteLater();
        _socket = nullptr;

        // Delete Rtcm Parsing instance
        delete(_rtcm_parsing);
        _rtcm_parsing = nullptr;
    }
    quit();
    wait();
}

void NTRIPTCPLink::run(void)
{
  qCDebug(NTRIPLog) << "NTRIP Thread started";
    _hardwareConnect();

    // Init VRS Timer
    if(_isVRSEnable) {
        _vrsSendTimer = new QTimer();
        _vrsSendTimer->setInterval(_vrsSendRateMSecs);
        _vrsSendTimer->setSingleShot(false);
        QObject::connect(_vrsSendTimer, &QTimer::timeout, this, &NTRIPTCPLink::_sendNMEA);
        _vrsSendTimer->start();
    }

    exec();
}

void NTRIPTCPLink::_hardwareConnect()
{
  qCDebug(NTRIPLog) << "Connecting to NTRIP Server: " << _hostAddress << ":" << _port;
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

    // If mountpoint is specified, send an http get request for data
    if ( !_mountpoint.isEmpty()) {
        qCDebug(NTRIPLog) << "Sending HTTP request, using mount point: " << _mountpoint;
        // TODO(zdanek) Add support for NTRIP v2
        QString digest = QString(_username + ":"  + _password).toUtf8().toBase64();
        QString auth = QString("Authorization: Basic %1\r\n").arg(digest);
        QString query = "GET /%1 HTTP/1.0\r\n"
                        "User-Agent: NTRIP QGroundControl\r\n"
                        "%2"
                        "Connection: close\r\n\r\n";

        _socket->write(query.arg(_mountpoint).arg(auth).toUtf8());
        _state = NTRIPState::waiting_for_http_response;
    } else {
        // If no mountpoint is set, assume we will just get data from the tcp stream
        _state = NTRIPState::waiting_for_rtcm_header;
    }

    qCDebug(NTRIPLog) << "NTRIP Socket connected";
}

void NTRIPTCPLink::_parse(const QByteArray &buffer) {
  qCDebug(NTRIPLog) << "Parsing " << buffer.size() << " bytes";
  for (const uint8_t byte : buffer) {
    if (_state == NTRIPState::waiting_for_rtcm_header) {
      if (byte != RTCM3_PREAMBLE && byte != RTCM2_PREAMBLE) {
        qCDebug(NTRIPLog) << "NOT RTCM 2/3 preamble, ignoring byte " << byte;
        continue;
      }
      _state = NTRIPState::accumulating_rtcm_packet;
    }

    if (_rtcm_parsing->addByte(byte)) {
      _state = NTRIPState::waiting_for_rtcm_header;
      QByteArray message((char *)_rtcm_parsing->message(),
                         static_cast<int>(_rtcm_parsing->messageLength()));
      uint16_t id = _rtcm_parsing->messageId();
      uint8_t version = _rtcm_parsing->rtcmVersion();
      qCDebug(NTRIPLog) << "RTCM version " << version;
      qCDebug(NTRIPLog) << "RTCM message ID " << id;
      qCDebug(NTRIPLog) << "RTCM message size " << message.size();

      if (version == 2) {
        qCWarning(NTRIPLog) << "RTCM 2 not supported";
        emit error("Server sent RTCM 2 message. Not supported!");
        continue;
      } else if (version != 3) {
        qCWarning(NTRIPLog) << "Unknown RTCM version " << version;
        emit error("Server sent unknown RTCM version");
        continue;
      }

      //      uint16_t id = ((uint8_t)message[3] << 4) | ((uint8_t)message[4] >> 4);
      if (_whitelist.empty() || _whitelist.contains(id)) {
        qCDebug(NTRIPLog) << "Sending message ID [" << id << "] of size " << message.length();
        emit RTCMDataUpdate(message);
      } else {
        qCDebug(NTRIPLog) << "Ignoring " << id;
      }
      _rtcm_parsing->reset();
    }
  }
}

void NTRIPTCPLink::_readBytes(void)
{
  qCDebug(NTRIPLog) << "Reading bytes";
    if (!_socket) {
        return;
    }
    if(_state == NTRIPState::waiting_for_http_response) {

      //Content-Type: gnss/sourcetable
      // Content-Type: gnss/sourcetable

      QString line = _socket->readLine();
      qCDebug(NTRIPLog) << "Server responded with " << line;
        if (line.contains("200")){
            qCDebug(NTRIPLog) << "Server responded with " << line;
            if (line.contains("SOURCETABLE")) {
                qCDebug(NTRIPLog) << "Server responded with SOURCETABLE, not supported";
                emit error("NTRIP Server responded with SOURCETABLE. Bad mountpoint?");
                _state = NTRIPState::uninitialised;
            } else {
                _state = NTRIPState::waiting_for_rtcm_header;
            }
        } else if (line.contains("401")) {
            qCWarning(NTRIPLog) << "Server responded with " << line;
            emit error("NTRIP Server responded with 401 Unauthorized");
            _state = NTRIPState::uninitialised;
        } else {
            qCWarning(NTRIPLog) << "Server responded with " << line;
            // TODO: Handle failure. Reconnect?
            // Just move into parsing mode and hope for now.
            _state = NTRIPState::waiting_for_rtcm_header;
        }
    }


//    throw new Exception("Got SOURCETABLE - Bad ntrip mount point\n\n" + line);

    if (_state == NTRIPState::uninitialised) {
      qCDebug(NTRIPLog) << "NTRIP State is uninitialised. Discarding bytes";
      _socket->readAll();
      return;
    }

    QByteArray bytes = _socket->readAll();
    _parse(bytes);
}

void NTRIPTCPLink::_sendNMEA() {
  // TODO(zdanek) check if this is proper NMEA message
  qCDebug(NTRIPLog) << "Sending NMEA";
  QGeoCoordinate gcsPosition = _toolbox->qgcPositionManager()->gcsPosition();

  if (!gcsPosition.isValid()) {
    qCDebug(NTRIPLog) << "GCS Position is not valid";
    if (_takePosFromVehicle) {
      Vehicle *vehicle = _toolbox->multiVehicleManager()->activeVehicle();
      if (vehicle) {
        qCDebug(NTRIPLog) << "Active vehicle found. Using vehicle position";
        gcsPosition = vehicle->coordinate();
      } else {
        qCDebug(NTRIPLog) << "No active vehicle";
        return;
      }
    } else {
      qCDebug(NTRIPLog) << "No valid GCS position";
      return;
    }
  }

  double lat = gcsPosition.latitude();
    double lng = gcsPosition.longitude();
    double alt = gcsPosition.altitude();

    qCDebug(NTRIPLog) << "lat : " << lat << " lon : " << lng << " alt : " << alt;

    QString time = QDateTime::currentDateTimeUtc().toString("hhmmss.zzz");

    if(lat != 0 || lng != 0) {
        double latdms = (int) lat + (lat - (int) lat) * .6f;
        double lngdms = (int) lng + (lng - (int) lng) * .6f;
        if(isnan(alt)) alt = 0.0;

        QString line = QString("$GP%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15")
                .arg("GGA", time,
                     QString::number(qFabs(latdms * 100), 'f', 2), lat < 0 ? "S" : "N",
                     QString::number(qFabs(lngdms * 100), 'f', 2), lng < 0 ? "W" : "E",
                     "1", "10", "1",
                     QString::number(alt, 'f', 2),
                     "M", "0", "M", "0.0", "0");

        // Calculrate checksum and send message
        QString checkSum = _getCheckSum(line);
        QString* nmeaMessage = new QString(line + "*" + checkSum + "\r\n");

        // Write nmea message
        if(_socket) {
            _socket->write(nmeaMessage->toUtf8());
        }

        qCDebug(NTRIPLog) << "NMEA Message : " << nmeaMessage->toUtf8();
    }
}

QString NTRIPTCPLink::_getCheckSum(QString line) {
  qCDebug(NTRIPLog) << "Calculating checksum";
    QByteArray temp_Byte = line.toUtf8();
    const char* buf = temp_Byte.constData();

    char character;
    int checksum = 0;

    for(int i = 0; i < line.length(); i++) {
        character = buf[i];
        switch(character) {
        case '$':
            // Ignore the dollar sign
            break;
        case '*':
            // Stop processing before the asterisk
            i = line.length();
            continue;
        default:
            // First value for the checksum
            if(checksum == 0) {
                // Set the checksum to the value
                checksum = character;
            }
            else {
                // XOR the checksum with this character's value
                checksum = checksum ^ character;
            }
        }
    }

    return QString("%1").arg(checksum, 0, 16);
}
