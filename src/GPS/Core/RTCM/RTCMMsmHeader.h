#pragma once

#include <bit>
#include <cstdint>
#include <optional>

#include <QtCore/QByteArrayView>

#include "GPSReceiverReports.h"

/// The header of an RTCM Multiple Signal Message (MSM1 to MSM7, messages 1071 to 1137): the observations of one
/// constellation at one epoch, of which the header names the satellites and the signals.
struct RTCMMsmHeader
{
    GPSConstellation constellation = GPSConstellation::Unknown;
    /// 1 to 7: the observables the message has of each signal.
    uint8_t msmType = 0;
    uint16_t stationId = 0;
    /// The epoch as the constellation counts it, which the messages of one epoch share.
    uint32_t epochTime = 0;
    /// Whether another MSM message of this epoch and station follows.
    bool moreMessages = false;
    /// One bit for each satellite the message has observations of; the most significant bit is satellite 1.
    uint64_t satelliteMask = 0;
    /// One bit for each signal observed of any of those satellites; the most significant bit is signal 1.
    uint32_t signalMask = 0;

    int satelliteCount() const { return std::popcount(satelliteMask); }

    /// Decodes one whole RTCM frame. Empty unless it is a valid frame of an MSM message that holds the header and the
    /// cell mask after it, which has at most MAX_CELLS cells.
    [[nodiscard]] static std::optional<RTCMMsmHeader> fromFrame(QByteArrayView frame);

    /// The standard lets a message have this many cells, one for each of its satellites and each of its signals.
    static constexpr int MAX_CELLS = 64;
};
