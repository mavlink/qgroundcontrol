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
void
TaisyncSettings::_readBytes()
{
    QByteArray bytesIn = _tcpSocket->read(_tcpSocket->bytesAvailable());
    qCDebug(TaisyncSettingsLog) << "Taisync settings data:" << bytesIn.size();
}

