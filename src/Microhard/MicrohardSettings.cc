/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MicrohardSettings.h"
#include "MicrohardManager.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#include "VideoManager.h"

//-----------------------------------------------------------------------------
MicrohardSettings::MicrohardSettings(QObject* parent)
    : MicrohardHandler(parent)
{
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::start()
{
    qCDebug(MicrohardLog) << "Start Microhard Settings";
    return _start(MICROHARD_SETTINGS_PORT, QHostAddress(qgcApp()->toolbox()->microhardManager()->remoteIPAddr()));
}

//-----------------------------------------------------------------------------
bool
MicrohardSettings::requestLinkStatus()
{
    return _request("/v1/baseband.json");
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
    QString s_data = QString::fromStdString(bytesIn.toStdString());

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

