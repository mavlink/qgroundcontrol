/****************************************************************************
 *
 *   Copyright (c) 2012-2014 PX4 Development Team. All rights reserved.
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

/**
 * @file GPSProtocol.h
 * @author Thomas Gubler <thomasgubler@student.ethz.ch>
 * @author Julian Oes <julian@oes.ch>
 */

#pragma once

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <numbers>
#include <span>

#include "GPSBaseStationConfig.h"
#include "GPSProtocolIO.h"

inline constexpr int GPS_READ_BUFFER_SIZE = 150;
inline constexpr float GPS_PI = std::numbers::pi_v<float>;
inline constexpr float GPS_DEG_TO_RAD = GPS_PI / 180.0f;
inline constexpr double GPS_RAD_TO_DEG = 180.0 / std::numbers::pi;

// TODO: this number seems wrong
#define GPS_EPOCH_SECS ((time_t) 1234567890ULL)

class GPSProtocol
{
public:
    static constexpr int ReadCancelled = -ECANCELED;

    int ioError() const { return _io_error; }

    enum class OutputMode : uint8_t
    {
        GPS = 0,  ///< normal GPS output
        RTCM = 2  ///< request RTCM output. This is used for (fixed position) base stations
    };

    /**
     * Receiver constellation selection
     * No bits set should keep the receiver's default config
     */
    enum class GNSSSystemsMask : int32_t
    {
        RECEIVER_DEFAULTS = 0,
        ENABLE_GPS = 1 << 0,
        ENABLE_SBAS = 1 << 1,
        ENABLE_GALILEO = 1 << 2,
        ENABLE_BEIDOU = 1 << 3,
        ENABLE_GLONASS = 1 << 4,
        ENABLE_NAVIC = 1 << 5
    };

    struct GPSConfig
    {
        GPSBaseStationConfig base;
        uint8_t dynamicModel = 0;
        uint8_t outputRateHz = 0;
        OutputMode output_mode;
        GNSSSystemsMask gnss_systems;
        bool require_gnss_config = false;
    };

    explicit GPSProtocol(GPSProtocolIO io);
    virtual ~GPSProtocol() = default;

    /**
     * configure the device
     * @param baud Input and output parameter: if set to 0, the baudrate will be automatically detected and set to
     *             the detected baudrate. If not 0, a fixed baudrate is used.
     * @param config GPS Config
     * @return 0 on success, <0 otherwise
     */
    virtual int configure(unsigned& baud, const GPSConfig& config) = 0;

    /**
     * receive & handle new data from the device
     * @param timeout [ms]
     * @return <0 on error, otherwise a bitset:
     *         bit 0 set: got gps position update
     *         bit 1 set: got satellite info update
     */
    virtual int receive(unsigned timeout) = 0;
    virtual int consume(std::span<const uint8_t> bytes);
    GPSDecodeResult decode(std::span<const uint8_t> bytes);

    /**
     * Whether the receiver is configured and ready to accept injected data. Gates all
     * injection (RTCM corrections and moving-baseline) so nothing is written to the device
     * mid-configuration. Defaults to true; drivers with a configuration handshake override it.
     */
    virtual bool receiverReady() const { return true; }

protected:
    virtual int decodeByte(uint8_t) { return 0; }

    virtual void flushDecoded() {}

    virtual const GPSPositionReport* positionReport() const { return nullptr; }

    virtual const GPSSatelliteReport* satelliteReport() const { return nullptr; }

    class Operation
    {
    public:
        Operation(GPSProtocol& driver, unsigned timeoutMs)
            : _driver(driver)
            , _previous(driver._operationDeadline)
        {
            _driver._operationDeadline.untilUs =
                std::min(_previous.untilUs, driver.nowUs() + uint64_t(timeoutMs) * 1000);
        }

        ~Operation() { _driver._operationDeadline = _previous; }

    private:
        GPSProtocol& _driver;
        GPSDeadline _previous;
    };

