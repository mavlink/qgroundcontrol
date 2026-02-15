#include "NTRIP.h"
#include "NTRIPSettings.h"
#include "Fact.h"
#include "FactGroup.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "RTCMMavlink.h"
#include "SettingsManager.h"
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QSslError>

#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "PositionManager.h"
#include <QtPositioning/QGeoCoordinate>
#include <QtCore/QDateTime>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QtGlobal>

#include <QtQml/QQmlEngine>
#include <QtQml/QJSEngine>

QGC_LOGGING_CATEGORY(NTRIPLog, "qgc.ntrip")

// Register the QML type without constructing the singleton up-front.
// This avoids creating NTRIPManager during a temporary Q(Core)Application
// used for command-line parsing, which could lead to it being destroyed
// early and leaving a dangling static pointer.
static QObject* _ntripManagerQmlProvider(QQmlEngine*, QJSEngine*)
{
    return NTRIPManager::instance();
}

static void _ntripManagerRegisterQmlTypes()
{
    qmlRegisterSingletonType<NTRIPManager>("QGroundControl.NTRIP", 1, 0, "NTRIPManager", _ntripManagerQmlProvider);
}
Q_COREAPP_STARTUP_FUNCTION(_ntripManagerRegisterQmlTypes)

NTRIPManager* NTRIPManager::_instance = nullptr;

// constructor
NTRIPManager::NTRIPManager(QObject* parent)
    : QObject(parent)
    , _ntripStatus(tr("Disconnected"))
{
    qCDebug(NTRIPLog) << "NTRIPManager created";
    _startupTimer.start();

    _rtcmMavlink = qgcApp() ? qgcApp()->findChild<RTCMMavlink*>() : nullptr;
    if (!_rtcmMavlink) {
        QObject* parentObj = qgcApp() ? static_cast<QObject*>(qgcApp()) : static_cast<QObject*>(this);
        // Ensure an RTCMMavlink helper exists for forwarding RTCM messages
        _rtcmMavlink = new RTCMMavlink(parentObj);
        _rtcmMavlink->setObjectName(QStringLiteral("RTCMMavlink"));
        qCDebug(NTRIPLog) << "NTRIP Created RTCMMavlink helper";
    }

    // Force NTRIP checkbox OFF during app startup and keep it OFF until first settings tick.
    {
        NTRIPSettings* settings = SettingsManager::instance()->ntripSettings();
        if (settings && settings->ntripServerConnectEnabled()) {
            Fact* fact = settings->ntripServerConnectEnabled();

            // Force OFF immediately
            fact->setRawValue(false);

            // While !_forcedOffOnce, if something tries to flip it ON, flip it back OFF.
            _ntripEnableConn = connect(fact, &Fact::rawValueChanged,
                                       this, [this, fact]() {
                if (!_forcedOffOnce) {
                    const bool wantOn = fact->rawValue().toBool();
                    if (wantOn) {
                        qCDebug(NTRIPLog) << "NTRIP Startup: coercing ntripServerConnectEnabled back to false";
                        fact->setRawValue(false);
                    }
                }
            });
        }
    }

    // Check settings periodically
    _settingsCheckTimer = new QTimer(this);
    connect(_settingsCheckTimer, &QTimer::timeout, this, &NTRIPManager::_checkSettings);
    _settingsCheckTimer->start(1000); // Check every second

    _ggaTimer = new QTimer(this);
    _ggaTimer->setInterval(5000);
    connect(_ggaTimer, &QTimer::timeout, this, &NTRIPManager::_sendGGA);

    connect(qApp, &QCoreApplication::aboutToQuit, this, &NTRIPManager::stopNTRIP, Qt::QueuedConnection);

    _checkSettings();
}

// destructor
NTRIPManager::~NTRIPManager()
{
    qCDebug(NTRIPLog) << "NTRIPManager destroyed";
    stopNTRIP();
    // Clear singleton pointer in case we were destroyed (e.g., if created under
    // a short-lived Q(Core)Application).
    if (_instance == this) {
        _instance = nullptr;
    }
}

RTCMParser::RTCMParser()
{
    reset();
}

void RTCMParser::reset()
{
    _state = WaitingForPreamble;
    _messageLength = 0;
    _bytesRead = 0;
    _lengthBytesRead = 0;
    _crcBytesRead = 0;
}

bool RTCMParser::addByte(uint8_t byte)
{
    switch (_state) {
    case WaitingForPreamble:
        if (byte == RTCM3_PREAMBLE) {
            _buffer[0] = byte;
            _bytesRead = 1;
            _state = ReadingLength;
            _lengthBytesRead = 0;
        }
        break;

    case ReadingLength:
        _lengthBytes[_lengthBytesRead++] = byte;
        _buffer[_bytesRead++] = byte;
        if (_lengthBytesRead == 2) {
            // Extract 10-bit length from bytes (6 reserved bits + 10 length bits)
            _messageLength = ((_lengthBytes[0] & 0x03) << 8) | _lengthBytes[1];
            if (_messageLength > 0 && _messageLength < 1021) { // Valid RTCM3 length
                _state = ReadingMessage;
            } else {
                reset(); // Invalid length, restart
            }
        }
        break;

    case ReadingMessage:
        _buffer[_bytesRead++] = byte;
        if (_bytesRead >= _messageLength + 3) { // +3 for header
            _state = ReadingCRC;
            _crcBytesRead = 0;
        }
        break;

    case ReadingCRC:
        _crcBytes[_crcBytesRead++] = byte;
        if (_crcBytesRead == 3) {
            // Message complete - for simplicity, we'll skip CRC validation
            return true;
        }
        break;
    }
    return false;
}

uint16_t RTCMParser::messageId()
{
    if (_messageLength >= 2) {
        // Message ID is in bits 14-25 of the message (after the 24-bit header)
        return ((_buffer[3] << 4) | (_buffer[4] >> 4)) & 0xFFF;
    }
    return 0;
}

