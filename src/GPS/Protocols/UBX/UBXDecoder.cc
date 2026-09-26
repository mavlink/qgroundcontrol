#include "UBX/UBXDecoder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <string_view>

#include <QtCore/QByteArrayView>
#include <QtCore/QString>

#include "GPSEventSink.h"
#include "GPSFamilyProtocol.h"
#include "GPSProtocolMath.h"
#include "GPSStreamDemux.h"
#include "NMEASentence.h"
#include "UBX/UBXFamily.h"
#include "UBXMessageSchema.h"

namespace Msg = UBX::Msg;

namespace {

/// Once part of an epoch has arrived, a read this long without data ends it.
constexpr std::chrono::milliseconds PACKET_TIMEOUT{8};
/// Assembled epochs expire by time, so reads return often enough to publish them.
constexpr std::chrono::milliseconds EPOCH_READ_SLICE{200};
/// Longest antenna baseline whose GNSS heading is used.
constexpr float HEADING_MAX_BASELINE_M = 10.f;

constexpr uint8_t PVT_VALID_DATE = 0x01;
constexpr uint8_t PVT_VALID_TIME = 0x02;
constexpr uint8_t PVT_FULLY_RESOLVED = 0x04;
constexpr uint8_t FIX_OK = 0x01;
constexpr uint8_t DIFFERENTIAL_SOLUTION = 0x02;
constexpr uint8_t TIMEUTC_VALID_UTC = 0x04;

GPSPositionReport::FixType navigationFix(uint8_t wireFix, uint8_t flags, bool hasCarrierFlags)
{
    using Fix = GPSPositionReport::FixType;
    if (!(flags & FIX_OK)) {
        return Fix::NoFix;
    }
    switch (wireFix) {
        case 0:
        case 5:
            return Fix::NoFix;
        case 1:
            return Fix::Extrapolated;
        case 2:
            // Correction flags cannot establish a three-dimensional navigation solution.
            return Fix::Fix2D;
        case 3:
        case 4:
            break;
        default:
            return Fix::Unknown;
    }
    if (hasCarrierFlags) {
        const uint8_t carrier = flags >> 6;
        if (carrier == 1) {
            return Fix::RTKFloat;
        }
        if (carrier == 2) {
            return Fix::RTKFixed;
        }
    }
    return flags & DIFFERENTIAL_SOLUTION ? Fix::Differential : Fix::Fix3D;
}

bool velocityValid(GPSPositionReport::FixType fix)
{
    return fix != GPSPositionReport::FixType::Unknown && fix != GPSPositionReport::FixType::NoFix;
}

/// Receiver time that is not valid publishes 0, the defined "unavailable" value, so a receiver that lost time never
/// keeps reporting the last known time.
template <typename Time>
uint64_t utcMicroseconds(const Time& time)
{
    tm fields{};
    fields.tm_year = time.year - 1900;
    fields.tm_mon = time.month - 1;
    fields.tm_mday = time.day;
    fields.tm_hour = time.hour;
    fields.tm_min = time.min;
    fields.tm_sec = time.sec;
    return GPSProtocolMath::utcMicroseconds(fields, time.nano);
}

/// Normalizes a relative position heading in 1e-5 deg to [-pi, pi].
float headingRadians(int32_t heading)
{
    float radians = heading * GPSProtocolMath::DEG_TO_RAD * 1e-5f;
    if (radians > GPSProtocolMath::PI) {
        radians -= 2.f * GPSProtocolMath::PI;
    } else if (radians < -GPSProtocolMath::PI) {
        radians += 2.f * GPSProtocolMath::PI;
    }
    return radians;
}

/// A field the protocol specifies as NUL-terminated; the device provides the terminator, so it is enforced at the
/// field's last byte.
template <size_t N>
QByteArrayView cString(const uint8_t (&field)[N])
{
    const QByteArrayView bytes(field, N - 1);
    const auto end = bytes.indexOf('\0');
    return end < 0 ? bytes : bytes.first(end);
}

/// Fixed-size satellite blocks follow the header; a truncated block ends the report at the preceding entry.
template <typename Header, typename Block, typename Count, typename Assign>
void decodeSatelliteBlocks(std::span<const uint8_t> payload, GPSDecodedSatellites& report, Count count, Assign assign)
{
    const auto header = UBX::MessageCodec<Header>::block(payload);
    if (!header) {
        return;
    }
    const auto satelliteCount = std::min<size_t>(count(*header), GPSDecodedSatellites::SAT_INFO_MAX_SATELLITES);
    if (satelliteCount == 0) {
        (void) report.ensureConstellation(GPSConstellation::Unknown);
    }
    for (size_t index = 0; index < satelliteCount; ++index) {
        const auto block =
            UBX::MessageCodec<Block>::block(payload, UBX::WIRE_SIZE<Header> + index * UBX::WIRE_SIZE<Block>);
        if (!block) {
            return;
        }
        assign(report, *block);
    }
}

QString portName(uint16_t portId)
{
    switch (portId) {
        case 0x0000:
            return QStringLiteral("I2C");
        case 0x0100:
            return QStringLiteral("UART1");
        case 0x0201:
            return QStringLiteral("UART2");
        case 0x0300:
            return QStringLiteral("USB");
        case 0x0400:
            return QStringLiteral("SPI");
        default:
            return QStringLiteral("unknown");
    }
}

}  // namespace

