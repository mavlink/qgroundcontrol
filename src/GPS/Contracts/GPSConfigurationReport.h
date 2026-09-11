#pragma once

#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include <optional>

#include "GPSCommandTransaction.h"
#include "GPSReceiverSetting.h"
#include "GPSTransportResult.h"

enum class GPSConfigurationStatus
{
    NotConfigured,
    Ready,
    Unsupported,
    Cancelled,
    TransportError,
    Failed,
};

struct GPSConfigurationResult
{
    GPSConfigurationStatus status = GPSConfigurationStatus::NotConfigured;
    QString error;
    std::optional<GPSReadResult> transportRead = {};
    std::optional<GPSWriteResult> transportWrite = {};
};

Q_DECLARE_METATYPE(GPSConfigurationResult)

/// Command acceptance and independently queried receiver state are separate evidence.
struct GPSSettingReport
{
    enum class RequestState
    {
        Requested,
        Acknowledged,
        Rejected,
        TimedOut,
        Cancelled,
        TransportError
    };
    enum class ReadbackState
    {
        Unverifiable,
        Reported
    };

    GPSReceiverSetting id = GPSReceiverSetting::Unknown;
    QString label;
    QString units;
    QVariant requestedValue;
    RequestState requestState = RequestState::Requested;
    ReadbackState readbackState = ReadbackState::Unverifiable;
    QVariant reportedValue;
    bool comparisonApplicable = false;
    bool matchesRequested = false;
    QString detail;
};
Q_DECLARE_METATYPE(GPSSettingReport)

struct GPSConfigurationReport
{
    QList<GPSSettingReport> settings;
    QList<GPSCommandResult> commands;
    quint64 sessionId = 0;
    qint64 monotonicTimestampUs = 0;
    bool active = false;
};
Q_DECLARE_METATYPE(GPSConfigurationReport)
