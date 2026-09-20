#pragma once

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "GPSAsciiProtocol.h"

/// LG290P(03), independently implemented from Quectel's GNSS Protocol Specification V1.1.
/// Role/base changes require explicit per-connection permission to save and restart.
/// Without permission, externally saved matching settings are required.
class GPSNativeQuectel final : public GPSAsciiProtocol
{
public:
    GPSNativeQuectel(GPSProtocolIO io, GPSNativePositionReport* position,
                     GPSNativeSatelliteReport* satellites = nullptr);

    int configure(unsigned& baud, const GPSConfig& config) override;

    bool receiverReady() const override { return _configured && !ioError(); }

    int receive(unsigned timeout) override;

protected:
    int decodeByte(uint8_t byte) override;
    void flushDecoded() override;
    int handleReceiverLine(std::string_view line) override;

private:
    enum class SurveyPhase
    {
        Off,
        AwaitingBoot,
        Verifying,
        Monitoring,
    };

    using ReplyHandler = std::function<GPSCommandOutcome(std::string_view)>;

    GPSCommandOutcome _transact(const std::string& command, ReplyHandler handler, unsigned timeoutMs = 1000);
    bool _acknowledge(const std::string& command, unsigned timeoutMs = 1000);
    bool _identify(unsigned timeoutMs = 1000);
    bool _verifyRole(bool requireMatch = true);
    bool _verifyBase(bool requireMatch = true);
    std::string _baseCommand() const;
    bool _saveConfiguration();
    bool _setMessageRate(std::string_view name, unsigned rate, std::string_view version = {});
    bool _restart(bool requireRoleMatch = true, bool startSurveySession = false);
    bool _handleSurvey(std::string_view body);
    void _revokeSurvey();
    void _expireSurvey();
    void _publishSurvey();
    int _fail(const char* reason);

    ReplyHandler _replyHandler;
    GPSCommandOutcome _reply = GPSCommandOutcome::Pending;
    std::string _firmware;
    std::string _surveyRestartCommand;
    EcefMeters _fixedECEF;
    OutputMode _outputMode = OutputMode::GPS;
    unsigned _receiverRole = 0;
    uint64_t _lastSurveyUs = 0;
    std::optional<unsigned> _lastSurveyTow;
    std::optional<GPSNativeSurveyReport> _surveyReport;
    SurveyPhase _surveyPhase = SurveyPhase::Off;
    bool _baseMatches = false;
    bool _baseHasDistance = false;
    bool _persistentSaveAcknowledged = false;
    bool _persistentSaveUncertain = false;
    bool _configured = false;
    bool _expectingBoot = false;
    bool _sawBoot = false;
    bool _restartRejected = false;
};
