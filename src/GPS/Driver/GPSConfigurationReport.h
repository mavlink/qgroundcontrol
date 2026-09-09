#pragma once

#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include "GPSReceiverSetting.h"

/// Command acceptance and independently queried receiver state are separate evidence.
struct GPSSettingReport
{
    enum class RequestState
    {
        Requested,
        Acknowledged,
        Rejected
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
    quint64 sessionId = 0;
    qint64 monotonicTimestampUs = 0;
    bool active = false;
};
Q_DECLARE_METATYPE(GPSConfigurationReport)
