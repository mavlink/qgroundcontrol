#include "GPSCorrectionManager.h"

#include <algorithm>
#include <chrono>
#include <utility>

#include <QtCore/QScopeGuard>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkInterface>

#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(GPSCorrectionManagerLog, "GPS.Corrections.GPSCorrectionManager")

GPSCorrectionManager::GPSCorrectionManager(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _diagnosticsTask(_scheduler, this)
    , _healthTask(_scheduler, this)
    , _router(this)
    , _eventModel(this)
    , _rtcmMavlink(this)
    , _udpInput(0, this)
{
    qCDebug(GPSCorrectionManagerLog) << this;
    _router.setOutput(QStringLiteral("mavlink"), {.admit = [this](const GPSCorrectionFrame& frame) {
                          QList<GPSCorrectionRouter::Admission> results;
                          for (const auto& admission : _rtcmMavlink.submitToOutputs(frame.data)) {
                              results.append({admission.id,
                                              {admission.queuedBytes, admission.session,
                                               admission.complete ? GPSCorrectionReason::None
                                                                  : GPSCorrectionReason::DestinationUnavailable},
                                              admission.complete});
                          }
                          if (results.isEmpty()) {
                              results.append({QStringLiteral("mavlink"), {}, false});
                          }
                          return results;
                      }});
    _scheduleHealthSample();
    _sourceModel.setRows(_router.sourceDiagnostics());
    _destinationModel.setRows(_router.destinationDiagnostics());
}

GPSCorrectionManager::~GPSCorrectionManager()
{
    qCDebug(GPSCorrectionManagerLog) << this;
    _notifications.close();
    shutdown();
}

void GPSCorrectionManager::setUdpOutputConfiguration(const UdpOutputConfiguration& configuration)
{
    if (_shutdown || configuration == _udpOutputConfiguration) {
        return;
    }
    _udpOutputConfiguration = configuration;
    _applyUdpOutput();
}

void GPSCorrectionManager::_applyUdpOutput()
{
    if (!_udpOutputConfiguration || _shutdown) {
        return;
    }
    const QString id = QStringLiteral("udpOutput");
    const GPSNotificationQueue::Scope publish(_notifications);
    const bool enabled = _udpOutputConfiguration->enabled;
    const QString address = _udpOutputConfiguration->address.trimmed();
    const quint16 port = _udpOutputConfiguration->port;
    const QHostAddress target(address);
    // Forwarding to this host's own UDP input would feed the selected stream back into itself.
    const bool loops = _udpInputConfiguration && _udpInputConfiguration->enabled &&
                       port == _udpInputConfiguration->port &&
                       (target.isLoopback() || QNetworkInterface::allAddresses().contains(target));
    if (enabled && !loops && _udpOutput.isEnabled() && _udpOutput.address() == target.toString() &&
        _udpOutput.port() == port) {
        return;
    }
    removeSink(id);
    _udpOutput.stop();
    if (!enabled) {
        return;
    }
    if (loops) {
        qCWarning(GPSCorrectionManagerLog) << "Not forwarding corrections to this host's own UDP input port" << port;
        return;
    }
    if (_udpOutput.configure(address, port)) {
        setOutput(id, GPSCorrectionRouter::admissionOnlyOutput(id, [this](const GPSCorrectionFrame& frame) {
                      return static_cast<quint64>(_udpOutput.forward(frame.data));
                  }));
    }
}

void GPSCorrectionManager::setUdpInputConfiguration(const UdpInputConfiguration& input)
{
    if (_shutdown || input == _udpInputConfiguration) {
        return;
    }
    _udpInputConfiguration = input;
    const GPSNotificationQueue::Scope publish(_notifications);
    _applyUdpOutput();
    const auto configuration = _udpConfigurationRevision.advance(this);
    const bool enabled = input.enabled;
    const auto current = [this, configuration]() { return configuration.isCurrent() && !_shutdown; };
    _udpRegistration.reset();
    if (!current()) {
        return;
    }
    _udpInput.stop();
    if (!current()) {
        return;
    }
    disconnect(&_udpInput, nullptr, this, nullptr);
    _udpInput.configure(input.port, input.validate);
    if (!current() || !enabled) {
        return;
    }
    auto registration = registerSource(GPSCorrectionSource::Udp);
    if (!current()) {
        return;
    }
    _udpRegistration = std::move(registration);
    const auto token = _udpRegistration.token();
    connect(&_udpInput, &RTCMUdpInput::frameReceived, this,
            [this, token](const GPSCorrectionFrame& frame) { acceptIngress(token.event(frame)); });
    connect(&_udpInput, &RTCMUdpInput::frameRejected, this,
            [this, token](const GPSCorrectionFrame& frame, GPSCorrectionReason reason) {
                acceptIngress(token.event(frame, reason));
            });
    const bool started = _udpInput.start();
    if (current() && !started) {
        _udpRegistration.reset();
    }
}

void GPSCorrectionManager::applyRoutingConfiguration(const RoutingConfiguration& configuration)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    _router.applyConfiguration(configuration);
    _scheduleSourcesChanged();
}

