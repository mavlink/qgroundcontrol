#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

#include <QtCore/QString>

#include "GPSAsciiProtocol.h"

/// Native N4 ASCII controller for UM980/UM982, R4.10 firmware with the documented rover modes.
/// Averaging has a maximum time, not an accuracy target; completion requires receiver FIXEDPOS evidence.
/// Commands affect the current port and are never saved to flash by this controller. Not hardware-qualified.
class GPSNativeUnicore final : public GPSAsciiProtocol
{
public:
    GPSNativeUnicore(GPSProtocolIO io, GPSNativePositionReport* position,
                     GPSNativeSatelliteReport* satellites = nullptr);

    int configure(unsigned& baud, const GPSConfig& config) override;

    bool receiverReady() const override { return _ready && !ioError(); }

    std::string_view model() const { return _model; }

    std::string_view firmware() const { return _firmware; }

protected:
    int handleReceiverLine(std::string_view line) override;
    int decodeByte(uint8_t byte) override;
    void flushDecoded() override;
    void servicePendingCommands() override;

private:
    enum class Reply
    {
        Acknowledgment,
        Version,
        Mode,
        FixedPosition,
    };

    enum class Mode
    {
        Rover,
        FixedBase,
        AveragingBase,
    };

    bool _execute(std::string command, Reply reply = Reply::Acknowledgment);
    bool _identify(unsigned& baud);
    int _configurationFailed(const QString& reason = {});
    void _handleVersion(std::string_view body);
    void _handleMode(std::string_view body);
    void _handlePosition(std::string_view body);
    void _publishBase(bool valid, bool active);
    void _invalidateBase();
    void _expireBase();

    static constexpr unsigned COMMAND_TIMEOUT_MS = 1500;
    static constexpr uint64_t BASE_STATUS_TIMEOUT_US = 5000000;

    std::string _model;
    std::string _firmware;
    std::string _command;
    QString _configurationDetail;
    Reply _expectedReply = Reply::Acknowledgment;
    Mode _expectedMode = Mode::Rover;
    GPSCommandOutcome _replyOutcome = GPSCommandOutcome::Pending;
    EcefMeters _fixedECEF;
    EcefMeters _baseECEF;
    uint64_t _lastBaseStatus = 0;
    std::optional<uint64_t> _lastBaseEpoch;
    bool _commandActive = false;
    bool _ready = false;
    bool _base = false;
    bool _monitorBase = false;
    bool _baseValid = false;
    bool _averaging = false;
};
