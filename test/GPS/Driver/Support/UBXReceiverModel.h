#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>

#include "GPSTestClock.h"
#include "Support/ScriptedReceiver.h"

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
        Cancelled,
        AckWithoutChange
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
    bool delayOptionalAck = false;
    bool staleDisableAck = false;
    bool staleSbasAck = false;
    bool coalesceReplies = false;
    bool rejectRtcmActivation = false;
    DisableReply disableReply = DisableReply::Ack;
    DisableReply sbasReply = DisableReply::Ack;
    ReadbackReply readbackReply = ReadbackReply::Value;
    quint32 faultReadbackKey = 0;
    unsigned timeMode = 0;
    unsigned sbasEnabled = 0;
    unsigned sbasL1caEnabled = 0;
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
    uint16_t legacyMeasurementInterval = 0;
    uint8_t legacyDynamicModel = 0;
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
    int disableAcksRead = 0;
    int timeModeReads = 0;
    int sbasCommands = 0;
    int sbasReads = 0;
    int optionalAckDelays = 0;
    int resetCommands = 0;
    int failedReads = 0;
    QByteArray lastDisablePayload;

private:
    struct Response
    {
        QByteArray bytes;
        bool disableAck = false;
    };

    void reset(ScriptedReceiver& receiver) override;
    std::optional<QByteArray> takeCommand(QByteArray& pending) override;
    GPSWriteResult handleCommand(ScriptedReceiver& receiver, const QByteArray& command,
                                 const ScriptedReceiver::WriteContext& context) override;
    std::optional<bool> handleBaudrate(ScriptedReceiver& receiver, unsigned baudrate) override;
    void onTransportReadWait(ScriptedReceiver& receiver, int timeoutMs) override;
    void onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline) override;
    int readChunkSize(const ScriptedReceiver& receiver, int requested, int available) const override;
    bool coalesceReads(const ScriptedReceiver& receiver) const override;
    bool _handleFrame(ScriptedReceiver& receiver, const QByteArray& frame);
    bool _handleLowLevelFrame(ScriptedReceiver& receiver, const QByteArray& frame);
    bool _handleLegacyFrame(ScriptedReceiver& receiver, uint16_t message, const QByteArray& payload);
    QHash<quint32, quint64> _valsetValues(const QByteArray& payload);
    bool _replyToSetting(ScriptedReceiver& receiver, uint8_t messageId, DisableReply reply, bool disable = false);
    bool _replyToReadback(ScriptedReceiver& receiver, uint8_t messageId, QByteArray payload, quint32 key);
    void _queueAck(ScriptedReceiver& receiver, uint8_t messageId, bool accepted, bool disableAck = false);
    static QByteArray _frame(uint8_t messageClass, uint8_t messageId, const QByteArray& payload);
    static QByteArray _frame(uint16_t message, std::span<const uint8_t> payload);
    void _queueResponse(ScriptedReceiver& receiver, const Response& response);
    QByteArray _lowLevelIdentityPayload();

    GPSTestClock* _clock;
    ScriptedReceiver* _receiver = nullptr;
    bool _modern = false;
    bool _supportsTimeMode = true;
    bool _readError = false;
    QByteArray _version;
    QList<Response> _incoming;
    bool _delayNextValsetAck = false;
    std::optional<Response> _heldValsetAck;
    QByteArray _heldBaudAck;
    QByteArray _partialWriteProbe;
    bool _identityDelivered = false;
};
