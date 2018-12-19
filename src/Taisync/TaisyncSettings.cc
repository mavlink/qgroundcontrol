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

static const char* kGetReq = "GET %1 HTTP/1.1\r\n\r\n";

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
    if(_tcpSocket) {
        QString req = QString(kGetReq).arg("/v1/baseband.json");
        //qCDebug(TaisyncVerbose) << "Request" << req;
        _tcpSocket->write(req.toUtf8());
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::requestDevInfo()
{
    if(_tcpSocket) {
        QString req = QString(kGetReq).arg("/v1/device.json");
        //qCDebug(TaisyncVerbose) << "Request" << req;
        _tcpSocket->write(req.toUtf8());
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::requestFreqScan()
{
    if(_tcpSocket) {
        QString req = QString(kGetReq).arg("/v1/freqscan.json");
        //qCDebug(TaisyncVerbose) << "Request" << req;
        _tcpSocket->write(req.toUtf8());
        return true;
    }
    return false;
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