UBXDecoder::UBXDecoder(bool satelliteInfoEnabled)
    : _satelliteInfo(satelliteInfoEnabled)
{}

GPSReceiveUpdates UBXDecoder::onFrame(const GPSFrame& frame, GPSDecodeContext& context)
{
    if (frame.kind == GPSFrameKind::RTCM3) {
        context.sink().publishRTCM(frame.bytes);
        return {};
    }
    return _decode(static_cast<uint16_t>(frame.messageId), frame.payload, context);
}

void UBXDecoder::flush(GPSDecodeContext& context)
{
    context.drainDeferredFrames();
    if (_state.mode.assembleEpochs) {
        _epochs.expire(context.nowUs(), [this, &context](const auto& report) { _publishEpoch(report, context); });
    }
}

void UBXDecoder::setMode(const Mode& mode, GPSStreamDemux& stream)
{
    _state.mode = mode;
    _epochs = {};
    stream.setEnabled(GPSFrameKind::RTCM3, mode.corrections);
    stream.reset(GPSFrameKind::UBX);
}

bool UBXDecoder::completeReceiveCycle(GPSReceiveUpdates handled)
{
    const bool navigationComplete =
        _state.mode.assembleEpochs ? handled.testFlag(GPSReceiveUpdate::Position) : (_gotPosition && _gotVelocity);
    const bool ready = (_state.timeModeReadbackPending && _state.timeModeReadback.has_value()) ||
                       (_state.controller.readbackPending() && _state.controller.readbackReady()) ||
                       (_state.configured ? navigationComplete : handled != GPSReceiveUpdates{});
    if (ready) {
        _gotPosition = false;
        _gotVelocity = false;
    }
    return ready;
}

std::chrono::milliseconds UBXDecoder::nextReadSlice(std::chrono::milliseconds timeout) const
{
    if (_gotPosition || _gotVelocity) {
        return PACKET_TIMEOUT;
    }
    return _state.mode.assembleEpochs ? std::min(timeout, EPOCH_READ_SLICE) : timeout;
}

void UBXDecoder::_publishEpoch(const GPSDecodedPosition& report, GPSDecodeContext& context)
{
    _position = report;
    context.sink().publishPosition(report);
}

GPSReceiveUpdates UBXDecoder::_decode(uint16_t message, std::span<const uint8_t> payload, GPSDecodeContext& context)
{
    const auto* schema = UBX::messageSchema(message);
    if (!UBX::validPayload(message, payload, schema)) {
        return {};
    }
    const auto publish = [this, &context](const auto& report) { _publishEpoch(report, context); };
    if (message == Msg::NAV_EOE.value() && _state.mode.assembleEpochs) {
        const uint32_t tow = LittleEndian::read<uint32_t>(payload, 0).value_or(UBXNavigationEpoch::WEEK_MS);
        if (tow < UBXNavigationEpoch::WEEK_MS) {
            _epochs.end(tow, publish);
        }
        return GPSReceiveUpdate::Activity;
    }
    if (!_accept(message, payload)) {
        return {};
    }
    UBXNavigationEpoch::Epoch* epoch = nullptr;
    if (_state.mode.assembleEpochs && schema && schema->towOffset >= 0) {
        const auto tow = LittleEndian::read<uint32_t>(payload, static_cast<size_t>(schema->towOffset)).value_or(0);
        epoch = _epochs.find(tow, context.nowUs(), publish);
        if (!epoch) {
            return GPSReceiveUpdate::Activity;
        }
        _epochHasHighPrecision = epoch->highPrecision;
    }
    const auto updates = _decodeHandled(message, payload, epoch ? epoch->position : _position, context);
    if (epoch) {
        if (message == Msg::NAV_PVT.value()) {
            epoch->positionValid = epoch->velocityValid = true;
        } else if (message == Msg::NAV_POSLLH.value()) {
            epoch->positionValid = true;
        } else if (message == Msg::NAV_VELNED.value()) {
            epoch->velocityValid = true;
        } else if (message == Msg::NAV_HPPOSLLH.value() && updates.testFlag(GPSReceiveUpdate::Position)) {
            epoch->highPrecision = true;
        }
        return GPSReceiveUpdate::Activity;
    }
    if (updates.testFlag(GPSReceiveUpdate::Position)) {
        context.sink().publishPosition(_position);
    }
    if (updates.testFlag(GPSReceiveUpdate::Satellites)) {
        context.sink().publishSatellites(_satellites);
    }
    return updates;
}

