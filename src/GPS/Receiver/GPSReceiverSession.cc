#include "GPSReceiverSession.h"

#include <utility>

#include "GPSCorrectionManager.h"
#include "GPSPositionService.h"
#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"
#include "RTCMFramer.h"

QGC_LOGGING_CATEGORY(GPSReceiverSessionLog, "GPS.Receiver.GPSReceiverSession")

GPSReceiverSession::GPSReceiverSession(quint64 id, Connection connection, QObject* parent)
    : QObject(parent)
    , _sessionId(id)
    , _connection(std::move(connection))
    , _base(gpsBaseStationStateFor(_connection.config))
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
    if (_retired || _connection.role == GPSReceiver::PositionOnly || !manager) {
        return;
    }
    _correctionManager = manager;
    _registered = true;
    _corrections = manager->registerSource(GPSCorrectionSource::LocalReceiver, sourceInstance);
}

bool GPSReceiverSession::start(const GPSReceiver::ProviderFactory& factory,
                               GPSProvider::TransportFactory transportFactory)
{
    if (_retired || _provider) {
        return false;
    }
    const GPSType type = _connection.type;
    _provider = factory ? factory(std::move(transportFactory), type, _connection.config, this)
                        : new GPSProvider(std::move(transportFactory), type, _connection.config, this);
    if (!_provider) {
        return false;
    }
    GPSProvider* const provider = _provider;
    provider->setEndsWhenIdle(_connection.role == GPSReceiver::ConfiguredBase);
    (void) connect(
        provider, &GPSProvider::finished, this,
        [this]() {
            if (!_retired) {
                emit ended(GPSConnectionError::DeviceError, {});
            }
        },
        Qt::QueuedConnection);
    (void) connect(provider, &GPSProvider::finished, provider, &QObject::deleteLater);
    const auto relay = [this, provider]<typename... Args>(void (GPSProvider::*signal)(Args...),
                                                          void (GPSReceiverSession::*handler)(Args...)) {
        (void) connect(
            provider, signal, this,
            [this, handler](Args... args) {
                if (_current()) {
                    (this->*handler)(args...);
                }
            },
            Qt::QueuedConnection);
    };
    relay(&GPSProvider::RTCMDataUpdate, &GPSReceiverSession::_forwardCorrection);
    relay(&GPSProvider::satelliteInfoUpdate, &GPSReceiverSession::satelliteInfoUpdate);
    relay(&GPSProvider::positionUpdate, &GPSReceiverSession::positionUpdate);
    relay(&GPSProvider::surveyInStatus, &GPSReceiverSession::_onSurvey);
    relay(&GPSProvider::connectionError, &GPSReceiverSession::ended);
    relay(&GPSProvider::receiverReady, &GPSReceiverSession::_onReady);
    relay(&GPSProvider::receiverDetected, &GPSReceiverSession::_onDetected);
    _started = true;
    provider->start();
    return true;
}

void GPSReceiverSession::registerPosition(GPSPositionService* service, GPSSourceHealth* producer)
{
    if (_retired || !service || _position) {
        return;
    }
    auto registration =
        service->registerPositionSource(GPSPositionService::SelectedSource::Receiver, producer, _sessionId);
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
    const QPointer<GPSProvider> provider = _provider;
    if (provider) {
        (void) disconnect(provider, nullptr, this, nullptr);
        // Registration observers may delete the owner; the worker must already be independent.
        provider->setParent(nullptr);
        provider->stop();
    }
    _position.reset();
    _corrections.reset();
    if (provider && !_started) {
        delete provider.data();
    }
}

GPSObservation GPSReceiverSession::observation(const GPSPositionReport& report) const
{
    const quint64 receivedAtUs = MonotonicClock::nowUs();
    auto observation = _base.position ? GPSObservation::fromSurveyedPosition(_base.position->first,
                                                                             _base.position->second, receivedAtUs)
                                      : GPSObservation::fromNavigation(report.navigation, receivedAtUs);
    observation.sessionId = _sessionId;
    return observation;
}

void GPSReceiverSession::_onSurvey(const GPSSurveyReport& report)
{
    if (_connection.role != GPSReceiver::ConfiguredBase) {
        return;
    }
    _base.applySurvey(report);
    emit surveyInStatus(report);
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
    if (_connection.type != GPSType::automatic) {
        return;
    }
    _detectedType = type;
    qCDebug(GPSReceiverSessionLog) << "Detected receiver family:" << type;
    emit receiverDetected(type);
}

void GPSReceiverSession::_forwardCorrection(const QByteArray& data, qint64 receivedAtMs)
{
    if (!_correctionManager) {
        if (_connection.role != GPSReceiver::PositionOnly) {
            qCWarning(GPSReceiverSessionLog) << "Correction manager not ready; dropping" << data.size() << "bytes";
        }
        return;
    }
    const bool valid = RTCMFramer::isValidFrame(data);
    _correctionManager->acceptIngress(
        _corrections.event(data, receivedAtMs, RTCMFramer::frameMessageId(data), valid, false,
                           valid ? GPSCorrectionReason::None : GPSCorrectionReason::InvalidFrame));
}
