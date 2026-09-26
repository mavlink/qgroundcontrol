#pragma once

#include <array>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>

#include "UBX/Generated/UBXMessageIds.h"
#include "WireFields.h"

/// UBX payload records with the fields QGC reads or writes. Records of decoded messages carry their message ID;
/// bytes without a field are skipped on decode and zero on encode.
namespace UBX {

struct NavPosllh
{
    static constexpr MessageId ID = Msg::NAV_POSLLH;
    int32_t lon;     ///< 1e-7 deg
    int32_t lat;     ///< 1e-7 deg
    int32_t height;  ///< mm above the ellipsoid
    int32_t hMSL;    ///< mm above mean sea level
    uint32_t hAcc;   ///< mm
    uint32_t vAcc;   ///< mm
};

struct NavHpposllh
{
    static constexpr MessageId ID = Msg::NAV_HPPOSLLH;
    int8_t flags;  ///< invalidLlh
    int32_t lon;
    int32_t lat;
    int32_t height;
    int32_t hMSL;
    int8_t lonHp;     ///< 1e-9 deg
    int8_t latHp;     ///< 1e-9 deg
    int8_t heightHp;  ///< 0.1 mm
    int8_t hMSLHp;    ///< 0.1 mm
    uint32_t hAcc;    ///< 0.1 mm
    uint32_t vAcc;    ///< 0.1 mm
};

struct NavDop
{
    static constexpr MessageId ID = Msg::NAV_DOP;
    uint16_t vDOP;  ///< 0.01
    uint16_t hDOP;  ///< 0.01
};

struct NavSol
{
    static constexpr MessageId ID = Msg::NAV_SOL;
    uint8_t gpsFix;
    uint8_t flags;
    uint8_t numSV;
};

struct NavPvt
{
    static constexpr MessageId ID = Msg::NAV_PVT;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t valid;  ///< validDate, validTime, fullyResolved
    int32_t nano;
    uint8_t fixType;
    uint8_t flags;  ///< gnssFixOK, diffSoln, carrSoln in bits 7..6
    uint8_t numSV;
    int32_t lon;
    int32_t lat;
    int32_t height;
    int32_t hMSL;
    uint32_t hAcc;
    uint32_t vAcc;
    int32_t gSpeed;   ///< mm/s
    int32_t headMot;  ///< 1e-5 deg
};

struct NavTimeUtc
{
    static constexpr MessageId ID = Msg::NAV_TIMEUTC;
    int32_t nano;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t valid;  ///< validUTC in bit 2
};

struct NavVelned
{
    static constexpr MessageId ID = Msg::NAV_VELNED;
    uint32_t gSpeed;  ///< cm/s
    int32_t heading;  ///< 1e-5 deg
};

struct NavStatus
{
    static constexpr MessageId ID = Msg::NAV_STATUS;
    uint8_t flags2;  ///< spoofDetState in bits 4..3
};

struct NavSvin
{
    static constexpr MessageId ID = Msg::NAV_SVIN;
    uint32_t dur;
    int32_t meanX;  ///< cm
    int32_t meanY;
    int32_t meanZ;
    int8_t meanXHP;  ///< 0.1 mm
    int8_t meanYHP;
    int8_t meanZHP;
    uint32_t meanAcc;  ///< 0.1 mm
    uint8_t valid;
    uint8_t active;
};

struct NavRelposned
{
    static constexpr MessageId ID = Msg::NAV_RELPOSNED;
    int32_t relPosLength;   ///< cm
    int32_t relPosHeading;  ///< 1e-5 deg
    int8_t relPosHPLength;  ///< 0.1 mm
    uint32_t accHeading;    ///< 1e-5 deg
    uint32_t flags;
};

struct NavDaheading
{
    static constexpr MessageId ID = Msg::NAV_DAHEADING;
    int32_t relPosLength;   ///< mm
    int32_t relPosHeading;  ///< 1e-5 deg
    uint32_t accHeading;    ///< 1e-5 deg
    uint32_t flags;
};

struct NavSat
{
    uint8_t numSvs;
};

struct NavSatSatellite
{
    uint8_t gnssId;
    uint32_t flags;  ///< svUsed in bit 3
};

struct NavSvinfo
{
    uint8_t numCh;
};

struct NavSvinfoChannel
{
    uint8_t flags;  ///< svUsed in bit 0
};

/// MON-HW of u-blox 6.
struct MonHw6
{
    static constexpr MessageId ID = Msg::MON_HW;
    uint16_t noisePerMS;
    uint16_t agcCnt;
    uint8_t jamInd;
};

/// MON-HW of u-blox 7 and later; protocol 27 deprecates it for MON-RF.
struct MonHw7
{
    static constexpr MessageId ID = Msg::MON_HW;
    uint16_t noisePerMS;
    uint16_t agcCnt;
    uint8_t jamInd;
};

struct MonRfBlock
{
    uint8_t flags;  ///< jammingState in bits 1..0
    uint16_t noisePerMS;
    uint16_t agcCnt;
    uint8_t jamInd;
};

/// MON-RF with its first RF block; F9P reports two blocks and X20 three.
struct MonRf
{
    static constexpr MessageId ID = Msg::MON_RF;
    MonRfBlock block;
};

/// SEC-SIG header: v2 and v3 flags, or v1 jamFlags; the per-band groups that follow are not read.
struct SecSig
{
    static constexpr MessageId ID = Msg::SEC_SIG;
    uint8_t version;
    uint8_t flags;
    uint8_t jamFlags;
};

struct MonVer
{
    uint8_t swVersion[30];
    uint8_t hwVersion[10];
};

struct MonVerExtension
{
    uint8_t extension[30];
};

struct RxmRtcm
{
    static constexpr MessageId ID = Msg::RXM_RTCM;
    uint8_t flags;  ///< crcFailed in bit 0, msgUsed in bits 2..1
};

struct RxmCor
{
    static constexpr MessageId ID = Msg::RXM_COR;
    uint32_t statusInfo;  ///< protocol, errStatus and msgUsed
};

/// ACK-ACK and ACK-NAK.
struct Ack
{
    static constexpr MessageId ID = Msg::ACK_ACK;
    uint16_t msg;
};

inline constexpr uint8_t MON_COMMS_MAX_PORTS = 8;

struct MonCommsPort
{
    uint16_t portId;
    uint16_t txPending;
    uint8_t txUsage;
    uint8_t txPeakUsage;
    uint16_t rxPending;
    uint8_t rxUsage;
    uint16_t overrunErrs;
    uint32_t skipped;
};

struct MonComms
{
    static constexpr MessageId ID = Msg::MON_COMMS;
    uint8_t version;
    uint8_t nPorts;
    uint8_t txErrors;
    MonCommsPort ports[MON_COMMS_MAX_PORTS];
};

struct CfgPrt
{
    uint8_t portID = 0;
    uint32_t mode = 0;
    uint32_t baudRate = 0;
    uint16_t inProtoMask = 0;
    uint16_t outProtoMask = 0;
};

struct CfgRate
{
    uint16_t measRate = 0;  ///< ms
    uint16_t navRate = 0;   ///< measurement cycles
    uint16_t timeRef = 0;   ///< 0 UTC, 1 GPS time
};

struct CfgNav5
{
    uint16_t mask = 0;
    uint8_t dynModel = 0;
    uint8_t fixMode = 0;
};

struct CfgMsg
{
    uint16_t msg = 0;
    uint8_t rate = 0;
};

/// A CFG-MSG poll reply: the message's output rate on each I/O port.
struct CfgMsgRates
{
    static constexpr MessageId ID = Msg::CFG_MSG;
    uint16_t msg;
    uint8_t rates[6];
};

struct CfgTmode3
{
    uint16_t flags = 0;  ///< mode in bits 7..0, lla in bit 8
    int32_t ecefXOrLat = 0;
    int32_t ecefYOrLon = 0;
    int32_t ecefZOrAlt = 0;
    int8_t ecefXOrLatHP = 0;
    int8_t ecefYOrLonHP = 0;
    int8_t ecefZOrAltHP = 0;
    uint32_t fixedPosAcc = 0;
    uint32_t svinMinDur = 0;
    uint32_t svinAccLimit = 0;
};

}  // namespace UBX