NTRIPTCPLink::NTRIPTCPLink(const QString& hostAddress,
                           int port,
                           const QString& username,
                           const QString& password,
                           const QString& mountpoint,
                           const QString& whitelist,
                           bool useSpartn,
                           QObject* parent)
    : QObject(parent)
    , _hostAddress(hostAddress)
    , _port(port)
    , _username(username)
    , _password(password)
    , _mountpoint(mountpoint)
    , _useSpartn(useSpartn)
{


    if (_useSpartn) {
        // SPARTN path does not use RTCM whitelist or parser
        if (!whitelist.isEmpty()) {
            qCDebug(NTRIPLog) << "NTRIP SPARTN enabled; ignoring RTCM whitelist:" << whitelist;
        }
        // Initialize SPARTN header-strip guard
        _spartnBuf.clear();
        _spartnNeedHeaderStrip = true;

        // Helpful hint if user left the default RTCM port
        if (_port != 2102) {
            qCWarning(NTRIPLog) << "NTRIP SPARTN is enabled but port is" << _port << "(expected 2102 for TLS)";
        }
    } else {
        // RTCM path: build whitelist and parser as before
        for (const auto& msg : whitelist.split(',')) {
            int msg_int = msg.toInt();
            if (msg_int)
                _whitelist.append(msg_int);
        }
        qCDebug(NTRIPLog) << "NTRIP whitelist:" << _whitelist;
        if (_whitelist.empty()) {
            qCDebug(NTRIPLog) << "NTRIP whitelist is empty; all RTCM message IDs will be forwarded.";
        }
        if (!_rtcmParser) {
            _rtcmParser = new RTCMParser();
        }
        _rtcmParser->reset();
    }

    _state = NTRIPState::uninitialised;
}

NTRIPTCPLink::~NTRIPTCPLink()
{
    _stopping.store(true);

    if (_socket) {
        QObject::disconnect(_readyReadConn);
        _socket->disconnectFromHost();
        _socket->close();
        delete _socket;
        _socket = nullptr;
    }

    if (_rtcmParser) {
        delete _rtcmParser;
        _rtcmParser = nullptr;
    }
}

void NTRIPTCPLink::start()
{
    // This runs in the worker thread after QThread::started is emitted
    _hardwareConnect();
}

void NTRIPTCPLink::_hardwareConnect()
{
    if (_stopping.load()) {
        return;
    }

    qCDebug(NTRIPLog) << "NTRIP connectToHost" << _hostAddress << ":" << _port << " mount=" << _mountpoint
                       << " tls=" << (_useSpartn ? "yes" : "no");

    // allocate the appropriate socket type
    // SPARTN selection (TLS on 2102 or when explicitly enabled)
    if (_useSpartn) {
        QSslSocket* sslSocket = new QSslSocket(this);
        _socket = sslSocket;
    } else {
        _socket = new QTcpSocket(this);
    }

    _socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    _socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    _socket->setReadBufferSize(0);

    QObject::connect(_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError code) {
        // Suppress errors generated during intentional shutdown
        if (_stopping.load() || !_socket) {
            qCDebug(NTRIPLog) << "NTRIP suppressing socket error during intentional stop:" << int(code);
            return;
        }

        QString msg = _socket->errorString();

        // If the peer closes before we receive any HTTP headers, try to give a helpful hint.
        if (code == QAbstractSocket::RemoteHostClosedError && _state == NTRIPState::waiting_for_http_response) {
            const QByteArray leftover = _socket->readAll();
            if (!leftover.isEmpty()) {
                qCWarning(NTRIPLog) << "NTRIP peer closed; trailing bytes before close:" << leftover;
                msg += QString(" (peer closed after %1 bytes)").arg(leftover.size());
            } else {
                if (_port == 2102) {
                    msg += " (hint: port 2102 expects HTTPS/TLS; current socket is plain TCP)";
                } else if (!_mountpoint.isEmpty()) {
                    msg += " (peer closed before HTTP response; check mountpoint and credentials)";
                }
            }
        }

        qCWarning(NTRIPLog) << "NTRIP socket error code:" << int(code)
                            << " msg:" << msg
                            << " state:" << int(_socket->state())
                            << " peer:"  << _socket->peerAddress().toString() << ":" << _socket->peerPort()
                            << " local:" << _socket->localAddress().toString() << ":" << _socket->localPort();
        emit error(msg);
    }, Qt::DirectConnection);

    // If the server cleanly disconnects, capture reason and emit error so NTRIPManager updates status
    QObject::connect(_socket, &QTcpSocket::disconnected, this, [this]() {
        // Suppress disconnect noise during intentional shutdown
        if (_stopping.load() || !_socket) {
            qCDebug(NTRIPLog) << "NTRIP suppressing disconnect signal during intentional stop";
            return;
        }

        const QByteArray trailing = _socket->readAll();
        QString reason;
        if (!trailing.isEmpty()) {
            reason = QString::fromUtf8(trailing).trimmed();
            qCWarning(NTRIPLog) << "NTRIP disconnected; trailing bytes:" << trailing;
        } else {
            reason = "Server disconnected";
            qCWarning(NTRIPLog) << "NTRIP disconnected cleanly by server";
        }
        // Emit richer context on disconnect
        const qint64 t0 = this->property("ntrip_postok_t0_ms").toLongLong();
        const bool saw = this->property("ntrip_saw_rtcm").toBool();
        const qint64 dt = (t0 > 0) ? (QDateTime::currentMSecsSinceEpoch() - t0) : -1;
        qCWarning(NTRIPLog) << "NTRIP disconnect context:"
                            << "rtcm_received_first_byte=" << (saw ? "yes" : "no")
                            << "ms_since_200=" << dt
                            << "peer=" << _socket->peerAddress().toString() << ":" << _socket->peerPort()
                            << "local=" << _socket->localAddress().toString() << ":" << _socket->localPort();
        emit error(reason);  // Will call NTRIPManager::_tcpError()
    }, Qt::DirectConnection);

    _readyReadConn = QObject::connect(_socket, &QTcpSocket::readyRead,this, &NTRIPTCPLink::_readBytes,Qt::DirectConnection);

    auto sendHttpRequest = [this]() {
        if (!_mountpoint.isEmpty()) {
            qCDebug(NTRIPLog) << "NTRIP Sending HTTP request";
            const QByteArray authB64 = QString(_username + ":" + _password).toUtf8().toBase64();
            QString query =
                "GET /%1 HTTP/1.1\r\n"
                "Host: %2\r\n"
                "Ntrip-Version: Ntrip/2.0\r\n"
                "User-Agent: QGC-NTRIP\r\n"
                "Connection: keep-alive\r\n"
                "Accept: */*\r\n"
                "Authorization: Basic %3\r\n"
                "\r\n";
            const QByteArray req = query.arg(_mountpoint).arg(_hostAddress).arg(QString::fromUtf8(authB64)).toUtf8();
            const qint64 written = _socket->write(req);
            _socket->flush();

            // Extra diagnostics: request line and auth presence (length only)
            const QString reqStr = QString::fromUtf8(req);
            const int cr = reqStr.indexOf("\r");
            const QString firstLine = (cr > 0) ? reqStr.left(cr).trimmed() : QString();
            const int authIdx = reqStr.indexOf("Authorization: Basic ");
            int b64Len = -1;
            if (authIdx >= 0) {
                const int eol = reqStr.indexOf("\r\n", authIdx);
                if (eol > authIdx) b64Len = eol - (authIdx + 21);
            }
            qCDebug(NTRIPLog) << "NTRIP HTTP request first line:" << firstLine;
            qCDebug(NTRIPLog) << "NTRIP HTTP auth present:" << (authIdx >= 0) << "auth_b64_len:" << b64Len;

            qCDebug(NTRIPLog) << "NTRIP HTTP request bytes written:" << written;
            _state = NTRIPState::waiting_for_http_response;
        }
        else {
            _state = NTRIPState::waiting_for_rtcm_header;
            emit connected();
        }
        qCDebug(NTRIPLog) << "NTRIP Socket connected"
                           << "local" << _socket->localAddress().toString() << ":" << _socket->localPort()
                           << "-> peer" << _socket->peerAddress().toString() << ":" << _socket->peerPort();
    };

    if (_useSpartn) {
        QSslSocket* sslSocket = qobject_cast<QSslSocket*>(_socket);
        Q_ASSERT(sslSocket);

        QObject::connect(sslSocket, &QSslSocket::encrypted, this, [this, sendHttpRequest]() {
            qCDebug(NTRIPLog) << "SPARTN TLS connection established";
            sendHttpRequest();
        }, Qt::DirectConnection);

        QObject::connect(sslSocket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
                         this, [this](const QList<QSslError> &errors) {
            for (const QSslError &e : errors) {
                qCWarning(NTRIPLog) << "TLS Error:" << e.errorString();
            }
        }, Qt::DirectConnection);

        sslSocket->connectToHostEncrypted(_hostAddress, static_cast<quint16>(_port));

        if (!sslSocket->waitForEncrypted(10000)) {
            qCDebug(NTRIPLog) << "NTRIP TLS socket failed to establish encryption";
            emit error(_socket->errorString());
            delete _socket;
            _socket = nullptr;
            return;
        }

        if (sslSocket->isEncrypted()) {
            sendHttpRequest();
        }
    } else {
        QTcpSocket* tcpSocket = qobject_cast<QTcpSocket*>(_socket);
        Q_ASSERT(tcpSocket);

        tcpSocket->connectToHost(_hostAddress, static_cast<quint16>(_port));

        if (!tcpSocket->waitForConnected(10000)) {
            qCDebug(NTRIPLog) << "NTRIP Socket failed to connect";
            emit error(_socket->errorString());
            delete _socket;
            _socket = nullptr;
            return;
        }

        sendHttpRequest();
    }
}

