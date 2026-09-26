#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>

#include "GPSBaseStationConfig.h"
#include "GPSDecodedReports.h"
#include "GPSReceiveUpdates.h"
#include "GPSSurveyClock.h"

class GPSDecodeContext;
struct GPSFrame;

namespace SBF {

struct PVTGeodetic;

/// Decodes SBF block candidates and RTCM3 frames into events. Base stations output only PVTGeodetic, which carries
/// both the navigation solution and the survey-in status. Positions are published per receiver epoch once no block of
/// that epoch arrived for EPOCH_MAX_AGE.
class Decoder
{
public:
    explicit Decoder(bool satelliteInfoEnabled)
        : _satelliteInfoEnabled(satelliteInfoEnabled)
    {}

    /// Stops reporting the base and forgets survey progress, as a new configuration starts.
    void stopBase();

    /// Forgets pending and published epochs, so a new configuration accepts any receiver time.
    void resetEpochs();

    /// Starts reporting survey status for @a base; a survey-in is timed from @a nowUs.
    void startBase(const GPSBaseStationConfig& base, uint64_t nowUs);

    /// Whether the receiver is configured as a base. A position datum other than WGS84 ends it.
    [[nodiscard]] bool baseRunning() const { return _baseRunning; }

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context);

    /// Delivers queued RTCM3 frames and publishes epochs that expired.
    void flush(GPSDecodeContext& context);

private:
    struct NavigationEpoch
    {
        uint64_t receiverTimeMs = 0;
        uint64_t receiptUs = 0;
        GPSDecodedPosition position;
        bool hasPosition = false;
    };

    static constexpr std::chrono::microseconds EPOCH_MAX_AGE{200000};
    static constexpr uint64_t WEEK_MS = 604800000;

    GPSReceiveUpdates _decodeBlock(const GPSFrame& frame, GPSDecodeContext& context);
    NavigationEpoch* _navigationEpoch(uint64_t receiverTimeMs, GPSDecodeContext& context);
    void _finishEpoch(std::optional<NavigationEpoch>& epoch, GPSDecodeContext& context);
    /// Fills @a position from one PVTGeodetic block. @return whether its coordinates are usable.
    bool _applyPVTGeodetic(const PVTGeodetic& pvt, GPSDecodedPosition& position, GPSDecodeContext& context) const;
    void _publishSurveyStatus(const PVTGeodetic& pvt, const GPSDecodedPosition& position, bool coordinatesValid,
                              GPSDecodeContext& context);

    bool _satelliteInfoEnabled = true;
    bool _baseRunning = false;
    GPSBaseStationConfig _base;
    GPSSurveyClock _surveyClock;
    bool _surveyActive = false;
    std::array<std::optional<NavigationEpoch>, 2> _epochs;
    std::optional<uint64_t> _lastPublishedEpoch;
};

}  // namespace SBF
