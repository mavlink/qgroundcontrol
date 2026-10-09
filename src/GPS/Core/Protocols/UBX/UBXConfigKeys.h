// Configuration keys as the u-blox interface descriptions (F9 HPG, M10, X20) define them.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>

#include "LittleEndian.h"

namespace UBX {

/// Value size in bytes from bits 28-30 of a configuration key id, or 0 for a reserved size.
[[nodiscard]] constexpr size_t cfgKeyValueBytes(uint32_t keyId)
{
    switch ((keyId >> 28) & 0x7) {
        case 1:  // A one-bit value still occupies a byte.
        case 2:
            return 1;
        case 3:
            return 2;
        case 4:
            return 4;
        case 5:
            return 8;
        default:
            return 0;
    }
}

/// A configuration key whose value is written and read as T.
template <typename T>
struct CfgKey
{
    static_assert(std::is_arithmetic_v<T>);

    uint32_t id;
};

/// Builds every key below, so each one checks its value type against the size bits of its id.
template <typename T, uint32_t Id>
[[nodiscard]] consteval CfgKey<T> cfgKey()
{
    static_assert(cfgKeyValueBytes(Id) == sizeof(T), "the value type size must match the key id size bits");
    return {Id};
}

/// A port's CFG-MSGOUT key is the family's I2C key id plus the port's offset.
enum class MsgOutPort : uint32_t
{
    UART1 = 1,
    USB = 3,
};

/// A CFG-MSGOUT output rate key family, named by its I2C key.
struct MsgOutKey
{
    CfgKey<uint8_t> i2c;

    [[nodiscard]] constexpr CfgKey<uint8_t> port(MsgOutPort port) const
    {
        return {i2c.id + static_cast<uint32_t>(port)};
    }
};

}  // namespace UBX

