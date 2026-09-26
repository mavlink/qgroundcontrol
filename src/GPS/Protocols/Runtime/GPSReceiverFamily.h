#pragma once

#include <cstdint>
#include <memory>
#include <span>

#include <QtCore/QLatin1StringView>

#include "GPSBaseStationConfig.h"
#include "GPSStreamDemux.h"
#include "GPSTask.h"
#include "GPSType.h"

class GPSCommandChannel;
class GPSFamilyProtocol;
class QLoggingCategory;

/// A family's logging category function, as declared by Q_DECLARE_LOGGING_CATEGORY; usable as qCWarning(category).
using GPSLogCategory = const QLoggingCategory& (*)();

/// Recognises one frame, framed with every framer enabled, as traffic only this family's receivers send.
/// @return what identified it, such as "UBX frames", or an empty view.
using GPSSignatureFunction = QLatin1StringView (*)(const GPSFrame& frame);

/// Sends the family's read-only identity query at the link's current rate and waits for its answer. It never changes
/// receiver settings. @a protocol is the family's own, created for the probe. @return whether a receiver of this
/// family answered, even one the family cannot configure.
using GPSProbeFunction = GPSTask<bool> (*)(GPSCommandChannel& channel, GPSFamilyProtocol& protocol);

/// The receiver configuration requested for one configure() attempt.
struct GPSConfig
{
    GPSBaseStationConfig base{};
    bool allowPersistentChanges = false;
    /// A rate receiver detection just found the receiver at. When configure() gets no rate, a baud search
    /// (GPSCommandChannel::detectBaud()) tries this rate first, even one the family does not list.
    unsigned detectedBaud = 0;
};

/// Optional base-station requests a family implements beyond survey-in and fixed positions.
struct GPSConfigurationSupport
{
    bool receiverAveraging = false;
    bool persistentChanges = false;
    bool compactObservations = false;
};

struct GPSFamilyOptions
{
    bool satelliteInfoEnabled = true;
};

/// Static description of one receiver family. Each family defines one descriptor, which the family table
/// (Common/GPSReceiverFamilies.cc) lists for type lookup.
struct GPSReceiverFamily
{
    GPSType type = GPSType::passive;
    QLatin1StringView name{};
    GPSLogCategory logCategory = nullptr;
    GPSStreamConfig stream{};
    GPSConfigurationSupport support{};
    /// Baud rates the family probes, in probing order; empty when it needs an explicit rate.
    std::span<const unsigned> baudCandidates{};
    /// Rate passed to configure() when none is selected; zero requests detection.
    unsigned autoBaudRate = 0;
    std::unique_ptr<GPSFamilyProtocol> (*create)(const GPSFamilyOptions& options) = nullptr;
    /// Receiver detection (GPSReceiverDetector) recognises the family's traffic with this, and its mismatch hints name
    /// the family; null when its traffic has no signature.
    GPSSignatureFunction signature = nullptr;
    /// Receiver detection sends this identity query; a family without one is never detected.
    GPSProbeFunction probe = nullptr;
};

/// The `create` of a family whose protocol is constructed from the options: `.create = &gpsCreateProtocol<Protocol>`.
template <typename Protocol>
std::unique_ptr<GPSFamilyProtocol> gpsCreateProtocol(const GPSFamilyOptions& options)
{
    return std::make_unique<Protocol>(options);
}
