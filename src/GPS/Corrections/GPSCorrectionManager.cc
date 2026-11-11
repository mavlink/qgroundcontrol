#include "GPSCorrectionManager.h"

#include <algorithm>
#include <chrono>
#include <utility>

#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkInterface>

#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"
#include "RTCMFramer.h"
#include "RTCMStationPosition.h"
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

GPSCorrectionManager::GPSCorrectionManager(QObject* parent, RuntimeScheduler* scheduler,
                                           const Dependencies& dependencies)
    : QObject(parent)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
    , _refreshTask(_scheduler, this)
    , _tickTask(_scheduler, this)
    , _selectedRate([scheduler = _scheduler]() { return scheduler->nowUs(); })
    , _udpInput(this, _scheduler)
    , _logDirectory(dependencies.logDirectory)
    , _log(dependencies.maxLogFileBytes)
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
        if (_log.isOpen() && !_log.flush()) {
            emit logChanged();
        }
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
    _rtcmMavlink.setVehicles(configuration.vehicles);
    const bool inputChanged = !previous || previous->udpInput != configuration.udpInput;
    const bool enabledChanged = (previous && previous->udpInput.enabled) != configuration.udpInput.enabled;
    const bool inputNotified = inputChanged && (_applyUdpInput() || enabledChanged);
    // The output also depends on the input port, which it must not feed back into.
    const bool outputNotified = (inputChanged || previous->udpOutput != configuration.udpOutput) && _applyUdpOutput();
    if (!previous || previous->messageFilter != configuration.messageFilter) {
        _applyMessageFilter();
    }
    _refresh();
    if (inputNotified) {
        emit udpInputChanged();
    }
    if (outputNotified) {
        emit udpOutputChanged();
    }
}

void GPSCorrectionManager::_applyMessageFilter()
{
    _messageFilter.clear();
    _leftOutMessages.clear();
    QString error;
    for (const auto& token : _configuration->messageFilter.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        const QString entry = token.trimmed();
        if (entry.isEmpty()) {
            continue;
        }
        bool isNumber = false;
        const int id = entry.toInt(&isNumber);
        if (!isNumber || id < 1 || id > MAX_MESSAGE_ID) {
            // Skipping the entry could leave an empty filter, which takes every message.
            qCWarning(GPSCorrectionManagerLog) << "Invalid RTCM message filter entry" << entry;
            //: %1 is the text the user entered, %2 the largest message ID
            error = tr("No corrections are sent: the RTCM message filter entry \"%1\" is not a message ID. Enter "
                       "message IDs from 1 to %2, separated by commas.")
                        .arg(entry, QString::number(MAX_MESSAGE_ID));
            break;
        }
        _messageFilter.insert(id);
    }
    _messageFilterInvalid = error;
}