void NTRIPTCPLink::_parse(const QByteArray& buffer)
{
    if (_stopping.load()) {
        return;
    }

    static bool logged_empty_once = false;
    if (!logged_empty_once && _whitelist.empty()) {
        qCDebug(NTRIPLog) << "NTRIP whitelist is empty at runtime; forwarding all RTCM messages.";
        logged_empty_once = true;
    }

    // RTCM v3 transport sizes
    constexpr int kRtcmHeaderSize = 3;  // preamble (1) + length (2)

    for (char ch : buffer) {
        const uint8_t byte = static_cast<uint8_t>(static_cast<unsigned char>(ch));

        if (_state == NTRIPState::waiting_for_rtcm_header) {
            if (byte != RTCM3_PREAMBLE) {
                continue;
            }
            _state = NTRIPState::accumulating_rtcm_packet;
        }

        if (_rtcmParser->addByte(byte)) {
            _state = NTRIPState::waiting_for_rtcm_header;

            // Build exact on-wire frame: [D3 | len(2) | payload | CRC(3)]
            const int payload_len = static_cast<int>(_rtcmParser->messageLength());
            QByteArray message(reinterpret_cast<const char*>(_rtcmParser->message()),
                               kRtcmHeaderSize + payload_len);

            const uint8_t* crc_ptr = _rtcmParser->crcBytes();
            const int crc_len = _rtcmParser->crcSize();
            message.append(reinterpret_cast<const char*>(crc_ptr), crc_len);

            const uint16_t id = _rtcmParser->messageId();

            if (_whitelist.empty() || _whitelist.contains(id)) {
                qCDebug(NTRIPLog) << "NTRIP RTCM packet id" << id << "len" << message.length();
                emit RTCMDataUpdate(message);
                qCDebug(NTRIPLog) << "NTRIP Sending" << id << "of size" << message.length();
            } else {
                qCDebug(NTRIPLog) << "NTRIP Ignoring" << id;
            }

            _rtcmParser->reset();
        }
    }
}

void NTRIPTCPLink::_handleSpartnData(const QByteArray& dataIn)
{
    if (dataIn.isEmpty()) {
        return;
    }

    // Accumulate so we can remove a response header exactly once if it ever appears here.
    _spartnBuf.append(dataIn);

    if (_spartnNeedHeaderStrip) {
        const bool looksHttp = _spartnBuf.startsWith("HTTP/") || _spartnBuf.startsWith("ICY ");
        if (looksHttp) {
            const int headerEnd = _spartnBuf.indexOf("\r\n\r\n");
            if (headerEnd < 0) {
                // Header incomplete; cap buffer to avoid growth on a bad stream.
                if (_spartnBuf.size() > 32768) {
                    _spartnBuf = _spartnBuf.right(32768);
                }
                return;
            }
            // Drop the header and keep the payload.
            _spartnBuf.remove(0, headerEnd + 4);
        }
        _spartnNeedHeaderStrip = false;
    }

    if (_spartnBuf.isEmpty()) {
        return;
    }

    // Deliver raw SPARTN bytes to whoever is consuming them (e.g., a GNSS forwarder).
    emit SPARTNDataUpdate(_spartnBuf);

    // Clear after handing off.
    _spartnBuf.clear();
}

