#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

#include <QtCore/QByteArrayView>

#include "GPSBaseStationConfig.h"
#include "GPSCommandTransaction.h"
#include "GPSReceiverFamily.h"
#include "GPSTask.h"
#include "UBXConfigurationValues.h"
#include "UBXPlan.h"

class GPSCommandChannel;
class UBXDecoder;

/// Configures a u-blox receiver and runs its streaming services as coroutines on a GPSCommandChannel. The receiver
/// settings come from UBXPlan; this class holds the control flow that depends on replies: baud detection and the
/// UART handoff, jamming-detection fallback, time mode, survey-in and RTCM activation.
class UBXConfigurator
{
public:
    explicit UBXConfigurator(UBXDecoder& decoder);

    GPSTask<bool> configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud);

    /// Diagnostics polls, RTCM activation after survey-in, and disabling unexpected output, as decoding requested.
    GPSTask<void> serviceStreaming(GPSCommandChannel& channel);

private:
    /// How an RTCM activation ended. An unanswered one is resumed by a later streaming service.
    enum class RTCMActivation : uint8_t
    {
        Active,
        Failed,
        Unanswered,
    };

    /// The verdict on a CFG-MSG rate.
    struct RateConfirmation
    {
        GPSCommandOutcome outcome;
        /// No reply to the rate or its poll is still to arrive, so a later CFG-MSG reply can be attributed.
        bool settled;
    };

    /// The legacy RTCM activation outputs settled so far, which a resumed activation does not repeat.
    struct LegacyRTCMProgress
    {
        size_t next = 0;
        bool stationPosition = false;
        bool observations = false;
    };

    [[nodiscard]] UBX::Plan::Target _target() const;
    [[nodiscard]] bool _baseStationUnsupported() const;
    GPSTask<bool> _identify();
    GPSTask<bool> _setUpPort(unsigned detectedBaud, unsigned desiredBaud);
    GPSTask<bool> _configureDevice();
    GPSTask<bool> _configureJammingDetection();
    GPSTask<bool> _configureLegacyDevice();
    GPSTask<bool> _restartSurveyIn();
    GPSTask<bool> _restartLegacySurveyIn();
    GPSTask<bool> _disableTimeMode();
    GPSTask<bool> _waitForSurveyStop();
    GPSTask<RTCMActivation> _activateRTCMOutput();
    GPSTask<RTCMActivation> _activateLegacyRTCMOutput();
    GPSTask<void> _requestCommsDiagnostics();
    GPSTask<void> _disableUnexpectedMessage(uint16_t message);

    /// Writes one UBX frame as a command attempt: header, payload and checksum in separate writes.
    GPSTask<bool> _send(UBX::MessageId message, QByteArrayView payload, GPSConfigurationStep step);
    GPSTask<bool> _sendAcknowledged(UBX::MessageId message, QByteArrayView payload);
    GPSTask<GPSCommandResult> _waitForAck(UBX::MessageId message);
    GPSTask<bool> _setMessageRate(UBX::Plan::MessageRate rate);
    GPSTask<bool> _setMessageRateAcknowledged(UBX::Plan::MessageRate rate);
    GPSTask<GPSCommandOutcome> _setMessageRateReply(UBX::Plan::MessageRate rate);
    /// As above, but polls a rate left unanswered, as an M8 may acknowledge up to a second late: ReadbackVerified or
    /// Rejected from the polled rates or a NAK, TimedOut when the poll is unanswered too. A poll that expires with only
    /// some of its replies is unsettled.
    GPSTask<RateConfirmation> _confirmMessageRate(UBX::Plan::MessageRate rate);
    GPSTask<bool> _runPlan(const std::vector<UBX::Plan::ValsetBatch>& batches);
    /// An ambiguous VALSET refuses later ones, unless each is proven by readback.
    [[nodiscard]] bool _valsetsRefused() const;
    void _load(const UBX::Plan::ValsetBatch& batch);
    GPSTask<bool> _writeValset(const UBX::Plan::ValsetBatch& batch);
    GPSTask<GPSCommandResult> _transactValset(const UBX::Plan::ValsetBatch& batch);
    /// Waits for the loaded batch's acknowledgement; an optional batch left unanswered is settled by readback.
    GPSTask<GPSCommandResult> _awaitValsetAck();
    /// Proves the loaded batch by CFG-VALGET, when its acknowledgement cannot be trusted.
    GPSTask<GPSCommandResult> _verifyValset(GPSConfigurationStep step);
    GPSTask<bool> _verifyValue(UBX::CfgKey<uint8_t> key, uint8_t value);

    UBXDecoder& _decoder;
    GPSCommandChannel* _channel = nullptr;
    GPSBaseStationConfig _base;
    UBX::CheckedValsetBatch<UBX::Plan::VALSET_CAPACITY> _valset;
    /// A VALSET left unanswered, and not settled by readback, leaves the receiver state unknown; later VALSETs are
    /// refused unless verified.
    bool _valsetAckAmbiguous = false;
    LegacyRTCMProgress _legacyRTCM;
    uint64_t _lastDisableUs = 0;
    uint64_t _nextCommsPollUs = 0;
};
