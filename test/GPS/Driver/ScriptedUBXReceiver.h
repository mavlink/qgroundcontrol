#pragma once

#include <atomic>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>

#include "GPSTransport.h"

/// Stateful UBX peer: receiver configuration survives destruction of a GPSDriver.
class ScriptedUBXReceiver : public GPSTransport
{
public:
    enum class Model
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
        Oversized,
        WriteError,
        ReadError,
        Cancelled
    };

    ScriptedUBXReceiver(Model model, std::atomic_bool& stopRequested);

    GPSOpenResult open() override { return {GPSOpenStatus::Opened}; }

    bool fatalError() const override { return _readError; }

    unsigned fixedBaudrate() const override { return 115200; }

    bool setBaudrate(unsigned) override { return true; }

    GPSReadResult read(uint8_t* buffer, int length, int timeoutMs) override;
    GPSWriteResult write(const uint8_t* buffer, int length) override;

    bool modern() const { return _modern; }

    bool wireValid = true;
    bool corruptVersionReplies = false;
    bool delayOptionalAck = false;
    bool staleDisableAck = false;
    bool staleSbasAck = false;
    bool coalesceReplies = false;
    DisableReply disableReply = DisableReply::Ack;
    DisableReply sbasReply = DisableReply::Ack;
    ReadbackReply readbackReply = ReadbackReply::Value;
    quint32 faultReadbackKey = 0;
    unsigned timeMode = 0;
    unsigned sbasEnabled = 0;
    unsigned sbasL1caEnabled = 0;
    unsigned dynamicModel = 0;
    unsigned surveyDuration = 0;
    unsigned surveyAccuracy = 0;
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

    bool _handleFrame(const QByteArray& frame);
    QHash<quint32, quint64> _valsetValues(const QByteArray& payload);
    bool _replyToSetting(uint8_t messageId, DisableReply reply, bool disable = false);
    bool _replyToReadback(uint8_t messageId, QByteArray payload, quint32 key);
    void _queueAck(uint8_t messageId, bool accepted, bool disableAck = false);
    static QByteArray _frame(uint8_t messageClass, uint8_t messageId, const QByteArray& payload);

    std::atomic_bool& _stopRequested;
    bool _modern = false;
    bool _supportsTimeMode = true;
    bool _readError = false;
    QByteArray _version;
    QByteArray _outgoing;
    QList<Response> _incoming;
    bool _delayNextValsetAck = false;
    std::optional<Response> _heldValsetAck;
};
