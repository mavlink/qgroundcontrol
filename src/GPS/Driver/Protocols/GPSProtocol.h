#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <functional>
#include <numbers>
#include <optional>
#include <span>
#include <string_view>

#include <QtCore/QString>

#include "GPSBaseStationConfig.h"
#include "GPSDecodedBatch.h"
#include "GPSEllipsoidPosition.h"
#include "GPSProtocolIO.h"
#include "RTCMStreamDecoder.h"

class GPSRawAckMatcher;

inline constexpr int GPS_READ_BUFFER_SIZE = 150;
inline constexpr float GPS_PI = std::numbers::pi_v<float>;
inline constexpr float GPS_DEG_TO_RAD = GPS_PI / 180.0f;
inline constexpr double GPS_RAD_TO_DEG = 180.0 / std::numbers::pi;
inline constexpr time_t GPS_UTC_PLAUSIBILITY_FLOOR_SECS = static_cast<time_t>(1234567890ULL);

class GPSProtocol
{
public:
    GPSProtocolError ioError() const { return _ioError; }

    bool hasIOError() const { return _ioError != GPSProtocolError::None; }

    const QString& ioErrorDetail() const { return _ioErrorDetail; }

    void finishConfigurationEvidence() { failCommandWrite(GPSCommandOutcome::Written); }

    struct GPSConfig
    {
        GPSBaseStationConfig base{};
        bool allowPersistentChanges = false;
    };

    explicit GPSProtocol(GPSProtocolIO io, bool satelliteInfoEnabled = true);
    GPSProtocol(const GPSProtocol&) = delete;
    GPSProtocol& operator=(const GPSProtocol&) = delete;
    GPSProtocol(GPSProtocol&&) = delete;
    GPSProtocol& operator=(GPSProtocol&&) = delete;
    virtual ~GPSProtocol() = default;

    /// Configures the receiver. A zero @a baud requests detection and returns the detected rate.
    /// @return true when the receiver is ready; on failure, ioError() and ioErrorDetail() describe any I/O fault.
    [[nodiscard]] virtual bool configure(unsigned& baud, const GPSConfig& config) = 0;

    /// Reads and decodes for up to @a timeout ms.
    /// @return GPSDecodedBatch update flags; failures are reported through ioError(), never the return value.
    virtual int receive(unsigned timeout) = 0;
    virtual int consume(std::span<const uint8_t> bytes);
    GPSDecodeResult decode(std::span<const uint8_t> bytes);

    /**
     * Whether the receiver is configured and ready to accept injected data. Gates all
     * injection (RTCM corrections and moving-baseline) so nothing is written to the device
     * mid-configuration. Defaults to true; drivers with a configuration handshake override it.
     */
    virtual bool receiverReady() const { return true; }

    /// Model and firmware reported during configuration; empty when the family does not report them.
    virtual std::string receiverIdentity() const { return {}; }

protected:
    /// Optional base-station requests a protocol implements beyond survey-in and fixed positions.
    struct ConfigurationSupport
    {
        bool receiverAveraging = false;
        bool persistentChanges = false;
        bool compactObservations = false;
    };

    [[nodiscard]] bool validateConfiguration(const GPSConfig& config, ConfigurationSupport support) const;

    [[nodiscard]] bool validateConfiguration(const GPSConfig& config) const
    {
        return validateConfiguration(config, ConfigurationSupport{});
    }

    virtual int decodeByte(uint8_t) { return 0; }

    virtual void flushDecoded() {}

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

    /// Category for this receiver family. Driver categories are children of GPS.Driver.Protocols, so enabling
    /// the parent enables every driver.
    virtual const QLoggingCategory& logCategory() const;

    /// printf-style message; GCC and Clang check the format against its arguments.
    void log(GPSProtocolLogLevel level, const char* format, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);

    uint64_t nowUs() const { return _io.nowUs(); }

    void waitFor(std::chrono::microseconds duration)
    {
        if (hasIOError()) {
            return;
        }
        const auto now = nowUs();
        const auto remaining = _operationDeadline.untilUs > now ? _operationDeadline.untilUs - now : 0;
        duration = std::min(duration, std::chrono::microseconds(std::min<uint64_t>(remaining, INT64_MAX)));
        if (_io.wait && !_io.wait(duration)) {
            _ioError = GPSProtocolError::Cancelled;
            _ioErrorDetail.clear();
        }
    }

    virtual void servicePendingCommands() {}

    void serviceControls();

    /// Reads until updates arrive, the timeout expires, or I/O fails. @return update flags.
    int receiveDecoded(unsigned timeout);

    /// Read one bounded chunk, then return to the command matcher even when it contains only an ACK.
    /// @return update flags.
    int readAndDecode(unsigned timeout);

    /// Start one command attempt; its write time counts toward the subsequent awaitCommand deadline.
    bool writeCommand(GPSConfigurationStep step, std::span<const uint8_t> bytes);

    /// Decodes received traffic until @a reply resolves, the command deadline expires, or I/O fails.
    GPSCommandResult awaitCommand(const std::function<GPSCommandOutcome()>& reply);

