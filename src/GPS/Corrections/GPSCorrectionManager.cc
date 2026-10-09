#include "GPSCorrectionManager.h"

#include <algorithm>
#include <chrono>
#include <utility>

#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkInterface>

#include "QGCLoggingCategory.h"
#include "RuntimeScheduler.h"

QGC_LOGGING_CATEGORY(GPSCorrectionManagerLog, "GPS.Corrections.GPSCorrectionManager")

namespace {
/// Whether this host's own sockets may receive datagrams sent to @a target. The UDP input is dual-stack, so an
/// IPv4-mapped IPv6 target reaches it as the IPv4 address does, and multicast loops back to local group members.
bool reachesThisHost(const QHostAddress& target)
{
    const auto matches = [&target](const QHostAddress& address) {
        return !address.isNull() && address.isEqual(target, QHostAddress::TolerantConversion);
    };
    if (target.isLoopback() || target.isBroadcast() || target.isMulticast() || matches(QHostAddress::Any)) {
        return true;
    }
    return std::ranges::any_of(QNetworkInterface::allInterfaces(), [&matches](const QNetworkInterface& interface) {
        return std::ranges::any_of(interface.addressEntries(), [&matches](const QNetworkAddressEntry& entry) {
            return matches(entry.ip()) || matches(entry.broadcast());
        });
    });
}
}  // namespace

GPSCorrectionSourceHandle::GPSCorrectionSourceHandle(GPSCorrectionManager* manager,
                                                     GPSCorrectionSettings::CorrectionSource source,
                                                     const QString& instance)
    : _manager(manager)
    , _source(source)
    , _instance(instance)
{
    manager->_beginSource(source);
}

GPSCorrectionSourceHandle::~GPSCorrectionSourceHandle()
{
    if (_manager) {
        _manager->_endSource(_source);
    }
}

void GPSCorrectionSourceHandle::submit(const QByteArray& data, qint64 receivedAtMs, const QString& instance) const
{
    if (_manager) {
        _manager->_submit(_source, instance.isEmpty() ? _instance : instance, data, receivedAtMs);
    }
}

