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
 * @file gps_helper.h
 * @author Thomas Gubler <thomasgubler@student.ethz.ch>
 * @author Julian Oes <julian@oes.ch>
 */

#pragma once

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <span>

#include "GPSBaseStationConfig.h"
#include "GPSProtocolIO.h"

#ifndef GPS_PLATFORM_HEADER
#define GPS_PLATFORM_HEADER "GPSDriverPlatform.h"
#endif

#include GPS_PLATFORM_HEADER

#ifndef GPS_READ_BUFFER_SIZE
#define GPS_READ_BUFFER_SIZE 150  ///< buffer size for the read() call. Messages can be longer than that.
#endif

#ifndef M_PI_F
#define M_PI_F 3.14159265358979323846f
#endif

// TODO: this number seems wrong
#define GPS_EPOCH_SECS ((time_t) 1234567890ULL)

class GPSProtocol
{
public:
    static constexpr int ReadCancelled = -ECANCELED;

    int ioError() const { return _io_error; }

    enum class OutputMode : uint8_t
    {
        GPS = 0,     ///< normal GPS output
        GPSAndRTCM,  ///< normal GPS+RTCM output
        RTCM         ///< request RTCM output. This is used for (fixed position) base stations
    };

    /**
     * Bitmask for GPS_1_GNSS and GPS_2_GNSS
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

    enum class InterfaceProtocolsMask : int32_t
    {
        ALL_DISABLED = 0,
        I2C_IN_PROT_UBX = 1 << 0,
        I2C_IN_PROT_NMEA = 1 << 1,
        I2C_IN_PROT_RTCM3X = 1 << 2,
        I2C_OUT_PROT_UBX = 1 << 3,
        I2C_OUT_PROT_NMEA = 1 << 4,
        I2C_OUT_PROT_RTCM3X = 1 << 5
    };

    struct GPSConfig
    {
        GPSBaseStationConfig base;
        OutputMode output_mode;
        GNSSSystemsMask gnss_systems;
        InterfaceProtocolsMask interface_protocols;
        bool cfg_wipe;
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
    virtual int consume(std::span<const uint8_t> bytes) = 0;

    /**
     * Whether the receiver is configured and ready to accept injected data. Gates all
     * injection (RTCM corrections and moving-baseline) so nothing is written to the device
     * mid-configuration. Defaults to true; drivers with a configuration handshake override it.
     */
    virtual bool receiverReady() const { return true; }

protected:
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
        GPSProtocolDeadline _previous;
    };

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
        GPSProtocolDeadline deadline{
            std::min(_operationDeadline.untilUs, nowUs() + uint64_t(std::max(timeout, 0)) * 1000)};
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

    void surveyInStatus(GPSSurveyReport& status)
    {
        if (_io.survey)
            _io.survey(status);
    }

    /** got an RTCM message from the device */
    void gotRTCMMessage(uint8_t* buf, int buf_length)
    {
        if (_io.rtcm)
            _io.rtcm({buf, static_cast<size_t>(buf_length)});
    }

    /** got a relative position message from the device */
    void gotRelativePositionMessage(GPSRelativeReport& gnss_relative)
    {
        if (_io.relativePosition)
            _io.relativePosition(gnss_relative);
    }

    /**
     * Convert a broken-down UTC time to microseconds since the Unix epoch, if the date is after the GPS epoch.
     * @param utc broken-down UTC time (normalized in place)
     * @param nsec sub-second part [ns], may be negative
     * @return microseconds since the Unix epoch, 0 if the date is implausible or NO_MKTIME is defined
     */
    uint64_t timeFromUtc(tm& utc, int32_t nsec);

    /**
     * Convert an ECEF (Earth Centered Earth Fixed) coordinate to LLA WGS84 (Lat, Lon, Alt).
     * Ported from: https://stackoverflow.com/a/25428344
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

    GPSProtocolIO _io;
    int _io_error = 0;
    GPSProtocolDeadline _operationDeadline;
    bool _servicingControls = false;
};

inline bool operator&(GPSProtocol::GNSSSystemsMask a, GPSProtocol::GNSSSystemsMask b)
{
    return static_cast<int32_t>(a) & static_cast<int32_t>(b);
}

inline bool operator&(GPSProtocol::InterfaceProtocolsMask a, GPSProtocol::InterfaceProtocolsMask b)
{
    return static_cast<int32_t>(a) & static_cast<int32_t>(b);
}
