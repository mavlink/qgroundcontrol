#pragma once

#include <optional>

#include <QtCore/QByteArrayView>

#include "GPSReceiverConfig.h"

/// Where a base station says its antenna is: the antenna reference point of RTCM message 1005 or 1006.
struct RTCMStationPosition
{
    GPSEllipsoidPosition position{};
    /// Height of the antenna reference point above the survey marker, which only message 1006 carries.
    std::optional<double> antennaHeightMeters;

    /// Decodes one whole RTCM frame. Empty unless it is a valid 1005 or 1006 frame of the standard's length whose
    /// coordinates lie near the Earth's surface, which the zero coordinates of an unset base do not.
    [[nodiscard]] static std::optional<RTCMStationPosition> fromFrame(QByteArrayView frame);
};
