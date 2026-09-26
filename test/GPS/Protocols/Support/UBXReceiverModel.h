#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>

#include "GPSProtocolError.h"
#include "GPSReceiverReports.h"
#include "GPSTestClock.h"
#include "ScriptedReceiver.h"

/// Stateful UBX model: receiver configuration survives transport sessions.
class UBXReceiverModel : public ScriptedReceiver::Model
{
public:
    enum class Receiver
    {
        M8PBase,
        F9P,
        M8N,
        M9N,
        M10,
        M8PRover,
        F9R,
        Unidentified,
        U6,
        M8NEarly
    };
    enum class SurveyReply
    {
        Stopped,
        Active,
        Valid,
        Silent,
        BadChecksum,
        BadLength
    };
    enum class DisableReply
    {
        Ack,
        Nak,
        Timeout,
        WrongAck,
        CorruptAck,
        WriteError,
        ReadError,
        Cancelled
    };
    enum class ReadbackReply
    {
        Value,
        Nak,
        Timeout,
        AckOnly,
        WrongMessage,
        WrongKey,
        WrongValue,
        WrongLayer,
        WrongPosition,
        WrongVersion,
        Corrupt,
        Truncated,
        DuplicateValue,
        Oversized,
        WriteError,
        ReadError,
        Cancelled
    };
    enum class RateAck
    {
        OnTime,
        Late,     ///< After the host's timeout, ahead of the reply to the next command.
        Lost,     ///< The rate is applied, but its reply is lost.
        Ignored,  ///< Neither applied nor answered.
    };

    UBXReceiverModel(Receiver receiver, GPSTestClock& clock);

    void queueFrame(uint8_t messageClass, uint8_t messageId, const QByteArray& payload);
    void queueFrame(uint16_t message, std::span<const uint8_t> payload);
    void queueBytes(const QByteArray& bytes);
    void queueBytes(std::span<const uint8_t> bytes);
    void queueSurveyReply(SurveyReply reply);
    void queueBufferWarning(bool validChecksum = true);

    void attach(ScriptedReceiver& receiver);

    std::optional<GPSWriteResult> interceptLowLevelWrite(const QByteArray& bytes);

    bool readError() const { return _readError; }

    bool modern() const { return _modern; }

    GPSTestClock& clock() const { return *_clock; }

    bool lowLevelProtocolBehavior = false;
    bool wireValid = true;
    bool corruptVersionReplies = false;
    /// The reply to CFG-SEC-JAMDET arrives late: after the host's timeout, ahead of the next reply.
    bool delayOptionalAck = false;
    /// The late reply is a NAK, as from firmware without CFG-SEC-JAMDET, which also rejects its readback.
    bool delayOptionalNak = false;
    bool staleDisableAck = false;
    bool coalesceReplies = false;
    /// Rejects RTCM activation: the VALSET with the 1 Hz base rate, or on older profiles that CFG-RATE and each RTCM
    /// CFG-MSG rate.
    bool rejectRtcmActivation = false;
    /// Pre-protocol-27 wire receivers reject CFG-MSG rates and polls of these messages, as firmware without them does.
    std::vector<uint16_t> unsupportedMessages;
    /// Pre-protocol-27 wire receivers answer the CFG-MSG rate of this message as rateAck says.
    uint16_t rateAckMessage = 0;
    RateAck rateAck = RateAck::OnTime;
    /// Pre-protocol-27 wire receivers answer a CFG-MSG poll with the rates and an ACK, or not at all.
    bool silentRatePoll = false;
    /// The ACK or NAK that ends a CFG-MSG poll arrives this long after the poll, in order with other replies.
    std::chrono::microseconds ratePollAckDelay{0};
    /// Replies arrive this long after their command, in order, as from a slow M8.
    std::chrono::microseconds replyDelay{0};
    unsigned ratePolls = 0;
    /// Protocol 27+ wire receivers NAK a CFG-VALSET or CFG-VALGET with one of these keys, as firmware without them
    /// does (F9 before HPG 1.50 has no CFG-SEC-JAMDET).
    std::vector<uint32_t> unsupportedKeys;
    /// The NAK of a CFG-VALSET with an unsupported key is lost.
    bool loseUnsupportedNak = false;
    DisableReply disableReply = DisableReply::Ack;
    ReadbackReply readbackReply = ReadbackReply::Value;
    quint32 faultReadbackKey = 0;
    unsigned timeMode = 0;
    unsigned navigationModel = 0;
    unsigned surveyDuration = 0;
    unsigned surveyAccuracy = 0;
    uint32_t fixedAccuracy = 0;
    unsigned retainedSurveyDuration = 0;
    bool surveyStopStuck = false;
    int surveyStopReads = 0;
    QList<SurveyReply> surveyReplies;
    unsigned surveyPolls = 0;
    unsigned readbackRequests = 0;
    bool rejectDisable = false;
    bool rejectStart = false;
    unsigned transportOperations = 0;
    uint32_t legacyFixedAccuracy = 0;
    bool legacy = false;
    std::string module = "ZED-F9P";
    std::string hardware;
    std::string protocol;
    unsigned receiverBaud = 0;
    unsigned hostBaud = 0;
    bool usb = false;
    bool loseBaudAck = false;
    bool ignoreBaudChange = false;
    bool corruptIdentity = false;
    bool silencePortConfiguration = false;
    bool rejectAfterBaudChange = false;
    bool nakAfterBaudChange = false;
    bool lateBaudAckDelivered = false;
    unsigned configurationWrites = 0;
    unsigned unidentifiedWrites = 0;
    std::vector<unsigned> identityBauds;
    std::vector<unsigned> hostBauds;
    bool failPollWrite = false;
    uint32_t failValsetKey = 0;
    GPSWriteStatus valsetWriteFailure = GPSWriteStatus::Error;
    GPSProtocolError pollReadError = GPSProtocolError::None;
    QString pollReadDetail = QStringLiteral("Receiver link lost: Gerät");
    size_t readChunk = 7;
    unsigned commsPolls = 0;
    bool failCommsWrite = false;
    unsigned starts = 0;
    unsigned statusCallbacks = 0;
    unsigned rtcmEnables = 0;
    std::vector<uint32_t> modes;
    std::map<uint32_t, uint32_t> startSettings;
    std::map<uint32_t, uint32_t> currentSettings;
    std::map<uint16_t, uint8_t> messageRates;
    uint64_t disabledAt = 0;
    uint64_t startedAt = 0;
    GPSIntegrityReport integrity;
    unsigned integrityCount = 0;
    int disableCommands = 0;
    int timeModeReads = 0;
    int optionalAckDelays = 0;
    int resetCommands = 0;
    int failedReads = 0;

private:
    struct Response
    {
        QByteArray bytes;
    };

