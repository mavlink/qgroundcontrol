#include "GPSCorrectionManager.h"

#include <algorithm>
#include <utility>

#include <QtCore/QScopeGuard>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkInterface>

#include "GPSCorrectionSettings.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSCorrectionManagerLog, "GPS.Corrections.GPSCorrectionManager")

GPSCorrectionManager::GPSCorrectionManager(QObject* parent)
    : QObject(parent)
    , _router(this)
    , _eventModel(this)
    , _diagnosticsTimer(this)
    , _healthTimer(this)
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
    _diagnosticsTimer.setSingleShot(true);
    _diagnosticsTimer.setInterval(100);
    connect(&_diagnosticsTimer, &QTimer::timeout, this, &GPSCorrectionManager::_refreshDiagnostics);
    _healthTimer.setInterval(1000);
    connect(&_healthTimer, &QTimer::timeout, this, [this]() {
        _router.sampleReceivedByteRates(GPSCorrectionFrame::monotonicNowMs());
        _refreshDiagnostics();
    });
    _healthTimer.start();
    _sourceModel.setRows(_router.sourceDiagnostics());
    _destinationModel.setRows(_router.destinationDiagnostics());
}

GPSCorrectionManager::~GPSCorrectionManager()
{
    qCDebug(GPSCorrectionManagerLog) << this;
    _notifications.close();
    shutdown();
}

void GPSCorrectionManager::_applyUdpOutputSettings()
{
    if (!_settings || _shutdown) {
        return;
    }
    const QString id = QStringLiteral("udpOutput");
    const GPSNotificationQueue::Scope publish(_notifications);
    const bool enabled = _settings->rtcmUdpOutputEnabled()->rawValue().toBool();
    const QString address = _settings->rtcmUdpOutputAddress()->rawValue().toString().trimmed();
    const auto port = static_cast<quint16>(_settings->rtcmUdpOutputPort()->rawValue().toUInt());
    if (enabled && _udpOutput.isEnabled() && _udpOutput.address() == QHostAddress(address).toString() &&
        _udpOutput.port() == port) {
        return;
    }
    removeSink(id);
    _udpOutput.stop();
    if (!enabled) {
        return;
    }
    // Forwarding to this host's own UDP input would feed the selected stream back into itself.
    const QHostAddress target(address);
    if (_settings->rtcmUdpInputEnabled()->rawValue().toBool() &&
        port == _settings->rtcmUdpInputPort()->rawValue().toUInt() &&
        (target.isLoopback() || QNetworkInterface::allAddresses().contains(target))) {
        qCWarning(GPSCorrectionManagerLog) << "Not forwarding corrections to this host's own UDP input port" << port;
        return;
    }
    if (_udpOutput.configure(address, port)) {
        setOutput(id, GPSCorrectionRouter::admissionOnlyOutput(id, [this](const GPSCorrectionFrame& frame) {
                      return static_cast<quint64>(_udpOutput.forward(frame.data));
                  }));
    }
}

void GPSCorrectionManager::init(GPSCorrectionSettings* settings)
{
    if (_settings || !settings || _shutdown) {
        return;
    }
    _settings = settings;
    const GPSNotificationQueue::Scope publish(_notifications);
    for (const Fact* fact : {settings->correctionSource(), settings->correctionSourceInstance()}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSCorrectionManager::_applyRoutingSettings);
    }
    for (const Fact* fact :
         {settings->rtcmUdpInputEnabled(), settings->rtcmUdpInputPort(), settings->rtcmUdpValidate()}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSCorrectionManager::_applyUdpInputSettings);
    }
    for (const Fact* fact :
         {settings->rtcmUdpOutputEnabled(), settings->rtcmUdpOutputAddress(), settings->rtcmUdpOutputPort(),
          settings->rtcmUdpInputEnabled(), settings->rtcmUdpInputPort()}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSCorrectionManager::_applyUdpOutputSettings);
    }
    _applyRoutingSettings();
    _applyUdpInputSettings();
    _applyUdpOutputSettings();
}

void GPSCorrectionManager::_applyRoutingSettings()
{
    if (!_settings || _shutdown) {
        return;
    }
    RoutingConfiguration configuration;
    configuration.instance = _settings->correctionSourceInstance()->rawValue().toString();
    switch (_settings->correctionSource()->rawValue().toInt()) {
        case GPSCorrectionSettings::LocalReceiver:
            configuration.source = GPSCorrectionSource::LocalReceiver;
            configuration.policy = RoutingPolicy::Manual;
            break;
        case GPSCorrectionSettings::Ntrip:
            configuration.source = GPSCorrectionSource::Ntrip;
            configuration.policy = RoutingPolicy::Manual;
            break;
        case GPSCorrectionSettings::Udp:
            configuration.source = GPSCorrectionSource::Udp;
            configuration.policy = RoutingPolicy::Manual;
            break;
        case GPSCorrectionSettings::Automatic:
            break;
        default:
            qCWarning(GPSCorrectionManagerLog) << "Invalid correction source; using automatic selection";
            break;
    }
    applyRoutingConfiguration(configuration);
}

void GPSCorrectionManager::_applyUdpInputSettings()
{
    if (!_settings || _shutdown) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    const auto configuration = _udpConfigurationRevision.advance(this);
    const bool enabled = _settings->rtcmUdpInputEnabled()->rawValue().toBool();
    const bool validate = _settings->rtcmUdpValidate()->rawValue().toBool();
    const quint16 port = static_cast<quint16>(_settings->rtcmUdpInputPort()->rawValue().toUInt());
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
    _udpInput.configure(port, validate);
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
    } else if (!_diagnosticsTimer.isActive()) {
        _diagnosticsTimer.start();
    }
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
    _healthTimer.stop();
    _diagnosticsTimer.stop();
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