// Each key is followed by its type in the interface description.
namespace UBX::Cfg {
inline constexpr auto ITFM_ENABLE = cfgKey<uint8_t, 0x1041000d>();                // L
inline constexpr auto NAVHPG_DGNSSMODE = cfgKey<uint8_t, 0x20140011>();           // E1
inline constexpr auto NAVSPG_DYNMODEL = cfgKey<uint8_t, 0x20110021>();            // E1
inline constexpr auto NAVSPG_FIXMODE = cfgKey<uint8_t, 0x20110011>();             // E1
inline constexpr auto NAVSPG_UTCSTANDARD = cfgKey<uint8_t, 0x2011001c>();         // E1
inline constexpr auto ODO_OUTLPCOG = cfgKey<uint8_t, 0x10220004>();               // L
inline constexpr auto ODO_OUTLPVEL = cfgKey<uint8_t, 0x10220003>();               // L
inline constexpr auto ODO_USE_COG = cfgKey<uint8_t, 0x10220002>();                // L
inline constexpr auto ODO_USE_ODO = cfgKey<uint8_t, 0x10220001>();                // L
inline constexpr auto RATE_MEAS = cfgKey<uint16_t, 0x30210001>();                 // U2
inline constexpr auto RATE_NAV = cfgKey<uint16_t, 0x30210002>();                  // U2
inline constexpr auto RATE_TIMEREF = cfgKey<uint8_t, 0x20210003>();               // E1
inline constexpr auto SEC_JAMDET_SENSITIVITY_HI = cfgKey<uint8_t, 0x10f60051>();  // L
inline constexpr auto SIGNAL_HEALTH_L5 = cfgKey<uint8_t, 0x10320001>();           // L
inline constexpr auto TMODE_FIXED_POS_ACC = cfgKey<uint32_t, 0x4003000f>();       // U4
inline constexpr auto TMODE_HEIGHT = cfgKey<int32_t, 0x4003000b>();               // I4
inline constexpr auto TMODE_HEIGHT_HP = cfgKey<int8_t, 0x2003000e>();             // I1
inline constexpr auto TMODE_LAT = cfgKey<int32_t, 0x40030009>();                  // I4
inline constexpr auto TMODE_LAT_HP = cfgKey<int8_t, 0x2003000c>();                // I1
inline constexpr auto TMODE_LON = cfgKey<int32_t, 0x4003000a>();                  // I4
inline constexpr auto TMODE_LON_HP = cfgKey<int8_t, 0x2003000d>();                // I1
inline constexpr auto TMODE_MODE = cfgKey<uint8_t, 0x20030001>();                 // E1
inline constexpr auto TMODE_POS_TYPE = cfgKey<uint8_t, 0x20030002>();             // E1
inline constexpr auto TMODE_SVIN_ACC_LIMIT = cfgKey<uint32_t, 0x40030011>();      // U4
inline constexpr auto TMODE_SVIN_MIN_DUR = cfgKey<uint32_t, 0x40030010>();        // U4
inline constexpr auto UART1INPROT_NMEA = cfgKey<uint8_t, 0x10730002>();           // L
inline constexpr auto UART1INPROT_RTCM3X = cfgKey<uint8_t, 0x10730004>();         // L
inline constexpr auto UART1INPROT_SPARTN = cfgKey<uint8_t, 0x10730005>();         // L
inline constexpr auto UART1INPROT_UBX = cfgKey<uint8_t, 0x10730001>();            // L
inline constexpr auto UART1OUTPROT_NMEA = cfgKey<uint8_t, 0x10740002>();          // L
inline constexpr auto UART1OUTPROT_RTCM3X = cfgKey<uint8_t, 0x10740004>();        // L
inline constexpr auto UART1OUTPROT_UBX = cfgKey<uint8_t, 0x10740001>();           // L
inline constexpr auto UART1_BAUDRATE = cfgKey<uint32_t, 0x40520001>();            // U4
inline constexpr auto UART1_DATABITS = cfgKey<uint8_t, 0x20520003>();             // E1
inline constexpr auto UART1_PARITY = cfgKey<uint8_t, 0x20520004>();               // E1
inline constexpr auto UART1_STOPBITS = cfgKey<uint8_t, 0x20520002>();             // E1
inline constexpr auto USBINPROT_NMEA = cfgKey<uint8_t, 0x10770002>();             // L
inline constexpr auto USBINPROT_RTCM3X = cfgKey<uint8_t, 0x10770004>();           // L
inline constexpr auto USBINPROT_SPARTN = cfgKey<uint8_t, 0x10770005>();           // L
inline constexpr auto USBINPROT_UBX = cfgKey<uint8_t, 0x10770001>();              // L
inline constexpr auto USBOUTPROT_NMEA = cfgKey<uint8_t, 0x10780002>();            // L
inline constexpr auto USBOUTPROT_RTCM3X = cfgKey<uint8_t, 0x10780004>();          // L
inline constexpr auto USBOUTPROT_UBX = cfgKey<uint8_t, 0x10780001>();             // L

inline constexpr MsgOutKey MSGOUT_RTCM_3X_TYPE1005{cfgKey<uint8_t, 0x209102bd>()};
static_assert(MSGOUT_RTCM_3X_TYPE1005.port(MsgOutPort::UART1).id == 0x209102be);
static_assert(MSGOUT_RTCM_3X_TYPE1005.port(MsgOutPort::USB).id == 0x209102c0);
inline constexpr MsgOutKey MSGOUT_RTCM_3X_TYPE1074{cfgKey<uint8_t, 0x2091035e>()};
static_assert(MSGOUT_RTCM_3X_TYPE1074.port(MsgOutPort::UART1).id == 0x2091035f);
static_assert(MSGOUT_RTCM_3X_TYPE1074.port(MsgOutPort::USB).id == 0x20910361);
inline constexpr MsgOutKey MSGOUT_RTCM_3X_TYPE1077{cfgKey<uint8_t, 0x209102cc>()};
static_assert(MSGOUT_RTCM_3X_TYPE1077.port(MsgOutPort::UART1).id == 0x209102cd);
static_assert(MSGOUT_RTCM_3X_TYPE1077.port(MsgOutPort::USB).id == 0x209102cf);
inline constexpr MsgOutKey MSGOUT_RTCM_3X_TYPE1084{cfgKey<uint8_t, 0x20910363>()};
static_assert(MSGOUT_RTCM_3X_TYPE1084.port(MsgOutPort::UART1).id == 0x20910364);
static_assert(MSGOUT_RTCM_3X_TYPE1084.port(MsgOutPort::USB).id == 0x20910366);
inline constexpr MsgOutKey MSGOUT_RTCM_3X_TYPE1087{cfgKey<uint8_t, 0x209102d1>()};
static_assert(MSGOUT_RTCM_3X_TYPE1087.port(MsgOutPort::UART1).id == 0x209102d2);
static_assert(MSGOUT_RTCM_3X_TYPE1087.port(MsgOutPort::USB).id == 0x209102d4);
inline constexpr MsgOutKey MSGOUT_RTCM_3X_TYPE1094{cfgKey<uint8_t, 0x20910368>()};
static_assert(MSGOUT_RTCM_3X_TYPE1094.port(MsgOutPort::UART1).id == 0x20910369);
static_assert(MSGOUT_RTCM_3X_TYPE1094.port(MsgOutPort::USB).id == 0x2091036b);
inline constexpr MsgOutKey MSGOUT_RTCM_3X_TYPE1097{cfgKey<uint8_t, 0x20910318>()};
static_assert(MSGOUT_RTCM_3X_TYPE1097.port(MsgOutPort::UART1).id == 0x20910319);
static_assert(MSGOUT_RTCM_3X_TYPE1097.port(MsgOutPort::USB).id == 0x2091031b);
inline constexpr MsgOutKey MSGOUT_RTCM_3X_TYPE1124{cfgKey<uint8_t, 0x2091036d>()};
static_assert(MSGOUT_RTCM_3X_TYPE1124.port(MsgOutPort::UART1).id == 0x2091036e);
static_assert(MSGOUT_RTCM_3X_TYPE1124.port(MsgOutPort::USB).id == 0x20910370);
inline constexpr MsgOutKey MSGOUT_RTCM_3X_TYPE1127{cfgKey<uint8_t, 0x209102d6>()};
static_assert(MSGOUT_RTCM_3X_TYPE1127.port(MsgOutPort::UART1).id == 0x209102d7);
static_assert(MSGOUT_RTCM_3X_TYPE1127.port(MsgOutPort::USB).id == 0x209102d9);
inline constexpr MsgOutKey MSGOUT_RTCM_3X_TYPE1230{cfgKey<uint8_t, 0x20910303>()};
static_assert(MSGOUT_RTCM_3X_TYPE1230.port(MsgOutPort::UART1).id == 0x20910304);
static_assert(MSGOUT_RTCM_3X_TYPE1230.port(MsgOutPort::USB).id == 0x20910306);
inline constexpr MsgOutKey MSGOUT_UBX_MON_RF{cfgKey<uint8_t, 0x20910359>()};
static_assert(MSGOUT_UBX_MON_RF.port(MsgOutPort::UART1).id == 0x2091035a);
static_assert(MSGOUT_UBX_MON_RF.port(MsgOutPort::USB).id == 0x2091035c);
inline constexpr MsgOutKey MSGOUT_UBX_NAV_DOP{cfgKey<uint8_t, 0x20910038>()};
static_assert(MSGOUT_UBX_NAV_DOP.port(MsgOutPort::UART1).id == 0x20910039);
static_assert(MSGOUT_UBX_NAV_DOP.port(MsgOutPort::USB).id == 0x2091003b);
inline constexpr MsgOutKey MSGOUT_UBX_NAV_EOE{cfgKey<uint8_t, 0x2091015f>()};
static_assert(MSGOUT_UBX_NAV_EOE.port(MsgOutPort::UART1).id == 0x20910160);
static_assert(MSGOUT_UBX_NAV_EOE.port(MsgOutPort::USB).id == 0x20910162);
inline constexpr MsgOutKey MSGOUT_UBX_NAV_HPPOSLLH{cfgKey<uint8_t, 0x20910033>()};
static_assert(MSGOUT_UBX_NAV_HPPOSLLH.port(MsgOutPort::UART1).id == 0x20910034);
static_assert(MSGOUT_UBX_NAV_HPPOSLLH.port(MsgOutPort::USB).id == 0x20910036);
inline constexpr MsgOutKey MSGOUT_UBX_NAV_PVT{cfgKey<uint8_t, 0x20910006>()};
static_assert(MSGOUT_UBX_NAV_PVT.port(MsgOutPort::UART1).id == 0x20910007);
static_assert(MSGOUT_UBX_NAV_PVT.port(MsgOutPort::USB).id == 0x20910009);
inline constexpr MsgOutKey MSGOUT_UBX_NAV_RELPOSNED{cfgKey<uint8_t, 0x2091008d>()};
static_assert(MSGOUT_UBX_NAV_RELPOSNED.port(MsgOutPort::UART1).id == 0x2091008e);
static_assert(MSGOUT_UBX_NAV_RELPOSNED.port(MsgOutPort::USB).id == 0x20910090);
inline constexpr MsgOutKey MSGOUT_UBX_NAV_SAT{cfgKey<uint8_t, 0x20910015>()};
static_assert(MSGOUT_UBX_NAV_SAT.port(MsgOutPort::UART1).id == 0x20910016);
static_assert(MSGOUT_UBX_NAV_SAT.port(MsgOutPort::USB).id == 0x20910018);
inline constexpr MsgOutKey MSGOUT_UBX_NAV_STATUS{cfgKey<uint8_t, 0x2091001a>()};
static_assert(MSGOUT_UBX_NAV_STATUS.port(MsgOutPort::UART1).id == 0x2091001b);
static_assert(MSGOUT_UBX_NAV_STATUS.port(MsgOutPort::USB).id == 0x2091001d);
inline constexpr MsgOutKey MSGOUT_UBX_NAV_SVIN{cfgKey<uint8_t, 0x20910088>()};
static_assert(MSGOUT_UBX_NAV_SVIN.port(MsgOutPort::UART1).id == 0x20910089);
static_assert(MSGOUT_UBX_NAV_SVIN.port(MsgOutPort::USB).id == 0x2091008b);
inline constexpr MsgOutKey MSGOUT_UBX_NAV_TIMEGPS{cfgKey<uint8_t, 0x20910047>()};
static_assert(MSGOUT_UBX_NAV_TIMEGPS.port(MsgOutPort::UART1).id == 0x20910048);
static_assert(MSGOUT_UBX_NAV_TIMEGPS.port(MsgOutPort::USB).id == 0x2091004a);
inline constexpr MsgOutKey MSGOUT_UBX_RXM_RAWX{cfgKey<uint8_t, 0x209102a4>()};
static_assert(MSGOUT_UBX_RXM_RAWX.port(MsgOutPort::UART1).id == 0x209102a5);
static_assert(MSGOUT_UBX_RXM_RAWX.port(MsgOutPort::USB).id == 0x209102a7);
inline constexpr MsgOutKey MSGOUT_UBX_RXM_SFRBX{cfgKey<uint8_t, 0x20910231>()};
static_assert(MSGOUT_UBX_RXM_SFRBX.port(MsgOutPort::UART1).id == 0x20910232);
static_assert(MSGOUT_UBX_RXM_SFRBX.port(MsgOutPort::USB).id == 0x20910234);
inline constexpr MsgOutKey MSGOUT_UBX_SEC_SIG{cfgKey<uint8_t, 0x20910634>()};
static_assert(MSGOUT_UBX_SEC_SIG.port(MsgOutPort::UART1).id == 0x20910635);
static_assert(MSGOUT_UBX_SEC_SIG.port(MsgOutPort::USB).id == 0x20910637);
}  // namespace UBX::Cfg

