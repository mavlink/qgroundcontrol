#pragma once

#include <cmath>
#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <variant>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "../Support/GPSRuntimeTestIO.h"
#include "../Support/GPSTestClock.h"
#include "../Support/ScriptedReceiver.h"
#include "GPSConfigurationEvidence.h"
#include "GPSDecodedReports.h"
#include "GPSProtocolEvent.h"
#include "GPSReceiverFamily.h"
#include "GPSRuntimeIO.h"

namespace GPSRuntimeTest {

inline QString number(double value)
{
    return std::isnan(value) ? QStringLiteral("nan") : QString::number(value, 'g', 17);
}

template <typename T>
QString optional(const std::optional<T>& value)
{
    return value ? QString::number(*value) : QStringLiteral("-");
}

inline QString summary(const GPSDecodedPosition& report)
{
    const auto& n = report.navigation;
    return QStringLiteral(
               "position t=%1 utc=%2 fix=%3 lat=%4 lon=%5 msl=%6 ell=%7 hacc=%8 vacc=%9 hdop=%10 vdop=%11 "
               "speed=%12 course=%13 heading=%14 headingAcc=%15 used=%16 velocity=%17")
        .arg(n.timestampUs)
        .arg(n.utcTimeUs)
        .arg(static_cast<int>(n.fixType))
        .arg(number(n.latitudeDegrees), number(n.longitudeDegrees), number(n.altitudeMslMeters),
             number(n.altitudeEllipsoidMeters), number(n.horizontalAccuracyMeters), number(n.verticalAccuracyMeters),
             number(n.horizontalDop), number(n.verticalDop), number(n.speedMetersPerSecond))
        .arg(number(n.courseRadians), number(n.headingRadians), number(n.headingAccuracyRadians),
             optional(n.satellitesUsed))
        .arg(int(report.velocityValid));
}

inline QString summary(const GPSIntegrityReport& report)
{
    return QStringLiteral("integrity t=%1 jam=%2/%3 spoof=%4/%5 rf=%6/%7/%8/%9 corr=%10/%11/%12/%13")
        .arg(report.timestampUs)
        .arg(report.jamming.timestampUs)
        .arg(static_cast<int>(report.jamming.state))
        .arg(report.spoofing.timestampUs)
        .arg(static_cast<int>(report.spoofing.state))
        .arg(report.rf.timestampUs)
        .arg(optional(report.rf.noisePerMillisecond), optional(report.rf.automaticGainControl),
             optional(report.rf.jammingIndicator))
        .arg(report.corrections.timestampUs)
        .arg(static_cast<int>(report.corrections.use))
        .arg(report.corrections.crcFailed ? QString::number(int(*report.corrections.crcFailed)) : QStringLiteral("-"))
        .arg(static_cast<int>(report.corrections.protocol));
}

inline QString summary(const GPSDecodedSatellites& report)
{
    QString result = QStringLiteral("satellites full=%1 count=%2").arg(int(report.fullSnapshot)).arg(int(report.count));
    for (uint8_t index = 0; index < report.count; ++index) {
        const auto& system = report.constellations[index];
        result += QStringLiteral(" [%1 %2/%3 %4/%5]")
                      .arg(static_cast<int>(system.constellation))
                      .arg(system.inViewTimestampUs)
                      .arg(system.inView)
                      .arg(system.inUseTimestampUs)
                      .arg(optional(system.inUse));
    }
    return result;
}

inline QString summary(const GPSDecodedSatelliteUsage& report)
{
    return QStringLiteral("usage t=%1 used=%2").arg(report.timestampUs).arg(optional(report.usedCount));
}

inline QString summary(const GPSDecodedSurvey& report)
{
    const auto& survey = report.survey;
    return QStringLiteral("survey t=%1 lat=%2 lon=%3 alt=%4 acc=%5 duration=%6 valid=%7 active=%8")
        .arg(report.timestamp)
        .arg(number(survey.position.latitudeDegrees), number(survey.position.longitudeDegrees),
             number(survey.position.altitudeMeters),
             survey.meanAccuracyMeters ? number(*survey.meanAccuracyMeters) : QStringLiteral("-"))
        .arg(survey.duration.count())
        .arg(int(survey.valid))
        .arg(int(survey.active));
}

inline QString summary(const GPSRTCMFrame& frame)
{
    return QStringLiteral("rtcm %1").arg(QString::fromLatin1(frame.bytes.toHex()));
}

/// One line per event, with the batch's update flags first.
inline QStringList summary(const GPSEventBatch& batch)
{
    QStringList lines{QStringLiteral("batch updates=%1").arg(batch.updates.toInt())};
    for (const auto& event : batch.events) {
        lines.append(std::visit([](const auto& report) { return summary(report); }, event));
    }
    return lines;
}

inline QString summary(const GPSConfigurationEvidence& evidence)
{
    return QStringLiteral("%1 outcome=%2 %3-%4 accepted=%5 written=%6 uncertain=%7 required=%8")
        .arg(QString::fromStdString(evidence.command))
        .arg(static_cast<int>(evidence.outcome))
        .arg(evidence.startedAtUs)
        .arg(evidence.finishedAtUs)
        .arg(evidence.acceptedBytes)
        .arg(evidence.writtenBytes)
        .arg(evidence.uncertainBytes)
        .arg(int(evidence.required));
}

inline QString summary(const GPSCommandResult& result)
{
    return summary(result.evidence) +
           QStringLiteral(" rate=%1").arg(int(result.affectedSettings.contains(GPSReceiverSetting::OutputRateHz)));
}

/// A receiver on a virtual clock. Each command write schedules its scripted replies after their latency, unsolicited
/// output arrives at scheduled times, and time only passes while a read waits for data.
class LatencyModel : public ScriptedReceiver::Model
{
public:
    struct Reply
    {
        QByteArray bytes;
        uint64_t latencyUs = 1000;
    };