bool UBXDecoder::_accept(uint16_t message, std::span<const uint8_t> payload)
{
    const auto& mode = _state.mode;
    const auto disable = [this, message] {
        _state.requests.disableMessage = message;
        return false;
    };
    switch (message) {
        case Msg::CFG_TMODE3.value():
            return _state.timeModeReadbackPending;
        case Msg::CFG_VALGET.value():
            return (_state.controller.readbackPending() || _state.controller.readbackReplyOutstanding()) &&
                   payload.size() >= 4 && payload.size() <= UBX::MAX_CONTROL_PAYLOAD_SIZE;
        case Msg::CFG_MSG.value():
            return _state.controller.ratePollPending();
        case Msg::MON_COMMS.value():
        case Msg::NAV_SVIN.value():
        case Msg::MON_VER.value():
            return true;
        case Msg::NAV_PVT.value():
            return mode.navigation && (mode.useNavPvt || disable());
        case Msg::INF_DEBUG.value():
        case Msg::INF_ERROR.value():
        case Msg::INF_NOTICE.value():
        case Msg::INF_WARNING.value():
            return payload.size() < UBX::MAX_CONTROL_PAYLOAD_SIZE;
        case Msg::NAV_POSLLH.value():
        case Msg::NAV_SOL.value():
        case Msg::NAV_TIMEUTC.value():
        case Msg::NAV_VELNED.value():
            return mode.navigation && (!mode.useNavPvt || disable());
        case Msg::NAV_STATUS.value():
        case Msg::NAV_DOP.value():
        case Msg::NAV_RELPOSNED.value():
        case Msg::NAV_DAHEADING.value():
        case Msg::NAV_HPPOSLLH.value():
        case Msg::MON_HW.value():
        case Msg::MON_RF.value():
        case Msg::SEC_SIG.value():
        case Msg::RXM_RTCM.value():
        case Msg::RXM_COR.value():
            return mode.navigation;
        case Msg::NAV_SAT.value():
        case Msg::NAV_SVINFO.value():
            return (_satelliteInfo || disable()) && mode.navigation;
        case Msg::ACK_ACK.value():
            return _state.controller.awaitingAcknowledgement() || _state.controller.ratePollPending();
        case Msg::ACK_NAK.value():
            return _state.controller.awaitingAcknowledgement() || _state.controller.readbackPending() ||
                   _state.controller.readbackReplyOutstanding() || _state.controller.ratePollPending() ||
                   _state.timeModeReadbackPending;
        default:
            return disable();
    }
}

GPSReceiveUpdates UBXDecoder::_decodeHandled(uint16_t message, std::span<const uint8_t> payload,
                                             GPSDecodedPosition& position, GPSDecodeContext& context)
{
    const uint64_t now = context.nowUs();
    switch (message) {
        case Msg::NAV_PVT.value():
            return _decodeNavPvt(payload, position, now);
        case Msg::NAV_POSLLH.value():
        case Msg::NAV_HPPOSLLH.value():
        case Msg::NAV_SOL.value():
        case Msg::NAV_DOP.value():
        case Msg::NAV_TIMEUTC.value():
        case Msg::NAV_VELNED.value():
            return _decodeNavigation(message, payload, position, now);
        case Msg::NAV_RELPOSNED.value():
        case Msg::NAV_DAHEADING.value():
            return _decodeHeading(message, payload, position);
        case Msg::NAV_SAT.value():
        case Msg::NAV_SVINFO.value():
            _decodeSatellites(message, payload);
            for (uint8_t index = 0; index < _satellites.count; ++index) {
                auto& system = _satellites.constellations[index];
                system.inViewTimestampUs = now;
                if (system.inUse) {
                    system.inUseTimestampUs = now;
                }
            }
            return GPSReceiveUpdate::Satellites;
        case Msg::NAV_SVIN.value():
            return _decodeSurveyIn(payload, context);
        case Msg::NAV_STATUS.value():
        case Msg::MON_HW.value():
        case Msg::MON_RF.value():
        case Msg::SEC_SIG.value():
        case Msg::RXM_RTCM.value():
        case Msg::RXM_COR.value():
            if (!_decodeIntegrity(message, payload, now)) {
                return {};
            }
            context.sink().publishIntegrity(_integrity);
            return GPSReceiveUpdate::Activity;
        case Msg::MON_VER.value():
            _decodeMonVer(payload);
            // Polled only while configuring, which waits for it as an acknowledgement.
            _state.controller.accept(UBX::Acknowledgement{message, true});
            return GPSReceiveUpdate::Activity;
        default:
            return _decodeControl(message, payload, now);
    }
}

