#include "GPSReceiverSession.h"

#include <cmath>
#include <limits>
#include <utility>
#include <variant>

#include "GPSCorrectionManager.h"
#include "GPSReceiverDescriptor.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSReceiverSessionLog, "GPS.Receiver.GPSReceiverSession")

void GPSBaseStationState::applyStatus(const GPSBaseStatus& status)
{
    if (!mode) {
        return;
    }
    state = status.state;
    if (status.fixedCheck) {
        fixedCheck = status.fixedCheck;
    }
    const bool located =
        std::isfinite(status.position.latitudeDegrees) && std::isfinite(status.position.longitudeDegrees);
    const double accuracy =
        status.meanAccuracyMeters.value_or(assumedAccuracyMeters.value_or(std::numeric_limits<double>::quiet_NaN()));
    if (status.state == GPSBaseState::Valid && located && std::isfinite(accuracy)) {
        position = std::pair(status.position, accuracy);
    } else {
        position.reset();
    }
}

GPSReceiverConfig gpsReceiverConfigFor(const GPSBaseStationConfig& base, GPSType type, uint32_t baudRate,
                                       bool allowPersistentChanges, QString* error)
{
    GPSReceiverConfig config{.baudRate = baudRate, .allowPersistentChanges = allowPersistentChanges};
    if (type != GPSType::Passive) {
        config.base = base;
        // The option is hidden for receivers that cannot send MSM4, so it never blocks their connection.
        config.base.compactObservations = base.compactObservations && gpsReceiverCapabilities(type).compactObservations;
        // So is the choice of constellations for receivers that send their own set.
        if (!gpsReceiverCapabilities(type).correctionConstellations) {
            config.base.correctionConstellations = GPS_ALL_CORRECTION_CONSTELLATIONS;
        }
    }
    QString diagnostic = gpsReceiverConfigError(type, config);
    if (error) {
        *error = std::move(diagnostic);
    }
    return config;
}

GPSBaseStationState gpsBaseStationStateFor(GPSType type, const GPSReceiverConfig& config)
{
    GPSBaseStationState state;
    if (type == GPSType::Passive) {
        return state;
    }
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&config.base.mode)) {
        state.mode = BaseModeDefinition::Mode::BaseFixed;
        state.assumedAccuracyMeters = fixed->accuracyMeters;
    } else if (const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&config.base.mode)) {
        state.mode = BaseModeDefinition::Mode::BaseSurveyIn;
        state.assumedAccuracyMeters = survey->accuracyMeters;
    } else {
        state.mode = BaseModeDefinition::Mode::BaseReceiverAveraging;
    }
    return state;
}

GPSReceiverSession::GPSReceiverSession(quint64 id, Connection connection, QObject* parent)
    : QObject(parent)
    , _sessionId(id)
    , _connection(std::move(connection))
    , _base(gpsBaseStationStateFor(_connection.type, _connection.config))
{
    qCDebug(GPSReceiverSessionLog) << this;
}

GPSReceiverSession::~GPSReceiverSession()
{
    retire();
    qCDebug(GPSReceiverSessionLog) << this;
}

void GPSReceiverSession::registerCorrections(GPSCorrectionManager* manager, const QString& sourceInstance)
{
    if (_retired || !_connection.forwardsCorrections || !manager) {
        return;
    }
    _correction = manager->openSource(GPSCorrectionSettings::LocalReceiver, sourceInstance);
}

bool GPSReceiverSession::start(const GPSReceiverWorkerFactory& factory,
                               GPSReceiverWorker::TransportFactory transportFactory)
{
    if (_retired || _worker) {
        return false;
    }
    const GPSType type = _connection.type;
    _worker = factory ? factory(std::move(transportFactory), type, _connection.config, this)
                      : new GPSReceiverWorker(std::move(transportFactory), type, _connection.config, this);
    if (!_worker) {
        return false;
    }
    GPSReceiverWorker* const worker = _worker;
    worker->setEndsWhenIdle(_connection.role == RTKSettings::ConfiguredBase);
    worker->setDetectionHint(_connection.detectionHint);
    (void) connect(
        worker, &GPSReceiverWorker::finished, this,
        [this]() { _onReport(GPSReceiverEnded{GPSConnectionError::DeviceError, {}}); }, Qt::QueuedConnection);
    (void) connect(worker, &GPSReceiverWorker::finished, worker, &QObject::deleteLater);
    (void) connect(worker, &GPSReceiverWorker::reported, this, &GPSReceiverSession::_onReport, Qt::QueuedConnection);
    worker->start();
    return true;
}

void GPSReceiverSession::registerPosition(PositionManager* positionManager, GPSSourceHealth* producer,
                                          PositionManager::Receiver receiver)
{
    if (_retired || !positionManager || _position) {
        return;
    }
    auto registration = positionManager->registerReceiver(producer, _sessionId, receiver);
    // An observer that retired this session during registration leaves the registration to end here.
    if (!_retired) {
        _position = std::move(registration);
    }
}

void GPSReceiverSession::retire()
{
    if (std::exchange(_retired, true)) {
        return;
    }
    const QPointer<GPSReceiverWorker> worker = _worker;
    if (worker) {
        (void) disconnect(worker, nullptr, this, nullptr);
        // Registration observers may delete the owner; the worker must already be independent.
        worker->setParent(nullptr);
        worker->stop();
    }
    _position.reset();
    _correction.reset();
}

GPSObservation GPSReceiverSession::observation(const GPSPositionReport& report, quint64 receivedAtUs) const
{
    auto observation = _base.position ? GPSObservation::fromSurveyedPosition(_base.position->first,
                                                                             _base.position->second, receivedAtUs)
                                      : GPSObservation::fromNavigation(report.navigation, receivedAtUs);
    observation.sessionId = _sessionId;
    return observation;
}

void GPSReceiverSession::_onReport(const GPSReceiverEvent& event)
{
    if (!_retired && std::visit([this](const auto& report) { return _take(report); }, event)) {
        emit reported(event);
    }
}

bool GPSReceiverSession::_take(const GPSBaseStatus& report)
{
    if (_connection.role != RTKSettings::ConfiguredBase) {
        return false;
    }
    _base.applyStatus(report);
    return true;
}

bool GPSReceiverSession::_take(const GPSReceiverReady& ready)
{
    if (std::exchange(_identity, ready.identity) != ready.identity) {
        qCDebug(GPSReceiverSessionLog) << "Receiver identity:" << ready.identity;
    }
    _detecting.reset();
    if (_connection.type == GPSType::Automatic) {
        _detected = ready.detected;
    }
    return true;
}

bool GPSReceiverSession::_take(const GPSDetectionProgress& progress)
{
    if (_connection.type != GPSType::Automatic) {
        return false;
    }
    _detecting = progress;
    return true;
}

bool GPSReceiverSession::_take(const GPSReceiverDetected& detected)
{
    if (_connection.type != GPSType::Automatic && _connection.type != GPSType::Passive) {
        return false;
    }
    _detecting.reset();
    _detectedType = detected.type;
    qCDebug(GPSReceiverSessionLog) << "Detected receiver family:" << detected.type;
    return true;
}

bool GPSReceiverSession::_take(const GPSReceiverRTCM& rtcm)
{
    if (_correction) {
        _correction->submit(rtcm.frame, rtcm.receivedAtMs);
    }
    return false;
}
