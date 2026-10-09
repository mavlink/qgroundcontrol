#include <algorithm>
#include <array>
#include <string_view>

#include <QtCore/QByteArrayView>
#include <QtCore/QString>

#include "GPSFamilyProtocol.h"
#include "GPSProtocolMath.h"
#include "GPSReceiverFamilies.h"
#include "GPSStreamDemux.h"
#include "GPSTime.h"
#include "MonotonicClock.h"
#include "NMEASentence.h"
#include "UBX/UBXMessageSchema.h"
#include "UBX/UBXProtocol.h"

namespace Msg = UBX::Msg;

namespace {

/// Once part of an epoch has arrived, a read this long without data ends it.
constexpr std::chrono::milliseconds PACKET_TIMEOUT{8};
/// Assembled epochs expire by time, so reads return often enough to publish them.
constexpr std::chrono::milliseconds EPOCH_READ_SLICE{200};

constexpr uint8_t PVT_VALID_DATE = 0x01;
constexpr uint8_t PVT_VALID_TIME = 0x02;
constexpr uint8_t PVT_FULLY_RESOLVED = 0x04;
constexpr uint8_t FIX_OK = 0x01;
constexpr uint8_t DIFFERENTIAL_SOLUTION = 0x02;

GPSPositionReport::FixType navigationFix(uint8_t wireFix, uint8_t flags)
{
    using Fix = GPSPositionReport::FixType;
    if (!(flags & FIX_OK)) {
        return Fix::NoFix;
    }
    switch (wireFix) {
        case 0:
            return Fix::NoFix;
        case 5:
            // Time only: a receiver in time mode, such as a base station, holds its surveyed or fixed position.
            return Fix::Fix3D;
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
    const uint8_t carrier = flags >> 6;
    if (carrier == 1) {
        return Fix::RTKFloat;
    }
    if (carrier == 2) {
        return Fix::RTKFixed;
    }
    return flags & DIFFERENTIAL_SOLUTION ? Fix::Differential : Fix::Fix3D;
}

bool velocityValid(GPSPositionReport::FixType fix)
{
    return fix != GPSPositionReport::FixType::Unknown && fix != GPSPositionReport::FixType::NoFix;
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

/// MON-RF antStatus and MON-HW aStatus; the supervisor's INIT and DONTKNOW are unknown.
GPSIntegrityReport::AntennaState antennaState(uint8_t status)
{
    switch (status) {
        case 2:
            return GPSIntegrityReport::AntennaState::Ok;
        case 3:
            return GPSIntegrityReport::AntennaState::Short;
        case 4:
            return GPSIntegrityReport::AntennaState::Open;
        default:
            return GPSIntegrityReport::AntennaState::Unknown;
    }
}

/// Fixed-size satellite blocks follow the header, as many as the validated payload holds.
template <typename Header, typename Block, typename Count, typename Assign>
void decodeSatelliteBlocks(std::span<const uint8_t> payload, GPSDecodedSatellites& report, Count count, Assign assign)
{
    const auto satelliteCount =
        std::min<size_t>(count(Wire::decode<Header>(payload)), GPSDecodedSatellites::SAT_INFO_MAX_SATELLITES);
    if (satelliteCount == 0) {
        (void) report.ensureConstellation(GPSConstellation::Unknown);
    }
    for (size_t index = 0; index < satelliteCount; ++index) {
        assign(report, Wire::decode<Block>(payload, UBX::WIRE_SIZE<Header> + index * UBX::WIRE_SIZE<Block>));
    }
}

}  // namespace

namespace UBX {

GPSReceiveUpdates Protocol::onFrame(const GPSFrame& frame, GPSDecodeContext& context)
{
    if (frame.kind == GPSFrameKind::RTCM3) {
        context.publishRTCM(frame.bytes);
        return {};
    }
    return _decode(static_cast<uint16_t>(frame.messageId), frame.payload, context);
}

void Protocol::flush(GPSDecodeContext& context)
{
    if (_mode.navigation) {
        _epochs.expire(context);
    }
}

bool Protocol::armDecodeOnly(const GPSConfig& config, GPSDecodeContext& context)
{
    _base = config.base;
    _setMode({.navigation = true, .corrections = true}, context.stream());
    _ready = true;
    return true;
}

bool Protocol::armNavigationDecode(GPSNavigationDecode decode, GPSDecodeContext& context)
{
    _setMode({.navigation = true, .corrections = decode.corrections}, context.stream());
    return true;
}

void Protocol::_setMode(const Mode& mode, GPSStreamDemux& stream)
{
    _mode = mode;
    _epochs = {};
    stream.setEnabled(GPSFrameKind::RTCM3, mode.corrections);
}

bool Protocol::completeReceiveCycle(GPSReceiveUpdates handled)
{
    const bool ready = (_timeModeReadbackPending && _timeModeReadback.has_value()) ||
                       (_controller.readbackPending() && _controller.readbackReady()) ||
                       (_ready ? handled.testFlag(GPSReceiveUpdate::Position) : handled != GPSReceiveUpdates{});
    if (ready) {
        _epochStarted = false;
    }
    return ready;
}

std::chrono::milliseconds Protocol::nextReadSlice(std::chrono::milliseconds timeout) const
{
    if (_epochStarted) {
        return PACKET_TIMEOUT;
    }
    return _mode.navigation ? (std::min) (timeout, EPOCH_READ_SLICE) : timeout;
}

GPSReceiveUpdates Protocol::_decode(uint16_t message, std::span<const uint8_t> payload, GPSDecodeContext& context)
{
    const auto* schema = UBX::messageSchema(message);
    if (!UBX::validPayload(message, payload, schema)) {
        return {};
    }
    if (message == Msg::NAV_EOE.value() && _mode.navigation) {
        const uint32_t tow = LittleEndian::read<uint32_t>(payload, 0).value_or(GPSTime::WEEK_MS);
        if (tow < GPSTime::WEEK_MS) {
            _epochs.end(tow, context);
        }
        return GPSReceiveUpdate::Activity;
    }
    if (!_accept(message, payload)) {
        return {};
    }
    // Only navigation decoding accepts the messages with a time of week.
    if (schema && schema->towOffset >= 0) {
        const auto tow = LittleEndian::read<uint32_t>(payload, static_cast<size_t>(schema->towOffset)).value_or(0);
        if (auto* epoch = _epochs.find(tow, context)) {
            _decodeEpoch(message, payload, *epoch);
        }
        return GPSReceiveUpdate::Activity;
    }
    const auto updates = _decodeHandled(message, payload, context);
    if (updates.testFlag(GPSReceiveUpdate::Satellites)) {
        context.publishSatellites(_satellites);
    }
    return updates;
}

bool Protocol::_accept(uint16_t message, std::span<const uint8_t> payload)
{
    const bool navigation = _mode.navigation;
    const auto disable = [this, message] {
        _requests.disableMessage = message;
        return false;
    };
    if (std::ranges::find(NAVIGATION_MESSAGES, message, &MessageId::value) != NAVIGATION_MESSAGES.end()) {
        return navigation;
    }
    switch (message) {
        case Msg::CFG_TMODE3.value():
            return _timeModeReadbackPending;
        case Msg::CFG_VALGET.value():
            return _controller.readbackPending() || _controller.readbackReplyOutstanding();
        case Msg::CFG_MSG.value():
            return _controller.ratePollPending();
        case Msg::NAV_SVIN.value():
        case Msg::MON_VER.value():
            return true;
        case Msg::INF_DEBUG.value():
        case Msg::INF_ERROR.value():
        case Msg::INF_NOTICE.value():
        case Msg::INF_WARNING.value():
            return payload.size() < UBX::MAX_CONTROL_PAYLOAD_SIZE;
        case Msg::ACK_ACK.value():
            return _controller.awaitingAcknowledgement() || _controller.ratePollPending();
        case Msg::ACK_NAK.value():
            return _controller.awaitingAcknowledgement() || _controller.readbackPending() ||
                   _controller.readbackReplyOutstanding() || _controller.ratePollPending() || _timeModeReadbackPending;
        default:
            return disable();
    }
}

void Protocol::_decodeEpoch(uint16_t message, std::span<const uint8_t> payload, EpochAssembler::Epoch& epoch)
{
    auto& navigation = epoch.position.navigation;
    switch (message) {
        case Msg::NAV_PVT.value(): {
            const auto pvt = Wire::decode<UBX::NavPvt>(payload);
            navigation.fixType = navigationFix(pvt.fixType, pvt.flags);
            epoch.position.velocityValid = velocityValid(navigation.fixType);
            navigation.satellitesUsed = gpsSatellitesUsed(pvt.numSV);
            // NAV-HPPOSLLH of the same epoch supersedes the position.
            if (!epoch.highPrecision) {
                navigation.latitudeDegrees = UBX::latitudeDegrees(pvt.lat);
                navigation.longitudeDegrees = UBX::longitudeDegrees(pvt.lon);
                navigation.altitudeMslMeters = pvt.hMSL * 1e-3;
                navigation.altitudeEllipsoidMeters = pvt.height * 1e-3;
                navigation.horizontalAccuracyMeters = static_cast<float>(pvt.hAcc) * 1e-3f;
                navigation.verticalAccuracyMeters = static_cast<float>(pvt.vAcc) * 1e-3f;
            }
            navigation.speedMetersPerSecond = static_cast<float>(pvt.gSpeed) * 1e-3f;
            navigation.courseRadians = static_cast<float>(pvt.headMot * 1e-5 * GPSProtocolMath::DEG_TO_RAD);
            // Receiver time that is not valid publishes 0, the defined "unavailable" value, so a receiver that lost
            // time never keeps reporting the last known time.
            const bool timeValid =
                (pvt.valid & PVT_VALID_DATE) && (pvt.valid & PVT_VALID_TIME) && (pvt.valid & PVT_FULLY_RESOLVED);
            navigation.utcTimeUs = timeValid ? GPSProtocolMath::utcMicroseconds(pvt.year, pvt.month, pvt.day, pvt.hour,
                                                                                pvt.min, pvt.sec, pvt.nano)
                                             : 0;
            epoch.hasPvt = true;
            _epochStarted = true;
            break;
        }
        case Msg::NAV_HPPOSLLH.value(): {
            const auto hp = Wire::decode<UBX::NavHpposllh>(payload);
            if (hp.flags != 0) {
                break;
            }
            navigation.latitudeDegrees = UBX::latitudeDegrees(hp.lat) + hp.latHp * 1e-9;
            navigation.longitudeDegrees = UBX::longitudeDegrees(hp.lon) + hp.lonHp * 1e-9;
            navigation.altitudeMslMeters = hp.hMSL * 1e-3 + hp.hMSLHp * 1e-4;
            navigation.altitudeEllipsoidMeters = hp.height * 1e-3 + hp.heightHp * 1e-4;
            navigation.horizontalAccuracyMeters = static_cast<float>(hp.hAcc) * 1e-4f;
            navigation.verticalAccuracyMeters = static_cast<float>(hp.vAcc) * 1e-4f;
            epoch.highPrecision = true;
            _epochStarted = true;
            break;
        }
        case Msg::NAV_DOP.value(): {
            const auto dop = Wire::decode<UBX::NavDop>(payload);
            navigation.horizontalDop = dop.hDOP * UBX::DOP_PER_UNIT;
            navigation.verticalDop = dop.vDOP * UBX::DOP_PER_UNIT;
            break;
        }
        default:
            break;
    }
}

GPSReceiveUpdates Protocol::_decodeHandled(uint16_t message, std::span<const uint8_t> payload,
                                           GPSDecodeContext& context)
{
    const uint64_t now = context.nowUs();
    switch (message) {
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
        case Msg::NAV_STATUS.value(): {
            const auto status = Wire::decode<UBX::NavStatus>(payload);
            _integrity.spoofing.state = GPSIntegrityReport::spoofingStateFromValue((status.flags2 >> 3) & 0x03);
            _integrity.spoofing.timestampUs = now;
            break;
        }
        case Msg::MON_HW.value(): {
            const auto hardware = Wire::decode<UBX::MonHw>(payload);
            _integrity.antenna = {.timestampUs = now, .state = antennaState(hardware.aStatus)};
            if (!_secSigSeen) {
                _integrity.jamming = {.timestampUs = now,
                                      .state = GPSIntegrityReport::jammingStateFromValue((hardware.flags >> 2) & 0x03)};
            }
            break;
        }
        case Msg::MON_RF.value(): {
            // Each RF block (F9P reports two, X20 three) has its own jamming monitor and antenna supervisor; the worst
            // state any block reports stands.
            auto jamming = GPSIntegrityReport::JammingState::Unknown;
            auto antenna = GPSIntegrityReport::AntennaState::Unknown;
            for (size_t offset = UBX::WIRE_SIZE<UBX::MonRf> - UBX::WIRE_SIZE<UBX::MonRfBlock>; offset < payload.size();
                 offset += UBX::WIRE_SIZE<UBX::MonRfBlock>) {
                const auto block = Wire::decode<UBX::MonRfBlock>(payload, offset);
                jamming = (std::max) (jamming, GPSIntegrityReport::jammingStateFromValue(block.flags & 0x03));
                antenna = (std::max) (antenna, antennaState(block.antStatus));
            }
            if (!_secSigSeen) {
                _integrity.jamming = {.timestampUs = now, .state = jamming};
            }
            _integrity.antenna = {.timestampUs = now, .state = antenna};
            break;
        }
        case Msg::SEC_SIG.value(): {
            const auto signal = Wire::decode<UBX::SecSig>(payload);
            // v1 keeps jamFlags at offset 4. The per-band groups of v2 and v3 are not read; spoofing still comes
            // from NAV-STATUS.
            const uint8_t flags = signal.version == 1 ? signal.jamFlags : signal.flags;
            // jamDetEnabled in bit 0, then the jamming state as MON-RF reports it.
            _integrity.jamming.state = (flags & 0x01) ? GPSIntegrityReport::jammingStateFromValue((flags >> 1) & 0x03)
                                                      : GPSIntegrityReport::JammingState::Unknown;
            _integrity.jamming.timestampUs = now;
            _secSigSeen = true;
            break;
        }
        case Msg::MON_VER.value():
            _decodeMonVer(payload);
            // Polled only while configuring, which waits for it as an acknowledgement.
            _controller.accept(UBX::Acknowledgement{message, true});
            return GPSReceiveUpdate::Activity;
        case Msg::INF_ERROR.value():
        case Msg::INF_WARNING.value(): {
            const QByteArrayView text(payload);
            // Before logging: a message handler may decode more traffic, which reuses the frame's storage.
            const bool outputOverflow = _mode.navigation && text.startsWith("txbuf");
            const auto end = text.indexOf('\0');
            const QString warning =
                QStringLiteral("ubx msg: %1").arg(QString::fromUtf8(end < 0 ? text : text.first(end)));
            // Receivers repeat a warning, such as an output overflow every epoch; only its first report warns.
            if (warning == _lastWarning && MonotonicClock::fresh(_lastWarningUs, now, REPEATED_WARNING_INTERVAL)) {
                qCDebug(UBXProtocolLog).noquote() << warning;
            } else {
                _lastWarning = warning;
                _lastWarningUs = now;
                qCWarning(UBXProtocolLog).noquote() << warning;
            }
            if (outputOverflow) {
                _integrity.outputOverflowUs = now;
                context.publishIntegrity(_integrity);
            }
            return {};
        }
        case Msg::CFG_TMODE3.value():
            if (payload[0] == 0 && payload[1] == 0) {
                _timeModeReadback = payload[2];
            }
            return {};
        case Msg::CFG_VALGET.value():
            _controller.accept(UBX::decodeConfigurationValues(payload));
            return {};
        case Msg::CFG_MSG.value(): {
            const auto rates = Wire::decode<UBX::CfgMsgRates>(payload);
            _controller.accept(UBX::MessageRates{rates.msg, std::to_array(rates.rates)});
            return {};
        }
        case Msg::ACK_ACK.value():
        case Msg::ACK_NAK.value():
            _controller.accept(
                UBX::Acknowledgement{Wire::decode<UBX::Ack>(payload).msg, message == Msg::ACK_ACK.value()});
            return GPSReceiveUpdate::Activity;
        default:
            return {};
    }
    // The integrity messages above update the integrity report.
    context.publishIntegrity(_integrity);
    return GPSReceiveUpdate::Activity;
}

GPSReceiveUpdates Protocol::_decodeSurveyIn(std::span<const uint8_t> payload, GPSDecodeContext& context)
{
    const auto svin = Wire::decode<UBX::NavSvin>(payload);
    _surveyStopped = svin.active == 0 && svin.valid == 0;
    if (!_mode.navigation) {
        return GPSReceiveUpdate::Activity;
    }
    GPSSurveyReport status{};
    status.position = GPSProtocolMath::fromEcef({
        .x = (static_cast<double>(svin.meanX) + static_cast<double>(svin.meanXHP) * 0.01) * 0.01,
        .y = (static_cast<double>(svin.meanY) + static_cast<double>(svin.meanYHP) * 0.01) * 0.01,
        .z = (static_cast<double>(svin.meanZ) + static_cast<double>(svin.meanZHP) * 0.01) * 0.01,
    });
    status.duration = std::chrono::seconds(svin.dur);
    status.meanAccuracyMeters = svin.meanAcc * 1e-4;
    status.valid = (svin.valid & 1) != 0;
    status.active = (svin.active & 1) != 0;
    context.publishSurvey(status);
    if (svin.valid == 1 && svin.active == 0) {
        _requests.rtcmActivation = true;
    }
    return GPSReceiveUpdate::Activity;
}

void Protocol::_decodeMonVer(std::span<const uint8_t> payload)
{
    auto& identity = _identity;
    identity = {};
    const auto version = Wire::decode<UBX::MonVer>(payload);
    identity.firmware = cString(version.swVersion).toByteArray();
    // https://forum.u-blox.com/index.php/9432/need-help-decoding-ubx-mon-ver-hardware-string
    const QByteArrayView hardware = cString(version.hwVersion);
    identity.hardware = hardware.toByteArray();
    identity.board = UBX::boardFromHardwareVersion(hardware);
    if (identity.board == UBX::Board::unknown) {
        qCWarning(UBXProtocolLog).noquote() << QStringLiteral("unknown board hw: %1").arg(QString::fromUtf8(hardware));
    }
    identity.protocol27 = UBX::receiverProfile(identity.board).protocol27;
    for (size_t offset = UBX::WIRE_SIZE<UBX::MonVer>; offset < payload.size();
         offset += UBX::WIRE_SIZE<UBX::MonVerExtension>) {
        const auto block = Wire::decode<UBX::MonVerExtension>(payload, offset);
        const QByteArrayView extension = cString(block.extension);
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
            identity.protocolVersion = text.toByteArray();
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

void Protocol::_decodeSatellites(uint16_t message, std::span<const uint8_t> payload)
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

}  // namespace UBX
