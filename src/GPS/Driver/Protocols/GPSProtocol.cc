/****************************************************************************
 *
 *   Copyright (c) 2012-2018 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#include "GPSProtocol.h"

#include <math.h>
#include <stdlib.h>
#include <thread>
#include <time.h>

#include "GPSProtocolTime.h"
#include "GPSReceiverConfig.h"
#include "MonotonicClock.h"
#include <GeographicLib/Geocentric.hpp>

/**
 * @file GPSProtocol.cc
 *
 * @author Thomas Gubler <thomasgubler@student.ethz.ch>
 * @author Julian Oes <julian@oes.ch>
 */

GPSProtocol::GPSProtocol(GPSProtocolIO io)
    : _io(std::move(io))
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

bool GPSProtocol::validateConfiguration(const GPSConfig& config, bool allowReceiverAveraging,
                                        bool supportsPersistentChanges) const
{
    if (config.allowPersistentChanges && !supportsPersistentChanges) {
        log(GPSProtocolLogLevel::Warning, "Persistent configuration is not supported by this driver");
        return false;
    }
    if (config.output_mode != OutputMode::GPS && config.output_mode != OutputMode::RTCM) {
        log(GPSProtocolLogLevel::Warning, "Invalid receiver output mode");
        return false;
    }
    if (config.output_mode == OutputMode::RTCM) {
        if (!allowReceiverAveraging && config.base.surveyMode == GPSBaseStationConfig::SurveyMode::ReceiverManaged) {
            log(GPSProtocolLogLevel::Warning, "Receiver-managed averaging is not supported by this driver");
            return false;
        }
        const auto error = gpsValidateBaseStationConfig(config.base);
        if (error != GPSReceiverConfigError::None) {
            log(GPSProtocolLogLevel::Warning, "Invalid base station configuration (%d)", static_cast<int>(error));
            return false;
        }
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

    if (epoch > GPS_EPOCH_SECS) {
        return static_cast<uint64_t>(epoch) * 1000000ULL + nsec / 1000;
    }

    return 0;
}

int GPSProtocol::readAndDecode(unsigned timeout)
{
    uint8_t buffer[GPS_READ_BUFFER_SIZE];
    const int count = read(buffer, sizeof(buffer), static_cast<int>(timeout));
    if (count < 0) {
        return count;
    }
    const int updates = consume({buffer, static_cast<size_t>(count)});
    return ioError() ? ioError() : updates;
}

bool GPSProtocol::writeCommand(GPSConfigurationStep step, std::span<const uint8_t> bytes)
{
    const Operation operation(*this, static_cast<unsigned>(step.timeout.count()));
    beginCommandWrite(std::move(step.command), step.affectedSettings, step.required);
    return write(bytes.data(), static_cast<int>(bytes.size())) == static_cast<int>(bytes.size());
}

GPSCommandResult GPSProtocol::awaitCommand(GPSConfigurationStep step, const std::function<GPSCommandOutcome()>& reply)
{
    const auto timeout = static_cast<unsigned>(step.timeout.count());
    return awaitCommand(std::move(step), [this, timeout] { readAndDecode(timeout); }, reply);
}

GPSCommandResult GPSProtocol::awaitCommand(GPSConfigurationStep step, const std::function<void()>& pump,
                                           const std::function<GPSCommandOutcome()>& reply)
{
    if (_commandCompleted) {
        return _commandWrite;
    }
    const auto timeout = static_cast<unsigned>(step.timeout.count());
    const Operation operation(*this, timeout);
    _operationDeadline.untilUs =
        std::min(_operationDeadline.untilUs, _commandWrite.evidence.startedAtUs + uint64_t(timeout) * 1000);
    _commandWrite.evidence.command = std::move(step.command);
    _commandWrite.evidence.required = step.required;
    _commandWrite.affectedSettings = step.affectedSettings;
    const auto outcome = GPSCommandTransaction::await(
        _operationDeadline.untilUs, [this] { return nowUs(); }, reply, pump,
        [this] {
            return ioError() == ReadCancelled ? GPSCommandOutcome::Cancelled
                   : ioError()                ? GPSCommandOutcome::TransportError
                                              : GPSCommandOutcome::Pending;
        });
    return completeCommand(outcome);
}

void GPSProtocol::beginCommandWrite(std::string command, GPSReceiverSettingSet settings, bool required)
{
    if (ioError()) {
        return;
    }
    failCommandWrite(GPSCommandOutcome::Written);
    _commandWrite = {};
    _commandWrite.evidence.startedAtUs = nowUs();
    _commandWrite.evidence.command = std::move(command);
    _commandWrite.affectedSettings = settings;
    _commandWrite.evidence.required = required;
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
        if (handled) {
            return handled;
        }
    } while (nowUs() < deadline);
    return -1;
}

void GPSProtocol::serviceControls()
{
    if (_servicingControls || ioError()) {
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
            if ((updates & 1) && positionReport()) {
                _decoded.events.emplace_back(*positionReport());
            }
            if ((updates & 2) && satelliteReport()) {
                _decoded.events.emplace_back(*satelliteReport());
            }
        }
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
