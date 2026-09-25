#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

#include "GPSAsciiProtocol.h"

/// LG290P(03), independently implemented from Quectel's GNSS Protocol Specification V1.1.
/// Role/base changes require explicit per-connection permission to save and restart.
/// Without permission, externally saved matching settings are required.
class QuectelProtocol final : public GPSAsciiProtocol
{
public:
    explicit QuectelProtocol(GPSProtocolIO io, bool satelliteInfoEnabled = true);

    bool configure(unsigned& baud, const GPSConfig& config) override;

    bool receiverReady() const override { return _configured && !hasIOError(); }

    std::string receiverIdentity() const override { return _firmware; }

    int receive(unsigned timeout) override;

protected:
    int decodeByte(uint8_t byte) override;
    void flushDecoded() override;
    int handleReceiverLine(std::string_view line) override;

private:
    const QLoggingCategory& logCategory() const override;
    enum class SurveyPhase
    {
        Off,
        AwaitingBoot,
        Verifying,
        Monitoring,
    };

    GPSCommandResult _transact(const std::string& command, GPSReplyMatcher reply, unsigned timeoutMs = 1000);
    GPSCommandResult _acknowledgement(const std::string& command, unsigned timeoutMs = 1000);
    bool _acknowledge(const std::string& command, unsigned timeoutMs = 1000);
    bool _identify(unsigned timeoutMs = 1000);
    bool _verifyRole(bool requireMatch = true);
    bool _verifyBase(bool requireMatch = true);
    std::string _baseCommand() const;
    bool _saveConfiguration();
    bool _setMessageRate(std::string_view name, unsigned rate, std::string_view version = {});
    bool _restart(bool requireRoleMatch = true);
    bool _handleSurvey(std::string_view body);
    void _revokeSurvey();
    void _expireSurvey();
    void _publishSurvey();
    bool _fail(const char* reason);

    struct SurveySession
    {
        std::string restartCommand;
        std::optional<unsigned> lastTow = std::nullopt;
        std::optional<GPSDecodedSurvey> report = std::nullopt;
        SurveyPhase phase = SurveyPhase::Off;
    };

    SurveySession _survey;
    std::string _firmware;
    EcefMeters _fixedECEF;
    unsigned _receiverRole = 0;
    bool _baseMatches = false;
    bool _baseHasDistance = false;
    bool _persistentSaveAcknowledged = false;
    bool _persistentSaveUncertain = false;
    bool _configured = false;
    bool _expectingBoot = false;
    bool _sawBoot = false;
    bool _restartRejected = false;
};