    template <typename... Args>
    void log(GPSProtocolLogLevel level, const char* format, Args... args) const
    {
        if (!_io.log)
            return;
        char message[1024]{};
        if constexpr (sizeof...(Args) == 0)
            std::snprintf(message, sizeof(message), "%s", format);
        else
            std::snprintf(message, sizeof(message), format, args...);
        _io.log(level, message);
    }

    uint64_t nowUs() const { return _io.nowUs(); }

    void waitFor(std::chrono::microseconds duration)
    {
        if (_io_error)
            return;
        const auto now = nowUs();
        const auto remaining = _operationDeadline.untilUs > now ? _operationDeadline.untilUs - now : 0;
        duration = std::min(duration, std::chrono::microseconds(std::min<uint64_t>(remaining, INT64_MAX)));
        if (_io.wait && !_io.wait(duration))
            _io_error = ReadCancelled;
    }

    virtual void servicePendingCommands() {}

    void serviceControls();

    int receiveDecoded(unsigned timeout);

    GPSCommandResult awaitCommand(std::string command, unsigned timeout, const std::function<void()>& pump,
                                  const std::function<GPSCommandOutcome()>& reply, bool required = true,
                                  uint32_t affectedSettings = 0)
    {
        const Operation operation(*this, timeout);
        _operationDeadline.untilUs =
            std::min(_operationDeadline.untilUs, _commandWrite.startedAtUs + uint64_t(timeout) * 1000);
        auto result = _commandWrite;
        result.command = std::move(command);
        result.required = required;
        result.affectedSettings = affectedSettings;
        result.outcome = GPSCommandTransaction::await(
            _operationDeadline.untilUs, [this] { return nowUs(); }, reply, pump,
            [this] {
                return ioError() == ReadCancelled ? GPSCommandOutcome::Cancelled
                       : ioError()                ? GPSCommandOutcome::TransportError
                                                  : GPSCommandOutcome::Pending;
            });
        result.finishedAtUs = nowUs();
        if (_io.commandFinished)
            _io.commandFinished(result);
        return result;
    }

    void beginCommandWrite()
    {
        _commandWrite = {};
        _commandWrite.startedAtUs = nowUs();
    }

    int remainingMilliseconds(uint64_t deadline) const
    {
        const auto now = nowUs();
        return now >= deadline ? 0 : static_cast<int>(std::min<uint64_t>((deadline - now + 999) / 1000, INT32_MAX));
    }

    /**
     * read from device
     * @param buf: pointer to read buffer
     * @param buf_length: size of read buffer
     * @param timeout: timeout in ms
     * @return: 0 for nothing read, or poll timed out
     *	    < 0 for error
     *	    > 0 number of bytes read
     */
    int read(uint8_t* buf, int buf_length, int timeout)
    {
        if (_io_error)
            return _io_error;
        if (!buf || buf_length <= 0)
            return 0;
        GPSDeadline deadline{std::min(_operationDeadline.untilUs, nowUs() + uint64_t(std::max(timeout, 0)) * 1000)};
        const auto result = _io.read ? _io.read({buf, static_cast<size_t>(buf_length)}, deadline)
                                     : GPSProtocolReadResult{GPSReadStatus::Error};
        if (result.status == GPSReadStatus::Data && result.bytesRead >= 0 && result.bytesRead <= buf_length)
            return result.bytesRead;
        if (result.status == GPSReadStatus::TimedOut)
            return 0;
        _io_error = result.status == GPSReadStatus::Cancelled ? ReadCancelled : -EIO;
        return _io_error;
    }

    /**
     * write to the device
     * @param buf
     * @param buf_length
     * @return num written bytes, -1 on error
     */
    int write(const void* buf, int buf_length)
    {
        if (_io_error)
            return _io_error;
        if (!buf || buf_length < 0)
            return _io_error = -EINVAL;
        const auto result = _io.write ? _io.write({static_cast<const uint8_t*>(buf), static_cast<size_t>(buf_length)},
                                                  _operationDeadline)
                                      : GPSProtocolWriteResult{};
        _commandWrite.acceptedBytes += result.acceptedBytes;
        _commandWrite.writtenBytes += result.writtenBytes;
        _commandWrite.uncertainBytes += result.uncertainBytes;
        if (result.status == GPSWriteStatus::Completed && result.writtenBytes == buf_length)
            return result.writtenBytes;
        if (result.status == GPSWriteStatus::Unsupported)
            return -1;
        _io_error = result.status == GPSWriteStatus::Cancelled ? ReadCancelled : -EIO;
        return _io_error;
    }

