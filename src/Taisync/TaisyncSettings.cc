/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TaisyncManager.h"
#include "TaisyncSettings.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#include "VideoManager.h"

static const char* kPostReq =
    "POST %1 HTTP/1.1\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %2\r\n\r\n"
    "%3";

static const char* kGetReq      = "GET %1 HTTP/1.1\r\n\r\n";
static const char* kRadioURI    = "/v1/radio.json";
static const char* kVideoURI    = "/v1/video.json";
static const char* kRTSPURI     = "/v1/rtspuri.json";
static const char* kIPAddrURI   = "/v1/ipaddr.json";

//-----------------------------------------------------------------------------
TaisyncSettings::TaisyncSettings(QObject* parent)
    : TaisyncHandler(parent)
{
}

//-----------------------------------------------------------------------------
bool TaisyncSettings::start()
{
    qCDebug(TaisyncLog) << "Start Taisync Settings";
#if defined(__ios__) || defined(__android__)
    return _start(TAISYNC_SETTINGS_PORT);
#else
    return _start(TAISYNC_SETTINGS_PORT, QHostAddress(qgcApp()->toolbox()->taisyncManager()->remoteIPAddr()));
#endif
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::requestLinkStatus()
{
    return _request("/v1/baseband.json");
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::requestDevInfo()
{
    return _request("/v1/device.json");
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::requestFreqScan()
{
    return _request("/v1/freqscan.json");
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::requestVideoSettings()
{
    return _request(kVideoURI);
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::requestRadioSettings()
{
    return _request(kRadioURI);
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::requestIPSettings()
{
    return _request(kIPAddrURI);
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::requestRTSPURISettings()
{
    return _request(kRTSPURI);
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::_request(const QString& request)
{
    if(_tcpSocket) {
        QString req = QString(kGetReq).arg(request);
        //qCDebug(TaisyncVerbose) << "Request" << req;
        _tcpSocket->write(req.toUtf8());
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::_post(const QString& post, const QString &postPayload)
{
    if(_tcpSocket) {
        QString req = QString(kPostReq).arg(post).arg(postPayload.size()).arg(postPayload);
        qCDebug(TaisyncVerbose) << "Post" << req;
        _tcpSocket->write(req.toUtf8());
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::setRadioSettings(const QString& mode, const QString& channel)
{
    static const char* kRadioPost = "{\"mode\":\"%1\",\"freq\":\"%2\"}";
    QString post = QString(kRadioPost).arg(mode).arg(channel);
    return _post(kRadioURI, post);
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::setVideoSettings(const QString& output, const QString& mode, const QString& rate)
{
    static const char* kVideoPost = "{\"decode\":\"%1\",\"mode\":\"%2\",\"maxbitrate\":\"%3\"}";
    QString post = QString(kVideoPost).arg(output).arg(mode).arg(rate);
    return _post(kVideoURI, post);
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::setRTSPSettings(const QString& uri, const QString& account, const QString& password)
{
    static const char* kRTSPPost = "{\"rtspURI\":\"%1\",\"account\":\"%2\",\"passwd\":\"%3\"}";
    QString post = QString(kRTSPPost).arg(uri).arg(account).arg(password);
    return _post(kRTSPURI, post);
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::setIPSettings(const QString& localIP, const QString& remoteIP, const QString& netMask)
{
    static const char* kRTSPPost = "{\"ipaddr\":\"%1\",\"netmask\":\"%2\",\"usbEthIp\":\"%3\"}";
    QString post = QString(kRTSPPost).arg(localIP).arg(netMask).arg(remoteIP);
    return _post(kIPAddrURI, post);
}

//-----------------------------------------------------------------------------
void
TaisyncSettings::_readBytes()
{
    QByteArray bytesIn = _tcpSocket->read(_tcpSocket->bytesAvailable());
    //-- Go straight to Json payload
    int idx = bytesIn.indexOf('{');
    //-- We may receive more than one response within one TCP packet.
    while(idx >= 0) {
        bytesIn = bytesIn.mid(idx);
        idx = bytesIn.indexOf('}');
        if(idx > 0) {
            QByteArray data = bytesIn.left(idx + 1);
            emit updateSettings(data);
            bytesIn = bytesIn.mid(idx+1);
            idx = bytesIn.indexOf('{');
        }
    }
}

