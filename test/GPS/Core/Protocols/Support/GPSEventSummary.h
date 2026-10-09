#pragma once

// One line of text per decoded event and command evidence, for suites that compare runtime output as text.

#include <cmath>
#include <optional>
#include <variant>

#include <QtCore/QString>
#include <QtCore/QStringList>

#include "GPSCommand.h"
#include "GPSProtocolEvent.h"

namespace GPSTest {

inline QString summaryNumber(double value)
{
    return std::isnan(value) ? QStringLiteral("nan") : QString::number(value, 'g', 17);
}

template <typename T>
QString summaryOptional(const std::optional<T>& value)
{
    return value ? QString::number(*value) : QStringLiteral("-");
}

inline QString summary(const GPSDecodedPosition& report)
{
    const auto& n = report.navigation;
    return QStringLiteral(
               "position t=%1 utc=%2 fix=%3 lat=%4 lon=%5 msl=%6 ell=%7 hacc=%8 vacc=%9 hdop=%10 vdop=%11 "
               "speed=%12 course=%13 used=%14 velocity=%15")
        .arg(n.timestampUs)
        .arg(n.utcTimeUs)
        .arg(static_cast<int>(n.fixType))
        .arg(summaryNumber(n.latitudeDegrees), summaryNumber(n.longitudeDegrees), summaryNumber(n.altitudeMslMeters),
             summaryNumber(n.altitudeEllipsoidMeters), summaryNumber(n.horizontalAccuracyMeters),
             summaryNumber(n.verticalAccuracyMeters), summaryNumber(n.horizontalDop), summaryNumber(n.verticalDop),
             summaryNumber(n.speedMetersPerSecond))
        .arg(summaryNumber(n.courseRadians), summaryOptional(n.satellitesUsed))
        .arg(int(report.velocityValid));
}

inline QString summary(const GPSIntegrityReport& report)
{
    return QStringLiteral("integrity t=%1 jam=%2/%3 spoof=%4/%5 antenna=%6/%7 overflow=%8")
        .arg(report.timestampUs)
        .arg(report.jamming.timestampUs)
        .arg(static_cast<int>(report.jamming.state))
        .arg(report.spoofing.timestampUs)
        .arg(static_cast<int>(report.spoofing.state))
        .arg(report.antenna.timestampUs)
        .arg(static_cast<int>(report.antenna.state))
        .arg(report.outputOverflowUs);
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
                      .arg(summaryOptional(system.inUse));
    }
    return result;
}

inline QString summary(const GPSDecodedSatelliteUsage& report)
{
    return QStringLiteral("usage t=%1 used=%2").arg(report.timestampUs).arg(summaryOptional(report.usedCount));
}

inline QString summary(const GPSSurveyReport& survey)
{
    return QStringLiteral("survey t=%1 lat=%2 lon=%3 alt=%4 acc=%5 duration=%6 valid=%7 active=%8")
        .arg(survey.timestampUs)
        .arg(summaryNumber(survey.position.latitudeDegrees), summaryNumber(survey.position.longitudeDegrees),
             summaryNumber(survey.position.altitudeMeters),
             survey.meanAccuracyMeters ? summaryNumber(*survey.meanAccuracyMeters) : QStringLiteral("-"))
        .arg(survey.duration.count())
        .arg(int(survey.valid))
        .arg(int(survey.active));
}

inline QString summary(const GPSRTCMFrame& frame)
{
    return QStringLiteral("rtcm %1").arg(QString::fromLatin1(frame.bytes.toHex()));
}

inline QString summary(const GPSInputProtocol& input)
{
    return QStringLiteral("input %1").arg(static_cast<int>(input.family));
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
    return QStringLiteral("%1 outcome=%2 started=%3 accepted=%4 written=%5 required=%6")
        .arg(QString::fromUtf8(evidence.command))
        .arg(static_cast<int>(evidence.outcome))
        .arg(evidence.startedAtUs)
        .arg(evidence.acceptedBytes)
        .arg(evidence.writtenBytes)
        .arg(int(evidence.required));
}

}  // namespace GPSTest
