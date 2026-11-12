#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

#include "GPSCommandChannel.h"
#include "GPSReceiverConfig.h"
#include "Protocols/Support/GPSTestClock.h"
#include "Protocols/Support/ScriptedReceiver.h"

namespace GPSTest {

/// Stateful UBX model: receiver configuration survives transport sessions. A receiver preset reports its recorded
/// identity; otherwise the module, hardware, protocol and legacy fields describe the receiver, which reports a
/// firmware version of that module.
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
        ReadError
    };
    enum class RateAck
    {
        OnTime,
        Late,     ///< After the host's timeout, ahead of the reply to the next command.
        Lost,     ///< The rate is applied, but its reply is lost.
        Ignored,  ///< Neither applied nor answered.
    };

    /// A receiver the module, hardware, protocol and legacy fields describe.
    explicit UBXReceiverModel(GPSTestClock& clock);
    /// A receiver that reports @a receiver's recorded identity.
    UBXReceiverModel(Receiver receiver, GPSTestClock& clock);

    void queueFrame(uint8_t messageClass, uint8_t messageId, const QByteArray& payload);
    void queueFrame(uint16_t message, std::span<const uint8_t> payload);
    void queueBytes(const QByteArray& bytes);
    void queueBytes(std::span<const uint8_t> bytes);
    void queueSurveyReply(SurveyReply reply);

    /// The write failure failValsetKey asks for, for a link that writes through this model.
    std::optional<GPSWriteResult> interceptWrite(const QByteArray& bytes);

    bool readError() const { return _readError; }

    GPSTestClock& clock() const { return *_clock; }

    /// False once the host sent what a driver must not send to this receiver. A message the model does not take is
    /// answered as a receiver answers one it does not know: with ACK-NAK in class CFG, which a CFG-VALSET or
    /// CFG-VALGET that cannot be read gets too, and not at all in another class. A receiver before protocol 27 takes
    /// CFG-PRT, CFG-MSG, CFG-RATE, CFG-NAV5 and CFG-TMODE3 here, fewer messages than such a receiver has, set and
    /// polled in the forms it has of them: CFG-PRT for one port or several, polled for one port or for the port in
    /// use, and CFG-MSG for the port in use or for all six. A poll's answer holds what the host set of the message and
    /// zero for the rest, as the model has no factory configuration, and another length of the message gets ACK-NAK.
    /// The write fails of bytes that are no frame, and of a MON-VER or NAV-SVIN poll the driver must not send as it
    /// is.
    bool wireValid = true;
    bool corruptVersionReplies = false;
    /// The reply to CFG-SEC-JAMDET arrives late: after the host's timeout, ahead of the next reply.
    bool delayOptionalAck = false;
    /// The late reply is a NAK, as from firmware without CFG-SEC-JAMDET, which also rejects its readback.
    bool delayOptionalNak = false;
    bool staleDisableAck = false;
    bool coalesceReplies = false;
    /// Rejects RTCM activation: the VALSET with the 1 Hz base rate, or on legacy receivers that CFG-RATE and each RTCM
    /// CFG-MSG rate.
    bool rejectRtcmActivation = false;
    /// Legacy receivers reject CFG-MSG rates and polls of these messages, as firmware without them does.
    std::vector<uint16_t> unsupportedMessages;
    /// Legacy receivers answer the CFG-MSG rate of this message as rateAck says.
    uint16_t rateAckMessage = 0;
    RateAck rateAck = RateAck::OnTime;
    /// Legacy receivers answer a CFG-MSG poll with the rates and an ACK, or not at all.
    bool silentRatePoll = false;
    /// The ACK or NAK that ends a CFG-MSG poll arrives this long after the poll, in order with other replies.
    std::chrono::microseconds ratePollAckDelay{0};
    /// Replies arrive this long after their command, in order, as from a slow M8.
    std::chrono::microseconds replyDelay{0};
    unsigned ratePolls = 0;
    /// Protocol 27+ receivers NAK a CFG-VALSET or CFG-VALGET with one of these keys, as firmware without them does
    /// (F9 before HPG 1.50 has no CFG-SEC-JAMDET).
    std::vector<uint32_t> unsupportedKeys;
    /// The NAK of a CFG-VALSET with an unsupported key is lost.
    bool loseUnsupportedNak = false;
    DisableReply disableReply = DisableReply::Ack;
    /// How the receiver answers a readback, a CFG-TMODE3 poll or a CFG-VALGET. It answers a poll of class CFG with the
    /// polled message and then acknowledges it, so an answer is followed by an ACK-ACK that names the polled message.
    ReadbackReply readbackReply = ReadbackReply::Value;
    quint32 faultReadbackKey = 0;
    /// That ACK-ACK arrives this long after the readback, in order with other replies: output the receiver sends
    /// unasked can come between the answer and it.
    std::chrono::microseconds readbackAckDelay{0};
    /// Legacy receivers reject the fixed position of a CFG-TMODE3.
    bool rejectFixedPosition = false;
    /// The time mode in effect: 0 disabled, 1 survey-in, 2 fixed.
    unsigned timeMode = 0;
    /// Where the receiver finds itself outside time mode: it then sends ownPositionFix there OWN_POSITION_DELAY after
    /// time mode was disabled, once. Unset, as by default, the receiver has no position of its own.
    std::optional<GPSEllipsoidPosition> ownPosition;

    /// The fix of that position: NAV-PVT fixType, and its horizontal and vertical accuracy in millimetres.
    struct OwnPositionFix
    {
        uint8_t fixType = 3;
        uint32_t horizontalAccuracyMm = 1500;
        uint32_t verticalAccuracyMm = 2500;
    };

    OwnPositionFix ownPositionFix;
    static constexpr std::chrono::seconds OWN_POSITION_DELAY{1};
    unsigned navigationModel = 0;
    unsigned surveyDuration = 0;
    uint32_t fixedAccuracy = 0;
    /// The duration a survey the receiver kept from an earlier session reports until time mode is disabled.
    unsigned retainedSurveyDuration = 0;
    /// The survey stays active after time mode is disabled.
    bool surveyStopStuck = false;
    /// Replies to successive NAV-SVIN polls, the last repeating; without any, the survey state answers.
    QList<SurveyReply> surveyReplies;
    unsigned surveyPolls = 0;
    bool rejectStart = false;
    unsigned transportOperations = 0;
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
    unsigned configurationWrites = 0;
    std::vector<unsigned> identityBauds;
    uint32_t failValsetKey = 0;
    GPSWriteStatus valsetWriteFailure = GPSWriteStatus::Error;
    size_t readChunk = 7;
    unsigned starts = 0;
    unsigned rtcmEnables = 0;
    /// Every time mode written, applied or not.
    std::vector<uint32_t> modes;
    std::map<uint32_t, uint32_t> startSettings;
    std::map<uint32_t, uint32_t> currentSettings;
    std::map<uint16_t, uint8_t> messageRates;
    uint64_t disabledAt = 0;
    uint64_t startedAt = 0;
    int optionalAckDelays = 0;
    int resetCommands = 0;

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
    std::optional<bool> handleBaudRate(ScriptedReceiver& receiver, unsigned baudRate) override;
    void onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline) override;
    int readChunkSize(const ScriptedReceiver& receiver, int requested, int available) const override;
    bool coalesceReads(const ScriptedReceiver& receiver) const override;
    bool _handleFrame(ScriptedReceiver& receiver, const QByteArray& frame);
    bool _handleValset(ScriptedReceiver& receiver, uint16_t message, const QByteArray& payload);
    bool _handleLegacyFrame(ScriptedReceiver& receiver, uint16_t message, const QByteArray& payload);
    bool _handleLegacyPorts(ScriptedReceiver& receiver, const QByteArray& payload);
    bool _handleLegacyMessageRate(ScriptedReceiver& receiver, const QByteArray& payload);
    /// Answers a poll of @a message with @a payload, and acknowledges it.
    bool _answerPoll(ScriptedReceiver& receiver, uint16_t message, const QByteArray& payload);
    /// Answers @a message as one the receiver does not take, which the host must not send it.
    bool _refuse(ScriptedReceiver& receiver, uint16_t message);
    /// Disables time mode as disableReply says; @a apply applies the rest of the command if time mode is disabled.
    bool _disableTimeMode(ScriptedReceiver& receiver, uint8_t messageId, const std::function<void()>& apply);
    bool _replyToSetting(ScriptedReceiver& receiver, uint8_t messageId, DisableReply reply);
    bool _replyToReadback(ScriptedReceiver& receiver, uint8_t messageId, QByteArray payload, quint32 key);
    bool _replyToRatePoll(ScriptedReceiver& receiver, uint16_t output);
    void _queueAck(ScriptedReceiver& receiver, uint8_t messageId, bool accepted,
                   std::chrono::microseconds delay = std::chrono::microseconds::zero());
    static QByteArray _frame(uint8_t messageClass, uint8_t messageId, const QByteArray& payload);
    static QByteArray _frame(uint16_t message, std::span<const uint8_t> payload);
    /// An ACK-ACK or ACK-NAK of @a message.
    static QByteArray _ackFrame(uint16_t message, bool accepted);
    /// Queues @a response to arrive after @a delay or replyDelay, whichever is longer, and after earlier replies.
    void _queueResponse(ScriptedReceiver& receiver, const Response& response,
                        std::chrono::microseconds delay = std::chrono::microseconds::zero());
    /// Delivers the delayed replies due by now.
    void _deliverDelayed(ScriptedReceiver& receiver);
    QByteArray _identityPayload();
    /// A NAV-PVT of ownPositionFix at ownPosition.
    QByteArray _ownPositionFrame() const;

    GPSTestClock* _clock;
    ScriptedReceiver* _receiver = nullptr;
    bool _readError = false;
    /// A preset's recorded MON-VER payload; empty when the fields describe the receiver.
    QByteArray _presetIdentity;
    bool _delayNextValsetAck = false;
    /// Queued ahead of the next response.
    std::optional<Response> _heldReply;
    std::deque<DelayedResponse> _delayed;
    QByteArray _heldBaudAck;
    std::optional<uint64_t> _ownPositionDueUs;
    /// The I/O ports a CFG-MSG holds a rate for.
    static constexpr qsizetype PORTS = 6;
    /// What a legacy receiver was last set to: each port of CFG-PRT, CFG-RATE, and the CFG-MSG rates on every port.
    std::map<uint8_t, QByteArray> _ports;
    QByteArray _legacyRate = QByteArray(6, '\0');
    std::map<uint16_t, std::array<uint8_t, PORTS>> _portRates;
};

}  // namespace GPSTest
