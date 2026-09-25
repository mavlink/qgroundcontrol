#include "GPSProtocol.h"

#include <cstdarg>
#include <math.h>
#include <stdlib.h>
#include <thread>
#include <time.h>

#include <QtCore/QScopeGuard>

#include "GPSProtocolTime.h"
#include "GPSRawAckMatcher.h"
#include "GPSReceiverCapabilities.h"
#include "GPSReceiverConfig.h"
#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"
#include <GeographicLib/Geocentric.hpp>

QGC_LOGGING_CATEGORY(GPSProtocolLog, "GPS.Driver.Protocols")

GPSProtocol::GPSProtocol(GPSProtocolIO io, bool satelliteInfoEnabled)
    : _satellites(satelliteInfoEnabled ? &_satelliteStorage : nullptr)
    , _io(std::move(io))
{
    if (!_io.nowUs) {
        _io.nowUs = MonotonicClock::nowUs;
    }
    if (!_io.wait) {
        _io.wait = [](std::chrono::microseconds duration) {
            std::this_thread::sleep_for(duration);
            return true;
        };
    }
}

const QLoggingCategory& GPSProtocol::logCategory() const
{
    return GPSProtocolLog();
}

void GPSProtocol::log(GPSProtocolLogLevel level, const char* format, ...) const
{
    if (!_io.log) {
        return;
    }
    va_list arguments;
    va_start(arguments, format);
    const QString message = QString::vasprintf(format, arguments);
    va_end(arguments);
    _io.log(logCategory(), level, message);
}

bool GPSProtocol::validateConfiguration(const GPSConfig& config, ConfigurationSupport support) const
{
    const GPSReceiverConfig physical{.role = GPSReceiverConfig::Role::RTKBase,
                                     .base = config.base,
                                     .allowPersistentChanges = config.allowPersistentChanges};
    const GPSReceiverCapabilities supported{.surveyIn = true,
                                            .receiverAveraging = support.receiverAveraging,
                                            .persistentConfiguration = support.persistentChanges,
                                            .compactObservations = support.compactObservations};
    const auto error = gpsValidateReceiverPhysicalConfig(physical, supported);
    if (error != GPSReceiverConfigError::None) {
        log(GPSProtocolLogLevel::Warning, "Invalid receiver physical configuration (%d)", static_cast<int>(error));
        return false;
    }
    return true;
}

GPSProtocol::EcefMeters GPSProtocol::toEcef(const GPSEllipsoidPosition& position)
{
    EcefMeters result;
    GeographicLib::Geocentric::WGS84().Forward(position.latitudeDegrees, position.longitudeDegrees,
                                               position.altitudeMeters, result.x, result.y, result.z);
    return result;
}

GPSEllipsoidPosition GPSProtocol::fromEcef(const EcefMeters& position)
{
    GPSEllipsoidPosition result;
    double height;
    GeographicLib::Geocentric::WGS84().Reverse(position.x, position.y, position.z, result.latitudeDegrees,
                                               result.longitudeDegrees, height);
    result.altitudeMeters = static_cast<float>(height);
    return result;
}

uint64_t GPSProtocol::timeFromUtc(tm& utc, int32_t nsec)
{
    const time_t epoch = gpsTimeToEpoch(utc);

    if (epoch > GPS_UTC_PLAUSIBILITY_FLOOR_SECS) {
        return static_cast<uint64_t>(epoch) * 1000000ULL + nsec / 1000;
    }

    return 0;
}

int GPSProtocol::readAndDecode(unsigned timeout)
{
    uint8_t buffer[GPS_READ_BUFFER_SIZE];
    const int count = read(buffer, sizeof(buffer), static_cast<int>(timeout));
    if (count < 0) {
        return 0;
    }
    return consume({buffer, static_cast<size_t>(count)});
}

bool GPSProtocol::writeCommand(GPSConfigurationStep step, std::span<const uint8_t> bytes)
{
    const Operation operation(*this, static_cast<unsigned>(step.timeout.count()));
    beginCommandWrite(std::move(step));
    return write(bytes.data(), static_cast<int>(bytes.size()));
}

GPSCommandResult GPSProtocol::awaitCommand(const std::function<GPSCommandOutcome()>& reply)
{
    if (_commandCompleted) {
        return _commandWrite;
    }
    const Operation operation(*this, remainingMilliseconds(_commandDeadline.untilUs));
    _operationDeadline.untilUs = std::min(_operationDeadline.untilUs, _commandDeadline.untilUs);
    const auto outcome = GPSCommandTransaction::await(
        _operationDeadline.untilUs, [this] { return nowUs(); }, reply,
        [this] { readAndDecode(remainingMilliseconds(_commandDeadline.untilUs)); },
        [this] { return ioCommandOutcome(); });
    return completeCommand(outcome);
}

GPSCommandResult GPSProtocol::transact(GPSConfigurationStep step, std::string_view wire)
{
    const auto clearReply = qScopeGuard([this] {
        _reply.reset();
        _rawReply = nullptr;
        _replyMatcher = {};
    });
    if (hasIOError()) {
        GPSCommandResult failed;
        failed.evidence.command = std::move(step.command);
        failed.evidence.required = step.required;
        failed.evidence.outcome = ioCommandOutcome();
        return failed;
    }
    _reply = GPSCommandOutcome::Pending;
    if (!writeCommand(std::move(step), {reinterpret_cast<const uint8_t*>(wire.data()), wire.size()})) {
        return completeCommand(hasIOError() ? ioCommandOutcome() : GPSCommandOutcome::TransportError);
    }
    return awaitCommand([this] { return _reply.value_or(GPSCommandOutcome::Pending); });
}