namespace Wire {
template <>
struct Layout<UBX::NavPosllh>
{
    using T = UBX::NavPosllh;
    static constexpr size_t SIZE = 28;
    static constexpr auto FIELDS = std::tuple{Field<&T::lon, 4>{},   Field<&T::lat, 8>{},   Field<&T::height, 12>{},
                                              Field<&T::hMSL, 16>{}, Field<&T::hAcc, 20>{}, Field<&T::vAcc, 24>{}};
};

template <>
struct Layout<UBX::NavHpposllh>
{
    using T = UBX::NavHpposllh;
    static constexpr size_t SIZE = 36;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::flags, 3>{},   Field<&T::lon, 8>{},    Field<&T::lat, 12>{},   Field<&T::height, 16>{},
                   Field<&T::hMSL, 20>{},   Field<&T::lonHp, 24>{}, Field<&T::latHp, 25>{}, Field<&T::heightHp, 26>{},
                   Field<&T::hMSLHp, 27>{}, Field<&T::hAcc, 28>{},  Field<&T::vAcc, 32>{}};
};

template <>
struct Layout<UBX::NavDop>
{
    using T = UBX::NavDop;
    static constexpr size_t SIZE = 18;
    static constexpr auto FIELDS = std::tuple{Field<&T::vDOP, 10>{}, Field<&T::hDOP, 12>{}};
};