    /// Writes @a wire and waits until the decoder calls resolveReply(), the step times out, or I/O fails.
    /// Received traffic keeps decoding while the reply is pending.
    GPSCommandResult transact(GPSConfigurationStep step, std::string_view wire);

    /// As above, and also resolves the reply from raw received bytes for replies that need not be complete frames.
    GPSCommandResult transact(GPSConfigurationStep step, std::string_view wire, GPSRawAckMatcher& reply);

    /// True while a transact() reply is outstanding and unresolved.
    bool replyPending() const { return _reply == GPSCommandOutcome::Pending; }

    /// Resolves the outstanding transact() reply; Pending and later replies are ignored.
    void resolveReply(GPSCommandOutcome outcome)
    {
        if (replyPending()) {
            _reply = outcome;
        }
    }

    void beginCommandWrite(GPSConfigurationStep step);

    void failCommandWrite(GPSCommandOutcome outcome) { (void) completeCommand(outcome); }

    /// Retire before notifying observers; repeated completion returns the retained evidence without republishing.
    GPSCommandResult completeCommand(GPSCommandOutcome outcome);

    int remainingMilliseconds(uint64_t deadline) const { return GPSDeadline{deadline}.remainingMilliseconds(nowUs()); }

    /// Reads up to @a buf_length bytes within @a timeout ms.
    /// @return bytes read, 0 when nothing arrived, or -1 after an I/O failure recorded in ioError().
    int read(uint8_t* buf, int buf_length, int timeout)
    {
        if (hasIOError()) {
            return -1;
        }
        if (!buf || buf_length <= 0) {
            return 0;
        }
        GPSDeadline deadline{std::min(_operationDeadline.untilUs, nowUs() + uint64_t(std::max(timeout, 0)) * 1000)};
        const auto result =
            _io.read ? _io.read({buf, static_cast<size_t>(buf_length)}, deadline) : GPSReadResult{GPSReadStatus::Error};
        _ioErrorDetail = result.detail;
        if (result.status == GPSReadStatus::Data && result.bytesRead >= 0 && result.bytesRead <= buf_length) {
            return result.bytesRead;
        }
        if (result.status == GPSReadStatus::TimedOut && result.bytesRead == 0) {
            return 0;
        }
        _ioError =
            result.status == GPSReadStatus::Cancelled ? GPSProtocolError::Cancelled : GPSProtocolError::Transport;
        if (_ioError != GPSProtocolError::Cancelled && _io.log) {
            _io.log(logCategory(), GPSProtocolLogLevel::Warning,
                    QStringLiteral("Receiver read failed (status %1): %2")
                        .arg(static_cast<int>(result.status))
                        .arg(_ioErrorDetail));
        }
        return -1;
    }

    /// Writes all of @a buf under the current command deadline.
    /// @return true when every byte was accepted and written. An unsupported write fails without a sticky error.
    [[nodiscard]] bool write(const void* buf, int buf_length)
    {
        if (hasIOError()) {
            return false;
        }
        if (!buf || buf_length < 0) {
            _ioErrorDetail.clear();
            _ioError = GPSProtocolError::InvalidArgument;
            return false;
        }
        const GPSDeadline deadline{
            std::min(_operationDeadline.untilUs, _commandCompleted ? UINT64_MAX : _commandDeadline.untilUs)};
        const auto result =
            _io.write ? _io.write({static_cast<const uint8_t*>(buf), static_cast<size_t>(buf_length)}, deadline)
                      : GPSWriteResult{};
        _ioErrorDetail = result.detail;
        _commandWrite.evidence.acceptedBytes += result.acceptedBytes;
        _commandWrite.evidence.writtenBytes += result.writtenBytes;
        _commandWrite.evidence.uncertainBytes += result.uncertainBytes();
        if (result.status == GPSWriteStatus::Completed && result.acceptedBytes == buf_length &&
            result.writtenBytes == buf_length && result.uncertainBytes() == 0) {
            return true;
        }
        if (result.status == GPSWriteStatus::Unsupported && result.acceptedBytes == 0 && result.writtenBytes == 0) {
            failCommandWrite(GPSCommandOutcome::TransportError);
            return false;
        }
        _ioError =
            result.status == GPSWriteStatus::Cancelled ? GPSProtocolError::Cancelled : GPSProtocolError::Transport;
        failCommandWrite(result.status == GPSWriteStatus::Cancelled ? GPSCommandOutcome::Cancelled
                                                                    : GPSCommandOutcome::TransportError);
        return false;
    }

    /// Changes the link baud rate. An unsupported change fails without a sticky error.
    bool setBaudrate(unsigned baudrate)
    {
        if (hasIOError()) {
            return false;
        }
        _ioErrorDetail.clear();
        const auto result = _io.setBaudrate ? _io.setBaudrate(baudrate) : GPSBaudStatus::Unsupported;
        if (result == GPSBaudStatus::Configured) {
            return true;
        }
        if (result != GPSBaudStatus::Unsupported) {
            _ioError = result == GPSBaudStatus::Cancelled ? GPSProtocolError::Cancelled : GPSProtocolError::Transport;
        }
        return false;
    }

