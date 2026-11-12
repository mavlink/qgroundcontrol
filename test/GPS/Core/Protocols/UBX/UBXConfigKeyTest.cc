#include "UBXConfigKeyTest.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <set>
#include <span>
#include <type_traits>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QHash>

#include "GPSProtocolRuntime.h"
#include "GPSReceiverFamilies.h"
#include "LittleEndian.h"
#include "Protocols/Support/GPSBaseStationConfigs.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/Support/UBXReceiverModel.h"
#include "Protocols/fixtures/UBXConfigKeyDatabase.h"
#include "UBX/UBXConfigKeys.h"
#include "UBX/UBXFrame.h"
#include "UBX/UBXPlan.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {

using DatabaseKey = UBXConfigKeyDatabase::Key;

/// A key constant of the driver under its name in the interface descriptions.
struct NamedKey
{
    const char* name;
    uint32_t id;
    /// The size of the value type the driver writes, and whether it is signed.
    size_t valueSize;
    bool valueSigned;
};

template <typename T>
constexpr NamedKey named(const char* name, UBX::CfgKey<T> key)
{
    return {name, key.id, sizeof(T), std::is_signed_v<T>};
}

// The database name of a constant is its own name after "CFG_", so no name is written twice.
#define UBX_KEY(name) named("CFG_" #name, UBX::Cfg::name)
#define UBX_OUTPUT(name)                                                           \
    named("CFG_" #name "_I2C", UBX::Cfg::name.i2c),                                \
        named("CFG_" #name "_UART1", UBX::Cfg::name.port(UBX::MsgOutPort::UART1)), \
        named("CFG_" #name "_USB", UBX::Cfg::name.port(UBX::MsgOutPort::USB))

/// Every key constant of UBXConfigKeys.h; _plannedKeys and _writtenKeys fail for a key that is missing here.
constexpr NamedKey NAMED_KEYS[] = {
    UBX_KEY(ITFM_ENABLE),
    UBX_KEY(NAVHPG_DGNSSMODE),
    UBX_KEY(NAVSPG_DYNMODEL),
    UBX_KEY(NAVSPG_FIXMODE),
    UBX_KEY(NAVSPG_UTCSTANDARD),
    UBX_KEY(ODO_OUTLPCOG),
    UBX_KEY(ODO_OUTLPVEL),
    UBX_KEY(ODO_USE_COG),
    UBX_KEY(ODO_USE_ODO),
    UBX_KEY(RATE_MEAS),
    UBX_KEY(RATE_NAV),
    UBX_KEY(RATE_TIMEREF),
    UBX_KEY(SEC_JAMDET_SENSITIVITY_HI),
    UBX_KEY(SIGNAL_HEALTH_L5),
    UBX_KEY(TMODE_FIXED_POS_ACC),
    UBX_KEY(TMODE_HEIGHT),
    UBX_KEY(TMODE_HEIGHT_HP),
    UBX_KEY(TMODE_LAT),
    UBX_KEY(TMODE_LAT_HP),
    UBX_KEY(TMODE_LON),
    UBX_KEY(TMODE_LON_HP),
    UBX_KEY(TMODE_MODE),
    UBX_KEY(TMODE_POS_TYPE),
    UBX_KEY(TMODE_SVIN_ACC_LIMIT),
    UBX_KEY(TMODE_SVIN_MIN_DUR),
    UBX_KEY(UART1INPROT_NMEA),
    UBX_KEY(UART1INPROT_RTCM3X),
    UBX_KEY(UART1INPROT_SPARTN),
    UBX_KEY(UART1INPROT_UBX),
    UBX_KEY(UART1OUTPROT_NMEA),
    UBX_KEY(UART1OUTPROT_RTCM3X),
    UBX_KEY(UART1OUTPROT_UBX),
    UBX_KEY(UART1_BAUDRATE),
    UBX_KEY(UART1_DATABITS),
    UBX_KEY(UART1_PARITY),
    UBX_KEY(UART1_STOPBITS),
    UBX_KEY(USBINPROT_NMEA),
    UBX_KEY(USBINPROT_RTCM3X),
    UBX_KEY(USBINPROT_SPARTN),
    UBX_KEY(USBINPROT_UBX),
    UBX_KEY(USBOUTPROT_NMEA),
    UBX_KEY(USBOUTPROT_RTCM3X),
    UBX_KEY(USBOUTPROT_UBX),
    UBX_OUTPUT(MSGOUT_RTCM_3X_TYPE1005),
    UBX_OUTPUT(MSGOUT_RTCM_3X_TYPE1074),
    UBX_OUTPUT(MSGOUT_RTCM_3X_TYPE1077),
    UBX_OUTPUT(MSGOUT_RTCM_3X_TYPE1084),
    UBX_OUTPUT(MSGOUT_RTCM_3X_TYPE1087),
    UBX_OUTPUT(MSGOUT_RTCM_3X_TYPE1094),
    UBX_OUTPUT(MSGOUT_RTCM_3X_TYPE1097),
    UBX_OUTPUT(MSGOUT_RTCM_3X_TYPE1124),
    UBX_OUTPUT(MSGOUT_RTCM_3X_TYPE1127),
    UBX_OUTPUT(MSGOUT_RTCM_3X_TYPE1230),
    UBX_OUTPUT(MSGOUT_UBX_MON_RF),
    UBX_OUTPUT(MSGOUT_UBX_NAV_DOP),
    UBX_OUTPUT(MSGOUT_UBX_NAV_EOE),
    UBX_OUTPUT(MSGOUT_UBX_NAV_HPPOSLLH),
    UBX_OUTPUT(MSGOUT_UBX_NAV_PVT),
    UBX_OUTPUT(MSGOUT_UBX_NAV_RELPOSNED),
    UBX_OUTPUT(MSGOUT_UBX_NAV_SAT),
    UBX_OUTPUT(MSGOUT_UBX_NAV_STATUS),
    UBX_OUTPUT(MSGOUT_UBX_NAV_SVIN),
    UBX_OUTPUT(MSGOUT_UBX_NAV_TIMEGPS),
    UBX_OUTPUT(MSGOUT_UBX_RXM_RAWX),
    UBX_OUTPUT(MSGOUT_UBX_RXM_SFRBX),
    UBX_OUTPUT(MSGOUT_UBX_SEC_SIG),
};

#undef UBX_KEY
#undef UBX_OUTPUT

const DatabaseKey* databaseKey(const char* name)
{
    for (const auto& key : UBXConfigKeyDatabase::keys) {
        if (std::strcmp(key.name, name) == 0) {
            return &key;
        }
    }
    return nullptr;
}

/// The database keys by id. A key renamed between interface descriptions has one id under both names.
const QHash<uint32_t, const DatabaseKey*>& databaseKeys()
{
    static const QHash<uint32_t, const DatabaseKey*> keys = [] {
        QHash<uint32_t, const DatabaseKey*> result;
        for (const auto& key : UBXConfigKeyDatabase::keys) {
            result.insert(key.id, &key);
        }
        return result;
    }();
    return keys;
}

bool isNamed(uint32_t id)
{
    return std::ranges::any_of(NAMED_KEYS, [id](const NamedKey& key) { return key.id == id; });
}

GPSBaseStationConfig surveyIn()
{
    return surveyInBase(1.25, 60s);
}

GPSBaseStationConfig fixedBase()
{
    return fixedBaseAt(-47.123456789, 8.987654321, 512.3456, 0.25f);
}

/// The CFG-VALSET and CFG-VALGET poll frames among @a commands, as the payload after the four header bytes.
struct KeyFrames
{
    std::vector<QByteArray> valsets;
    std::vector<QByteArray> polls;
};

KeyFrames keyFrames(const QList<QByteArray>& commands)
{
    constexpr uint8_t CFG_VALSET = 0x8a;
    constexpr uint8_t CFG_VALGET = 0x8b;
    KeyFrames frames;
    for (const QByteArray& command : commands) {
        if (command.size() < static_cast<qsizetype>(UBX::FRAME_OVERHEAD + UBX::CONFIGURATION_HEADER_SIZE) ||
            static_cast<uint8_t>(command[2]) != UBX::MsgClass::CFG) {
            continue;
        }
        const QByteArray entries = command.mid(UBX::HEADER_SIZE + UBX::CONFIGURATION_HEADER_SIZE,
                                               command.size() - UBX::FRAME_OVERHEAD - UBX::CONFIGURATION_HEADER_SIZE);
        if (static_cast<uint8_t>(command[3]) == CFG_VALSET) {
            frames.valsets.push_back(entries);
        } else if (static_cast<uint8_t>(command[3]) == CFG_VALGET) {
            frames.polls.push_back(entries);
        }
    }
    return frames;
}

}  // namespace

void UBXConfigKeyTest::_valueSizes()
{
    QVERIFY(std::size(UBXConfigKeyDatabase::keys) > 1000);
    for (const auto& key : UBXConfigKeyDatabase::keys) {
        // The size bits of every key id of the interface descriptions give the size of its value.
        QVERIFY2(UBX::cfgKeyValueBytes(key.id) == key.size, key.name);
        QVERIFY2(UBX::cfgKeyIsBoolean(key.id) == (key.type == 'L'), key.name);
        // The driver reads and writes values of up to four bytes.
        QVERIFY2(UBX::configurationValueBytes(key.id) == (key.size <= 4 ? key.size : 0), key.name);
    }
}

void UBXConfigKeyTest::_namedKeys_data()
{
    QTest::addColumn<int>("index");
    for (size_t index = 0; index < std::size(NAMED_KEYS); ++index) {
        QTest::newRow(NAMED_KEYS[index].name) << static_cast<int>(index);
    }
}

void UBXConfigKeyTest::_namedKeys()
{
    QFETCH(int, index);
    const NamedKey& key = NAMED_KEYS[index];
    const DatabaseKey* expected = databaseKey(key.name);
    QVERIFY2(expected, "The key database has no key of this name");
    QCOMPARE(key.id, expected->id);
    QCOMPARE(key.valueSize, size_t(expected->size));
    QCOMPARE(key.valueSigned, expected->type == 'I');
}

void UBXConfigKeyTest::_plannedKeys()
{
    namespace Plan = UBX::Plan;
    std::vector<Plan::ValsetBatch> batches{
        Plan::uart1Protocols(),
        Plan::uart1BaudRate(Plan::BASE_BAUD),
        Plan::jammingDetection(),
        Plan::interferenceMonitor(),
        Plan::disableRTCMOutput(),
        Plan::disableTimeMode(),
        Plan::surveyIn(std::get<GPSBaseStationConfig::SurveyIn>(surveyIn().mode)),
        Plan::fixedBase(std::get<GPSBaseStationConfig::Fixed>(fixedBase().mode)),
        Plan::rtcmOutput({}),
        Plan::rtcmOutput({.compactObservations = true}),
    };
    for (const auto& profile : UBX::RECEIVER_PROFILES) {
        for (auto& batch : Plan::portsAndNavigation(profile)) {
            batches.push_back(std::move(batch));
        }
        for (auto& batch : Plan::messageOutput(profile)) {
            batches.push_back(std::move(batch));
        }
    }
    for (const auto& output : Plan::UNEXPECTED_OUTPUT) {
        batches.push_back(Plan::disableMessage(output.key));
    }
    std::set<uint32_t> planned;
    for (const auto& batch : batches) {
        QVERIFY(!batch.items.empty());
        for (const auto& item : batch.items) {
            const std::vector<uint32_t> ids =
                item.msgOut ? std::vector<uint32_t>{UBX::MsgOutKey{{item.key}}.port(UBX::MsgOutPort::UART1).id,
                                                    UBX::MsgOutKey{{item.key}}.port(UBX::MsgOutPort::USB).id}
                            : std::vector<uint32_t>{item.key};
            for (const uint32_t id : ids) {
                const auto found = databaseKeys().constFind(id);
                QVERIFY2(found != databaseKeys().constEnd(), qPrintable(QString::number(id, 16)));
                QVERIFY2(isNamed(id), (*found)->name);
                // A value wider than the key would be cut off on the wire.
                QVERIFY2((*found)->size == 4 || item.value < (1u << (8 * (*found)->size)), (*found)->name);
                planned.insert(id);
            }
        }
    }
    // The plan writes every named key but the I2C keys, which only name the CFG-MSGOUT families.
    const auto writable = std::ranges::count_if(
        NAMED_KEYS, [](const NamedKey& key) { return !QByteArrayView(key.name).endsWith("_I2C"); });
    QCOMPARE(std::ssize(planned), writable);
}

void UBXConfigKeyTest::_writtenKeys_data()
{
    QTest::addColumn<bool>("fixed");
    QTest::addColumn<bool>("usb");
    QTest::addColumn<bool>("jammingDetection");
    QTest::newRow("survey-in on UART1") << false << false << true;
    QTest::newRow("fixed base on USB") << true << true << true;
    QTest::newRow("fixed base without CFG-SEC-JAMDET") << true << false << false;
}

void UBXConfigKeyTest::_writtenKeys()
{
    QFETCH(bool, fixed);
    QFETCH(bool, usb);
    QFETCH(bool, jammingDetection);
    // Firmware without CFG-SEC-JAMDET gets the interference monitor instead, with a warning.
    const GPSProtocolLogCapture log;
    GPSTestClock clock(GPSTestClock::START_US);
    ModelReceiver<UBXReceiverModel> receiver(clock);
    receiver.model.usb = usb;
    if (!jammingDetection) {
        receiver.model.unsupportedKeys = {UBX::Cfg::SEC_JAMDET_SENSITIVITY_HI.id};
    }
    GPSProtocolRuntime runtime = receiver.runtime(UBX::FAMILY);
    GPSConfig config;
    config.base = fixed ? fixedBase() : surveyIn();
    unsigned baud = 115200;
    QVERIFY2(runtime.configure(config, baud), qPrintable(runtime.errorDetail()));

    const KeyFrames frames = keyFrames(receiver.commands());
    QVERIFY(frames.valsets.size() > 5);
    std::set<uint32_t> written;
    for (const QByteArray& entries : frames.valsets) {
        // Each entry is a key and a value of the size the database gives the key, to the end of the payload.
        qsizetype offset = 0;
        while (offset < entries.size()) {
            const auto id = LittleEndian::read<uint32_t>(bytesOf(entries), static_cast<size_t>(offset));
            QVERIFY(id);
            const auto found = databaseKeys().constFind(*id);
            QVERIFY2(found != databaseKeys().constEnd(), qPrintable(QString::number(*id, 16)));
            QVERIFY2(isNamed(*id), (*found)->name);
            offset += sizeof(uint32_t) + (*found)->size;
            written.insert(*id);
        }
        QCOMPARE(offset, entries.size());
    }
    for (const QByteArray& keys : frames.polls) {
        QCOMPARE(keys.size() % sizeof(uint32_t), 0);
        for (qsizetype offset = 0; offset < keys.size(); offset += sizeof(uint32_t)) {
            const auto id = LittleEndian::read<uint32_t>(bytesOf(keys), static_cast<size_t>(offset));
            QVERIFY(id);
            QVERIFY2(databaseKeys().contains(*id), qPrintable(QString::number(*id, 16)));
            QVERIFY2(isNamed(*id), databaseKeys().value(*id)->name);
        }
    }
    QVERIFY(written.contains(UBX::Cfg::TMODE_MODE.id));
    QCOMPARE(written.contains(UBX::Cfg::TMODE_LAT.id), fixed);
    QCOMPARE(written.contains(UBX::Cfg::ITFM_ENABLE.id), !jammingDetection);
}

UT_REGISTER_TEST_LIGHTWEIGHT(UBXConfigKeyTest, TestLabel::Unit)