template <>
struct Layout<UBX::NavSol>
{
    using T = UBX::NavSol;
    static constexpr size_t SIZE = 52;
    static constexpr auto FIELDS = std::tuple{Field<&T::gpsFix, 10>{}, Field<&T::flags, 11>{}, Field<&T::numSV, 47>{}};
};

template <>
struct Layout<UBX::NavPvt>
{
    using T = UBX::NavPvt;
    static constexpr size_t SIZE = 92;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::year, 4>{},     Field<&T::month, 6>{},   Field<&T::day, 7>{},     Field<&T::hour, 8>{},
                   Field<&T::min, 9>{},      Field<&T::sec, 10>{},    Field<&T::valid, 11>{},  Field<&T::nano, 16>{},
                   Field<&T::fixType, 20>{}, Field<&T::flags, 21>{},  Field<&T::numSV, 23>{},  Field<&T::lon, 24>{},
                   Field<&T::lat, 28>{},     Field<&T::height, 32>{}, Field<&T::hMSL, 36>{},   Field<&T::hAcc, 40>{},
                   Field<&T::vAcc, 44>{},    Field<&T::gSpeed, 60>{}, Field<&T::headMot, 64>{}};
};

template <>
struct Layout<UBX::NavTimeUtc>
{
    using T = UBX::NavTimeUtc;
    static constexpr size_t SIZE = 20;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::nano, 8>{},  Field<&T::year, 12>{}, Field<&T::month, 14>{}, Field<&T::day, 15>{},
                   Field<&T::hour, 16>{}, Field<&T::min, 17>{},  Field<&T::sec, 18>{},   Field<&T::valid, 19>{}};
};

template <>
struct Layout<UBX::NavVelned>
{
    using T = UBX::NavVelned;
    static constexpr size_t SIZE = 36;
    static constexpr auto FIELDS = std::tuple{Field<&T::gSpeed, 20>{}, Field<&T::heading, 24>{}};
};

template <>
struct Layout<UBX::NavStatus>
{
    using T = UBX::NavStatus;
    static constexpr size_t SIZE = 16;
    static constexpr auto FIELDS = std::tuple{Field<&T::flags2, 7>{}};
};