// CFG-VALSET and CFG-VALGET key/value encoding.
namespace UBX {

/// Value width in bytes of @a key; 0 for reserved sizes and for 8-byte values, which QGC never reads or writes.
[[nodiscard]] constexpr unsigned configurationValueBytes(uint32_t key)
{
    const size_t width = cfgKeyValueBytes(key);
    return width <= sizeof(uint32_t) ? static_cast<unsigned>(width) : 0;
}

struct ConfigurationValue
{
    uint32_t key;
    uint32_t value;

    bool operator==(const ConfigurationValue&) const = default;
};

/// Reads only key/value entries; VALSET and VALGET validate their different headers separately.
class ConfigurationValueCursor
{
public:
    explicit ConfigurationValueCursor(std::span<const uint8_t> entries)
        : _remaining(entries)
    {}

    bool empty() const { return _remaining.empty(); }

    std::optional<ConfigurationValue> next()
    {
        if (!_valid || empty()) {
            return std::nullopt;
        }
        const auto key = LittleEndian::read<uint32_t>(_remaining, 0);
        const unsigned width = key ? configurationValueBytes(*key) : 0;
        if (!width || _remaining.size() < sizeof(uint32_t) + width) {
            _valid = false;
            return std::nullopt;
        }
        uint32_t value = 0;
        for (unsigned byte = 0; byte < width; ++byte) {
            value |= uint32_t(_remaining[sizeof(uint32_t) + byte]) << (8 * byte);
        }
        if ((*key >> 28) == 1 && value > 1) {
            _valid = false;
            return std::nullopt;
        }
        _remaining = _remaining.subspan(sizeof(uint32_t) + width);
        return ConfigurationValue{*key, value};
    }

private:
    std::span<const uint8_t> _remaining;
    bool _valid = true;
};

/// The values of one CFG-VALGET poll or response.
struct ConfigurationValues
{
    /// One poll reads at most this many keys.
    static constexpr size_t MAX_KEYS = 9;