GPSReceiveUpdates UBXDecoder::_decodeNavPvt(std::span<const uint8_t> payload, GPSDecodedPosition& position,
                                            uint64_t now)
{
    const auto pvt = UBX::MessageCodec<UBX::NavPvt>::decode(payload);
    if (!pvt) {
        return {};
    }
    auto& navigation = position.navigation;
    navigation.fixType = navigationFix(pvt->fixType, pvt->flags, true);
    position.velocityValid = velocityValid(navigation.fixType);
    navigation.satellitesUsed = pvt->numSV;
    // NAV-HPPOSLLH supersedes the position of an RTK fixed solution.
    if (_state.mode.assembleEpochs ? !_epochHasHighPrecision
                                   : navigation.fixType != GPSPositionReport::FixType::RTKFixed) {
        navigation.latitudeDegrees = UBX::latitudeDegrees(pvt->lat);
        navigation.longitudeDegrees = UBX::longitudeDegrees(pvt->lon);
        navigation.altitudeMslMeters = pvt->hMSL * 1e-3;
        navigation.altitudeEllipsoidMeters = pvt->height * 1e-3;
        navigation.horizontalAccuracyMeters = static_cast<float>(pvt->hAcc) * 1e-3f;
        navigation.verticalAccuracyMeters = static_cast<float>(pvt->vAcc) * 1e-3f;
        _gotPosition = true;
    }
    navigation.speedMetersPerSecond = static_cast<float>(pvt->gSpeed) * 1e-3f;
    navigation.courseRadians = static_cast<float>(pvt->headMot) * GPSProtocolMath::DEG_TO_RAD * 1e-5f;
    const bool timeValid =
        (pvt->valid & PVT_VALID_DATE) && (pvt->valid & PVT_VALID_TIME) && (pvt->valid & PVT_FULLY_RESOLVED);
    navigation.utcTimeUs = timeValid ? utcMicroseconds(*pvt) : 0;
    navigation.timestampUs = now;
    _gotVelocity = true;
    return GPSReceiveUpdate::Position;
}

GPSReceiveUpdates UBXDecoder::_decodeNavigation(uint16_t message, std::span<const uint8_t> payload,
                                                GPSDecodedPosition& position, uint64_t now)
{
    auto& navigation = position.navigation;
    switch (message) {
        case Msg::NAV_POSLLH.value(): {
            const auto posllh = UBX::MessageCodec<UBX::NavPosllh>::decode(payload);
            if (!posllh) {
                return {};
            }
            navigation.latitudeDegrees = UBX::latitudeDegrees(posllh->lat);
            navigation.longitudeDegrees = UBX::longitudeDegrees(posllh->lon);
            navigation.altitudeMslMeters = posllh->hMSL * 1e-3;
            navigation.altitudeEllipsoidMeters = posllh->height * 1e-3;
            navigation.horizontalAccuracyMeters = static_cast<float>(posllh->hAcc) * 1e-3f;
            navigation.verticalAccuracyMeters = static_cast<float>(posllh->vAcc) * 1e-3f;
            navigation.timestampUs = now;
            _gotPosition = true;
            return GPSReceiveUpdate::Position;
        }
        case Msg::NAV_HPPOSLLH.value(): {
            const auto hp = UBX::MessageCodec<UBX::NavHpposllh>::decode(payload);
            if (!hp || hp->flags != 0 ||
                (!_state.mode.assembleEpochs && navigation.fixType != GPSPositionReport::FixType::RTKFixed)) {
                return {};
            }
            navigation.latitudeDegrees = UBX::latitudeDegrees(hp->lat) + hp->latHp * 1e-9;
            navigation.longitudeDegrees = UBX::longitudeDegrees(hp->lon) + hp->lonHp * 1e-9;
            navigation.altitudeMslMeters = hp->hMSL * 1e-3 + hp->hMSLHp * 1e-4;
            navigation.altitudeEllipsoidMeters = hp->height * 1e-3 + hp->heightHp * 1e-4;
            navigation.horizontalAccuracyMeters = static_cast<float>(hp->hAcc) * 1e-4f;
            navigation.verticalAccuracyMeters = static_cast<float>(hp->vAcc) * 1e-4f;
            navigation.timestampUs = now;
            _gotPosition = true;
            return GPSReceiveUpdate::Position;
        }
        case Msg::NAV_SOL.value(): {
            const auto sol = UBX::MessageCodec<UBX::NavSol>::decode(payload);
            if (!sol) {
                return {};
            }
            navigation.fixType = navigationFix(sol->gpsFix, sol->flags, false);
            position.velocityValid = velocityValid(navigation.fixType);
            navigation.satellitesUsed = sol->numSV;
            return GPSReceiveUpdate::Activity;
        }
        case Msg::NAV_DOP.value(): {
            const auto dop = UBX::MessageCodec<UBX::NavDop>::decode(payload);
            if (!dop) {
                return {};
            }
            navigation.horizontalDop = dop->hDOP * UBX::DOP_PER_UNIT;
            navigation.verticalDop = dop->vDOP * UBX::DOP_PER_UNIT;
            return GPSReceiveUpdate::Activity;
        }
        case Msg::NAV_TIMEUTC.value(): {
            const auto utc = UBX::MessageCodec<UBX::NavTimeUtc>::decode(payload);
            if (!utc) {
                return {};
            }
            navigation.utcTimeUs = (utc->valid & TIMEUTC_VALID_UTC) ? utcMicroseconds(*utc) : 0;
            return GPSReceiveUpdate::Activity;
        }
        case Msg::NAV_VELNED.value(): {
            const auto velned = UBX::MessageCodec<UBX::NavVelned>::decode(payload);
            if (!velned) {
                return {};
            }
            navigation.speedMetersPerSecond = static_cast<float>(velned->gSpeed) * 1e-2f;
            navigation.courseRadians = static_cast<float>(velned->heading) * GPSProtocolMath::DEG_TO_RAD * 1e-5f;
            position.velocityValid = velocityValid(navigation.fixType);
            _gotVelocity = true;
            return GPSReceiveUpdate::Position;
        }
        default:
            return {};
    }
}