template <>
struct Layout<UBX::NavSvin>
{
    using T = UBX::NavSvin;
    static constexpr size_t SIZE = 40;
    static constexpr auto FIELDS = std::tuple{
        Field<&T::dur, 8>{},      Field<&T::meanX, 12>{},   Field<&T::meanY, 16>{},   Field<&T::meanZ, 20>{},
        Field<&T::meanXHP, 24>{}, Field<&T::meanYHP, 25>{}, Field<&T::meanZHP, 26>{}, Field<&T::meanAcc, 28>{},
        Field<&T::valid, 36>{},   Field<&T::active, 37>{}};
};

template <>
struct Layout<UBX::NavRelposned>
{
    using T = UBX::NavRelposned;
    static constexpr size_t SIZE = 64;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::relPosLength, 20>{}, Field<&T::relPosHeading, 24>{}, Field<&T::relPosHPLength, 35>{},
                   Field<&T::accHeading, 52>{}, Field<&T::flags, 60>{}};
};

template <>
struct Layout<UBX::NavDaheading>
{
    using T = UBX::NavDaheading;
    static constexpr size_t SIZE = 60;
    static constexpr auto FIELDS = std::tuple{Field<&T::relPosLength, 20>{}, Field<&T::relPosHeading, 24>{},
                                              Field<&T::accHeading, 48>{}, Field<&T::flags, 56>{}};
};

template <>
struct Layout<UBX::NavSat>
{
    using T = UBX::NavSat;
    static constexpr size_t SIZE = 8;
    static constexpr auto FIELDS = std::tuple{Field<&T::numSvs, 5>{}};
};

template <>
struct Layout<UBX::NavSatSatellite>
{
    using T = UBX::NavSatSatellite;
    static constexpr size_t SIZE = 12;
    static constexpr auto FIELDS = std::tuple{Field<&T::gnssId, 0>{}, Field<&T::flags, 8>{}};
};

template <>
struct Layout<UBX::NavSvinfo>
{
    using T = UBX::NavSvinfo;
    static constexpr size_t SIZE = 8;
    static constexpr auto FIELDS = std::tuple{Field<&T::numCh, 4>{}};
};

template <>
struct Layout<UBX::NavSvinfoChannel>
{
    using T = UBX::NavSvinfoChannel;
    static constexpr size_t SIZE = 12;
    static constexpr auto FIELDS = std::tuple{Field<&T::flags, 2>{}};
};

template <>
struct Layout<UBX::MonHw6>
{
    using T = UBX::MonHw6;
    static constexpr size_t SIZE = 68;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::noisePerMS, 16>{}, Field<&T::agcCnt, 18>{}, Field<&T::jamInd, 53>{}};
};

template <>
struct Layout<UBX::MonHw7>
{
    using T = UBX::MonHw7;
    static constexpr size_t SIZE = 60;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::noisePerMS, 16>{}, Field<&T::agcCnt, 18>{}, Field<&T::jamInd, 45>{}};
};

template <>
struct Layout<UBX::MonRfBlock>
{
    using T = UBX::MonRfBlock;
    static constexpr size_t SIZE = 24;
    static constexpr auto FIELDS = std::tuple{Field<&T::flags, 1>{}, Field<&T::noisePerMS, 12>{},
                                              Field<&T::agcCnt, 14>{}, Field<&T::jamInd, 16>{}};
};

template <>
struct Layout<UBX::MonRf>
{
    using T = UBX::MonRf;
    static constexpr size_t SIZE = 28;
    static constexpr auto FIELDS = std::tuple{Field<&T::block, 4>{}};
};

template <>
struct Layout<UBX::SecSig>
{
    using T = UBX::SecSig;
    static constexpr size_t SIZE = 5;
    static constexpr auto FIELDS = std::tuple{Field<&T::version, 0>{}, Field<&T::flags, 1>{}, Field<&T::jamFlags, 4>{}};
};

