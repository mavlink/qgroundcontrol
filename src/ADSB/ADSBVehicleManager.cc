#include "ADSBVehicleManager.h"
#include "MAVLinkLib.h"
#include "AppMessages.h"
#include "SettingsManager.h"
#include "ADSBVehicleManagerSettings.h"
#include "ADSBTCPLink.h"
#include "ADSBVehicle.h"
#include "QmlObjectListModel.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QThread>
#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(ADSBVehicleManagerLog, "ADSB.ADSBVehicleManager")

Q_APPLICATION_STATIC(ADSBVehicleManager, _adsbVehicleManager, SettingsManager::instance()->adsbVehicleManagerSettings());

ADSBVehicleManager::ADSBVehicleManager(ADSBVehicleManagerSettings *settings, QObject *parent)
    : QObject(parent)
    , _adsbSettings(settings)
    , _adsbVehicleCleanupTimer(new QTimer(this))
    , _adsbVehicles(new QmlObjectListModel(this))
{
    // qCDebug(ADSBVehicleManagerLog) << Q_FUNC_INFO << this;

    (void) qRegisterMetaType<ADSB::VehicleInfo_t>("ADSB::VehicleInfo_t");

    _adsbVehicleCleanupTimer->setSingleShot(false);
    _adsbVehicleCleanupTimer->setInterval(1000);
    (void) connect(_adsbVehicleCleanupTimer, &QTimer::timeout, this, &ADSBVehicleManager::_cleanupStaleVehicles);
    // Always run: vehicles can also arrive via mavlink messages, not just the tcp link
    _adsbVehicleCleanupTimer->start();

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
}

ADSBVehicleManager::~ADSBVehicleManager()
{
    _stop();

    // qCDebug(ADSBVehicleManagerLog) << Q_FUNC_INFO << this;
}

ADSBVehicleManager *ADSBVehicleManager::instance()
{
    return _adsbVehicleManager();
}

void ADSBVehicleManager::mavlinkMessageReceived(const mavlink_message_t &message)
{
    if (message.msgid != MAVLINK_MSG_ID_ADSB_VEHICLE) {
        return;
    }

    _handleADSBVehicle(message);
}

void ADSBVehicleManager::_handleADSBVehicle(const mavlink_message_t &message)
{
    mavlink_adsb_vehicle_t adsbVehicleMsg{};
    mavlink_msg_adsb_vehicle_decode(&message, &adsbVehicleMsg);

    if (adsbVehicleMsg.tslc > kMaxTimeSinceLastSeen) {
        return;
    }

    ADSB::VehicleInfo_t vehicleInfo{};

    vehicleInfo.availableFlags = ADSB::AvailableInfoTypes::fromInt(0);

    vehicleInfo.icaoAddress = adsbVehicleMsg.ICAO_address;
    vehicleInfo.lastContact = adsbVehicleMsg.tslc;

    if (adsbVehicleMsg.flags & ADSB_FLAGS_VALID_COORDS) {
        vehicleInfo.availableFlags |= ADSB::LocationAvailable;
        vehicleInfo.location.setLatitude(adsbVehicleMsg.lat / 1e7);
        vehicleInfo.location.setLongitude(adsbVehicleMsg.lon / 1e7);
    }

    if (adsbVehicleMsg.flags & ADSB_FLAGS_VALID_ALTITUDE) {
        vehicleInfo.availableFlags |= ADSB::AltitudeAvailable;
        vehicleInfo.location.setAltitude(adsbVehicleMsg.altitude / 1e3);
    }

    if (adsbVehicleMsg.flags & ADSB_FLAGS_VALID_HEADING) {
        vehicleInfo.availableFlags |= ADSB::HeadingAvailable;
        vehicleInfo.heading = adsbVehicleMsg.heading / 1e2;
    }

    if (adsbVehicleMsg.flags & ADSB_FLAGS_VALID_VELOCITY) {
        vehicleInfo.availableFlags |= ADSB::VelocityAvailable;
        vehicleInfo.velocity = adsbVehicleMsg.hor_velocity / 1e2;
    }

    if (adsbVehicleMsg.flags & ADSB_FLAGS_VALID_CALLSIGN) {
        vehicleInfo.availableFlags |= ADSB::CallsignAvailable;
        vehicleInfo.callsign = QString::fromLatin1(adsbVehicleMsg.callsign, sizeof(adsbVehicleMsg.callsign));
    }

    if (adsbVehicleMsg.flags & ADSB_FLAGS_VALID_SQUAWK) {
        vehicleInfo.availableFlags |= ADSB::SquawkAvailable;
        vehicleInfo.squawk = adsbVehicleMsg.squawk;
    }

    if (adsbVehicleMsg.flags & ADSB_FLAGS_SIMULATED) {
        vehicleInfo.simulated = true;
    }

    if (adsbVehicleMsg.flags & ADSB_FLAGS_VERTICAL_VELOCITY_VALID) {
        vehicleInfo.availableFlags |= ADSB::VerticalVelAvailable;
        vehicleInfo.verticalVel = adsbVehicleMsg.ver_velocity;
    }

    if (adsbVehicleMsg.flags & ADSB_FLAGS_BARO_VALID) {
        vehicleInfo.baro = true;
    }

    if (adsbVehicleMsg.flags & ADSB_FLAGS_SOURCE_UAT) {

    }

    adsbVehicleMsg.altitude_type = adsbVehicleMsg.altitude_type;
    adsbVehicleMsg.emitter_type = adsbVehicleMsg.emitter_type;

    adsbVehicleUpdate(vehicleInfo);
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
        _adsbVehicles->append(adsbVehicle);
        qCDebug(ADSBVehicleManagerLog) << "Added" << QString::number(adsbVehicle->icaoAddress());
    }
}