void GPSCorrectionManager::_refreshMessageFilterError(bool streamSelected, qint64 now)
{
    const auto age = MonotonicClock::age(std::chrono::milliseconds(_leftOutAtMs), std::chrono::milliseconds(now));
    if (streamSelected || !age || *age >= GPSCorrectionSelector::FRESHNESS_TIMEOUT) {
        _leftOutMessages.clear();
    }
    QString error = _messageFilterInvalid;
    if (error.isEmpty() && !_leftOutMessages.isEmpty()) {
        QList<int> ids(_leftOutMessages.cbegin(), _leftOutMessages.cend());
        std::ranges::sort(ids);
        QStringList names;
        for (const int id : ids.first((std::min) (ids.size(), MAX_LEFT_OUT_NAMED))) {
            names.append(QString::number(id));
        }
        if (ids.size() > MAX_LEFT_OUT_NAMED) {
            names.append(QStringLiteral("…"));
        }
        //: %1 is a list of RTCM message IDs, such as 1074, 1084, 1230
        error = tr("No corrections are sent: the RTCM message filter takes none of the messages that arrive (%1).")
                    .arg(names.join(QStringLiteral(", ")));
    }
    if (std::exchange(_messageFilterError, error) != error) {
        emit messageFilterChanged();
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
            // A second arg() pass would take an IPv6 zone such as "%1" in the address for a place marker.
            //: %1 is an IP address, %2 a UDP port
            error = tr("Not forwarding to %1 port %2: this computer's UDP input receives on that port.")
                        .arg(address, QString::number(output.port));
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
    const bool filterValid = _messageFilterInvalid.isEmpty();
    const int messageId = _messageFilter.isEmpty() ? 0 : RTCMFramer::frameMessageId(data);
    const bool taken = filterValid && (_messageFilter.isEmpty() || _messageFilter.contains(messageId));
    if (!taken) {
        // Named to the user should the filter leave the routing no stream to select.
        if (filterValid && _state != State::Fresh && _selector.routes(source)) {
            _leftOutMessages.insert(messageId);
            _leftOutAtMs = _scheduler->nowMs();
        }
    } else if (_selector.submit(source, instance, receivedAtMs, _scheduler->nowMs())) {
        _recordSelected({.source = static_cast<int>(source), .instanceId = instance}, data, receivedAtMs);
        _selectedRate.recordBytes(data.size());
        if (_rtcmMavlink.submitToOutputs(data) > 0) {
            emit vehicleBytesSubmittedChanged();
        }
        _forwardToUdpOutput(data);
        if (_log.isOpen() && !_log.write(data)) {
            emit logChanged();
        }
    }
    _scheduleRefresh();
}

void GPSCorrectionManager::_recordSelected(const GPSCorrectionStream& stream, const QByteArray& data,
                                           qint64 receivedAtMs)
{
    if (stream != _recordedStream) {
        _forgetSelected();
        _recordedStream = stream;
    }
    _messageCounts.record(RTCMFramer::frameMessageId(data), receivedAtMs);
    if (const auto station = RTCMStationPosition::fromFrame(data)) {
        const GPSEllipsoidPosition& position = station->position;
        _setBasePosition(QGeoCoordinate(position.latitudeDegrees, position.longitudeDegrees, position.altitudeMeters));
    }
}

void GPSCorrectionManager::_forgetSelected()
{
    _recordedStream = {};
    _messageCounts.clear();
    _setBasePosition(QGeoCoordinate());
}

void GPSCorrectionManager::_setBasePosition(const QGeoCoordinate& position)
{
    if (std::exchange(_basePosition, position) != position) {
        emit basePositionChanged();
    }
}

void GPSCorrectionManager::_forwardToUdpOutput(const QByteArray& data)
{
    if (!_udpOutput.isEnabled() || data.isEmpty()) {
        return;
    }
    QString error;
    // forward() returns the bytes sent and keeps the socket's reason when a datagram was not.
    if (_udpOutput.forward(data) <= 0) {
        //: %1 is an IP address, %2 a UDP port, %3 the reason the system gives
        error = tr("Not forwarding to %1 port %2: %3")
                    .arg(_udpOutput.address(), QString::number(_udpOutput.port()), _udpOutput.errorString());
    }
    if (std::exchange(_udpOutputError, error) != error) {
        emit udpOutputChanged();
    }
}

void GPSCorrectionManager::setLogToFile(bool log)
{
    if (_shutdown || log == _log.isOpen()) {
        return;
    }
    if (log) {
        const QString directory = _logDirectory ? _logDirectory() : QString();
        if (directory.isEmpty()) {
            qCWarning(GPSCorrectionManagerLog) << "No folder to log the corrections to";
        } else {
            _log.open(RTCMLogFile::fileName(directory, QDateTime::currentDateTimeUtc()));
        }
    } else {
        _log.close();
    }
    // Also for a log that could not start, so that a switch bound to the property goes back to off.
    emit logChanged();
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
    const qint64 now = _scheduler->nowMs();
    const std::optional<GPSCorrectionStream> selected = _selector.selectedStream(now);
    if (selected != _recordedStream) {
        _forgetSelected();
    }
    _messageCounts.expireIntervals(now);
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
    _refreshMessageFilterError(selected.has_value(), now);
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
    _log.close();
    _udpSource.reset();
    _selector.shutdown();
    _udpInput.stop();
}