template <>
struct Layout<UBX::MonVer>
{
    using T = UBX::MonVer;
    static constexpr size_t SIZE = 40;
    static constexpr auto FIELDS = std::tuple{Field<&T::swVersion, 0>{}, Field<&T::hwVersion, 30>{}};
};

template <>
struct Layout<UBX::MonVerExtension>
{
    using T = UBX::MonVerExtension;
    static constexpr size_t SIZE = 30;
    static constexpr auto FIELDS = std::tuple{Field<&T::extension, 0>{}};
};

template <>
struct Layout<UBX::RxmRtcm>
{
    using T = UBX::RxmRtcm;
    static constexpr size_t SIZE = 8;
    static constexpr auto FIELDS = std::tuple{Field<&T::flags, 1>{}};
};

template <>
struct Layout<UBX::RxmCor>
{
    using T = UBX::RxmCor;
    static constexpr size_t SIZE = 12;
    static constexpr auto FIELDS = std::tuple{Field<&T::statusInfo, 4>{}};
};

template <>
struct Layout<UBX::Ack>
{
    using T = UBX::Ack;
    static constexpr size_t SIZE = 2;
    static constexpr auto FIELDS = std::tuple{Field<&T::msg, 0>{}};
};

template <>
struct Layout<UBX::MonCommsPort>
{
    using T = UBX::MonCommsPort;
    static constexpr size_t SIZE = 40;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::portId, 0>{},       Field<&T::txPending, 2>{},  Field<&T::txUsage, 8>{},
                   Field<&T::txPeakUsage, 9>{},  Field<&T::rxPending, 10>{}, Field<&T::rxUsage, 16>{},
                   Field<&T::overrunErrs, 18>{}, Field<&T::skipped, 36>{}};
};

template <>
struct Layout<UBX::MonComms>
{
    using T = UBX::MonComms;
    static constexpr size_t SIZE = 328;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::version, 0>{}, Field<&T::nPorts, 1>{}, Field<&T::txErrors, 2>{}, Field<&T::ports, 8>{}};
};

template <>
struct Layout<UBX::CfgPrt>
{
    using T = UBX::CfgPrt;
    static constexpr size_t SIZE = 20;
    static constexpr auto FIELDS = std::tuple{Field<&T::portID, 0>{}, Field<&T::mode, 4>{}, Field<&T::baudRate, 8>{},
                                              Field<&T::inProtoMask, 12>{}, Field<&T::outProtoMask, 14>{}};
};

template <>
struct Layout<UBX::CfgRate>
{
    using T = UBX::CfgRate;
    static constexpr size_t SIZE = 6;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::measRate, 0>{}, Field<&T::navRate, 2>{}, Field<&T::timeRef, 4>{}};
};

template <>
struct Layout<UBX::CfgNav5>
{
    using T = UBX::CfgNav5;
    static constexpr size_t SIZE = 36;
    static constexpr auto FIELDS = std::tuple{Field<&T::mask, 0>{}, Field<&T::dynModel, 2>{}, Field<&T::fixMode, 3>{}};
};

template <>
struct Layout<UBX::CfgMsg>
{
    using T = UBX::CfgMsg;
    static constexpr size_t SIZE = 3;
    static constexpr auto FIELDS = std::tuple{Field<&T::msg, 0>{}, Field<&T::rate, 2>{}};
};

template <>
struct Layout<UBX::CfgMsgRates>
{
    using T = UBX::CfgMsgRates;
    static constexpr size_t SIZE = 8;
    static constexpr auto FIELDS = std::tuple{Field<&T::msg, 0>{}, Field<&T::rates, 2>{}};
};

template <>
struct Layout<UBX::CfgTmode3>
{
    using T = UBX::CfgTmode3;
    static constexpr size_t SIZE = 40;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::flags, 2>{},         Field<&T::ecefXOrLat, 4>{},    Field<&T::ecefYOrLon, 8>{},
                   Field<&T::ecefZOrAlt, 12>{},   Field<&T::ecefXOrLatHP, 16>{}, Field<&T::ecefYOrLonHP, 17>{},
                   Field<&T::ecefZOrAltHP, 18>{}, Field<&T::fixedPosAcc, 20>{},  Field<&T::svinMinDur, 24>{},
                   Field<&T::svinAccLimit, 28>{}};
};
}  // namespace Wire

