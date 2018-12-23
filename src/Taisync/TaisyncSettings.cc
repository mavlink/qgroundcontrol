/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    return _start(80, QHostAddress(TAISYNC_SETTINGS_TARGET));
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
    return _request("/v1/video.json");
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::requestRadioSettings()
{
    return _request(kRadioURI);
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
        qCDebug(TaisyncVerbose) << "Request" << req;
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
    QString post = QString(kRadioPost).arg(mode.size()).arg(channel);
    return _post(kRadioURI, post);
}

//-----------------------------------------------------------------------------
void
TaisyncSettings::_readBytes()
{
    QByteArray bytesIn = _tcpSocket->read(_tcpSocket->bytesAvailable());
    //qCDebug(TaisyncVerbose) << "Taisync settings data:" << bytesIn.size();
    //qCDebug(TaisyncVerbose) << QString(bytesIn);
    //-- Go straight to Json payload
    int idx = bytesIn.indexOf('{');
    if(idx > 0) {
        emit updateSettings(bytesIn.mid(idx));
    }
}

