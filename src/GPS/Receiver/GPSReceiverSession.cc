#include "GPSReceiverSession.h"

#include <cmath>
#include <limits>
#include <utility>

#include "GPSCorrectionManager.h"
#include "GPSReceiverDescriptor.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSReceiverSessionLog, "GPS.Receiver.GPSReceiverSession")

void GPSBaseStationState::applySurvey(const GPSSurveyReport& report)
{
    if (!mode || *mode == BaseModeDefinition::Mode::BaseFixed) {
        return;
    }
    const bool located =
        std::isfinite(report.position.latitudeDegrees) && std::isfinite(report.position.longitudeDegrees);
    const double accuracy = report.meanAccuracyMeters.value_or(
        surveyAccuracyLimitMeters.value_or(std::numeric_limits<double>::quiet_NaN()));
    if (report.valid && located && std::isfinite(accuracy)) {
        position = std::pair(report.position, accuracy);
    } else {
        position.reset();
    }
}

GPSReceiverConfig gpsReceiverConfigFor(const GPSBaseStationConfig& base, GPSType type, uint32_t baudRate,
                                       bool allowPersistentChanges, QString* error)
{
    GPSReceiverConfig config{.baudRate = baudRate, .allowPersistentChanges = allowPersistentChanges};
    if (type != GPSType::passive) {
        config.base = base;
        // The option is hidden for receivers that cannot send MSM4, so it never blocks their connection.
        config.base.compactObservations = base.compactObservations && gpsReceiverCapabilities(type).compactObservations;
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
    if (type == GPSType::passive) {
        return state;
    }
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&config.base.mode)) {
        state.mode = BaseModeDefinition::Mode::BaseFixed;
        state.position = std::pair(fixed->position, static_cast<double>(fixed->accuracyMeters));
    } else if (const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&config.base.mode)) {
        state.mode = BaseModeDefinition::Mode::BaseSurveyIn;
        state.surveyAccuracyLimitMeters = survey->accuracyMeters;
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
    (void) connect(
        worker, &GPSReceiverWorker::finished, this,
        [this]() {
            if (!_retired) {
                emit ended(GPSConnectionError::DeviceError, {});
            }
        },
        Qt::QueuedConnection);
    (void) connect(worker, &GPSReceiverWorker::finished, worker, &QObject::deleteLater);
    const auto relay = [this, worker]<typename... Args>(void (GPSReceiverWorker::*signal)(Args...),
                                                        void (GPSReceiverSession::*handler)(Args...)) {
        (void) connect(
            worker, signal, this,
            [this, handler](Args... args) {
                if (!_retired) {
                    (this->*handler)(args...);
                }
            },
            Qt::QueuedConnection);
    };
    relay(&GPSReceiverWorker::rtcmDataReceived, &GPSReceiverSession::_forwardCorrection);
    relay(&GPSReceiverWorker::satelliteInfoUpdated, &GPSReceiverSession::satelliteInfoUpdated);
    relay(&GPSReceiverWorker::positionUpdated, &GPSReceiverSession::positionUpdated);
    relay(&GPSReceiverWorker::surveyInStatusUpdated, &GPSReceiverSession::_onSurvey);
    relay(&GPSReceiverWorker::connectionError, &GPSReceiverSession::ended);
    relay(&GPSReceiverWorker::receiverReady, &GPSReceiverSession::_onReady);
    relay(&GPSReceiverWorker::receiverDetected, &GPSReceiverSession::_onDetected);
    relay(&GPSReceiverWorker::inputProblem, &GPSReceiverSession::inputProblem);
    worker->start();
    return true;
}

void GPSReceiverSession::registerPosition(PositionManager* positionManager, GPSSourceHealth* producer)
{
    if (_retired || !positionManager || _position) {
        return;
    }
    auto registration = positionManager->registerReceiver(producer, _sessionId);
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

void GPSReceiverSession::_onSurvey(const GPSSurveyReport& report)
{
    if (_connection.role != RTKSettings::ConfiguredBase) {
        return;
    }
    _base.applySurvey(report);
    emit surveyInStatusUpdated(report);
}

void GPSReceiverSession::_onReady(const QString& identity)
{
    if (std::exchange(_identity, identity) != identity) {
        qCDebug(GPSReceiverSessionLog) << "Receiver identity:" << identity;
    }
    emit receiverReady(identity);
}

void GPSReceiverSession::_onDetected(GPSType type)
{
    if (_connection.type != GPSType::automatic && _connection.type != GPSType::passive) {
        return;
    }
    _detectedType = type;
    qCDebug(GPSReceiverSessionLog) << "Detected receiver family:" << type;
    emit receiverDetected(type);
}

void GPSReceiverSession::_forwardCorrection(const QByteArray& data, qint64 receivedAtMs)
{
    if (_correction) {
        _correction->submit(data, receivedAtMs);
    }
}
