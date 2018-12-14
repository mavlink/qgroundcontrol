/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TaisyncSettings.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#include "VideoManager.h"


QGC_LOGGING_CATEGORY(TaisyncSettingsLog, "TaisyncSettingsLog")

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
void
TaisyncSettings::start()
{
    qCDebug(TaisyncSettingsLog) << "Start Taisync Settings";
    _start(TAISYNC_SETTINGS_PORT);
}

//-----------------------------------------------------------------------------
bool
TaisyncSettings::requestSettings()
{
    if(_tcpSocket) {
        QString req = QString(kGetReq).arg("/v1/baseband.json");
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
    qCDebug(TaisyncSettingsLog) << "Taisync settings data:" << bytesIn.size();
    qCDebug(TaisyncSettingsLog) << QString(bytesIn);
    if(bytesIn.contains("200 OK")) {
        //-- Link Status?
        int idx = bytesIn.indexOf('{');
        QJsonParseError jsonParseError;
        QJsonDocument doc = QJsonDocument::fromJson(bytesIn.mid(idx), &jsonParseError);
        if (jsonParseError.error != QJsonParseError::NoError) {
            qWarning() <<  "Unable to parse Taisync response:" << jsonParseError.errorString() << jsonParseError.offset;
            return;
        }
        QJsonObject jObj = doc.object();
        //-- Link Status?
        if(bytesIn.contains("\"flight\":")) {
            _linkConnected  = jObj["flight"].toBool(_linkConnected);
            _linkVidFormat  = jObj["videoformat"].toString(_linkVidFormat);
            _downlinkRSSI   = jObj["radiorssi"].toInt(_downlinkRSSI);
            _uplinkRSSI     = jObj["hdrssi"].toInt(_uplinkRSSI);
            emit linkChanged();
        }
    }
}