void NTRIPTCPLink::_readBytes()
{
    if (_stopping.load()) {
        return;
    }

    // SPARTN path: after HTTP 200 OK, feed raw bytes through the SPARTN handler
    if (_socket && _state == NTRIPState::waiting_for_spartn_data) {
        const QByteArray bytes = _socket->readAll();
        if (!bytes.isEmpty()) {
            _handleSpartnData(bytes);
        }
        return;
    }

    if (!_socket) {
        return;
    }

    // Static counters and flags scoped to this function (no header changes needed)
    static quint64 s_totalRtcm = 0;

    if (_state == NTRIPState::waiting_for_http_response) {
        QByteArray responseData = _socket->readAll();
        if (responseData.isEmpty()) {
            return;
        }

        QString response = QString::fromUtf8(responseData);
        qCDebug(NTRIPLog) << "NTRIP HTTP response received:" << response.left(200);

        // Dump full headers once for diagnostics
        const int hdrEnd = response.indexOf("\r\n\r\n");
        const QString headers = (hdrEnd >= 0) ? response.left(hdrEnd) : response;
        qCDebug(NTRIPLog) << "NTRIP HTTP response headers BEGIN >>>";
        for (const QString& line : headers.split("\r\n")) {
            if (!line.isEmpty()) qCDebug(NTRIPLog) << line;
        }
        qCDebug(NTRIPLog) << "NTRIP HTTP response headers <<< END";

        // Parse status line
        QStringList lines = response.split('\n');
        bool foundOkResponse = false;
        for (const QString& line : lines) {
            const QString trimmed = line.trimmed();
            if ((trimmed.startsWith("HTTP/") || trimmed.startsWith("ICY ")) &&
                (trimmed.contains(" 200 ") || trimmed.contains(" 201 "))) {
                foundOkResponse = true;
                qCDebug(NTRIPLog) << "NTRIP: Found OK response:" << trimmed;
                break;
            } else if ((trimmed.startsWith("HTTP/") || trimmed.startsWith("ICY ")) &&
                       (trimmed.contains(" 4") || trimmed.contains(" 5"))) {
                qCWarning(NTRIPLog) << "NTRIP: Server error response:" << trimmed;
                emit error(QString("NTRIP HTTP error: %1").arg(trimmed));

                // Null out _socket before disconnectFromHost() so the
                // synchronous 'disconnected' handler bails via !_socket guard.
                QTcpSocket* sock = _socket;
                _socket = nullptr;
                _state = NTRIPState::uninitialised;

                QObject::disconnect(_readyReadConn);
                sock->disconnectFromHost();
                sock->close();
                sock->deleteLater();

                if (!_stopping.load()) {
                    QTimer::singleShot(3000, this, [this]() {
                        if (!_stopping.load()) {
                            _hardwareConnect();
                        }
                    });
                }
                return;
            }
        }

        if (foundOkResponse) {
            // Mark the 200 OK time and reset first-RTCM flag using QObject properties.
            const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
            this->setProperty("ntrip_postok_t0_ms", nowMs);
            this->setProperty("ntrip_saw_rtcm", false);
            this->setProperty("ntrip_watchdog_fired", false);

            // Fire a one-shot watchdog that only logs if no RTCM arrives by ~28s.
            QTimer::singleShot(28000, this, [this]() {
                if (this->property("ntrip_watchdog_fired").toBool()) return;
                const bool saw = this->property("ntrip_saw_rtcm").toBool();
                if (!saw) {
                    this->setProperty("ntrip_watchdog_fired", true);
                    qCWarning(NTRIPLog)
                        << "NTRIP no RTCM received 28s after 200 OK. Likely caster timeout."
                        << "Check: duplicate login, entitlement/region, GGA validity.";
                }
            });

            _state = _useSpartn ? NTRIPState::waiting_for_spartn_data
                                : NTRIPState::waiting_for_rtcm_header;

            qCDebug(NTRIPLog) << "NTRIP: HTTP handshake complete, transitioning to data state";
            emit connected();

            // Process any remaining data after the headers
            if (hdrEnd >= 0) {
                QByteArray remainingData = responseData.mid(hdrEnd + 4);
                if (!remainingData.isEmpty()) {
                    qCDebug(NTRIPLog) << "NTRIP: Processing data after HTTP headers:" << remainingData.size() << "bytes";
                    if (_useSpartn) {
                        _handleSpartnData(remainingData);
                    } else {
                        _parse(remainingData);
                    }
                }
            }
        } else {
            qCDebug(NTRIPLog) << "NTRIP: Waiting for complete HTTP response...";
        }

        return; // done with HTTP response handling
    }

    // Data after handshake (RTCM path)
    QByteArray bytes = _socket->readAll();
    if (!bytes.isEmpty()) {
        // Mark first RTCM arrival and log timing/sample
        if (!this->property("ntrip_saw_rtcm").toBool()) {
            this->setProperty("ntrip_saw_rtcm", true);
            const qint64 t0 = this->property("ntrip_postok_t0_ms").toLongLong();
            const qint64 dt = (t0 > 0) ? (QDateTime::currentMSecsSinceEpoch() - t0) : -1;
            const QByteArray sample = bytes.left(48);
            const unsigned char b0 = static_cast<unsigned char>(sample.isEmpty() ? 0 : sample[0]);
            qCDebug(NTRIPLog) << "NTRIP first RTCM bytes at" << dt << "ms after 200 OK";
            qCDebug(NTRIPLog) << "NTRIP rx sample (hex, first 48B):" << sample.toHex(' ');
            qCDebug(NTRIPLog) << "NTRIP preamble_check first_byte=0x" << QByteArray::number(b0, 16);
        }

        s_totalRtcm += static_cast<quint64>(bytes.size());
        qCDebug(NTRIPLog) << "NTRIP rx bytes:" << bytes.size()
                           << "total:" << s_totalRtcm
                           << "bytesAvailable:" << _socket->bytesAvailable();

        _parse(bytes);
    }
}