GPSReceiveUpdates UBXDecoder::_decodeHeading(uint16_t message, std::span<const uint8_t> payload,
                                             GPSDecodedPosition& position)
{
    struct Heading
    {
        float baselineMeters;
        bool headingValid;
        uint32_t flags;
        int32_t heading;
        uint32_t accuracy;
    };

    std::optional<Heading> heading;
    if (message == Msg::NAV_RELPOSNED.value()) {
        if (const auto relposned = UBX::MessageCodec<UBX::NavRelposned>::decode(payload)) {
            heading = Heading{(relposned->relPosLength + relposned->relPosHPLength * 1e-2f) * 1e-2f,
                              (relposned->flags & (1 << 8)) != 0, relposned->flags, relposned->relPosHeading,
                              relposned->accHeading};
        }
    } else if (const auto daheading = UBX::MessageCodec<UBX::NavDaheading>::decode(payload)) {
        // NAV-DAHEADING moves NAV-RELPOSNED's heading-valid bit 8 to bit 6, and its length is in mm.
        heading = Heading{daheading->relPosLength * 1e-3f, (daheading->flags & (1 << 6)) != 0, daheading->flags,
                          daheading->relPosHeading, daheading->accHeading};
    }
    if (!heading) {
        return {};
    }
    const bool relPosValid = heading->flags & (1 << 2);
    const bool carrierSolutionFixed = heading->flags & (1 << 4);
    const bool qualified = heading->headingValid && relPosValid && heading->baselineMeters < HEADING_MAX_BASELINE_M &&
                           carrierSolutionFixed;
    position.navigation.headingRadians = qualified ? headingRadians(heading->heading) : NAN;
    position.navigation.headingAccuracyRadians =
        qualified ? heading->accuracy * GPSProtocolMath::DEG_TO_RAD * 1e-5f : NAN;
    return GPSReceiveUpdate::Activity;
}

GPSReceiveUpdates UBXDecoder::_decodeSurveyIn(std::span<const uint8_t> payload, GPSDecodeContext& context)
{
    const auto svin = UBX::MessageCodec<UBX::NavSvin>::decode(payload);
    if (!svin) {
        return {};
    }
    _state.surveyStopped = svin->active == 0 && svin->valid == 0;
    if (!_state.mode.navigation) {
        return GPSReceiveUpdate::Activity;
    }
    GPSDecodedSurvey status{};
    status.survey.position = GPSProtocolMath::fromEcef({
        .x = (static_cast<double>(svin->meanX) + static_cast<double>(svin->meanXHP) * 0.01) * 0.01,
        .y = (static_cast<double>(svin->meanY) + static_cast<double>(svin->meanYHP) * 0.01) * 0.01,
        .z = (static_cast<double>(svin->meanZ) + static_cast<double>(svin->meanZHP) * 0.01) * 0.01,
    });
    status.survey.duration = std::chrono::seconds(svin->dur);
    status.survey.meanAccuracyMeters = static_cast<double>(svin->meanAcc / 10) / 1000.0;
    status.survey.valid = (svin->valid & 1) != 0;
    status.survey.active = (svin->active & 1) != 0;
    context.sink().publishSurvey(status);
    if (svin->valid == 1 && svin->active == 0) {
        _state.requests.rtcmActivation = true;
    }
    return GPSReceiveUpdate::Activity;
}

