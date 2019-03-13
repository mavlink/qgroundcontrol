/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MicrohardSettings.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#include "VideoManager.h"

//-----------------------------------------------------------------------------
MicrohardSettings::MicrohardSettings(QObject* parent)
    : MicrohardHandler(parent)
{
}

//-----------------------------------------------------------------------------
bool MicrohardSettings::start()
{
    qCDebug(MicrohardLog) << "Start Microhard Settings";
    return false;
//    return _start(MICROHARD_SETTINGS_PORT, QHostAddress(qgcApp()->toolbox()->microhardManager()->remoteIPAddr()));
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::requestLinkStatus()
{
    return _request("/v1/baseband.json");
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::requestDevInfo()
{
    return _request("/v1/device.json");
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::requestFreqScan()
{
    return _request("/v1/freqscan.json");
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::requestVideoSettings()
{
    return false;
//    return _request(kVideoURI);
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::requestRadioSettings()
{
    return false;
//    return _request(kRadioURI);
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::requestIPSettings()
{
    return false;
//    return _request(kIPAddrURI);
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::requestRTSPURISettings()
{
    return false;
//    return _request(kRTSPURI);
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::_request(const QString& request)
{
    /*
    if(_tcpSocket) {
        QString req = QString(kGetReq).arg(request);
        //qCDebug(MicrohardVerbose) << "Request" << req;
        _tcpSocket->write(req.toUtf8());
        return true;
    }
    */
    return false;
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::_post(const QString& post, const QString &postPayload)
{
    /*
    if(_tcpSocket) {
        QString req = QString(kPostReq).arg(post).arg(postPayload.size()).arg(postPayload);
        qCDebug(MicrohardVerbose) << "Post" << req;
        _tcpSocket->write(req.toUtf8());
        return true;
    }
    */
    return false;
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::setRadioSettings(const QString& mode, const QString& channel)
{
//    static const char* kRadioPost = "{\"mode\":\"%1\",\"freq\":\"%2\"}";
//    QString post = QString(kRadioPost).arg(mode).arg(channel);
    return false;
//    return _post(kRadioURI, post);
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::setVideoSettings(const QString& output, const QString& mode, const QString& rate)
{
    return false;
/*
    static const char* kVideoPost = "{\"decode\":\"%1\",\"mode\":\"%2\",\"maxbitrate\":\"%3\"}";
    QString post = QString(kVideoPost).arg(output).arg(mode).arg(rate);
    return _post(kVideoURI, post);
    */
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::setRTSPSettings(const QString& uri, const QString& account, const QString& password)
{
    return false;
//    static const char* kRTSPPost = "{\"rtspURI\":\"%1\",\"account\":\"%2\",\"passwd\":\"%3\"}";
//    QString post = QString(kRTSPPost).arg(uri).arg(account).arg(password);
//    return _post(kRTSPURI, post);
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::setIPSettings(const QString& localIP, const QString& remoteIP, const QString& netMask)
{
    return false;
//    static const char* kRTSPPost = "{\"ipaddr\":\"%1\",\"netmask\":\"%2\",\"usbEthIp\":\"%3\"}";
//    QString post = QString(kRTSPPost).arg(localIP).arg(netMask).arg(remoteIP);
//    return _post(kIPAddrURI, post);
}

//-----------------------------------------------------------------------------
void
MicrohardSettings::_readBytes()
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