GPSCorrectionManager::GPSCorrectionManager(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
    , _refreshTask(_scheduler, this)
    , _tickTask(_scheduler, this)
    , _selectedRate([scheduler = _scheduler]() { return scheduler->nowUs(); })
    , _udpInput(this, _scheduler)
{
    qCDebug(GPSCorrectionManagerLog) << this;
    connect(&_udpInput, &RTCMUdpInput::frameReceived, this,
            [this](const QString& sender, const QByteArray& data, qint64 receivedAtMs) {
                if (_udpSource) {
                    _udpSource->submit(data, receivedAtMs, sender);
                }
            });
    _tickTask.scheduleRepeating(std::chrono::seconds(1), [this]() {
        _retryUdpInput();
        _selectedRate.refresh();
        _refresh();
    });
    _refresh();
}

std::unique_ptr<GPSCorrectionSourceHandle> GPSCorrectionManager::openSource(
    GPSCorrectionSettings::CorrectionSource source, const QString& instance)
{
    return std::unique_ptr<GPSCorrectionSourceHandle>(new GPSCorrectionSourceHandle(this, source, instance));
}

GPSCorrectionManager::~GPSCorrectionManager()
{
    qCDebug(GPSCorrectionManagerLog) << this;
    shutdown();
}

void GPSCorrectionManager::setConfiguration(const Configuration& configuration)
{
    if (_shutdown || configuration == _configuration) {
        return;
    }
    const std::optional<Configuration> previous = std::exchange(_configuration, configuration);
    _selector.configure(configuration.source, _scheduler->nowMs());
    const bool inputChanged = !previous || previous->udpInput != configuration.udpInput;
    const bool enabledChanged = (previous && previous->udpInput.enabled) != configuration.udpInput.enabled;
    const bool inputNotified = inputChanged && (_applyUdpInput() || enabledChanged);
    // The output also depends on the input port, which it must not feed back into.
    const bool outputNotified = (inputChanged || previous->udpOutput != configuration.udpOutput) && _applyUdpOutput();
    _refresh();
    if (inputNotified) {
        emit udpInputChanged();
    }
    if (outputNotified) {
        emit udpOutputChanged();
    }
}

bool GPSCorrectionManager::_applyUdpOutput()
{
    const UdpOutputConfiguration& output = _configuration->udpOutput;
    const QString address = output.address.trimmed();
    _udpOutput.stop();
    QString error;
    if (output.enabled) {
        // Forwarding to this host's own UDP input would feed the selected stream back into itself.
        if (_udpInputEnabled() && output.port == _configuration->udpInput.port &&
            reachesThisHost(QHostAddress(address))) {
            qCWarning(GPSCorrectionManagerLog)
                << "Not forwarding corrections to this host's own UDP input port" << output.port;
            //: %1 is an IP address, %2 a UDP port
            error = tr("Not forwarding to %1 port %2: this computer's UDP input receives on that port.")
                        .arg(address)
                        .arg(output.port);
        } else if (!_udpOutput.configure(address, output.port)) {
            error = tr("Enter an IP address and a port for UDP output.");
        }
    }
    return std::exchange(_udpOutputError, error) != error;
}

bool GPSCorrectionManager::_applyUdpInput()
{
    const QString previousError = _udpInputError;
    _udpSource.reset();
    _udpInput.stop();
    _udpInputError.clear();
    if (_udpInputEnabled()) {
        _startUdpInput();
    }
    return _udpInputError != previousError;
}

void GPSCorrectionManager::_startUdpInput()
{
    const quint16 port = _configuration->udpInput.port;
    QString error;
    if (!_udpInput.start(port, &error)) {
        const QString reason = tr("Cannot listen on UDP port %1: %2").arg(port).arg(error);
        // The tick retries every second, so only a new reason is worth a warning.
        if (std::exchange(_udpInputError, reason) != reason) {
            qCWarning(GPSCorrectionManagerLog) << "UDP correction input unavailable:" << reason;
        }
        return;
    }
    _udpInputError.clear();
    _udpSource = openSource(GPSCorrectionSettings::Udp, QString());
}

void GPSCorrectionManager::_retryUdpInput()
{
    if (!_udpInputEnabled() || _udpInputError.isEmpty()) {
        return;
    }
    const QString previousError = _udpInputError;
    _startUdpInput();
    if (_udpInputError != previousError) {
        emit udpInputChanged();
    }
}

bool GPSCorrectionManager::_udpInputEnabled() const
{
    return _configuration && _configuration->udpInput.enabled;
}

void GPSCorrectionManager::_beginSource(GPSCorrectionSettings::CorrectionSource source)
{
    _selector.beginSource(source, _scheduler->nowMs());
    _scheduleRefresh();
}

void GPSCorrectionManager::_endSource(GPSCorrectionSettings::CorrectionSource source)
{
    _selector.endSource(source, _scheduler->nowMs());
    _scheduleRefresh();
}

void GPSCorrectionManager::_submit(GPSCorrectionSettings::CorrectionSource source, const QString& instance,
                                   const QByteArray& data, qint64 receivedAtMs)
{
    if (_selector.submit(source, instance, receivedAtMs, _scheduler->nowMs())) {
        _selectedRate.recordBytes(data.size());
        if (_rtcmMavlink.submitToOutputs(data) > 0) {
            emit vehicleBytesSubmittedChanged();
        }
        (void) _udpOutput.forward(data);
    }
    _scheduleRefresh();
}

QString GPSCorrectionManager::sourceName(int source)
{
    switch (static_cast<GPSCorrectionSettings::CorrectionSource>(source)) {
        case GPSCorrectionSettings::LocalReceiver:
            return tr("Local base station");
        case GPSCorrectionSettings::Ntrip:
            return tr("NTRIP");
        case GPSCorrectionSettings::Udp:
            return tr("UDP");
        case GPSCorrectionSettings::HighestPriority:
            break;
    }
    return tr("Unclassified");
}

void GPSCorrectionManager::_refresh()
{
    const std::optional<GPSCorrectionStream> selected = _selector.selectedStream(_scheduler->nowMs());
    State state = State::Inactive;
    if (selected) {
        state = State::Fresh;
    } else {
        _selectedRate.resetRate();
        if (_selector.hasSources() || _udpInputEnabled()) {
            state = State::Waiting;
        }
    }
    const quint64 rate = static_cast<quint64>(qRound64(_selectedRate.bytesPerSec()));
    const bool rateChanged = std::exchange(_selectedBytesPerSecond, rate) != rate;
    const GPSCorrectionStream stream = selected.value_or(GPSCorrectionStream{});
    const bool streamChanged = std::exchange(_selected, stream) != stream;
    if (std::exchange(_state, state) != state || streamChanged) {
        emit stateChanged();
    }
    if (rateChanged) {
        emit selectedBytesPerSecondChanged();
    }
}

void GPSCorrectionManager::_scheduleRefresh()
{
    if (!_shutdown && !_refreshTask.active()) {
        _refreshTask.schedule(std::chrono::milliseconds(100), [this]() { _refresh(); });
    }
}

void GPSCorrectionManager::shutdown()
{
    if (_shutdown) {
        return;
    }
    _shutdown = true;
    _tickTask.cancel();
    _refreshTask.cancel();
    _rtcmMavlink.setOutputProvider({});
    _udpOutput.stop();
    _udpSource.reset();
    _selector.shutdown();
    _udpInput.stop();
}