    explicit LatencyModel(GPSTestClock& clock)
        : _clock(clock)
    {}

    /// Replies to successive writes of @a command; an empty reply is silence.
    void script(const QByteArray& command, QList<Reply> replies) { _script[command] = std::move(replies); }

    /// Unsolicited output delivered at @a atUs.
    void emitAt(uint64_t atUs, const QByteArray& bytes) { _schedule(atUs, bytes); }

    /// Rates at which replies are delivered; empty accepts every rate.
    QList<unsigned> answeringRates;
    /// Rates the link cannot be set to.
    QList<unsigned> unsupportedRates;
    /// Commands whose write fails with a transport error.
    QList<QByteArray> failingWrites;
    /// Commands after which the next idle read fails.
    QList<QByteArray> failingReadsAfter;
    int readChunk = 0;
    /// Commands are complete UBX frames, however many writes they take; otherwise each write is a command.
    bool ubxCommands = false;
    unsigned baud = 0;
    QList<unsigned> bauds;
    int reads = 0;

private:
    std::optional<QByteArray> takeCommand(QByteArray& pending) override
    {
        if (!ubxCommands) {
            return ScriptedReceiver::Model::takeCommand(pending);
        }
        if (pending.size() < 6) {
            return std::nullopt;
        }
        const qsizetype size = 8 + (static_cast<uint8_t>(pending[4]) | (static_cast<uint8_t>(pending[5]) << 8));
        if (pending.size() < size) {
            return std::nullopt;
        }
        QByteArray command = pending.left(size);
        pending.remove(0, size);
        return command;
    }

    GPSWriteResult handleCommand(ScriptedReceiver& receiver, const QByteArray& command,
                                 const ScriptedReceiver::WriteContext& context) override
    {
        Q_UNUSED(context)
        if (failingWrites.contains(command)) {
            return {GPSWriteStatus::Error, 0, 0, QStringLiteral("scripted write failure")};
        }
        if (failingReadsAfter.contains(command)) {
            receiver.failNextRead({GPSReadStatus::Error, 0, QStringLiteral("scripted read failure")});
        }
        auto found = _script.find(command);
        if (found != _script.end() && !found->second.isEmpty() &&
            (answeringRates.isEmpty() || answeringRates.contains(baud))) {
            const Reply reply = found->second.takeFirst();
            if (!reply.bytes.isEmpty()) {
                _schedule(_clock.nowUs() + reply.latencyUs, reply.bytes);
            }
        }
        return {GPSWriteStatus::Completed, static_cast<int>(command.size()), static_cast<int>(command.size())};
    }

    std::optional<bool> handleBaudrate(ScriptedReceiver& receiver, unsigned baudrate) override
    {
        Q_UNUSED(receiver)
        bauds.append(baudrate);
        if (unsupportedRates.contains(baudrate)) {
            return false;
        }
        baud = baudrate;
        return true;
    }

    void onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline) override
    {
        ++reads;
        if (!_pending.empty() && _pending.begin()->first <= deadline.untilUs) {
            const auto next = _pending.begin();
            _clock.advanceTo(next->first);
            receiver.queueReply(next->second);
            _pending.erase(next);
            return;
        }
        _clock.advanceTo(deadline.untilUs);
    }

    int readChunkSize(const ScriptedReceiver& receiver, int requested, int available) const override
    {
        Q_UNUSED(receiver)
        return readChunk > 0 ? std::min({requested, available, readChunk}) : std::min(requested, available);
    }

    void _schedule(uint64_t atUs, const QByteArray& bytes)
    {
        auto [slot, inserted] = _pending.try_emplace(atUs, bytes);
        if (!inserted) {
            slot->second += bytes;
        }
    }

    GPSTestClock& _clock;
    std::map<QByteArray, QList<Reply>> _script;
    std::map<uint64_t, QByteArray> _pending;
};

}  // namespace GPSRuntimeTest