GPSCorrectionSourceRegistration GPSCorrectionManager::registerSource(GPSCorrectionSource source,
                                                                     const QString& instance)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    auto registration = _router.registerSource(source, instance);
    _scheduleSourcesChanged();
    return registration;
}

void GPSCorrectionManager::acceptIngress(const GPSCorrectionIngress& ingress)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    const QPointer<GPSCorrectionManager> guard(this);
    ++_ingressDepth;
    const auto finishIngress = qScopeGuard([guard]() {
        if (guard) {
            --guard->_ingressDepth;
            guard->_scheduleSourcesChanged();
        }
    });
    _router.acceptIngress(ingress);
}

void GPSCorrectionManager::removeSink(const QString& id)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    _router.removeSink(id);
    _scheduleSourcesChanged();
}

void GPSCorrectionManager::setOutput(const QString& id, GPSCorrectionRouter::Output output)
{
    if (_shutdown) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    _router.setOutput(id, std::move(output));
    _scheduleSourcesChanged();
}

bool GPSCorrectionManager::hasSelectedStream() const
{
    return std::ranges::any_of(sourceInstances(), &GPSCorrectionStreamDiagnostic::selected);
}

GPSCorrectionStreamDiagnostic GPSCorrectionManager::selectedStream() const
{
    const auto instances = sourceInstances();
    const auto selected = std::ranges::find_if(instances, &GPSCorrectionStreamDiagnostic::selected);
    return selected != instances.cend() ? *selected : GPSCorrectionStreamDiagnostic{};
}

QString GPSCorrectionManager::sourceName(int source)
{
    switch (static_cast<GPSCorrectionSource>(source)) {
        case GPSCorrectionSource::LocalReceiver:
            return tr("Local base station");
        case GPSCorrectionSource::Ntrip:
            return tr("NTRIP");
        case GPSCorrectionSource::Udp:
            return tr("UDP");
        case GPSCorrectionSource::Unknown:
            break;
    }
    return tr("Unclassified");
}

void GPSCorrectionManager::_refreshDiagnostics()
{
    const GPSNotificationQueue::Scope publish(_notifications);
    const QPointer<GPSCorrectionManager> guard(this);
    // Model reset observers run synchronously.
    _eventModel.setEvents(_router.events());
    if (!guard) {
        return;
    }
    const auto sources = _router.sourceDiagnostics();
    _sourceModel.setRows(sources);
    if (!guard) {
        return;
    }
    _destinationModel.setRows(_router.destinationDiagnostics());
    if (!guard) {
        return;
    }
    if (auto instances = _router.sourceInstanceDiagnostics(); instances != _sourceInstances) {
        _sourceInstances = std::move(instances);
        _notifications.emitSignal(this, &GPSCorrectionManager::sourceInstancesChanged);
    }
    const auto selected = std::ranges::find_if(_sourceInstances, &GPSCorrectionStreamDiagnostic::selected);
    const quint64 rate =
        selected != _sourceInstances.cend() && selected->source > 0 && selected->source < sources.size()
            ? sources.at(selected->source).receivedBytesPerSecond
            : 0;
    if (std::exchange(_selectedBytesPerSecond, rate) != rate) {
        _notifications.emitSignal(this, &GPSCorrectionManager::selectedBytesPerSecondChanged);
    }
}

void GPSCorrectionManager::_scheduleSourcesChanged()
{
    if (_shutdown) {
        // Final diagnostics must include in-flight admission results.
        if (_ingressDepth == 0 && _finalDiagnosticsPending) {
            _finalDiagnosticsPending = false;
            QMetaObject::invokeMethod(this, &GPSCorrectionManager::_refreshDiagnostics, Qt::QueuedConnection);
        }
    } else if (!_diagnosticsTask.active()) {
        _diagnosticsTask.schedule(std::chrono::milliseconds(100), [this]() { _refreshDiagnostics(); });
    }
}

void GPSCorrectionManager::_scheduleHealthSample()
{
    _healthTask.schedule(std::chrono::milliseconds(1000), [this]() {
        if (_shutdown) {
            return;
        }
        // Re-arm first: refresh observers may shut down or delete the manager, which cancels the next sample.
        _scheduleHealthSample();
        _router.sampleReceivedByteRates(_scheduler->nowMs());
        _refreshDiagnostics();
    });
}

void GPSCorrectionManager::shutdown()
{
    if (_shutdown) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    const QPointer<GPSCorrectionManager> guard(this);
    _shutdown = true;
    _finalDiagnosticsPending = true;
    _healthTask.cancel();
    _diagnosticsTask.cancel();
    _rtcmMavlink.setOutputProvider({});
    if (!guard) {
        return;
    }
    _udpOutput.stop();
    if (!guard) {
        return;
    }
    _router.shutdown();
    if (!guard) {
        return;
    }
    _udpInput.stop();
    if (guard) {
        _scheduleSourcesChanged();
    }
}