    /**
     * set the Baudrate
     * @param baudrate
     * @return 0 on success, <0 otherwise
     */
    int setBaudrate(int baudrate)
    {
        if (_io_error)
            return _io_error;
        const auto result = _io.setBaudrate ? _io.setBaudrate(baudrate) : GPSBaudStatus::Unsupported;
        if (result == GPSBaudStatus::Configured)
            return 0;
        if (result == GPSBaudStatus::Unsupported)
            return -1;
        _io_error = result == GPSBaudStatus::Cancelled ? ReadCancelled : -EIO;
        return _io_error;
    }

    // A new configuration attempt starts a new I/O transaction. After a terminal
    // error, no command may be written until the caller explicitly retries.
    void resetIOError() { _io_error = 0; }

    void controlFailed()
    {
        if (!_io_error)
            _io_error = -EPROTO;
    }

    void publishSatellites(const GPSSatelliteReport& report)
    {
        _decoded.updates |= 2;
        _decoded.events.emplace_back(report);
    }

    void publishSatelliteUsage(std::optional<int> count)
    {
        _decoded.updates |= 2;
        _decoded.events.emplace_back(GPSSatelliteUsageReport{nowUs(), count});
    }

    void surveyInStatus(GPSSurveyReport& status)
    {
        status.timestamp = nowUs();
        _decoded.events.emplace_back(status);
    }

    /** got an RTCM message from the device */
    void gotRTCMMessage(uint8_t* buf, int buf_length)
    {
        if (buf_length < 0 || static_cast<size_t>(buf_length) > GPSRTCMReport{}.bytes.size())
            return;
        GPSRTCMReport report;
        report.size = static_cast<size_t>(buf_length);
        std::copy_n(buf, report.size, report.bytes.begin());
        _decoded.events.emplace_back(std::move(report));
    }

    /** got a relative position message from the device */
    void gotRelativePositionMessage(GPSRelativeReport& gnss_relative) { _decoded.events.emplace_back(gnss_relative); }

    /**
     * Convert a broken-down UTC time to microseconds since the Unix epoch, if the date is after the GPS epoch.
     * @param utc broken-down UTC time (normalized in place)
     * @param nsec sub-second part [ns], may be negative
     * @return microseconds since the Unix epoch, 0 if the date is implausible
     */
    uint64_t timeFromUtc(tm& utc, int32_t nsec);

    /**
     * Convert an ECEF (Earth Centered Earth Fixed) coordinate to LLA WGS84 (Lat, Lon, Alt).
     * @param ecef_x ECEF X-coordinate [m]
     * @param ecef_y ECEF Y-coordinate [m]
     * @param ecef_z ECEF Z-coordinate [m]
     * @param latitude [deg]
     * @param longitude [deg]
     * @param altitude [m]
     */
    static void ECEF2lla(double ecef_x, double ecef_y, double ecef_z, double& latitude, double& longitude,
                         float& altitude);

    /**
     * Convert an NMEA ddmm.mmmm (or dddmm.mmmm) coordinate to decimal degrees
     */
    static double nmeaToDegrees(double ddmm);

    GPSCommandResult _commandWrite;
    GPSDecodedBatch _decoded;
    GPSProtocolIO _io;
    int _io_error = 0;
    GPSDeadline _operationDeadline;
    bool _servicingControls = false;
};

inline bool operator&(GPSProtocol::GNSSSystemsMask a, GPSProtocol::GNSSSystemsMask b)
{
    return static_cast<int32_t>(a) & static_cast<int32_t>(b);
}