    // A new configuration attempt starts a new I/O transaction. After a terminal
    // error, no command may be written until the caller explicitly retries.
    void resetIOError()
    {
        _ioError = GPSProtocolError::None;
        _ioErrorDetail.clear();
    }

    void controlFailed()
    {
        if (!hasIOError()) {
            _ioError = GPSProtocolError::Protocol;
            _ioErrorDetail.clear();
        }
    }

    /// The command outcome that reports the sticky I/O failure; Pending while I/O is healthy.
    GPSCommandOutcome ioCommandOutcome() const
    {
        switch (_ioError) {
            case GPSProtocolError::None:
                return GPSCommandOutcome::Pending;
            case GPSProtocolError::Cancelled:
                return GPSCommandOutcome::Cancelled;
            case GPSProtocolError::Transport:
            case GPSProtocolError::Protocol:
            case GPSProtocolError::InvalidArgument:
                return GPSCommandOutcome::TransportError;
        }
        return GPSCommandOutcome::TransportError;
    }

    void publishIntegrity()
    {
        _integrity.timestampUs = nowUs();
        _decoded.events.emplace_back(_integrity);
        _decoded.updates |= GPSDecodedBatch::PROTOCOL_ACTIVITY;
    }

    void publishSatellites(const GPSNativeSatelliteReport& report)
    {
        _decoded.updates |= GPSDecodedBatch::SATELLITES_UPDATE;
        _decoded.events.emplace_back(report);
    }

    void publishPosition(const GPSNativePositionReport& report)
    {
        _decoded.updates |= GPSDecodedBatch::POSITION_UPDATE;
        _decoded.events.emplace_back(report);
    }

    void publishSatelliteUsage(std::optional<int> count)
    {
        _decoded.updates |= GPSDecodedBatch::SATELLITES_UPDATE;
        _decoded.events.emplace_back(GPSNativeSatelliteUsageReport{nowUs(), count});
    }

    void publishSurvey(GPSNativeSurveyReport& status)
    {
        status.timestamp = nowUs();
        _decoded.events.emplace_back(status);
    }

    /// Publishes survey-in progress; unknown coordinates remain NaN.
    void publishSurvey(bool active, bool valid, std::chrono::seconds duration,
                       const GPSEllipsoidPosition& position = {})
    {
        GPSNativeSurveyReport status{};
        status.survey.position = position;
        status.survey.duration = duration;
        status.survey.valid = valid;
        status.survey.active = active;
        publishSurvey(status);
    }

    /** got an RTCM message from the device */
    void gotRTCMMessage(const uint8_t* buf, int buf_length)
    {
        if (buf_length < 0 || static_cast<size_t>(buf_length) > GPSRTCMReport{}.bytes.size()) {
            return;
        }
        GPSRTCMReport report;
        report.size = static_cast<size_t>(buf_length);
        std::copy_n(buf, report.size, report.bytes.begin());
        _decoded.events.emplace_back(std::move(report));
    }

    void drainRTCM(RTCMStreamDecoder& decoder, bool enabled = true)
    {
        decoder.drain([this, enabled](std::span<const uint8_t> frame) {
            if (_decoded.events.size() + 2 >= GPSDecodedBatch::MAX_EVENTS) {
                return false;
            }
            if (enabled) {
                gotRTCMMessage(frame.data(), static_cast<int>(frame.size()));
                _decoded.updates |= GPSDecodedBatch::PROTOCOL_ACTIVITY;
            }
            return true;
        });
    }

    /**
     * Convert a broken-down UTC time to microseconds since the Unix epoch, if the date is plausible.
     * @param utc broken-down UTC time (normalized in place)
     * @param nsec sub-second part [ns], may be negative
     * @return microseconds since the Unix epoch, 0 if the date is implausible
     */
    uint64_t timeFromUtc(tm& utc, int32_t nsec);

    struct EcefMeters
    {
        double x = 0;
        double y = 0;
        double z = 0;
    };

    static EcefMeters toEcef(const GPSEllipsoidPosition& position);
    static GPSEllipsoidPosition fromEcef(const EcefMeters& position);

    GPSBaseStationConfig _baseConfig;
    GPSNativePositionReport _position;
    GPSNativeSatelliteReport _satelliteStorage;
    GPSNativeSatelliteReport* const _satellites;
    bool _commandCompleted = true;
    GPSCommandResult _commandWrite;
    GPSDeadline _commandDeadline;
    GPSDecodedBatch _decoded;
    GPSIntegrityReport _integrity;
    GPSProtocolIO _io;
    GPSProtocolError _ioError = GPSProtocolError::None;
    QString _ioErrorDetail;
    GPSDeadline _operationDeadline;
    std::optional<GPSCommandOutcome> _reply;
    GPSRawAckMatcher* _rawReply = nullptr;
    bool _servicingControls = false;
};