namespace UBX {
template <Wire::Record T>
inline constexpr size_t WIRE_SIZE = Wire::SIZE<T>;

inline constexpr size_t MAX_CONTROL_PAYLOAD_SIZE = 328;
inline constexpr size_t MON_HW_DEPRECATED_SIZE = 56;
inline constexpr double DEGREES_PER_COORDINATE = 1e-7;
inline constexpr float DOP_PER_UNIT = 0.01f;

/// Converts a 1e-7 degree coordinate; values beyond +/-@a limitDegrees come from a corrupt or
/// uninitialised solution and are reported as unavailable (NaN).
[[nodiscard]] inline double coordinateDegrees(int32_t value, int64_t limitDegrees)
{
    return std::llabs(value) <= limitDegrees * 10'000'000 ? value * DEGREES_PER_COORDINATE
                                                          : std::numeric_limits<double>::quiet_NaN();
}

[[nodiscard]] inline double latitudeDegrees(int32_t value)
{
    return coordinateDegrees(value, 90);
}

[[nodiscard]] inline double longitudeDegrees(int32_t value)
{
    return coordinateDegrees(value, 180);
}

/// Payload sizes a message may have: minimum plus a multiple of stride, up to maximum. towOffset locates the GPS time
/// of week of navigation messages that belong to an epoch, or is -1.
struct MessageSchema
{
    uint16_t message;
    size_t minimum;
    size_t maximum;
    size_t stride;
    int towOffset;
};

template <typename T>
[[nodiscard]] constexpr MessageSchema fixedSchema(int towOffset = -1)
{
    return {T::ID.value(), WIRE_SIZE<T>, WIRE_SIZE<T>, 1, towOffset};
}

template <typename Header, typename Block>
[[nodiscard]] constexpr MessageSchema repeatedSchema(MessageId message, size_t maximumBlocks)
{
    return {message.value(), WIRE_SIZE<Header>, WIRE_SIZE<Header> + WIRE_SIZE<Block> * maximumBlocks, WIRE_SIZE<Block>,
            -1};
}

inline constexpr std::array MESSAGE_SCHEMAS = {
    MessageSchema{Msg::NAV_PVT.value(), 84, WIRE_SIZE<NavPvt>, 8, 0},
    fixedSchema<NavPosllh>(0),
    fixedSchema<NavHpposllh>(4),
    fixedSchema<NavSol>(0),
    fixedSchema<NavStatus>(),
    fixedSchema<NavDop>(0),
    fixedSchema<NavRelposned>(4),
    fixedSchema<NavDaheading>(4),
    fixedSchema<NavTimeUtc>(0),
    fixedSchema<NavVelned>(0),
    fixedSchema<NavSvin>(),
    MessageSchema{Msg::NAV_EOE.value(), 4, 4, 1, 0},
    repeatedSchema<NavSat, NavSatSatellite>(Msg::NAV_SAT, 255),
    repeatedSchema<NavSvinfo, NavSvinfoChannel>(Msg::NAV_SVINFO, 255),
    MessageSchema{Msg::MON_VER.value(), WIRE_SIZE<MonVer>, 4090, WIRE_SIZE<MonVerExtension>, -1},
    MessageSchema{Msg::MON_HW.value(), MON_HW_DEPRECATED_SIZE, WIRE_SIZE<MonHw6>, 4, -1},
    repeatedSchema<MonRf, MonRfBlock>(Msg::MON_RF, 254),
    MessageSchema{Msg::MON_COMMS.value(), 8, WIRE_SIZE<MonComms>, WIRE_SIZE<MonCommsPort>, -1},
    MessageSchema{Msg::SEC_SIG.value(), 4, 4096, 1, -1},
    fixedSchema<RxmRtcm>(),
    fixedSchema<RxmCor>(),
    fixedSchema<Ack>(),
    MessageSchema{Msg::ACK_NAK.value(), WIRE_SIZE<Ack>, WIRE_SIZE<Ack>, 1, -1},
    MessageSchema{Msg::CFG_VALGET.value(), 4, MAX_CONTROL_PAYLOAD_SIZE, 1, -1},
    fixedSchema<CfgMsgRates>(),
    MessageSchema{Msg::CFG_TMODE3.value(), WIRE_SIZE<CfgTmode3>, WIRE_SIZE<CfgTmode3>, 1, -1},
};

[[nodiscard]] constexpr const MessageSchema* messageSchema(uint16_t message)
{
    for (const auto& schema : MESSAGE_SCHEMAS) {
        if (schema.message == message) {
            return &schema;
        }
    }
    return nullptr;
}

/// @param schema messageSchema(message), passed by callers that also need the schema.
[[nodiscard]] inline bool validPayload(uint16_t message, std::span<const uint8_t> payload, const MessageSchema* schema)
{
    if (!schema) {
        return true;
    }
    if (payload.size() < schema->minimum || payload.size() > schema->maximum ||
        (payload.size() - schema->minimum) % schema->stride != 0) {
        return false;
    }
    switch (message) {
        case Msg::NAV_SAT.value():
            return payload.size() == schema->minimum + schema->stride * size_t(payload[5]) && payload[4] == 1;
        case Msg::NAV_SVINFO.value():
            return payload.size() == schema->minimum + schema->stride * size_t(payload[4]);
        case Msg::MON_HW.value():
            return payload.size() == MON_HW_DEPRECATED_SIZE || payload.size() == WIRE_SIZE<MonHw7> ||
                   payload.size() == WIRE_SIZE<MonHw6>;
        case Msg::MON_RF.value():
            return payload[0] == 0 && payload[1] > 0 &&
                   payload.size() == schema->minimum + schema->stride * (size_t(payload[1]) - 1);
        case Msg::MON_COMMS.value():
            return payload[0] == 0 && payload.size() == schema->minimum + schema->stride * size_t(payload[1]);
        case Msg::SEC_SIG.value():
            return payload[0] == 1
                       ? payload.size() >= 5
                       : (payload[0] == 2 || payload[0] == 3) && payload.size() == 4 + 4 * size_t(payload[3]);
        case Msg::RXM_COR.value():
            return payload[0] == 1;
        default:
            return true;
    }
}

[[nodiscard]] inline bool validPayload(uint16_t message, std::span<const uint8_t> payload)
{
    return validPayload(message, payload, messageSchema(message));
}

/// Decodes records after checking the payload against its message schema, or, for records without a message ID
/// such as repeated blocks, against the record size.
template <Wire::Record T>
struct MessageCodec
{
    [[nodiscard]] static std::optional<T> decode(std::span<const uint8_t> bytes)
    {
        if constexpr (requires { T::ID; }) {
            if (!validPayload(T::ID.value(), bytes)) {
                return std::nullopt;
            }
            // MON-HW layouts share one message; the size selects the layout.
            if constexpr (std::is_same_v<T, MonHw6> || std::is_same_v<T, MonHw7>) {
                if (bytes.size() != WIRE_SIZE<T>) {
                    return std::nullopt;
                }
            }
        } else if (bytes.size() != WIRE_SIZE<T>) {
            return std::nullopt;
        }
        return Wire::decode<T>(bytes);
    }

    [[nodiscard]] static std::optional<T> block(std::span<const uint8_t> bytes, size_t offset = 0)
    {
        if (offset > bytes.size() || WIRE_SIZE<T> > bytes.size() - offset) {
            return std::nullopt;
        }
        return decode(bytes.subspan(offset, WIRE_SIZE<T>));
    }
};
}  // namespace UBX
