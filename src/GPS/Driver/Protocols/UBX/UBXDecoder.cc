#include <cmath>
#include <ctime>
#include <optional>
#include <string.h>
#include <string_view>

#include "NMEASentence.h"
#include "RTCMFramer.h"
#include "UBX/UBXProtocol.h"
#include "UBXMessageCodec.h"

namespace {
GPSPositionReport::FixType navigationFix(uint8_t wireFix, uint8_t flags, bool hasCarrierFlags)
{
    using Fix = GPSPositionReport::FixType;
    if (!(flags & UBX_RX_NAV_PVT_FLAGS_GNSSFIXOK)) {
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
    return flags & UBX_RX_NAV_PVT_FLAGS_DIFFSOLN ? Fix::Differential : Fix::Fix3D;
}

bool velocityValid(GPSPositionReport::FixType fix)
{
    return fix != GPSPositionReport::FixType::Unknown && fix != GPSPositionReport::FixType::NoFix;
}

/// Broken-down UTC fields shared by NAV-PVT and NAV-TIMEUTC. Invalid receiver time must publish 0, the
/// defined "unavailable" value, so a receiver that lost time never keeps reporting the last known time.
template <typename Time>
tm utcFields(const Time& time)
{
    tm fields{};
    fields.tm_year = time.year - 1900;
    fields.tm_mon = time.month - 1;
    fields.tm_mday = time.day;
    fields.tm_hour = time.hour;
    fields.tm_min = time.min;
    fields.tm_sec = time.sec;
    return fields;
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
}  // namespace

int UBXProtocol::parseChar(uint8_t byte)
{
    if (_rtcm_parsing && _frameDecoder.idle() && _rtcm_parsing->ownsByte(byte)) {
        _rtcm_parsing->addByte(byte);
        drainRTCM(*_rtcm_parsing);
        return 0;
    }
    // Keep the completed frame local: synchronous logging can reenter or reset the parser.
    const auto frame = _frameDecoder.consume(byte);
    if (!frame) {
        return 0;
    }
    if (_rtcm_parsing) {
        _rtcm_parsing->reset();
    }
    return decodeValidatedPayload(frame->message, std::span(frame->payload).first(frame->length));
}

bool UBXProtocol::payloadRxInit(uint16_t message, std::span<const uint8_t> payload)
{
    auto state = UBX_RXMSG_HANDLE;

    switch (message) {
        case UBX_MSG_CFG_TMODE3:
            if (!_timeModeReadback.pending) {
                state = UBX_RXMSG_IGNORE;
            }
            break;
        case UBX_MSG_CFG_VALGET:
            if (!_controller.readbackPending()) {
                state = UBX_RXMSG_IGNORE;
            } else if (payload.size() < 4 || payload.size() > UBX::MAX_CONTROL_PAYLOAD_SIZE) {
                state = UBX_RXMSG_ERROR_LENGTH;
            }
            break;
        case UBX_MSG_MON_COMMS:
            break;

        case UBX_MSG_NAV_PVT:
            if (!_decodeContext.navigation) {
                state = UBX_RXMSG_IGNORE;  // ignore if not _decodeContext.navigation

            } else if (!_decodeContext.useNavPvt) {
                state = UBX_RXMSG_DISABLE;  // disable if not using NAV-PVT
            }

            break;

        case UBX_MSG_INF_DEBUG:
        case UBX_MSG_INF_ERROR:
        case UBX_MSG_INF_NOTICE:
        case UBX_MSG_INF_WARNING:
            if (payload.size() >= UBX::MAX_CONTROL_PAYLOAD_SIZE) {
                state = UBX_RXMSG_ERROR_LENGTH;
            }

            break;

        case UBX_MSG_NAV_POSLLH:
        case UBX_MSG_NAV_SOL:
        case UBX_MSG_NAV_TIMEUTC:
        case UBX_MSG_NAV_VELNED:
            if (!_decodeContext.navigation) {
                state = UBX_RXMSG_IGNORE;  // ignore if not _decodeContext.navigation

            } else if (_decodeContext.useNavPvt) {
                state = UBX_RXMSG_DISABLE;  // disable if using NAV-PVT instead
            }

            break;

        case UBX_MSG_NAV_STATUS:
        case UBX_MSG_NAV_DOP:
        case UBX_MSG_NAV_RELPOSNED:
        case UBX_MSG_NAV_DAHEADING:
        case UBX_MSG_NAV_HPPOSLLH:
        case UBX_MSG_MON_HW:
        case UBX_MSG_MON_RF:
        case UBX_MSG_SEC_SIG:
        case UBX_MSG_RXM_RTCM:
        case UBX_MSG_RXM_COR:
            if (!_decodeContext.navigation) {
                state = UBX_RXMSG_IGNORE;  // ignore if not _decodeContext.navigation
            }

            break;

        case UBX_MSG_NAV_SAT:
        case UBX_MSG_NAV_SVINFO:
            if (_satellites == nullptr) {
                state = UBX_RXMSG_DISABLE;  // disable if sat info not requested

            } else if (!_decodeContext.navigation) {
                state = UBX_RXMSG_IGNORE;  // ignore if not _decodeContext.navigation
            }

            break;

        case UBX_MSG_NAV_SVIN:
            break;

        case UBX_MSG_MON_VER:
            break;  // unconditionally handle this message

        case UBX_MSG_ACK_ACK:
            if (!_controller.awaitingAcknowledgement()) {
                state = UBX_RXMSG_IGNORE;  // No command is awaiting acknowledgement.
            }

            break;

        case UBX_MSG_ACK_NAK:
            if (!_controller.awaitingAcknowledgement() && !_controller.readbackPending() &&
                !_timeModeReadback.pending) {
                state = UBX_RXMSG_IGNORE;  // No command is awaiting acknowledgement.
            }

            break;

        default:
            state = UBX_RXMSG_DISABLE;  // disable all other messages
            break;
    }

    if (state == UBX_RXMSG_DISABLE) {
        _pendingDisableMessage = message;
    }
    return state == UBX_RXMSG_HANDLE;
}

void UBXProtocol::decodeNavSat(std::span<const uint8_t> payload)
{
    constexpr GPSConstellation systems[] = {
        GPSConstellation::GPS,     GPSConstellation::SBAS, GPSConstellation::Galileo, GPSConstellation::BeiDou,
        GPSConstellation::Unknown, GPSConstellation::QZSS, GPSConstellation::GLONASS, GPSConstellation::NavIC};
    decodeSatelliteBlocks<ubx_payload_rx_nav_sat_part1_t, ubx_payload_rx_nav_sat_part2_t>(
        payload, *_satellites, [](const auto& header) { return header.numSvs; },
        [&systems](auto& report, const auto& wire) {
            report.addSatellite(wire.gnssId < std::size(systems) ? systems[wire.gnssId] : GPSConstellation::Unknown,
                                (wire.flags & 8) != 0);
        });
}

void UBXProtocol::decodeNavSvinfo(std::span<const uint8_t> payload)
{
    decodeSatelliteBlocks<ubx_payload_rx_nav_svinfo_part1_t, ubx_payload_rx_nav_svinfo_part2_t>(
        payload, *_satellites, [](const auto& header) { return header.numCh; },
        [](auto& report, const auto& wire) { report.addSatellite(GPSConstellation::Unknown, (wire.flags & 1) != 0); });
}

void UBXProtocol::decodeMonVer(std::span<const uint8_t> payload)
{
    _identity = {};
    auto decoded_payload_rx_mon_ver_part1 = UBX::MessageCodec<ubx_payload_rx_mon_ver_part1_t>::block(payload);
    if (!decoded_payload_rx_mon_ver_part1) {
        return;
    }
    auto payload_rx_mon_ver_part1 = *decoded_payload_rx_mon_ver_part1;
    // The protocol specifies these as nul-terminated strings, but the terminator comes
    // from the device, so enforce it before anything walks the field.
    payload_rx_mon_ver_part1.swVersion[sizeof(payload_rx_mon_ver_part1.swVersion) - 1] = 0;
    payload_rx_mon_ver_part1.hwVersion[sizeof(payload_rx_mon_ver_part1.hwVersion) - 1] = 0;
    memcpy(_identity.firmwareVersion, payload_rx_mon_ver_part1.swVersion, sizeof(_identity.firmwareVersion));

    // Device detection (See
    // https://forum.u-blox.com/index.php/9432/need-help-decoding-ubx-mon-ver-hardware-string)
    static constexpr struct
    {
        char hw_version[9];
        Board board;
    } known_boards[] = {
        {"00040005", Board::u_blox5},    {"00040007", Board::u_blox6}, {"00070000", Board::u_blox7},
        {"00080000", Board::u_blox8},    {"00190000", Board::u_blox9}, {"000A0000", Board::u_blox10},
        {"000B0000", Board::u_blox_X20},
    };

    bool known = false;

    for (const auto& known_board : known_boards) {
        if (strncmp((const char*) payload_rx_mon_ver_part1.hwVersion, known_board.hw_version,
                    sizeof(payload_rx_mon_ver_part1.hwVersion)) == 0) {
            _identity.board = known_board.board;
            known = true;
            break;
        }
    }

    if (!known) {
        log(GPSProtocolLogLevel::Warning, "unknown board hw: %s", payload_rx_mon_ver_part1.hwVersion);
    }
    _identity.protocol27 =
        _identity.board == Board::u_blox9 || _identity.board == Board::u_blox10 || _identity.board == Board::u_blox_X20;
    _identity.timeModeUnsupported = _identity.board == Board::u_blox5 || _identity.board == Board::u_blox6 ||
                                    _identity.board == Board::u_blox7 || _identity.board == Board::u_blox10;
    for (size_t offset = UBX::WIRE_SIZE<ubx_payload_rx_mon_ver_part1_t>; offset < payload.size();
         offset += UBX::WIRE_SIZE<ubx_payload_rx_mon_ver_part2_t>) {
        auto decoded_payload_rx_mon_ver_part2 =
            UBX::MessageCodec<ubx_payload_rx_mon_ver_part2_t>::block(payload, offset);
        if (!decoded_payload_rx_mon_ver_part2) {
            return;
        }
        auto payload_rx_mon_ver_part2 = *decoded_payload_rx_mon_ver_part2;
        // Part 2 complete: decode Part 2 buffer
        // Same as above: the protocol specifies a nul-terminated string, the device provides
        // the terminator, so enforce it before strstr() walks the field.
        payload_rx_mon_ver_part2.extension[sizeof(payload_rx_mon_ver_part2.extension) - 1] = 0;

        // "FWVER=" Firmware of product category and version
        const char* fwver_str = strstr((const char*) payload_rx_mon_ver_part2.extension, "FWVER=");

        if (fwver_str != nullptr) {
            if (strncmp(fwver_str, "FWVER=SPG ", 10) == 0 || strncmp(fwver_str, "FWVER=HPS ", 10) == 0) {
                _identity.timeModeUnsupported = true;
            }
            strncpy(_identity.firmwareVersion, fwver_str + strlen("FWVER="), sizeof(_identity.firmwareVersion) - 1);
            _identity.firmwareVersion[sizeof(_identity.firmwareVersion) - 1] = '\0';
            log(GPSProtocolLogLevel::Debug, "u-blox firmware version: %s", fwver_str + strlen("FWVER="));

            // Check if its a ZED-F9P-15B
            if ((_identity.board == Board::u_blox9) && strstr(fwver_str, "HPGL1L5")) {
                _identity.board = Board::u_blox9_F9P_L1L5;
            }
        }

        // "PROTVER=" Supported protocol version.
        const char* protver_str = strstr((const char*) payload_rx_mon_ver_part2.extension, "PROTVER=");

        if (protver_str != nullptr) {
            log(GPSProtocolLogLevel::Debug, "u-blox protocol version: %s", protver_str + strlen("PROTVER="));
            const std::string_view version(protver_str + strlen("PROTVER="));
            const auto major = NMEA::number<unsigned>(version.substr(0, version.find('.')));
            if (!major || *major == 0) {
                _identity.board = Board::unknown;
                return;
            }
            _identity.protocol27 = *major >= 27;
            if (*major < 20) {
                _identity.timeModeUnsupported = true;
            }
        }

        // "MOD=" Module identification. Set in production.
        const char* mod_str = strstr((const char*) payload_rx_mon_ver_part2.extension, "MOD=");

        if (mod_str != nullptr) {
            if (strncmp(mod_str, "MOD=NEO-M8P-0", 13) == 0 || strcmp(mod_str, "MOD=NEO-M8N") == 0 ||
                strcmp(mod_str, "MOD=NEO-M9N") == 0) {
                _identity.timeModeUnsupported = true;
            }
            strncpy(_identity.modelName, mod_str + strlen("MOD="), sizeof(_identity.modelName) - 1);
            _identity.modelName[sizeof(_identity.modelName) - 1] = '\0';
            _identity.isM8p = strstr(mod_str, "M8P") != nullptr;
            // in case of u-blox9 family, check if it's an F9P
            if (_identity.board == Board::u_blox9) {
                if (strstr(mod_str, "F9P")) {
                    _identity.board = Board::u_blox9_F9P_L1L2;
                }

            } else if (_identity.board == Board::u_blox10) {
                if (strstr(mod_str, "DAN-F10N")) {
                    _identity.board = Board::u_blox10_L1L5;
                }
            }

            log(GPSProtocolLogLevel::Debug, "u-blox module: %s", mod_str + strlen("MOD="));
        }
    }
}

int UBXProtocol::payloadRxDone(uint16_t message, std::span<const uint8_t> payload, GPSDecodedPosition& position)
{
    switch (message) {
        case UBX_MSG_NAV_PVT:
            return decodeNavPvt(payload, position);
        case UBX_MSG_NAV_POSLLH:
        case UBX_MSG_NAV_HPPOSLLH:
        case UBX_MSG_NAV_SOL:
        case UBX_MSG_NAV_DOP:
        case UBX_MSG_NAV_TIMEUTC:
        case UBX_MSG_NAV_VELNED:
            return decodeNavigation(message, payload, position);
        case UBX_MSG_NAV_RELPOSNED:
        case UBX_MSG_NAV_DAHEADING:
            return decodeHeading(message, payload, position);
        case UBX_MSG_NAV_SAT:
        case UBX_MSG_NAV_SVINFO:
            // Satellite entries were decoded from the validated payload.
            for (uint8_t index = 0; index < _satellites->count; ++index) {
                auto& system = _satellites->constellations[index];
                system.inViewTimestampUs = nowUs();
                if (system.inUse) {
                    system.inUseTimestampUs = system.inViewTimestampUs;
                }
            }
            return 2;
        case UBX_MSG_NAV_SVIN:
            return decodeSurveyIn(payload);
        case UBX_MSG_NAV_STATUS:
        case UBX_MSG_MON_HW:
        case UBX_MSG_MON_RF:
        case UBX_MSG_SEC_SIG:
        case UBX_MSG_RXM_RTCM:
        case UBX_MSG_RXM_COR: {
            const int handled = decodeIntegrity(message, payload);
            if (handled) {
                publishIntegrity();
            }
            return handled;
        }
        default:
            return decodeControl(message, payload);
    }
}

int UBXProtocol::decodeNavPvt(std::span<const uint8_t> payload, GPSDecodedPosition& position)
{
    const auto decoded = UBX::MessageCodec<ubx_payload_rx_nav_pvt_t>::decode(payload);
    if (!decoded) {
        return 0;
    }
    const auto& pvt = *decoded;
    position.navigation.fixType = navigationFix(pvt.fixType, pvt.flags, true);
    position.velocityValid = velocityValid(position.navigation.fixType);
    position.navigation.satellitesUsed = pvt.numSV;

    if (_decodeContext.assembleEpochs ? !_epochHasHighPrecision
                                      : position.navigation.fixType != GPSPositionReport::FixType::RTKFixed) {
        // When RTK is active and solid (fix=6), these values will be filled by HPPOSLLH:
        position.navigation.latitudeDegrees = UBX::latitudeDegrees(pvt.lat);
        position.navigation.longitudeDegrees = UBX::longitudeDegrees(pvt.lon);
        position.navigation.altitudeMslMeters = pvt.hMSL * 1e-3;
        position.navigation.altitudeEllipsoidMeters = pvt.height * 1e-3;
        position.navigation.horizontalAccuracyMeters = static_cast<float>(pvt.hAcc) * 1e-3f;
        position.navigation.verticalAccuracyMeters = static_cast<float>(pvt.vAcc) * 1e-3f;
        _got_posllh = true;
    }

    position.navigation.speedMetersPerSecond = static_cast<float>(pvt.gSpeed) * 1e-3f;
    position.navigation.courseRadians = static_cast<float>(pvt.headMot) * GPS_DEG_TO_RAD * 1e-5f;
    const bool timeValid = (pvt.valid & UBX_RX_NAV_PVT_VALID_VALIDDATE) &&
                           (pvt.valid & UBX_RX_NAV_PVT_VALID_VALIDTIME) &&
                           (pvt.valid & UBX_RX_NAV_PVT_VALID_FULLYRESOLVED);
    auto utc = utcFields(pvt);
    position.navigation.utcTimeUs = timeValid ? timeFromUtc(utc, pvt.nano) : 0;
    position.navigation.timestampUs = nowUs();
    _got_velned = true;
    return 1;
}

int UBXProtocol::decodeNavigation(uint16_t message, std::span<const uint8_t> payload, GPSDecodedPosition& position)
{
    switch (message) {
        case UBX_MSG_NAV_POSLLH: {
            const auto posllh = UBX::MessageCodec<ubx_payload_rx_nav_posllh_t>::decode(payload);
            if (!posllh) {
                return 0;
            }
            position.navigation.latitudeDegrees = UBX::latitudeDegrees(posllh->lat);
            position.navigation.longitudeDegrees = UBX::longitudeDegrees(posllh->lon);
            position.navigation.altitudeMslMeters = posllh->hMSL * 1e-3;
            position.navigation.altitudeEllipsoidMeters = posllh->height * 1e-3;
            position.navigation.horizontalAccuracyMeters = static_cast<float>(posllh->hAcc) * 1e-3f;  // mm to m
            position.navigation.verticalAccuracyMeters = static_cast<float>(posllh->vAcc) * 1e-3f;    // mm to m
            position.navigation.timestampUs = nowUs();
            _got_posllh = true;
            return 1;
        }
        case UBX_MSG_NAV_HPPOSLLH: {
            const auto hpposllh = UBX::MessageCodec<ubx_payload_rx_nav_hpposllh_t>::decode(payload);
            if (!hpposllh || hpposllh->flags != 0 ||
                (!_decodeContext.assembleEpochs &&
                 position.navigation.fixType != GPSPositionReport::FixType::RTKFixed)) {
                return 0;
            }
            // Regular precision (1e-7 deg, mm) plus high precision components (1e-9 deg, 0.1 mm).
            position.navigation.latitudeDegrees = UBX::latitudeDegrees(hpposllh->lat) + hpposllh->latHp * 1e-9;
            position.navigation.longitudeDegrees = UBX::longitudeDegrees(hpposllh->lon) + hpposllh->lonHp * 1e-9;
            position.navigation.altitudeMslMeters = hpposllh->hMSL * 1e-3 + hpposllh->hMSLHp * 1e-4;
            position.navigation.altitudeEllipsoidMeters = hpposllh->height * 1e-3 + hpposllh->heightHp * 1e-4;
            position.navigation.horizontalAccuracyMeters = static_cast<float>(hpposllh->hAcc) * 1e-4f;  // 0.1 mm to m
            position.navigation.verticalAccuracyMeters = static_cast<float>(hpposllh->vAcc) * 1e-4f;
            position.navigation.timestampUs = nowUs();
            _got_posllh = true;
            return 1;
        }
        case UBX_MSG_NAV_SOL: {
            const auto sol = UBX::MessageCodec<ubx_payload_rx_nav_sol_t>::decode(payload);
            if (!sol) {
                return 0;
            }
            position.navigation.fixType = navigationFix(sol->gpsFix, sol->flags, false);
            position.velocityValid = velocityValid(position.navigation.fixType);
            position.navigation.satellitesUsed = sol->numSV;
            return 1;
        }
        case UBX_MSG_NAV_DOP: {
            const auto dop = UBX::MessageCodec<ubx_payload_rx_nav_dop_t>::decode(payload);
            if (!dop) {
                return 0;
            }
            position.navigation.horizontalDop = dop->hDOP * UBX::DOP_PER_UNIT;
            position.navigation.verticalDop = dop->vDOP * UBX::DOP_PER_UNIT;
            return 1;
        }
        case UBX_MSG_NAV_TIMEUTC: {
            const auto timeutc = UBX::MessageCodec<ubx_payload_rx_nav_timeutc_t>::decode(payload);
            if (!timeutc) {
                return 0;
            }
            auto utc = utcFields(*timeutc);
            position.navigation.utcTimeUs =
                (timeutc->valid & UBX_RX_NAV_TIMEUTC_VALID_VALIDUTC) ? timeFromUtc(utc, timeutc->nano) : 0;
            return 1;
        }
        case UBX_MSG_NAV_VELNED: {
            const auto velned = UBX::MessageCodec<ubx_payload_rx_nav_velned_t>::decode(payload);
            if (!velned) {
                return 0;
            }
            position.navigation.speedMetersPerSecond = static_cast<float>(velned->gSpeed) * 1e-2f;
            position.navigation.courseRadians = static_cast<float>(velned->heading) * GPS_DEG_TO_RAD * 1e-5f;
            position.velocityValid = velocityValid(position.navigation.fixType);
            _got_velned = true;
            return 1;
        }
        default:
            return 0;
    }
}

int UBXProtocol::decodeHeading(uint16_t message, std::span<const uint8_t> payload, GPSDecodedPosition& position)
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
    if (message == UBX_MSG_NAV_RELPOSNED) {
        if (const auto relposned = UBX::MessageCodec<ubx_payload_rx_nav_relposned_t>::decode(payload)) {
            heading = Heading{(relposned->relPosLength + relposned->relPosHPLength * 1e-2f) * 1e-2f,  // cm -> m
                              (relposned->flags & (1 << 8)) != 0, relposned->flags, relposned->relPosHeading,
                              relposned->accHeading};
        }
    } else if (const auto daheading = UBX::MessageCodec<ubx_payload_rx_nav_daheading_t>::decode(payload)) {
        // NAV-DAHEADING moves NAV-RELPOSNED's heading-valid bit 8 to bit 6.
        heading = Heading{daheading->relPosLength * 1e-3f, (daheading->flags & (1 << 6)) != 0,  // mm -> m
                          daheading->flags, daheading->relPosHeading, daheading->accHeading};
    }
    if (!heading) {
        return 0;
    }
    const bool relPosValid = heading->flags & (1 << 2);
    const bool carrierSolutionFixed = heading->flags & (1 << 4);
    const bool qualified = heading->headingValid && relPosValid &&
                           heading->baselineMeters < UBX_HEADING_MAX_BASELINE_M && carrierSolutionFixed;
    position.navigation.headingRadians = qualified ? relPosHeadingToYaw(heading->heading) : NAN;
    position.navigation.headingAccuracyRadians = qualified ? heading->accuracy * GPS_DEG_TO_RAD * 1e-5f : NAN;
    return 1;
}

int UBXProtocol::decodeSurveyIn(std::span<const uint8_t> payload)
{
    const auto decoded = UBX::MessageCodec<ubx_payload_rx_nav_svin_t>::decode(payload);
    if (!decoded) {
        return 0;
    }
    const auto& svin = *decoded;
    _survey_in_stopped = svin.active == 0 && svin.valid == 0;
    if (!_decodeContext.navigation) {
        return 1;
    }

    GPSDecodedSurvey status{};
    status.survey.position = fromEcef({
        .x = (static_cast<double>(svin.meanX) + static_cast<double>(svin.meanXHP) * 0.01) * 0.01,
        .y = (static_cast<double>(svin.meanY) + static_cast<double>(svin.meanYHP) * 0.01) * 0.01,
        .z = (static_cast<double>(svin.meanZ) + static_cast<double>(svin.meanZHP) * 0.01) * 0.01,
    });
    status.survey.duration = std::chrono::seconds(svin.dur);
    status.survey.meanAccuracyMeters = static_cast<double>(svin.meanAcc / 10) / 1000.0;
    status.survey.valid = (svin.valid & 1) != 0;
    status.survey.active = (svin.active & 1) != 0;
    publishSurvey(status);

    if (svin.valid == 1 && svin.active == 0) {
        _rtcmActivationPending = true;
    }
    return 1;
}

int UBXProtocol::decodeIntegrity(uint16_t message, std::span<const uint8_t> payload)
{
    switch (message) {
        case UBX_MSG_NAV_STATUS: {
            const auto status = UBX::MessageCodec<ubx_payload_rx_nav_status_t>::decode(payload);
            if (!status) {
                return 0;
            }
            _integrity.spoofing.state = GPSIntegrityReport::spoofingStateFromValue(
                (status->flags2 & UBX_RX_NAV_STATUS_SPOOFDETSTATE_MASK) >> UBX_RX_NAV_STATUS_SPOOFDETSTATE_SHIFT);
            _integrity.spoofing.timestampUs = nowUs();
            return 1;
        }
        case UBX_MSG_MON_HW: {
            const auto applyHardware = [this](const auto& hardware) {
                _integrity.rf.noisePerMillisecond = hardware.noisePerMS;
                _integrity.rf.automaticGainControl = hardware.agcCnt;
                _integrity.rf.jammingIndicator = hardware.jamInd;
                _integrity.rf.timestampUs = nowUs();
                return 1;
            };
            // u-blox 6 and 7+ layouts differ in size; protocol 27+ deprecates the message.
            switch (payload.size()) {
                case UBX::WIRE_SIZE<ubx_payload_rx_mon_hw_ubx6_t>:
                    if (const auto hardware = UBX::MessageCodec<ubx_payload_rx_mon_hw_ubx6_t>::decode(payload)) {
                        return applyHardware(*hardware);
                    }
                    return 0;
                case UBX::WIRE_SIZE<ubx_payload_rx_mon_hw_ubx7_t>:
                    if (const auto hardware = UBX::MessageCodec<ubx_payload_rx_mon_hw_ubx7_t>::decode(payload)) {
                        return applyHardware(*hardware);
                    }
                    return 0;
                default:
                    return 0;
            }
        }
        case UBX_MSG_MON_RF: {
            const auto rf = UBX::MessageCodec<ubx_payload_rx_mon_rf_t>::decode(payload);
            if (!rf) {
                return 0;
            }
            // TODO: only block 0 is read. F9P reports 2 blocks, X20 3, each with its own noisePerMS,
            // agcCnt and cwSuppression (jamInd). cwSuppression is the CW notch in effect per front end,
            // i.e. the per-frequency mitigation state GNSS_BANDS wants; the block covering a SEC-SIG
            // center frequency comes from rfBlockGnssBand (HPG 2.10) or blockId on older firmware.
            _integrity.rf.noisePerMillisecond = rf->block[0].noisePerMS;
            _integrity.rf.automaticGainControl = rf->block[0].agcCnt;
            _integrity.rf.jammingIndicator = rf->block[0].jamInd;
            _integrity.rf.timestampUs = nowUs();
            if (!_got_sec_sig) {
                _integrity.jamming.state = GPSIntegrityReport::jammingStateFromValue(rf->block[0].flags & 0x03);
                _integrity.jamming.timestampUs = nowUs();
            }
            return 1;
        }
        case UBX_MSG_SEC_SIG: {
            const auto signal = UBX::MessageCodec<ubx_payload_rx_sec_sig_t>::decode(payload);
            if (!signal || (signal->version == 1 && payload.size() < 5)) {
                return 0;
            }
            const uint8_t flags = signal->version == 1 ? signal->jamFlags : signal->flags;
            auto jammingState = GPSIntegrityReport::JammingState::Unknown;

            // TODO: bits 6..4 of the same byte are spfState (v1: spfFlags at offset 8, bits 3..1).
            // spoofing_state still comes from NAV-STATUS spoofDetState, whose F9 value 3 means
            // "multiple indications"; SEC-SIG distinguishes indicated/suspected from affirmed/
            // detected, which is the DETECTED vs AFFECTED split the MAVLink GNSS_INTEGRITY rework
            // maps to.
            if (flags & 0x01) {
                const uint8_t jamState = (flags >> 1) & 0x03;
                // SEC-SIG jamState: 0 unknown, 1 none, 2 warning (jamming indicated).
                // Pre-v2 MON-RF also had 3 = critical. sensor_gps 2 is "mitigated";
                // commander only alerts on 3 (detected).
                jammingState = jamState >= 2 ? GPSIntegrityReport::JammingState::Critical
                                             : GPSIntegrityReport::jammingStateFromValue(jamState);
            }
            _integrity.jamming.state = jammingState;
            _integrity.jamming.timestampUs = nowUs();
            _got_sec_sig = true;

            // TODO: v2/v3 carry jamNumCentFreqs X4 groups after the header (bits 23..0 centFreq in
            // kHz, bit 24 jammed), one per in-use band. Not parsed: sensor_gps has nowhere to put
            // per-band state until the GNSS_BANDS message from mavlink/rfcs#30 lands, at which
            // point both the RX struct and payloadRxInit() length check need the repeated group.
            return 1;
        }
        case UBX_MSG_RXM_RTCM: {
            const auto rtcm = UBX::MessageCodec<ubx_payload_rx_rxm_rtcm_t>::decode(payload);
            if (!rtcm) {
                return 0;
            }
            _integrity.corrections.timestampUs = nowUs();
            _integrity.corrections.protocol = GPSIntegrityReport::CorrectionProtocol::RTCM3;
            _integrity.corrections.crcFailed = (rtcm->flags & UBX_RX_RXM_RTCM_CRCFAILED_MASK) != 0;
            _integrity.corrections.use = GPSIntegrityReport::correctionUseFromValue(
                (rtcm->flags & UBX_RX_RXM_RTCM_MSGUSED_MASK) >> UBX_RX_RXM_RTCM_MSGUSED_SHIFT);
            return 1;
        }
        case UBX_MSG_RXM_COR: {
            const auto correction = UBX::MessageCodec<ubx_payload_rx_rxm_cor_t>::decode(payload);
            if (!correction) {
                return 0;
            }
            const uint32_t status = correction->statusInfo;
            auto protocol = GPSIntegrityReport::CorrectionProtocol::Unknown;
            switch (status & UBX_RX_RXM_COR_PROTOCOL_MASK) {
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
            _integrity.corrections.timestampUs = nowUs();
            _integrity.corrections.protocol = protocol;
            _integrity.corrections.crcFailed =
                ((status & UBX_RX_RXM_COR_ERRSTATUS_MASK) >> UBX_RX_RXM_COR_ERRSTATUS_SHIFT) == 2;
            _integrity.corrections.use = GPSIntegrityReport::correctionUseFromValue(
                (status & UBX_RX_RXM_COR_MSGUSED_MASK) >> UBX_RX_RXM_COR_MSGUSED_SHIFT);
            return 1;
        }
        default:
            return 0;
    }
}

int UBXProtocol::decodeControl(uint16_t message, std::span<const uint8_t> payload)
{
    switch (message) {
        case UBX_MSG_INF_ERROR:
        case UBX_MSG_INF_WARNING: {
            const std::string_view text(reinterpret_cast<const char*>(payload.data()), payload.size());
            log(GPSProtocolLogLevel::Warning, "ubx msg: %.*s", int(text.size()), text.data());
            if (text.starts_with("txbuf")) {
                _comms.pending = true;
            }
            return 0;
        }
        case UBX_MSG_MON_COMMS:
            logCommsDiagnostics(payload);
            return 0;
        case UBX_MSG_CFG_TMODE3:
            if (_timeModeReadback.pending && payload[0] == 0 && payload[1] == 0) {
                _timeModeReadback.response = payload[2];
            }
            return 0;
        case UBX_MSG_CFG_VALGET:
            if (const auto values = UBX::decodeConfigurationValues(payload)) {
                _controller.accept(*values);
            }
            return 0;
        case UBX_MSG_MON_VER:
            // This is polled only on startup, and the startup code waits for an ack
            _controller.accept(UBX::Acknowledgement{UBX_MSG_MON_VER, true});
            return 1;
        case UBX_MSG_ACK_ACK:
        case UBX_MSG_ACK_NAK: {
            const auto acknowledgement = UBX::MessageCodec<ubx_payload_rx_ack_ack_t>::decode(payload);
            if (!acknowledgement) {
                return 0;
            }
            _controller.accept(UBX::Acknowledgement{acknowledgement->msg, message == UBX_MSG_ACK_ACK});
            return 1;
        }
        default:
            return 0;
    }
}

void UBXProtocol::logCommsDiagnostics(std::span<const uint8_t> payload)
{
    if (_comms.deadlineUs == 0 || nowUs() > _comms.deadlineUs) {
        return;
    }

    const auto decoded_status = UBX::MessageCodec<ubx_payload_rx_mon_comms_t>::decode(payload);
    if (!decoded_status) {
        return;
    }
    const auto& status = *decoded_status;

    if (status.version != 0 || status.nPorts > UBX_MON_COMMS_MAX_PORTS ||
        payload.size() != 8 + status.nPorts * UBX::WIRE_SIZE<ubx_payload_rx_mon_comms_port_t>) {
        return;
    }

    _comms.deadlineUs = 0;
    log(GPSProtocolLogLevel::Warning, "MON-COMMS after txbuf: txErrors=0x%02x ports=%u (snapshot after warning)",
        (unsigned) status.txErrors, (unsigned) status.nPorts);

    for (unsigned i = 0; i < status.nPorts; ++i) {
        const auto& port = status.ports[i];
        [[maybe_unused]] const char* name = "unknown";

        switch (port.portId) {
            case 0x0000:
                name = "I2C";
                break;
            case 0x0100:
                name = "UART1";
                break;
            case 0x0201:
                name = "UART2";
                break;
            case 0x0300:
                name = "USB";
                break;
            case 0x0400:
                name = "SPI";
                break;
            default:
                break;
        }

        log(GPSProtocolLogLevel::Warning,
            "MON-COMMS %s port=0x%04x txPending=%u txUsage=%u%% txPeakUsage=%u%% "
            "rxPending=%u rxUsage=%u%% overrunErrs=%u skipped=%lu",
            name, (unsigned) port.portId, (unsigned) port.txPending, (unsigned) port.txUsage,
            (unsigned) port.txPeakUsage, (unsigned) port.rxPending, (unsigned) port.rxUsage,
            (unsigned) port.overrunErrs, (unsigned long) port.skipped);
    }
}

void UBXProtocol::decodeInit()
{
    _frameDecoder.reset();
}

float UBXProtocol::relPosHeadingToYaw(int32_t heading) const
{
    float heading_rad = heading * GPS_DEG_TO_RAD * 1e-5f;

    // Normalize to [-pi, pi]
    if (heading_rad > GPS_PI) {
        heading_rad -= 2.f * GPS_PI;

    } else if (heading_rad < -GPS_PI) {
        heading_rad += 2.f * GPS_PI;
    }

    return heading_rad;
}

void UBXProtocol::calcChecksum(const uint8_t* buffer, const uint16_t length, ubx_checksum_t* checksum)
{
    for (uint16_t i = 0; i < length; i++) {
        checksum->ck_a = checksum->ck_a + buffer[i];
        checksum->ck_b = checksum->ck_b + checksum->ck_a;
    }
}

int UBXProtocol::decodeValidatedPayload(uint16_t message, std::span<const uint8_t> payload)
{
    const auto* schema = UBX::messageSchema(message);
    if (!UBX::validPayload(message, payload, schema)) {
        return 0;
    }
    if (message == UBX::NAV_EOE && payload.size() == 4 && _decodeContext.assembleEpochs) {
        uint32_t tow = 0;
        for (unsigned index = 0; index < 4; ++index) {
            tow |= uint32_t(payload[index]) << (index * 8);
        }
        if (tow < UBXNavigationEpoch::WEEK_MS) {
            _navigationEpochs.end(tow, [this](const auto& report) { publishEpoch(report); });
        }
        return GPSDecodedBatch::PROTOCOL_ACTIVITY;
    }
    if (!payloadRxInit(message, payload)) {
        return 0;
    }
    UBXNavigationEpoch::Epoch* epoch = nullptr;
    const auto publish = [this](const auto& report) { publishEpoch(report); };
    const bool timed = schema && schema->towOffset >= 0;
    if (_decodeContext.assembleEpochs && timed) {
        const size_t offset = schema->towOffset;
        const auto tow = LittleEndian::read<uint32_t>(payload, offset).value_or(0);
        epoch = _navigationEpochs.find(tow, nowUs(), publish);
        if (!epoch) {
            return GPSDecodedBatch::PROTOCOL_ACTIVITY;
        }
        _epochHasHighPrecision = epoch->highPrecision;
    }
    switch (message) {
        case UBX_MSG_NAV_SAT:
            *_satellites = {};
            decodeNavSat(payload);
            break;
        case UBX_MSG_NAV_SVINFO:
            *_satellites = {};
            decodeNavSvinfo(payload);
            break;
        case UBX_MSG_MON_VER:
            decodeMonVer(payload);
            break;
        default:
            break;
    }
    const int updates = payloadRxDone(message, payload, epoch ? epoch->position : _position);
    if (epoch) {
        if (message == UBX_MSG_NAV_PVT) {
            epoch->positionValid = epoch->velocityValid = true;
        } else if (message == UBX_MSG_NAV_POSLLH) {
            epoch->positionValid = true;
        } else if (message == UBX_MSG_NAV_VELNED) {
            epoch->velocityValid = true;
        } else if (message == UBX_MSG_NAV_HPPOSLLH && (updates & GPSDecodedBatch::POSITION_UPDATE)) {
            epoch->highPrecision = true;
        }
        return GPSDecodedBatch::PROTOCOL_ACTIVITY;
    }
    // ACKs and ancillary metadata are useful protocol activity, not new position epochs.
    if ((updates & GPSDecodedBatch::POSITION_UPDATE) && message != UBX_MSG_NAV_PVT && message != UBX_MSG_NAV_POSLLH &&
        message != UBX_MSG_NAV_HPPOSLLH && message != UBX_MSG_NAV_VELNED) {
        return (updates & ~GPSDecodedBatch::POSITION_UPDATE) | GPSDecodedBatch::PROTOCOL_ACTIVITY;
    }
    if (updates & GPSDecodedBatch::POSITION_UPDATE) {
        publishPosition(_position);
    }
    if ((updates & GPSDecodedBatch::SATELLITES_UPDATE) && _satellites) {
        publishSatellites(*_satellites);
    }
    return updates;
}

int UBXProtocol::decodeByte(uint8_t byte)
{
    return parseChar(byte);
}