void NTRIPTCPLink::debugFetchSourceTable()
{
    // Build request (include Authorization so the caster can tailor the table to your creds)
    const QByteArray authB64 = QString(_username + ":" + _password).toUtf8().toBase64();
    const QByteArray req =
        QByteArray("GET / HTTP/1.1\r\n")
        + "Host: " + _hostAddress.toUtf8() + "\r\n"
        + "User-Agent: QGC-NTRIP\r\n"
        + "Ntrip-Version: Ntrip/2.0\r\n"
        + "Accept: */*\r\n"
        + "Authorization: Basic " + authB64 + "\r\n"
        + "Connection: close\r\n\r\n";

    QByteArray all;

    if (_useSpartn || _port == 2102) {
        // TLS (e.g., SPARTN/2102)
        QSslSocket sock;
        sock.connectToHostEncrypted(_hostAddress, static_cast<quint16>(_port));
        if (!sock.waitForEncrypted(7000)) {
            qCWarning(NTRIPLog) << "SOURCETABLE TLS connect failed:" << sock.errorString();
            return;
        }
        if (sock.write(req) <= 0 || !sock.waitForBytesWritten(3000)) {
            qCWarning(NTRIPLog) << "SOURCETABLE TLS write failed:" << sock.errorString();
            return;
        }
        while (sock.waitForReadyRead(3000)) {
            all += sock.readAll();
            if (sock.bytesAvailable() == 0 && sock.state() != QAbstractSocket::ConnectedState) break;
        }
        sock.close();
    } else {
        // Plain TCP (e.g., RTCM/2101)
        QTcpSocket sock;
        sock.connectToHost(_hostAddress, static_cast<quint16>(_port));
        if (!sock.waitForConnected(5000)) {
            qCWarning(NTRIPLog) << "SOURCETABLE connect failed:" << sock.errorString();
            return;
        }
        if (sock.write(req) <= 0 || !sock.waitForBytesWritten(3000)) {
            qCWarning(NTRIPLog) << "SOURCETABLE write failed:" << sock.errorString();
            return;
        }
        while (sock.waitForReadyRead(3000)) {
            all += sock.readAll();
            if (sock.bytesAvailable() == 0 && sock.state() != QAbstractSocket::ConnectedState) break;
        }
        sock.close();
    }

    // Split headers/body and dump the body (the sourcetable)
    const int hdrEnd = all.indexOf("\r\n\r\n");
    const QByteArray body = (hdrEnd >= 0) ? all.mid(hdrEnd + 4) : all;

    qCDebug(NTRIPLog) << "----- NTRIP SOURCETABLE BEGIN -----";
    for (const QByteArray& line : body.split('\n')) {
        const QByteArray l = line.trimmed();
        if (!l.isEmpty()) qCDebug(NTRIPLog) << l;
    }
    qCDebug(NTRIPLog) << "------ NTRIP SOURCETABLE END ------";
}