    struct DelayedResponse
    {
        uint64_t atUs;
        Response response;
    };

    void reset(ScriptedReceiver& receiver) override;
    std::optional<QByteArray> takeCommand(QByteArray& pending) override;
    GPSWriteResult handleCommand(ScriptedReceiver& receiver, const QByteArray& command,
                                 const ScriptedReceiver::WriteContext& context) override;
    std::optional<bool> handleBaudrate(ScriptedReceiver& receiver, unsigned baudrate) override;
    void onTransportReadWait(ScriptedReceiver& receiver, std::chrono::milliseconds timeout) override;
    void onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline) override;
    int readChunkSize(const ScriptedReceiver& receiver, int requested, int available) const override;
    bool coalesceReads(const ScriptedReceiver& receiver) const override;
    bool _handleFrame(ScriptedReceiver& receiver, const QByteArray& frame);
    bool _handleLowLevelFrame(ScriptedReceiver& receiver, const QByteArray& frame);
    bool _handleLegacyFrame(ScriptedReceiver& receiver, uint16_t message, const QByteArray& payload);
    QHash<quint32, quint64> _valsetValues(const QByteArray& payload);
    bool _replyToSetting(ScriptedReceiver& receiver, uint8_t messageId, DisableReply reply);
    bool _replyToReadback(ScriptedReceiver& receiver, uint8_t messageId, QByteArray payload, quint32 key);
    bool _replyToRatePoll(ScriptedReceiver& receiver, uint16_t output);
    void _queueAck(ScriptedReceiver& receiver, uint8_t messageId, bool accepted,
                   std::chrono::microseconds delay = std::chrono::microseconds::zero());
    static QByteArray _frame(uint8_t messageClass, uint8_t messageId, const QByteArray& payload);
    static QByteArray _frame(uint16_t message, std::span<const uint8_t> payload);
    /// Queues @a response to arrive after @a delay or replyDelay, whichever is longer, and after earlier replies.
    void _queueResponse(ScriptedReceiver& receiver, const Response& response,
                        std::chrono::microseconds delay = std::chrono::microseconds::zero());
    /// Delivers the delayed replies due by now.
    void _deliverDelayed(ScriptedReceiver& receiver);
    QByteArray _lowLevelIdentityPayload();

    GPSTestClock* _clock;
    ScriptedReceiver* _receiver = nullptr;
    bool _modern = false;
    bool _supportsTimeMode = true;
    bool _readError = false;
    QByteArray _version;
    QList<Response> _incoming;
    bool _delayNextValsetAck = false;
    /// Queued ahead of the next response.
    std::optional<Response> _heldReply;
    std::deque<DelayedResponse> _delayed;
    QByteArray _heldBaudAck;
    QByteArray _partialWriteProbe;
    bool _identityDelivered = false;
};