void ADSBVehicleManager::_start(const QString &hostAddress, quint16 port)
{
    if (_adsbTcpLink) {
        qCWarning(ADSBVehicleManagerLog) << "TCP Link already started";
        return;
    }

    if (hostAddress.isEmpty()) {
        qCWarning(ADSBVehicleManagerLog) << "No ADSB server host address set";
        return;
    }

    // Run the link on a worker thread so socket reads and SBS-1 parsing stay off the ui thread.
    // The thread is unparented so QObject parent-child destruction can never delete it while it
    // is still running (which is fatal); _stop() manages its lifetime explicitly.
    _adsbTcpLinkThread = new QThread();
    _adsbTcpLinkThread->setObjectName(QStringLiteral("ADSBTCPLink"));

    _adsbTcpLink = new ADSBTCPLink(hostAddress, port); // unparented, deleted when the thread finishes
    _adsbTcpLink->moveToThread(_adsbTcpLinkThread);

    (void) connect(_adsbTcpLinkThread, &QThread::started, _adsbTcpLink, &ADSBTCPLink::init);
    (void) connect(_adsbTcpLinkThread, &QThread::finished, _adsbTcpLink, &QObject::deleteLater);
    (void) connect(_adsbTcpLink, &ADSBTCPLink::adsbVehicleUpdate, this, &ADSBVehicleManager::adsbVehicleUpdate, Qt::QueuedConnection);
    (void) connect(_adsbTcpLink, &ADSBTCPLink::errorOccurred, this, &ADSBVehicleManager::_linkError, Qt::QueuedConnection);

    _adsbTcpLinkThread->start();
}

void ADSBVehicleManager::_stop()
{
    if (!_adsbTcpLinkThread) {
        return;
    }

    _adsbTcpLinkThread->quit();
    if (!_adsbTcpLinkThread->wait(kThreadStopTimeoutMs)) {
        // Bail out without deleting or resetting state: destroying a still-running QThread is
        // fatal, and clearing the pointers would allow a second link to be started alongside it.
        qCWarning(ADSBVehicleManagerLog) << "ADSBTCPLink thread failed to stop";
        return;
    }
    // wait() succeeded so the thread has finished: delete it directly rather than with
    // deleteLater(), which would leak during shutdown once the event loop stops running.
    delete _adsbTcpLinkThread;
    _adsbTcpLinkThread = nullptr;
    _adsbTcpLink = nullptr; // deleted via the thread finished connection

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

    QGC::showAppMessage(msg);
}