void NTRIPTCPLink::sendNMEA(const QByteArray& sentence)
{
    if (_stopping.load()) {
        return;
    }
    if (!_socket || _socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    QByteArray line = sentence;

    // Validate or repair checksum in the form $CORE*XX
    if (line.size() >= 5 && line.at(0) == '$') {
        int star = line.lastIndexOf('*');
        if (star > 1) {
            // Calculate checksum over bytes between '$' and '*'
            quint8 calc = 0;
            for (int i = 1; i < star; ++i) {
                calc ^= static_cast<quint8>(line.at(i));
            }

            // Format calculated checksum as two uppercase hex digits
            QByteArray calcCks = QByteArray::number(calc, 16)
                                     .rightJustified(2, '0')
                                     .toUpper();

            bool needsRepair = false;
            if (star + 3 > line.size()) {
                // Not enough chars after '*', definitely needs repair
                needsRepair = true;
            } else {
                QByteArray txCks = line.mid(star + 1, 2).toUpper();
                if (txCks != calcCks) {
                    needsRepair = true;
                }
            }

            if (needsRepair) {
                // Remove any existing checksum and replace with correct one
                line = line.left(star + 1) + calcCks;
            }
        } else {
            // No '*' found, append one and correct checksum
            quint8 calc = 0;
            for (int i = 1; i < line.size(); ++i) {
                calc ^= static_cast<quint8>(line.at(i));
            }
            QByteArray calcCks = QByteArray::number(calc, 16)
                                     .rightJustified(2, '0')
                                     .toUpper();
            line.append('*').append(calcCks);
        }
    }

    // Ensure CRLF termination
    if (!line.endsWith("\r\n")) {
        line.append("\r\n");
    }

    qCDebug(NTRIPLog) << "NTRIP Sent NMEA:" << QString::fromUtf8(line.trimmed());

    const qint64 written = _socket->write(line);
    _socket->flush();

    qCDebug(NTRIPLog) << "NTRIP Socket state:" << (_socket ? _socket->state() : -1);
    qCDebug(NTRIPLog) << "NTRIP Bytes written:" << written;
}


void NTRIPTCPLink::requestStop()
{
    _stopping.store(true);

    if (_socket) {
        QObject::disconnect(_readyReadConn);
        _socket->disconnectFromHost();
        _socket->close();
        delete _socket;
        _socket = nullptr;
    }

    emit finished();   // let NTRIPManager's QThread quit in response
}

NTRIPManager* NTRIPManager::instance()
{
    if (!_instance) {
        // Prefer QGCApplication as parent if available, otherwise fall back to qApp
        QObject* parent = qgcApp() ? static_cast<QObject*>(qgcApp()) : static_cast<QObject*>(qApp);
        _instance = new NTRIPManager(parent);
    }
    return _instance;
}

void NTRIPManager::startNTRIP()
{
    if (_startStopBusy) {
        return;
    }
    _startStopBusy = true;   // guard begin

    qCDebug(NTRIPLog) << "startNTRIP: begin";

    if (_tcpLink) {
        _startStopBusy = false;  // already started; release guard
        return;
    }

    _ntripStatus = tr("Connecting...");
    emit ntripStatusChanged();


    NTRIPSettings* settings = SettingsManager::instance()->ntripSettings();
    QString host;
    int port = 2101;
    QString user;
    QString pass;
    QString mount;
    QString whitelist;

    if (settings) {
        if (settings->ntripServerHostAddress()) host = settings->ntripServerHostAddress()->rawValue().toString();
        if (settings->ntripServerPort())        port = settings->ntripServerPort()->rawValue().toInt();
        if (settings->ntripUsername())          user = settings->ntripUsername()->rawValue().toString();
        if (settings->ntripPassword())          pass = settings->ntripPassword()->rawValue().toString();
        if (settings->ntripMountpoint())        mount = settings->ntripMountpoint()->rawValue().toString();
        if (settings->ntripWhitelist())         whitelist = settings->ntripWhitelist()->rawValue().toString();
        if (settings->ntripUseSpartn())         _useSpartn = settings->ntripUseSpartn()->rawValue().toBool();
    } else {
        qCWarning(NTRIPLog) << "NTRIP settings group is null; starting with defaults";
    }

    qCDebug(NTRIPLog) << "startNTRIP: host=" << host
                      << " port=" << port
                      << " mount=" << mount
                      << " userIsEmpty=" << user.isEmpty();

    _tcpThread = new QThread(this);
    NTRIPTCPLink* link = new NTRIPTCPLink(host, port, user, pass, mount, whitelist, _useSpartn);
    _tcpLink = link;

    link->moveToThread(_tcpThread);

    connect(_tcpThread, &QThread::started, link, &NTRIPTCPLink::start);
    connect(link, &NTRIPTCPLink::finished, _tcpThread, &QThread::quit);
    connect(_tcpThread, &QThread::finished, _tcpThread, &QObject::deleteLater);
    connect(_tcpThread, &QThread::finished, link, &QObject::deleteLater);

    // Surface errors (including server disconnect reasons) to status/UI
    connect(_tcpLink, &NTRIPTCPLink::error,
            this, &NTRIPManager::_tcpError,
            Qt::QueuedConnection);

    connect(_tcpLink, &NTRIPTCPLink::connected, this, [this]() {
        _casterStatus = CasterStatus::Connected;
        emit casterStatusChanged(_casterStatus);

        const QString want = _useSpartn ? tr("Connected (SPARTN)") : tr("Connected");
        if (_ntripStatus != want) {
            _ntripStatus = want;
            emit ntripStatusChanged();
        }

        // >>> ONE-SHOT SOURCETABLE DUMP (enable with env var QGC_NTRIP_DUMP_TABLE=1)
        if (qEnvironmentVariableIsSet("QGC_NTRIP_DUMP_TABLE")) {
            static bool dumped = false;
            if (!dumped && _tcpLink) {
                dumped = true;
                QMetaObject::invokeMethod(_tcpLink, "debugFetchSourceTable", Qt::QueuedConnection);
            }
        }
        // <<<

        // Start rapid 1 Hz GGA until we get the first valid coord
        if (_ggaTimer && !_ggaTimer->isActive()) {
            _ggaTimer->setInterval(1000); // 1 Hz while waiting for fix
            _sendGGA();                   // try immediately
            _ggaTimer->start();
        }

    }, Qt::QueuedConnection);

    if (_useSpartn) {
        connect(_tcpLink, &NTRIPTCPLink::SPARTNDataUpdate, this, [this](const QByteArray& data){
            static uint32_t spartn_count = 0;
            if ((spartn_count++ % 50) == 0) {
                qCDebug(NTRIPLog) << "SPARTN bytes flowing:" << data.size();
            }
            if (_ntripStatus != tr("Connected (SPARTN)")) {
                _ntripStatus = tr("Connected (SPARTN)");
                emit ntripStatusChanged();
            }
            _casterStatus = CasterStatus::Connected;
            emit casterStatusChanged(_casterStatus);

            _rtcmDataReceived(data);
        }, Qt::QueuedConnection);
    } else {
        connect(_tcpLink, &NTRIPTCPLink::RTCMDataUpdate, this, &NTRIPManager::_rtcmDataReceived, Qt::QueuedConnection);
    }

    _tcpThread->start();
    qCDebug(NTRIPLog) << "NTRIP started";

    _startStopBusy = false;  // guard end
}

void NTRIPManager::stopNTRIP()
{
    if (_startStopBusy) {
        return;
    }
    _startStopBusy = true;   // guard begin

    if (_tcpLink) {
        // Prevent spurious "NTRIP error: ..." toasts during user-initiated shutdown
        disconnect(_tcpLink, &NTRIPTCPLink::error, this, &NTRIPManager::_tcpError);

        // Ask the worker to stop in its own thread
        QMetaObject::invokeMethod(_tcpLink, "requestStop", Qt::QueuedConnection);

        // If we still own the thread, stop and wait for it
        if (_tcpThread) {
            _tcpThread->quit();   // or just rely on 'finished' from requestStop()
            _tcpThread->wait();
            _tcpThread = nullptr;
        }

        // _tcpLink will be deleted via deleteLater() when the thread finishes
        _tcpLink = nullptr;

        _ntripStatus = tr("Disconnected");
        emit ntripStatusChanged();
        qCDebug(NTRIPLog) << "NTRIP stopped";
    }

    if (_ggaTimer && _ggaTimer->isActive()) {
        _ggaTimer->stop();
    }

    _startStopBusy = false;  // guard end
}

void NTRIPManager::_tcpError(const QString& errorMsg)
{
    qCWarning(NTRIPLog) << "NTRIP Server Error:" << errorMsg;
    _ntripStatus = errorMsg;
    emit ntripStatusChanged();
    _startStopBusy = false;  // make sure guard clears on error

    // Check for specific "no location provided" disconnect reason
    if (errorMsg.contains("NO_LOCATION_PROVIDED", Qt::CaseInsensitive)) {
        _onCasterDisconnected(errorMsg);
    } else {
        _casterStatus = CasterStatus::OtherError;
        emit casterStatusChanged(_casterStatus);
    }

    // Show error to user via QGC notification (toast)
    if (qgcApp()) {
        qgcApp()->showAppMessage(tr("NTRIP error: %1").arg(errorMsg));
    }
}

void NTRIPManager::_rtcmDataReceived(const QByteArray& data)
{

    qCDebug(NTRIPLog) << "NTRIP Forwarding RTCM to vehicle:" << data.size() << "bytes";

    // Lazily resolve the tool if we don't have it yet
    if (!_rtcmMavlink && qgcApp()) {
        _rtcmMavlink = qgcApp()->findChild<RTCMMavlink*>();
    }

    if (_rtcmMavlink) {
        // Forward raw RTCM bytes to the autopilot via MAVLink GPS_RTCM_DATA
        _rtcmMavlink->RTCMDataUpdate(data);

        if (_ntripStatus != tr("Connected")) {
            _ntripStatus = tr("Connected");
            emit ntripStatusChanged();
        }
    } else {
        // Tool not constructed yet — avoid crash, keep status, and don't spam
        static uint32_t drop_warn_counter = 0;
        if ((drop_warn_counter++ % 50) == 0) {
            qCWarning(NTRIPLog) << "RTCMMavlink tool not ready; dropping RTCM bytes:" << data.size();
        }
    }

    _startStopBusy = false;  // guard clears once bytes flow (or we tried)
}

static QByteArray _makeGGA(const QGeoCoordinate& coord, double altitude_msl)
{
    // UTC time hhmmss format
    const QTime utc = QDateTime::currentDateTimeUtc().time();
    const QString hhmmss = QString("%1%2%3")
        .arg(utc.hour(),   2, 10, QChar('0'))
        .arg(utc.minute(), 2, 10, QChar('0'))
        .arg(utc.second(), 2, 10, QChar('0'));

    // Convert decimal degrees to NMEA DMM with zero-padded minutes:
    // lat:  ddmm.mmmm, lon: dddmm.mmmm
    auto dmm = [](double deg, bool lat) -> QString {
        double a = qFabs(deg);
        int d = int(a);
        double m = (a - d) * 60.0;

        // Round to 4 decimals and handle 59.99995 -> 60.0000 carry into degrees
        int m10000 = int(m * 10000.0 + 0.5);
        double m_rounded = m10000 / 10000.0;
        if (m_rounded >= 60.0) {
            m_rounded -= 60.0;
            d += 1;
        }

        // Ensure minutes have two digits before the decimal (e.g. 08.1234)
        QString mm = QString::number(m_rounded, 'f', 4);
        if (m_rounded < 10.0) {
            mm.prepend("0");
        }

        if (lat) {
            return QString("%1%2").arg(d, 2, 10, QChar('0')).arg(mm);
        } else {
            return QString("%1%2").arg(d, 3, 10, QChar('0')).arg(mm);
        }
    };

    const bool latNorth = coord.latitude() >= 0.0;
    const bool lonEast  = coord.longitude() >= 0.0;

    const QString latField = dmm(coord.latitude(), true);
    const QString lonField = dmm(coord.longitude(), false);

    // IMPORTANT: include geoid separation as 0.0 and unit 'M'
    // Fields: time,lat,N,lon,E,fix,nsat,hdop,alt,M,geoid,M,age,station
    const QString core = QString("GPGGA,%1,%2,%3,%4,%5,1,12,1.0,%6,M,0.0,M,,")
        .arg(hhmmss)
        .arg(latField)
        .arg(latNorth ? "N" : "S")
        .arg(lonField)
        .arg(lonEast  ? "E" : "W")
        .arg(QString::number(altitude_msl, 'f', 1));

    // NMEA checksum over bytes between '$' and '*'
    quint8 cksum = 0;
    const QByteArray coreBytes = core.toUtf8();
    for (char ch : coreBytes) {
        cksum ^= static_cast<quint8>(ch);
    }

    const QString cks = QString("%1").arg(cksum, 2, 16, QChar('0')).toUpper();
    const QByteArray sentence = QByteArray("$") + coreBytes + QByteArray("*") + cks.toUtf8();
    return sentence;
}

void NTRIPManager::_sendGGA()
{
    if (!_tcpLink) {
        return;
    }

    QGeoCoordinate coord;
    double alt_msl = 0.0;
    bool validCoord = false;
    QString srcUsed;

    if (qgcApp()) {
        // PRIORITY 1: Raw GPS data from vehicle GPS facts (most important for RTK)
        MultiVehicleManager* mvm = MultiVehicleManager::instance();
        if (mvm) {
            if (Vehicle* veh = mvm->activeVehicle()) {
                FactGroup* gps = veh->gpsFactGroup();
                if (gps) {
                    // Try common GPS fact names
                    Fact* latF = gps->getFact(QStringLiteral("lat"));
                    Fact* lonF = gps->getFact(QStringLiteral("lon"));

                    // If "lat"/"lon" don't work, try alternative names
                    if (!latF) latF = gps->getFact(QStringLiteral("latitude"));
                    if (!lonF) lonF = gps->getFact(QStringLiteral("longitude"));
                    if (!latF) latF = gps->getFact(QStringLiteral("lat_deg"));
                    if (!lonF) lonF = gps->getFact(QStringLiteral("lon_deg"));
                    if (!latF) latF = gps->getFact(QStringLiteral("latitude_deg"));
                    if (!lonF) lonF = gps->getFact(QStringLiteral("longitude_deg"));

                    if (latF && lonF) {
                        const double glat = latF->rawValue().toDouble();
                        const double glon = lonF->rawValue().toDouble();

                        // GPS coordinates might be in 1e-7 degrees format (int32 scaled)
                        double lat_deg = glat;
                        double lon_deg = glon;

                        // Check if values look like scaled integers (> 1000 suggests 1e-7 format)
                        if (qAbs(glat) > 1000.0 || qAbs(glon) > 1000.0) {
                            lat_deg = glat * 1e-7;
                            lon_deg = glon * 1e-7;
                        }

                        if (qIsFinite(lat_deg) && qIsFinite(lon_deg) &&
                            !(lat_deg == 0.0 && lon_deg == 0.0) &&
                            qAbs(lat_deg) <= 90.0 && qAbs(lon_deg) <= 180.0) {

                            coord = QGeoCoordinate(lat_deg, lon_deg);
                            validCoord = true;

                            // Get altitude from GPS if available
                            Fact* altF = gps->getFact(QStringLiteral("alt"));
                            if (!altF) altF = gps->getFact(QStringLiteral("altitude"));
                            if (altF) {
                                double raw_alt = altF->rawValue().toDouble();
                                // Altitude might be in mm or cm, convert to meters
                                if (qAbs(raw_alt) > 10000.0) {  // > 10km suggests mm format
                                    alt_msl = raw_alt * 1e-3;   // mm to meters
                                } else if (qAbs(raw_alt) > 1000.0) {  // > 1km suggests cm format
                                    alt_msl = raw_alt * 1e-2;   // cm to meters
                                } else {
                                    alt_msl = raw_alt;          // already in meters
                                }
                                if (!qIsFinite(alt_msl)) alt_msl = 0.0;
                            }

                            srcUsed = QStringLiteral("GPS Raw");
                            qCDebug(NTRIPLog) << "NTRIP: Using raw GPS data for GGA"
                                              << "lat:" << lat_deg << "lon:" << lon_deg << "alt:" << alt_msl;
                        }
                    }
                } else {
                    // Vehicle exists but no GPS FactGroup yet
                    qCDebug(NTRIPLog) << "NTRIP: Vehicle found but no GPS FactGroup available yet";
                }
            } else {
                // MultiVehicleManager exists but no active vehicle yet
                static int no_vehicle_count = 0;
                if ((no_vehicle_count++ % 10) == 0) { // Log every 10th time to avoid spam
                    qCDebug(NTRIPLog) << "NTRIP: MultiVehicleManager found but no active vehicle yet";
                }
            }
        } else {
            // MultiVehicleManager not found yet - this is normal during early startup
            static int no_mvm_count = 0;
            if ((no_mvm_count++ % 10) == 0) { // Log every 10th time to avoid spam
                qCDebug(NTRIPLog) << "NTRIP: MultiVehicleManager not available yet (startup)";
            }
        }

        // PRIORITY 2: Vehicle global position estimate (EKF output)
        if (!validCoord) {
            MultiVehicleManager* mvm2 = MultiVehicleManager::instance();
            if (mvm2) {
                if (Vehicle* veh2 = mvm2->activeVehicle()) {
                    coord = veh2->coordinate();
                    if (coord.isValid() && !(coord.latitude() == 0.0 && coord.longitude() == 0.0)) {
                        validCoord = true;
                        double a = coord.altitude();
                        alt_msl = qIsFinite(a) ? a : 0.0;
                        srcUsed = QStringLiteral("Vehicle EKF");
                        qCDebug(NTRIPLog) << "NTRIP: Using vehicle EKF position for GGA" << coord;
                    }
                }
            }
        }

    }

    if (!validCoord) {
        qCDebug(NTRIPLog) << "NTRIP: No valid position available, skipping GGA.";
        return;
    }

    // Once we have a real valid coord, slow down GGA to 5 seconds
    if (_ggaTimer && _ggaTimer->interval() != 5000) {
        _ggaTimer->setInterval(5000);
        qCDebug(NTRIPLog) << "NTRIP: Real position acquired, reducing GGA frequency to 5s";
    }

    const QByteArray gga = _makeGGA(coord, alt_msl);

    // Debug: Log the GGA sentence being sent (but reduce spam)
    static int gga_send_count = 0;
    if ((gga_send_count++ % 5) == 0) { // Log every 5th GGA to reduce spam
        qCDebug(NTRIPLog) << "NTRIP: Sending GGA:" << gga;
        qCDebug(NTRIPLog) << "NTRIP: GGA coord:" << coord << "alt:" << alt_msl << "source:" << srcUsed;
    }

    QMetaObject::invokeMethod(_tcpLink, "sendNMEA",
                              Qt::QueuedConnection,
                              Q_ARG(QByteArray, gga));

    // Update GGA source for UI display
    if (!srcUsed.isEmpty() && srcUsed != _ggaSource) {
        _ggaSource = srcUsed;
        emit ggaSourceChanged();
    }
}

void NTRIPManager::_checkSettings()
{
    if (_startStopBusy) {
        qCDebug(NTRIPLog) << "NTRIP _checkSettings: busy, skipping";
        return;
    }

    NTRIPSettings* settings = SettingsManager::instance()->ntripSettings();
    Fact* enableFact = nullptr;
    if (settings && settings->ntripServerConnectEnabled()) {
        enableFact = settings->ntripServerConnectEnabled();
    }

    // During startup, keep forcing OFF until we observe the Fact stable at false
    // for 2 consecutive ticks. This defeats late restores from QML/settings.
    if (_startupSuppress) {
        if (enableFact) {
            if (enableFact->rawValue().toBool()) {
                qCDebug(NTRIPLog) << "NTRIP Startup: coercing OFF (late restore detected)";
                enableFact->setRawValue(false);
                _startupStableTicks = 0;   // reset stability counter on any true
            } else {
                _startupStableTicks++;
            }
        } else {
            _startupStableTicks = 0;       // no fact yet, not stable
        }

        // Release once we have seen false for 2 ticks
        if (_startupStableTicks >= 2) {
            _startupSuppress = false;
            if (!_forcedOffOnce) {
                _forcedOffOnce = true;
                if (_ntripEnableConn) {
                    disconnect(_ntripEnableConn);
                }
            }
            qCDebug(NTRIPLog) << "NTRIP Startup: suppression released; honoring user setting from now on";
        } else {
            return; // keep suppressing this tick
        }
    }

    bool shouldBeRunning = false;
    if (enableFact) {
        shouldBeRunning = enableFact->rawValue().toBool();
    } else {
        qCWarning(NTRIPLog) << "NTRIP settings missing; defaulting to disabled";
    }

    bool isRunning = (_tcpLink != nullptr);

    qCDebug(NTRIPLog) << "NTRIP _checkSettings:"
                      << " isRunning=" << isRunning
                      << " shouldBeRunning=" << shouldBeRunning;

    if (shouldBeRunning && !isRunning) {
        qCDebug(NTRIPLog) << "NTRIP _checkSettings: calling startNTRIP()";
        startNTRIP();
    } else if (!shouldBeRunning && isRunning) {
        qCDebug(NTRIPLog) << "NTRIP _checkSettings: calling stopNTRIP()";
        stopNTRIP();
    }
}

void NTRIPManager::_onCasterDisconnected(const QString& reason)
{
    qWarning() << "NTRIP: Disconnected by server:" << reason;
    if (reason.contains("NO_LOCATION_PROVIDED", Qt::CaseInsensitive)) {
        _casterStatus = CasterStatus::NoLocationError;
    } else {
        _casterStatus = CasterStatus::OtherError;
    }
    emit casterStatusChanged(_casterStatus);
}