bool UBXDecoder::_decodeIntegrity(uint16_t message, std::span<const uint8_t> payload, uint64_t now)
{
    switch (message) {
        case Msg::NAV_STATUS.value(): {
            const auto status = UBX::MessageCodec<UBX::NavStatus>::decode(payload);
            if (!status) {
                return false;
            }
            _integrity.spoofing.state = GPSIntegrityReport::spoofingStateFromValue((status->flags2 >> 3) & 0x03);
            _integrity.spoofing.timestampUs = now;
            return true;
        }
        case Msg::MON_HW.value(): {
            const auto apply = [this, now](const auto& hardware) {
                _integrity.rf.noisePerMillisecond = hardware.noisePerMS;
                _integrity.rf.automaticGainControl = hardware.agcCnt;
                _integrity.rf.jammingIndicator = hardware.jamInd;
                _integrity.rf.timestampUs = now;
                return true;
            };
            // The u-blox 6 and 7+ layouts differ in size; protocol 27 deprecates the message.
            if (const auto hardware = UBX::MessageCodec<UBX::MonHw6>::decode(payload)) {
                return apply(*hardware);
            }
            if (const auto hardware = UBX::MessageCodec<UBX::MonHw7>::decode(payload)) {
                return apply(*hardware);
            }
            return false;
        }
        case Msg::MON_RF.value(): {
            const auto rf = UBX::MessageCodec<UBX::MonRf>::decode(payload);
            if (!rf) {
                return false;
            }
            // Only the first RF block is read; F9P reports two and X20 three, each with its own CW suppression.
            _integrity.rf.noisePerMillisecond = rf->block.noisePerMS;
            _integrity.rf.automaticGainControl = rf->block.agcCnt;
            _integrity.rf.jammingIndicator = rf->block.jamInd;
            _integrity.rf.timestampUs = now;
            if (!_state.secSigSeen) {
                _integrity.jamming.state = GPSIntegrityReport::jammingStateFromValue(rf->block.flags & 0x03);
                _integrity.jamming.timestampUs = now;
            }
            return true;
        }
        case Msg::SEC_SIG.value(): {
            const auto signal = UBX::MessageCodec<UBX::SecSig>::decode(payload);
            if (!signal) {
                return false;
            }
            // v1 keeps jamFlags at offset 4. The per-band groups of v2 and v3 are not read; spoofing still comes
            // from NAV-STATUS.
            const uint8_t flags = signal->version == 1 ? signal->jamFlags : signal->flags;
            auto jammingState = GPSIntegrityReport::JammingState::Unknown;
            if (flags & 0x01) {
                // jamState: 0 unknown, 1 none, 2 warning; MON-RF before v2 also had 3, critical.
                const uint8_t jamState = (flags >> 1) & 0x03;
                jammingState = jamState >= 2 ? GPSIntegrityReport::JammingState::Critical
                                             : GPSIntegrityReport::jammingStateFromValue(jamState);
            }
            _integrity.jamming.state = jammingState;
            _integrity.jamming.timestampUs = now;
            _state.secSigSeen = true;
            return true;
        }
        case Msg::RXM_RTCM.value(): {
            const auto rtcm = UBX::MessageCodec<UBX::RxmRtcm>::decode(payload);
            if (!rtcm) {
                return false;
            }
            _integrity.corrections.timestampUs = now;
            _integrity.corrections.protocol = GPSIntegrityReport::CorrectionProtocol::RTCM3;
            _integrity.corrections.crcFailed = (rtcm->flags & 0x01) != 0;
            _integrity.corrections.use = GPSIntegrityReport::correctionUseFromValue((rtcm->flags >> 1) & 0x03);
            return true;
        }
        case Msg::RXM_COR.value(): {
            const auto correction = UBX::MessageCodec<UBX::RxmCor>::decode(payload);
            if (!correction) {
                return false;
            }
            const uint32_t status = correction->statusInfo;
            auto protocol = GPSIntegrityReport::CorrectionProtocol::Unknown;
            switch (status & 0x1f) {
                case 1:
                    protocol = GPSIntegrityReport::CorrectionProtocol::RTCM3;
                    break;
                case 2:
                    protocol = GPSIntegrityReport::CorrectionProtocol::SPARTN;
                    break;
                case 5:
                    protocol = GPSIntegrityReport::CorrectionProtocol::HAS;
                    break;
                case 29:
                    protocol = GPSIntegrityReport::CorrectionProtocol::PMP;
                    break;
                case 30:
                    protocol = GPSIntegrityReport::CorrectionProtocol::QZSSL6;
                    break;
            }
            _integrity.corrections.timestampUs = now;
            _integrity.corrections.protocol = protocol;
            // errStatus: 0 unknown, 1 error-free, 2 erroneous; msgUsed: 0 unknown, 1 not used, 2 used.
            _integrity.corrections.crcFailed = ((status >> 5) & 0x03) == 2;
            _integrity.corrections.use = GPSIntegrityReport::correctionUseFromValue((status >> 7) & 0x03);
            return true;
        }
        default:
            return false;
    }
}

