/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "ADSBVehicle.h"
#include "QmlObjectListModel.h"
#include "QGCLoggingCategory.h"

#include <QtCore/qapplicationstatic.h>
#include <QtCore/QTimer>
#include <qassert.h>

QGC_LOGGING_CATEGORY(ADSBVehicleManagerLog, "qgc.adsb.adsbvehiclemanager")

Q_APPLICATION_STATIC(ADSBVehicleManager, _adsbVehicleManager, SettingsManager::instance()->adsbVehicleManagerSettings());

ADSBVehicleManager::ADSBVehicleManager(ADSBVehicleManagerSettings *settings, QObject *parent)
    : QObject(parent)
    , _adsbSettings(settings)
    , _adsbVehicleCleanupTimer(new QTimer(this))
    , _adsbVehicles(new QmlObjectListModel(this))
{
    (void) qRegisterMetaType<ADSB::VehicleInfo_t>("ADSB::VehicleInfo_t");

    _adsbVehicleCleanupTimer->setSingleShot(false);
    _adsbVehicleCleanupTimer->setInterval(1000);
    (void) connect(_adsbVehicleCleanupTimer, &QTimer::timeout, this, &ADSBVehicleManager::_cleanupStaleVehicles);

    Fact* const adsbEnabled = _adsbSettings->adsbServerConnectEnabled();
    Fact* const hostAddress = _adsbSettings->adsbServerHostAddress();
    Fact* const port = _adsbSettings->adsbServerPort();

    (void) connect(adsbEnabled, &Fact::rawValueChanged, this, [this, hostAddress, port](QVariant value) {
        if (value.toBool()) {
            _start(hostAddress->rawValue().toString(), port->rawValue().toUInt());
        } else {
            _stop();
        }
    });

    if (adsbEnabled->rawValue().toBool()) {
        _start(hostAddress->rawValue().toString(), port->rawValue().toUInt());
    }

    // qCDebug(ADSBTCPLinkLog) << Q_FUNC_INFO << this;
}

ADSBVehicleManager::~ADSBVehicleManager()
{
    // qCDebug(ADSBTCPLinkLog) << Q_FUNC_INFO << this;
}

ADSBVehicleManager *ADSBVehicleManager::instance()
{
    return _adsbVehicleManager();
}

void ADSBVehicleManager::adsbVehicleUpdate(const ADSB::VehicleInfo_t &vehicleInfo)
{
    const uint32_t icaoAddress = vehicleInfo.icaoAddress;
    if (_adsbICAOMap.contains(icaoAddress)) {
        _adsbICAOMap[icaoAddress]->update(vehicleInfo);
        return;
    }

    if (vehicleInfo.availableFlags & ADSB::LocationAvailable) {
        ADSBVehicle* const adsbVehicle = new ADSBVehicle(vehicleInfo, this);
        _adsbICAOMap[icaoAddress] = adsbVehicle;
        (void) _adsbVehicles->append(adsbVehicle);
        qCDebug(ADSBVehicleManagerLog) << "Added" << QString::number(adsbVehicle->icaoAddress());
    }
}

void ADSBVehicleManager::_start(const QString &hostAddress, quint16 port)
{
    Q_ASSERT(!_adsbTcpLink);
    _adsbTcpLink = new ADSBTCPLink(QHostAddress(hostAddress), port, this);
    (void) connect(_adsbTcpLink, &ADSBTCPLink::adsbVehicleUpdate, this, &ADSBVehicleManager::adsbVehicleUpdate, Qt::AutoConnection);
    (void) connect(_adsbTcpLink, &ADSBTCPLink::errorOccurred, this, &ADSBVehicleManager::_linkError, Qt::AutoConnection);

    _adsbVehicleCleanupTimer->start();
}

void ADSBVehicleManager::_stop()
{
    Q_CHECK_PTR(_adsbTcpLink);
    _adsbTcpLink->deleteLater();
    _adsbTcpLink = nullptr;

    _adsbVehicleCleanupTimer->stop();

    _adsbVehicles->clearAndDeleteContents();
    _adsbICAOMap.clear();
}

void ADSBVehicleManager::_cleanupStaleVehicles()
{
    for (qsizetype i = _adsbVehicles->count() - 1; i >= 0; i--) {
        ADSBVehicle* const adsbVehicle = _adsbVehicles->value<ADSBVehicle*>(i);
        if (adsbVehicle->expired()) {
            qCDebug(ADSBVehicleManagerLog) << "Expired" << QString::number(adsbVehicle->icaoAddress());
            (void) _adsbVehicles->removeAt(i);
            (void) _adsbICAOMap.remove(adsbVehicle->icaoAddress());
            adsbVehicle->deleteLater();
        }
    }
}

void ADSBVehicleManager::_linkError(const QString &errorMsg, bool stopped)
{
    qCDebug(ADSBVehicleManagerLog) << errorMsg;

    QString msg = QStringLiteral("ADSB Server Error: %1").arg(errorMsg);

    if (stopped) {
        (void) msg.append("\nADSB has been disabled");
        _adsbSettings->adsbServerConnectEnabled()->setRawValue(false);
    }

    qgcApp()->showAppMessage(msg);
}
