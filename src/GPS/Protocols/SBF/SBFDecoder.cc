#include "SBF/SBFDecoder.h"

#include <cmath>
#include <numbers>
#include <span>
#include <utility>
#include <variant>

#include <QtCore/QString>

#include "Checksums.h"
#include "GPSEventSink.h"
#include "GPSFamilyProtocol.h"
#include "GPSFrame.h"
#include "GPSProtocolMath.h"
#include "GPSStreamDemux.h"
#include "SBF/Generated/SBFBlocks.h"
#include "SBF/SBFFamily.h"
#include "WireFields.h"

namespace {

constexpr double DNU = 100000.0;
/// The block CRC covers everything from the id field to the end of the block.
constexpr size_t CRC_START = 4;

GPSPositionReport::FixType fixType(uint8_t modeType)
{
    switch (modeType) {
        case 0:
            return GPSPositionReport::FixType::NoFix;
        case 2:
        case 6:
            return GPSPositionReport::FixType::Differential;
        case 5:
        case 8:
            return GPSPositionReport::FixType::RTKFloat;
        case 4:
        case 7:
            return GPSPositionReport::FixType::RTKFixed;
        default:
            return GPSPositionReport::FixType::Fix3D;
    }
}

}  // namespace

namespace SBF {

void Decoder::stopBase()
{
    _baseRunning = false;
    _surveyClock.reset();
    _surveyActive = false;
}

void Decoder::resetEpochs()
{
    _epochs = {};
    _lastPublishedEpoch.reset();
}

void Decoder::startBase(const GPSBaseStationConfig& base, uint64_t nowUs)
{
    _base = base;
    if (!std::holds_alternative<GPSBaseStationConfig::Fixed>(_base.mode)) {
        _surveyClock.start(nowUs);
    }
    _baseRunning = true;
}

GPSReceiveUpdates Decoder::onFrame(const GPSFrame& frame, GPSDecodeContext& context)
{
    if (frame.kind == GPSFrameKind::RTCM3) {
        context.sink().publishRTCM(frame.bytes);
        return {};
    }
    return _decodeBlock(frame, context);
}

GPSReceiveUpdates Decoder::_decodeBlock(const GPSFrame& frame, GPSDecodeContext& context)
{
    const auto header = Wire::decode<BlockHeader>(frame.bytes);
    if (header.length < Wire::SIZE<BlockHeader> || header.length > frame.bytes.size() ||
        header.crc != QGC::crc16Ccitt(frame.bytes.subspan(CRC_START, header.length - CRC_START))) {
        return {};
    }
    if (header.number() != BlockId::PVT_GEODETIC || header.length < Wire::SIZE<PVTGeodetic>) {
        return {};
    }
    const auto pvt = Wire::decode<PVTGeodetic>(frame.bytes);
    if (pvt.tow >= WEEK_MS || pvt.wnc == UINT16_MAX) {
        return {};
    }

    auto* epoch = _navigationEpoch(uint64_t(pvt.wnc) * WEEK_MS + pvt.tow, context);
    if (!epoch) {
        return GPSReceiveUpdate::Activity;
    }
    epoch->hasPosition = true;

    // PVTGeodetic Datum 0 is WGS84/ITRS. Datum 19 is the correction provider's unspecified datum.
    if (pvt.datum != 0) {
        epoch->position = {};
        epoch->position.navigation.fixType = GPSPositionReport::FixType::NoFix;
        if (_baseRunning) {
            qCWarning(SBFProtocolLog) << "Unsupported Septentrio position datum:" << unsigned(pvt.datum);
            context.failControl();
            context.setErrorDetail(QStringLiteral("Septentrio position datum is not WGS84/ITRS"));
            _baseRunning = false;
            context.stream().setEnabled(GPSFrameKind::RTCM3, false);
            context.sink().publishSurvey(false, false, {});
        }
        return GPSReceiveUpdate::Activity;
    }

    const bool coordinatesValid = _applyPVTGeodetic(pvt, epoch->position, context);
    if (_baseRunning) {
        _publishSurveyStatus(pvt, epoch->position, coordinatesValid, context);
    }
    return GPSReceiveUpdate::Activity;
}

bool Decoder::_applyPVTGeodetic(const PVTGeodetic& pvt, GPSDecodedPosition& position, GPSDecodeContext& context) const
{
    position.navigation.fixType = fixType(pvt.modeType());
    if (pvt.error != 0) {
        position.navigation.fixType = GPSPositionReport::FixType::NoFix;
    } else if (pvt.mode2D() && position.navigation.fixType >= GPSPositionReport::FixType::Fix3D) {
        position.navigation.fixType = GPSPositionReport::FixType::Fix2D;
    }

    // Any value beyond the specified maximum is invalid, not only the do-not-use value (-2*10^10).
    position.velocityValid = position.navigation.fixType > GPSPositionReport::FixType::NoFix && pvt.error == 0 &&
                             !(fabsf(pvt.vn) > 600.0f || fabsf(pvt.ve) > 600.0f || fabsf(pvt.vu) > 600.0f);

    const bool coordinatesValid = std::isfinite(pvt.latitude) && std::abs(pvt.latitude) <= std::numbers::pi / 2 &&
                                  std::isfinite(pvt.longitude) && std::abs(pvt.longitude) <= std::numbers::pi &&
                                  std::isfinite(pvt.height) && std::abs(pvt.height) <= DNU;
    if (!coordinatesValid || !std::isfinite(pvt.undulation) || std::abs(pvt.undulation) > DNU) {
        position.navigation.fixType = GPSPositionReport::FixType::NoFix;
    }

    const auto satellitesUsed = gpsSatellitesUsed(pvt.nrSV);  // 255 = do not use value
    position.navigation.satellitesUsed = satellitesUsed.value_or(UINT8_MAX);
    if (_satelliteInfoEnabled) {
        context.sink().publishSatelliteUsage(satellitesUsed);
    }

    position.navigation.latitudeDegrees = pvt.latitude * GPSProtocolMath::RAD_TO_DEG;
    position.navigation.longitudeDegrees = pvt.longitude * GPSProtocolMath::RAD_TO_DEG;
    position.navigation.altitudeEllipsoidMeters = pvt.height;
    position.navigation.altitudeMslMeters = pvt.height - static_cast<double>(pvt.undulation);

    // Accuracy is reported as 2DRMS in cm; halve it for the RMS convention used by the other drivers.
    position.navigation.horizontalAccuracyMeters =
        pvt.hAccuracy != UINT16_MAX ? static_cast<float>(pvt.hAccuracy) / 200.0f : NAN;
    position.navigation.verticalAccuracyMeters =
        pvt.vAccuracy != UINT16_MAX ? static_cast<float>(pvt.vAccuracy) / 200.0f : NAN;

    position.navigation.speedMetersPerSecond = sqrtf(pvt.vn * pvt.vn + pvt.ve * pvt.ve);
    position.navigation.courseRadians =
        std::isfinite(pvt.cog) && pvt.cog >= 0.0f && pvt.cog <= 360.0f ? pvt.cog * GPSProtocolMath::DEG_TO_RAD : NAN;

    // WNc/TOW is GNSS system time, not UTC. Without receiver UTC/leap information,
    // retain the epoch key internally and let the facade use reception UTC.
    position.navigation.utcTimeUs = 0;
    position.navigation.timestampUs = context.nowUs();
    return coordinatesValid;
}

void Decoder::_publishSurveyStatus(const PVTGeodetic& pvt, const GPSDecodedPosition& position, bool coordinatesValid,
                                   GPSDecodeContext& context)
{
    // Mode bit 6 means automatic base determination is still in progress, not completed.
    // Septentrio PolaRx5TR 5.5.0 Reference Guide, SBF Mode definition (p. 382).
    const bool active = !std::holds_alternative<GPSBaseStationConfig::Fixed>(_base.mode) && pvt.modeAutoSet();
    const bool valid = !pvt.modeAutoSet() && pvt.modeType() == 3 && !pvt.mode2D() && !pvt.error && coordinatesValid;
    // The final update on the active-to-inactive transition freezes the survey duration.
    if (active || _surveyActive) {
        (void) _surveyClock.update(context.nowUs());
    }
    _surveyActive = active;
    // PVT accuracy describes the navigation solution, not the averaged base survey.
    GPSEllipsoidPosition surveyPosition;
    if (coordinatesValid && !pvt.error) {
        surveyPosition = {.latitudeDegrees = position.navigation.latitudeDegrees,
                          .longitudeDegrees = position.navigation.longitudeDegrees,
                          .altitudeMeters = static_cast<float>(position.navigation.altitudeEllipsoidMeters)};
    }
    context.sink().publishSurvey(active, valid, _surveyClock.duration(), surveyPosition);
}

Decoder::NavigationEpoch* Decoder::_navigationEpoch(uint64_t receiverTimeMs, GPSDecodeContext& context)
{
    flush(context);
    if (_lastPublishedEpoch && receiverTimeMs <= *_lastPublishedEpoch) {
        return nullptr;
    }
    for (auto& epoch : _epochs) {
        if (epoch && epoch->receiverTimeMs == receiverTimeMs) {
            return &*epoch;
        }
    }
    auto slot = _epochs.begin();
    while (slot != _epochs.end() && *slot) {
        ++slot;
    }
    if (slot == _epochs.end()) {
        slot = _epochs[0]->receiverTimeMs < _epochs[1]->receiverTimeMs ? _epochs.begin() : _epochs.begin() + 1;
        if (receiverTimeMs <= (*slot)->receiverTimeMs) {
            return nullptr;
        }
        _finishEpoch(*slot, context);
    }
    *slot = NavigationEpoch{.receiverTimeMs = receiverTimeMs, .receiptUs = context.nowUs(), .position = {}};
    return &**slot;
}

void Decoder::_finishEpoch(std::optional<NavigationEpoch>& epoch, GPSDecodeContext& context)
{
    if (epoch->hasPosition && (!_lastPublishedEpoch || epoch->receiverTimeMs > *_lastPublishedEpoch)) {
        epoch->position.navigation.timestampUs = epoch->receiptUs;
        context.sink().publishPosition(epoch->position);
    }
    if (!_lastPublishedEpoch || epoch->receiverTimeMs > *_lastPublishedEpoch) {
        _lastPublishedEpoch = epoch->receiverTimeMs;
    }
    epoch.reset();
}

void Decoder::flush(GPSDecodeContext& context)
{
    context.drainDeferredFrames();
    if (_epochs[0] && _epochs[1] && _epochs[0]->receiverTimeMs > _epochs[1]->receiverTimeMs) {
        std::swap(_epochs[0], _epochs[1]);
    }
    const auto now = context.nowUs();
    for (auto& epoch : _epochs) {
        if (epoch && now >= epoch->receiptUs && std::chrono::microseconds(now - epoch->receiptUs) >= EPOCH_MAX_AGE) {
            _finishEpoch(epoch, context);
        }
    }
}

}  // namespace SBF
