#include "GPSConnectionSettingsController.h"

#include <QtCore/QPointer>

#include <utility>

#include "AutoConnectSettings.h"
#include "GPSReceiverAutoConnect.h"
#include "GPSSettings.h"
#include "NMEASourceManager.h"
#include "QGCLoggingCategory.h"
#include "RTKSettings.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(GPSConnectionSettingsControllerLog, "GPS.Integration.GPSConnectionSettingsController")

GPSConnectionSettingsController::GPSConnectionSettingsController(SettingsManager& settings,
                                                                 GPSReceiverAutoConnect& receiver,
                                                                 NMEASourceManager& nmea,
                                                                 std::function<bool()> suspended, QObject* parent)
    : QObject(parent)
    , _settings(settings)
    , _receiver(receiver)
    , _nmea(nmea)
    , _connectionsSuspended(std::move(suspended))
{
    qCDebug(GPSConnectionSettingsControllerLog) << this;
    auto* nmeaSettings = settings.autoConnectSettings();
    for (Fact* fact :
         {nmeaSettings->nmeaSource(), nmeaSettings->autoConnectNmeaPort(), nmeaSettings->autoConnectNmeaBaud(),
          nmeaSettings->nmeaUdpPort(), nmeaSettings->nmeaTcpHost(), nmeaSettings->nmeaTcpPort(),
          nmeaSettings->nmeaReceiverMode(), nmeaSettings->nmeaAutoConnect()}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSConnectionSettingsController::_updateNmeaSettings);
    }
    auto* receiverSettings = settings.rtkSettings();
    for (Fact* fact : {receiverSettings->connectionType(), receiverSettings->serialDevice(),
                       receiverSettings->networkReceiverType(), receiverSettings->receiverRole()}) {
        connect(fact, &Fact::rawValueChanged, this, [this]() { _updateReceiverSettings(true); });
    }
    for (Fact* fact :
         {receiverSettings->networkBaseHost(), receiverSettings->networkBasePort(), receiverSettings->udpLocalPort(),
          receiverSettings->useFixedBasePosition(), receiverSettings->surveyInAccuracyLimit(),
          receiverSettings->surveyInMinObservationDuration(), receiverSettings->fixedBasePositionLatitude(),
          receiverSettings->fixedBasePositionLongitude(), receiverSettings->fixedBasePositionAltitude(),
          receiverSettings->fixedBasePositionAccuracy(), receiverSettings->constellationMask(),
          receiverSettings->dynamicModel(), receiverSettings->outputRateHz(), receiverSettings->headingOffsetDeg(),
          settings.autoConnectSettings()->autoConnectRTKGPS(),
          settings.autoConnectSettings()->autoConnectNetworkRTKGPS()}) {
        connect(fact, &Fact::rawValueChanged, this, [this]() { _updateReceiverSettings(); });
    }
    _nmea.setSuspended(connectionsSuspended());
    _receiver.setSuspended(connectionsSuspended());
    _updateNmeaSettings();
    _updateReceiverSettings();
}

GPSConnectionSettingsController::~GPSConnectionSettingsController()
{
    qCDebug(GPSConnectionSettingsControllerLog) << this;
}

void GPSConnectionSettingsController::start()
{
    if (_initialized || _shutdown) {
        return;
    }
    _initialized = true;
    const QPointer<GPSConnectionSettingsController> guard(this);
    _updateNmeaSettings();
    if (!guard || _shutdown) {
        return;
    }
    _updateReceiverSettings();
    if (guard && !_shutdown) {
        updateConnections();
    }
}

void GPSConnectionSettingsController::shutdown()
{
    _shutdown = true;
    ++_nmeaSettingsRevision;
    ++_receiverSettingsRevision;
}

bool GPSConnectionSettingsController::connectionsSuspended() const
{
    return _connectionsSuspended && _connectionsSuspended();
}

void GPSConnectionSettingsController::beginReceiverSettingsBatch()
{
    ++_receiverSettingsBatchDepth;
}

void GPSConnectionSettingsController::endReceiverSettingsBatch()
{
    if (!_receiverSettingsBatchDepth || --_receiverSettingsBatchDepth) {
        return;
    }
    _updateReceiverSettings(std::exchange(_receiverRestartPending, false));
}

void GPSConnectionSettingsController::updateConnections()
{
    if (_shutdown) {
        return;
    }
    const QPointer<GPSConnectionSettingsController> guard(this);
    const bool suspended = _connectionsSuspended && _connectionsSuspended();
    _nmea.setSuspended(suspended);
    if (!guard || _shutdown) {
        return;
    }
    _receiver.setSuspended(suspended);
    if (!guard || _shutdown || suspended) {
        return;
    }
    _nmea.update();
    if (guard && !_shutdown) {
        _receiver.update();
    }
}

void GPSConnectionSettingsController::_updateNmeaSettings()
{
    if (_shutdown) {
        return;
    }
    const QPointer<GPSConnectionSettingsController> guard(this);
    const quint64 revision = ++_nmeaSettingsRevision;
    auto* settings = _settings.autoConnectSettings();
    const auto connection = GPSSettings::nmea(*settings);
    _nmea.setProfile(connection.profile);
    if (guard && !_shutdown && revision == _nmeaSettingsRevision) {
        _nmea.setAutoConnect(connection.automatic && _initialized);
    }
}

void GPSConnectionSettingsController::_updateReceiverSettings(bool restart)
{
    if (_shutdown) {
        return;
    }
    if (_receiverSettingsBatchDepth) {
        _receiverRestartPending |= restart;
        return;
    }
    const QPointer<GPSConnectionSettingsController> guard(this);
    const quint64 revision = ++_receiverSettingsRevision;
    auto* settings = &_settings;
    const auto connection = GPSSettings::receiver(*settings->rtkSettings(), *settings->autoConnectSettings());
    _receiver.setProfile(connection.profile, restart);
    if (!guard || _shutdown || revision != _receiverSettingsRevision) {
        return;
    }
    _receiver.setAutoConnect(connection.automatic && _initialized);
    if (guard && !_shutdown && revision == _receiverSettingsRevision && restart) {
        emit receiverSettingsChanged();
    }
}