    std::array<ConfigurationValue, MAX_KEYS> values{};
    size_t count = 0;
};

/// Decodes a CFG-VALGET response (version 1, RAM layer) of at most ConfigurationValues::MAX_KEYS values.
[[nodiscard]] inline std::optional<ConfigurationValues> decodeConfigurationValues(std::span<const uint8_t> payload)
{
    if (payload.size() < 4 || payload[0] != 1 || payload[1] || payload[2] || payload[3]) {
        return std::nullopt;
    }
    ConfigurationValues result;
    ConfigurationValueCursor cursor(payload.subspan(4));
    while (!cursor.empty()) {
        const auto entry = cursor.next();
        if (!entry || result.count == result.values.size()) {
            return std::nullopt;
        }
        result.values[result.count++] = *entry;
    }
    return result;
}

/// A CFG-VALSET payload for the RAM layer. An append error invalidates the entire batch, including previously
/// appended entries.
template <size_t Capacity>
struct CheckedValsetBatch
{
    static_assert(Capacity >= 4);
    std::array<uint8_t, Capacity> bytes{0, 1};  // RAM layer; no persistent writes.
    size_t size = 4;
    bool valid = true;

    std::span<const uint8_t> payload() const
    {
        return valid && size > 4 ? std::span<const uint8_t>(bytes).first(size) : std::span<const uint8_t>{};
    }

    /// The appended key/value entries, without the header.
    std::span<const uint8_t> entries() const { return std::span<const uint8_t>(bytes).subspan(4, size - 4); }

    bool append(uint32_t key, uint32_t value)
    {
        const unsigned width = configurationValueBytes(key);
        if (!valid || !width || (width < 4 && value >= (1u << (width * 8))) || ((key >> 28) == 1 && value > 1) ||
            sizeof(key) + width > bytes.size() - size) {
            valid = false;
            return false;
        }
        (void) LittleEndian::write(bytes, size, key);
        for (unsigned byte = 0; byte < width; ++byte) {
            bytes[size + sizeof(key) + byte] = static_cast<uint8_t>(value >> (8 * byte));
        }
        size += sizeof(key) + width;
        return true;
    }
};
}  // namespace UBX
