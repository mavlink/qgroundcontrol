/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ADSBVehicleManager.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "ADSBVehicleManagerSettings.h"
#include "ADSBTCPLink.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(ADSBVehicleManagerLog, "qgc.adsb.adsbvehiclemanager")

ADSBVehicleManager::ADSBVehicleManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
}

void ADSBVehicleManager::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
    connect(&_adsbVehicleCleanupTimer, &QTimer::timeout, this, &ADSBVehicleManager::_cleanupStaleVehicles);
    _adsbVehicleCleanupTimer.setSingleShot(false);
    _adsbVehicleCleanupTimer.start(1000);

    ADSBVehicleManagerSettings* settings = toolbox->settingsManager()->adsbVehicleManagerSettings();
    if (settings->adsbServerConnectEnabled()->rawValue().toBool()) {
        _tcpLink = new ADSBTCPLink(settings->adsbServerHostAddress()->rawValue().toString(), settings->adsbServerPort()->rawValue().toInt(), this);
        connect(_tcpLink, &ADSBTCPLink::adsbVehicleUpdate,  this, &ADSBVehicleManager::adsbVehicleUpdate,   Qt::QueuedConnection);
        connect(_tcpLink, &ADSBTCPLink::error,              this, &ADSBVehicleManager::_tcpError,           Qt::QueuedConnection);
    }
}

void ADSBVehicleManager::_cleanupStaleVehicles()
{
    // Remove all expired ADSB vehicles
    for (int i=_adsbVehicles.count()-1; i>=0; i--) {
        ADSBVehicle* adsbVehicle = _adsbVehicles.value<ADSBVehicle*>(i);
        if (adsbVehicle->expired()) {
            qCDebug(ADSBVehicleManagerLog) << "Expired " << QStringLiteral("%1").arg(adsbVehicle->icaoAddress(), 0, 16);
            _adsbVehicles.removeAt(i);
            _adsbICAOMap.remove(adsbVehicle->icaoAddress());
            adsbVehicle->deleteLater();
        }
    }
}

void ADSBVehicleManager::adsbVehicleUpdate(const ADSBVehicle::ADSBVehicleInfo_t vehicleInfo)
{
    uint32_t icaoAddress = vehicleInfo.icaoAddress;

    if (_adsbICAOMap.contains(icaoAddress)) {
        _adsbICAOMap[icaoAddress]->update(vehicleInfo);
    } else {
        if (vehicleInfo.availableFlags & ADSBVehicle::LocationAvailable) {
            ADSBVehicle* adsbVehicle = new ADSBVehicle(vehicleInfo, this);
            _adsbICAOMap[icaoAddress] = adsbVehicle;
            _adsbVehicles.append(adsbVehicle);
            qCDebug(ADSBVehicleManagerLog) << "Added " << QStringLiteral("%1").arg(adsbVehicle->icaoAddress(), 0, 16);
        }
    }
}

void ADSBVehicleManager::_tcpError(const QString errorMsg)
{
    _app->showAppMessage(tr("ADSB Server Error: %1").arg(errorMsg));
}
