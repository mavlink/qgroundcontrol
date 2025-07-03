/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "NTRIPTCPLink.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "RTCMMavlink.h"

QGC_LOGGING_CATEGORY(NTRIPManagerLog, "qgc.gps.ntripmanager")

NTRIPManager::NTRIPManager(QObject *parent)
    : QObject(parent)
{
    qCDebug(NTRIPManagerLog) << this;

    NTRIPSettings *settings = SettingsManager::instance()->ntripSettings();
    if (!settings->ntripServerConnectEnabled()->rawValue().toBool()) {
        qCDebug(NTRIPManagerLog) << "NTRIP Server is not enabled";
        return;
    }

    _rtcmMavlink = new RTCMMavlink(this);

    _tcpLink = new NTRIPTCPLink(
        settings->ntripServerHostAddress()->rawValue().toString(),
        settings->ntripServerPort()->rawValue().toInt(),
        settings->ntripUsername()->rawValue().toString(),
        settings->ntripPassword()->rawValue().toString(),
        settings->ntripMountpoint()->rawValue().toString(),
        settings->ntripWhitelist()->rawValue().toString(),
        settings->ntripEnableVRS()->rawValue().toBool(),
        this
    );

    (void) connect(_tcpLink, &NTRIPTCPLink::error, this, &NTRIPManager::_tcpError, Qt::QueuedConnection);
    (void) connect(_tcpLink, &NTRIPTCPLink::RTCMDataUpdate, _rtcmMavlink, &RTCMMavlink::RTCMDataUpdate);
}

NTRIPManager::~NTRIPManager()
{
    qCDebug(NTRIPManagerLog) << this;
}

void NTRIPManager::_tcpError(const QString &errorMsg)
{
    qgcApp()->showAppMessage(tr("NTRIP Server Error: %1").arg(errorMsg));
}