GPSReceiveUpdates UBXDecoder::_decodeControl(uint16_t message, std::span<const uint8_t> payload, uint64_t now)
{
    switch (message) {
        case Msg::INF_ERROR.value():
        case Msg::INF_WARNING.value(): {
            const QByteArrayView text(payload.data(), static_cast<qsizetype>(payload.size()));
            // Before logging: a message handler may decode more traffic, which reuses the frame's storage.
            if (text.startsWith("txbuf")) {
                _state.requests.commsDiagnostics = true;
            }
            const auto end = text.indexOf('\0');
            qCWarning(UBXProtocolLog).noquote()
                << QStringLiteral("ubx msg: %1").arg(QString::fromUtf8(end < 0 ? text : text.first(end)));
            return {};
        }
        case Msg::MON_COMMS.value():
            _logCommsDiagnostics(payload, now);
            return {};
        case Msg::CFG_TMODE3.value():
            if (payload[0] == 0 && payload[1] == 0) {
                _state.timeModeReadback = payload[2];
            }
            return {};
        case Msg::CFG_VALGET.value():
            _state.controller.accept(UBX::decodeConfigurationValues(payload));
            return {};
        case Msg::CFG_MSG.value():
            if (const auto rates = UBX::MessageCodec<UBX::CfgMsgRates>::decode(payload)) {
                _state.controller.accept(UBX::MessageRates{rates->msg, std::to_array(rates->rates)});
            }
            return {};
        case Msg::ACK_ACK.value():
        case Msg::ACK_NAK.value(): {
            const auto ack = UBX::MessageCodec<UBX::Ack>::decode(payload);
            if (!ack) {
                return {};
            }
            _state.controller.accept(UBX::Acknowledgement{ack->msg, message == Msg::ACK_ACK.value()});
            return GPSReceiveUpdate::Activity;
        }
        default:
            return {};
    }
}

void UBXDecoder::_decodeMonVer(std::span<const uint8_t> payload)
{
    auto& identity = _state.identity;
    identity = {};
    const auto version = UBX::MessageCodec<UBX::MonVer>::block(payload);
    if (!version) {
        return;
    }
    identity.firmware = cString(version->swVersion).toByteArray();
    // https://forum.u-blox.com/index.php/9432/need-help-decoding-ubx-mon-ver-hardware-string
    const QByteArrayView hardware = cString(version->hwVersion);
    identity.board = UBX::boardFromHardwareVersion(std::string_view(hardware.data(), size_t(hardware.size())));
    if (identity.board == UBX::Board::unknown) {
        qCWarning(UBXProtocolLog).noquote() << QStringLiteral("unknown board hw: %1").arg(QString::fromUtf8(hardware));
    }
    const auto profile = UBX::receiverProfile(identity.board);
    identity.protocol27 = profile.protocol27;
    identity.timeModeUnsupported = profile.timeModeUnsupported;
    for (size_t offset = UBX::WIRE_SIZE<UBX::MonVer>; offset < payload.size();
         offset += UBX::WIRE_SIZE<UBX::MonVerExtension>) {
        const auto block = UBX::MessageCodec<UBX::MonVerExtension>::block(payload, offset);
        if (!block) {
            return;
        }
        const QByteArrayView extension = cString(block->extension);
        // FWVER: product category and firmware version.
        if (const auto at = extension.indexOf("FWVER="); at >= 0) {
            const QByteArrayView field = extension.sliced(at);
            if (field.startsWith("FWVER=SPG ") || field.startsWith("FWVER=HPS ")) {
                identity.timeModeUnsupported = true;
            }
            identity.firmware = field.sliced(6).toByteArray();
            qCDebug(UBXProtocolLog) << "u-blox firmware version:" << identity.firmware;
            if (identity.board == UBX::Board::u_blox9 && field.contains("HPGL1L5")) {
                identity.board = UBX::Board::u_blox9_F9P_L1L5;
            }
        }
        // PROTVER: supported protocol version.
        if (const auto at = extension.indexOf("PROTVER="); at >= 0) {
            const QByteArrayView text = extension.sliced(at + 8);
            qCDebug(UBXProtocolLog) << "u-blox protocol version:" << text;
            const std::string_view protocol(text.data(), size_t(text.size()));
            const auto major = NMEA::number<unsigned>(protocol.substr(0, protocol.find('.')));
            if (!major || *major == 0) {
                identity.board = UBX::Board::unknown;
                return;
            }
            identity.protocol27 = *major >= 27;
            if (*major < 20) {
                identity.timeModeUnsupported = true;
            }
        }
        // MOD: module identification, set in production.
        if (const auto at = extension.indexOf("MOD="); at >= 0) {
            const QByteArrayView field = extension.sliced(at);
            if (field.startsWith("MOD=NEO-M8P-0") || field == "MOD=NEO-M8N" || field == "MOD=NEO-M9N") {
                identity.timeModeUnsupported = true;
            }
            identity.model = field.sliced(4).toByteArray();
            identity.isM8p = field.contains("M8P");
            if (identity.board == UBX::Board::u_blox9 && field.contains("F9P")) {
                identity.board = UBX::Board::u_blox9_F9P_L1L2;
            } else if (identity.board == UBX::Board::u_blox10 && field.contains("DAN-F10N")) {
                identity.board = UBX::Board::u_blox10_L1L5;
            }
            qCDebug(UBXProtocolLog) << "u-blox module:" << identity.model;
        }
    }
}

