/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "NTRIP.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "NTRIPSettings.h"

#include <QDebug>

NTRIP::NTRIP(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
}

void NTRIP::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
    
    NTRIPSettings* settings = qgcApp()->toolbox()->settingsManager()->ntripSettings();
    if (settings->ntripServerConnectEnabled()->rawValue().toBool()) {
        _rtcmMavlink = new RTCMMavlink(*toolbox);
        
        _tcpLink = new NTRIPTCPLink(settings->ntripServerHostAddress()->rawValue().toString(),
                                    settings->ntripServerPort()->rawValue().toInt(),
                                    settings->ntripUsername()->rawValue().toString(),
                                    settings->ntripPassword()->rawValue().toString(),
                                    settings->ntripMountpoint()->rawValue().toString(),
                                    settings->ntripWhitelist()->rawValue().toString(),
                                    this);
        connect(_tcpLink, &NTRIPTCPLink::error,              this, &NTRIP::_tcpError,           Qt::QueuedConnection);
        connect(_tcpLink, &NTRIPTCPLink::RTCMDataUpdate,   _rtcmMavlink, &RTCMMavlink::RTCMDataUpdate);
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
                           QObject* parent)
    : QThread       (parent)
    , _hostAddress  (hostAddress)
    , _port         (port)
    , _username     (username)
    , _password     (password)
    , _mountpoint   (mountpoint)
{
    moveToThread(this);
    start();
    for(const auto& msg: whitelist.split(',')){
        int msg_int = msg.toInt();
        if(msg_int)
            _whitelist.append(msg_int);
    }
    qCDebug(NTRIPLog) << "whitelist: " << _whitelist;
    if (!_rtcm_parsing) {
        _rtcm_parsing = new RTCMParsing();
    }
    _rtcm_parsing->reset();
    _state = NTRIPState::uninitialised;
}

NTRIPTCPLink::~NTRIPTCPLink(void)
{
    if (_socket) {
        QObject::disconnect(_socket, &QTcpSocket::readyRead, this, &NTRIPTCPLink::_readBytes);
        _socket->disconnectFromHost();
        _socket->deleteLater();
        _socket = nullptr;
    }
    quit();
    wait();
}

void NTRIPTCPLink::run(void)
{
    _hardwareConnect();
    exec();
}

void NTRIPTCPLink::_hardwareConnect()
{
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
    if ( !_mountpoint.isEmpty()){
        qCDebug(NTRIPLog) << "Sending HTTP request";
        QString auth = QString(_username + ":"  + _password).toUtf8().toBase64();
        QString query = "GET /%1 HTTP/1.0\r\nUser-Agent: NTRIP\r\nAuthorization: Basic %2\r\n\r\n";
        _socket->write(query.arg(_mountpoint).arg(auth).toUtf8());
        _state = NTRIPState::waiting_for_http_response;
    }
    // If no mountpoint is set, assume we will just get data from the tcp stream
    else{
        _state = NTRIPState::waiting_for_rtcm_header;
    }
    qCDebug(NTRIPLog) << "NTRIP Socket connected";
}

void NTRIPTCPLink::_parse(const QByteArray &buffer)
{
    for(const uint8_t& byte : buffer){
        if(_state == NTRIPState::waiting_for_rtcm_header){
            if(byte != RTCM3_PREAMBLE)
                continue;
            _state = NTRIPState::accumulating_rtcm_packet;
        }
        if(_rtcm_parsing->addByte(byte)){
            _state = NTRIPState::waiting_for_rtcm_header;
            QByteArray message((char*)_rtcm_parsing->message(), static_cast<int>(_rtcm_parsing->messageLength()));
            //TODO: Restore the following when upstreamed in Driver repo
            //uint16_t id = _rtcm_parsing->messageId();
            uint16_t id = ((uint8_t)message[3] << 4) | ((uint8_t)message[4] >> 4);
            if(_whitelist.empty() || _whitelist.contains(id)){
                emit RTCMDataUpdate(message);
                qCDebug(NTRIPLog) << "Sending " << id << "of size " << message.length();
            }
            else 
                qCDebug(NTRIPLog) << "Ignoring " << id;
            _rtcm_parsing->reset();
        }
    }
}

void NTRIPTCPLink::_readBytes(void)
{
    if (_socket) {
        if(_state == NTRIPState::waiting_for_http_response){
            QString line = _socket->readLine();
            if (line.contains("200")){
                _state = NTRIPState::waiting_for_rtcm_header;
            }
            else{
                qCWarning(NTRIPLog) << "Server responded with " << line;
                // TODO: Handle failure. Reconnect? 
                // Just move into parsing mode and hope for now.
                _state = NTRIPState::waiting_for_rtcm_header;
            }
        }
        QByteArray bytes = _socket->readAll();
        _parse(bytes);
    }
}