GPSCommandResult GPSProtocol::transact(GPSConfigurationStep step, std::string_view wire, GPSRawAckMatcher& reply)
{
    _rawReply = &reply;
    return transact(std::move(step), wire);
}

GPSCommandResult GPSProtocol::transact(GPSConfigurationStep step, std::string_view wire, GPSReplyMatcher reply)
{
    _replyMatcher = std::move(reply);
    return transact(std::move(step), wire);
}

GPSConfigurationSequence::Result GPSProtocol::runSequence(const GPSConfigurationSequence& sequence)
{
    for (size_t index = 0; index < sequence.steps.size(); ++index) {
        if (const auto* command = std::get_if<GPSConfigurationSequence::Command>(&sequence.steps[index])) {
            GPSCommandResult result;
            for (unsigned attempt = 0; attempt < std::max(command->attempts, 1U); ++attempt) {
                if (const auto* raw = std::get_if<GPSConfigurationSequence::RawReply>(&command->reply)) {
                    GPSRawAckMatcher matcher(raw->accepted, raw->rejected);
                    result = transact(command->step, command->wire, matcher);
                } else {
                    result = transact(command->step, command->wire, std::get<GPSReplyMatcher>(command->reply));
                }
                if (result.succeeded() || hasIOError()) {
                    break;
                }
            }
            if (!result.succeeded() && (command->step.required || hasIOError())) {
                return {.failedStep = index, .failedLabel = command->step.command, .outcome = result.evidence.outcome};
            }
            continue;
        }
        const auto& custom = std::get<GPSConfigurationSequence::Custom>(sequence.steps[index]);
        if ((!custom.run() && custom.required) || hasIOError()) {
            return {.failedStep = index, .failedLabel = custom.label, .outcome = ioCommandOutcome()};
        }
    }
    return {};
}

void GPSProtocol::beginCommandWrite(GPSConfigurationStep step)
{
    if (hasIOError()) {
        return;
    }
    failCommandWrite(GPSCommandOutcome::Written);
    _commandWrite = {};
    _commandWrite.evidence.startedAtUs = nowUs();
    _commandWrite.evidence.command = std::move(step.command);
    _commandWrite.affectedSettings = step.affectedSettings;
    _commandWrite.evidence.required = step.required;
    _commandDeadline.untilUs =
        std::min(_operationDeadline.untilUs,
                 _commandWrite.evidence.startedAtUs + uint64_t(std::max<int64_t>(step.timeout.count(), 0)) * 1000);
    _commandCompleted = false;
}

GPSCommandResult GPSProtocol::completeCommand(GPSCommandOutcome outcome)
{
    if (_commandCompleted) {
        return _commandWrite;
    }
    _commandWrite.evidence.outcome = outcome;
    _commandWrite.evidence.finishedAtUs = nowUs();
    _commandCompleted = true;
    // Completion observers may finish evidence again or begin another attempt.
    const auto result = _commandWrite;
    if (_io.commandFinished) {
        _io.commandFinished(result);
    }
    return result;
}

int GPSProtocol::receiveDecoded(unsigned timeout)
{
    const Operation operation(*this, timeout);
    const uint64_t deadline = _operationDeadline.untilUs;
    do {
        const int handled = readAndDecode(static_cast<unsigned>(remainingMilliseconds(deadline)));
        if (handled || hasIOError()) {
            return handled;
        }
    } while (nowUs() < deadline);
    return 0;
}

void GPSProtocol::serviceControls()
{
    if (_servicingControls || hasIOError()) {
        return;
    }
    _servicingControls = true;
    {
        // This budget belongs to pending receiver commands, independently of the completed read slice.
        const Operation operation(*this, 5000);
        servicePendingCommands();
    }
    _servicingControls = false;
    consume({});
}

GPSDecodeResult GPSProtocol::decode(std::span<const uint8_t> bytes)
{
    flushDecoded();
    size_t consumed = 0;
    // One completed frame can publish relative/survey data plus position and satellites.
    while (consumed < bytes.size() && _decoded.events.size() + 4 <= GPSDecodedBatch::MAX_EVENTS) {
        const int updates = decodeByte(bytes[consumed++]);
        if (updates > 0) {
            _decoded.updates |= updates;
        }
    }
    if (_rawReply) {
        _rawReply->append(bytes.first(consumed));
        resolveReply(_rawReply->outcome());
    }
    GPSDecodeResult result{consumed, std::move(_decoded)};
    _decoded = {};
    return result;
}

int GPSProtocol::consume(std::span<const uint8_t> bytes)
{
    int updates = 0;
    do {
        auto result = decode(bytes);
        bytes = bytes.subspan(result.bytesConsumed);
        updates |= result.batch.updates;
        if (_io.decoded) {
            _io.decoded(result.batch);
        }
        result.batch.events.clear();
        // A nested callback may already have populated or replenished the decoder's storage.
        if (_decoded.events.empty() && _decoded.events.capacity() < result.batch.events.capacity()) {
            _decoded.events.swap(result.batch.events);
        }
    } while (!bytes.empty());
    return updates;
}