void UBXDecoder::_decodeSatellites(uint16_t message, std::span<const uint8_t> payload)
{
    _satellites = {};
    if (message == Msg::NAV_SAT.value()) {
        constexpr GPSConstellation SYSTEMS[] = {
            GPSConstellation::GPS,     GPSConstellation::SBAS, GPSConstellation::Galileo, GPSConstellation::BeiDou,
            GPSConstellation::Unknown, GPSConstellation::QZSS, GPSConstellation::GLONASS, GPSConstellation::NavIC};
        decodeSatelliteBlocks<UBX::NavSat, UBX::NavSatSatellite>(
            payload, _satellites, [](const auto& header) { return header.numSvs; },
            [&SYSTEMS](auto& report, const auto& satellite) {
                report.addSatellite(
                    satellite.gnssId < std::size(SYSTEMS) ? SYSTEMS[satellite.gnssId] : GPSConstellation::Unknown,
                    (satellite.flags & 8) != 0);
            });
        return;
    }
    decodeSatelliteBlocks<UBX::NavSvinfo, UBX::NavSvinfoChannel>(
        payload, _satellites, [](const auto& header) { return header.numCh; },
        [](auto& report, const auto& channel) {
            report.addSatellite(GPSConstellation::Unknown, (channel.flags & 1) != 0);
        });
}

void UBXDecoder::_logCommsDiagnostics(std::span<const uint8_t> payload, uint64_t now)
{
    auto& deadline = _state.commsReplyDeadlineUs;
    if (deadline == 0 || now > deadline) {
        return;
    }
    const auto status = UBX::MessageCodec<UBX::MonComms>::decode(payload);
    if (!status || status->version != 0 || status->nPorts > UBX::MON_COMMS_MAX_PORTS ||
        payload.size() != 8 + status->nPorts * UBX::WIRE_SIZE<UBX::MonCommsPort>) {
        return;
    }
    deadline = 0;
    qCWarning(UBXProtocolLog).noquote() << QStringLiteral(
                                               "MON-COMMS after txbuf: txErrors=0x%1 ports=%2 (snapshot after warning)")
                                               .arg(status->txErrors, 2, 16, QLatin1Char('0'))
                                               .arg(status->nPorts);
    for (unsigned index = 0; index < status->nPorts; ++index) {
        const auto& port = status->ports[index];
        qCWarning(UBXProtocolLog).noquote()
            << QStringLiteral(
                   "MON-COMMS %1 port=0x%2 txPending=%3 txUsage=%4% txPeakUsage=%5% rxPending=%6 "
                   "rxUsage=%7% overrunErrs=%8 skipped=%9")
                   .arg(portName(port.portId))
                   .arg(port.portId, 4, 16, QLatin1Char('0'))
                   .arg(port.txPending)
                   .arg(port.txUsage)
                   .arg(port.txPeakUsage)
                   .arg(port.rxPending)
                   .arg(port.rxUsage)
                   .arg(port.overrunErrs)
                   .arg(port.skipped);
    }
}
