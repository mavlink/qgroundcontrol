#include "ChecksumsTest.h"

#include <array>
#include <span>

#include "CRC32.h"
#include "Checksums.h"

namespace {
template <size_t N>
constexpr std::array<uint8_t, N - 1> ascii(const char (&text)[N])
{
    std::array<uint8_t, N - 1> bytes{};
    for (size_t i = 0; i + 1 < N; ++i) {
        bytes[i] = static_cast<uint8_t>(text[i]);
    }
    return bytes;
}

constexpr auto CHECK_INPUT = ascii("123456789");
// UBX-CFG-RATE poll after the sync bytes: class, ID and a zero length (u-blox interface description).
constexpr std::array<uint8_t, 4> UBX_CFG_RATE_POLL{0x06, 0x08, 0x00, 0x00};

static_assert(QGC::fletcher8(UBX_CFG_RATE_POLL) == QGC::Fletcher8{0x0e, 0x30});
static_assert(QGC::fletcher8(CHECK_INPUT) == QGC::Fletcher8{0xdd, 0x15});
static_assert(QGC::crc16Xmodem(CHECK_INPUT) == 0x31c3);
static_assert(QGC::crc24q(CHECK_INPUT) == 0xcde703);
static_assert((QGC::crc32Update(CHECK_INPUT, 0xffffffff) ^ 0xffffffff) == 0xcbf43926);
static_assert(QGC::nmeaChecksum(ascii("PUBX,40,GLL,0,0,0,0")) == 0x5c);

std::span<const uint8_t> bytesOf(const QByteArray& bytes)
{
    return {reinterpret_cast<const uint8_t*>(bytes.constData()), static_cast<size_t>(bytes.size())};
}

quint32 checksum(const QString& algorithm, std::span<const uint8_t> bytes)
{
    if (algorithm == QLatin1String("fletcher8")) {
        const auto sum = QGC::fletcher8(bytes);
        return quint32(sum.b) << 8 | sum.a;
    }
    if (algorithm == QLatin1String("crc16Xmodem")) {
        return QGC::crc16Xmodem(bytes);
    }
    if (algorithm == QLatin1String("crc24q")) {
        return QGC::crc24q(bytes);
    }
    if (algorithm == QLatin1String("crc32")) {
        return QGC::crc32Update(bytes, 0xffffffff) ^ 0xffffffff;
    }
    return QGC::nmeaChecksum(bytes);
}
}  // namespace

void ChecksumsTest::_knownAnswers_data()
{
    QTest::addColumn<QString>("algorithm");
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<quint32>("expected");
    const QByteArray check("123456789");
    QTest::newRow("fletcher8-empty") << QStringLiteral("fletcher8") << QByteArray() << 0U;
    QTest::newRow("fletcher8-check") << QStringLiteral("fletcher8") << check << 0x15ddU;
    QTest::newRow("fletcher8-ubx-mon-ver-poll")
        << QStringLiteral("fletcher8") << QByteArray::fromHex("0a040000") << 0x340eU;
    QTest::newRow("crc16-empty") << QStringLiteral("crc16Xmodem") << QByteArray() << 0U;
    QTest::newRow("crc16-check") << QStringLiteral("crc16Xmodem") << check << 0x31c3U;
    QTest::newRow("crc24q-empty") << QStringLiteral("crc24q") << QByteArray() << 0U;
    QTest::newRow("crc24q-check") << QStringLiteral("crc24q") << check << 0xcde703U;
    QTest::newRow("crc32-empty") << QStringLiteral("crc32") << QByteArray() << 0U;
    QTest::newRow("crc32-check") << QStringLiteral("crc32") << check << 0xcbf43926U;
    QTest::newRow("nmea-empty") << QStringLiteral("nmea") << QByteArray() << 0U;
    QTest::newRow("nmea-ublox-txt") << QStringLiteral("nmea") << QByteArray("GPTXT,01,01,02,u-blox ag - www.u-blox.com")
                                    << 0x50U;
}

void ChecksumsTest::_knownAnswers()
{
    QFETCH(QString, algorithm);
    QFETCH(QByteArray, input);
    QFETCH(quint32, expected);
    QCOMPARE(checksum(algorithm, bytesOf(input)), expected);
}

void ChecksumsTest::_splitInputMatchesContiguous()
{
    const QByteArray input = QByteArray::fromHex("0602080000fa0001000100b5620a04");
    const auto bytes = bytesOf(input);
    const auto fletcher = QGC::fletcher8(bytes);
    const auto crc = QGC::crc32Update(bytes, 0xffffffff);
    for (size_t split = 0; split <= bytes.size(); ++split) {
        QCOMPARE(QGC::fletcher8(bytes.subspan(split), QGC::fletcher8(bytes.first(split))), fletcher);
        QCOMPARE(QGC::crc32Update(bytes.subspan(split), QGC::crc32Update(bytes.first(split), 0xffffffff)), crc);
    }
}

QGC_REGISTER_PORTABLE_TEST(ChecksumsTest, TestLabel::Unit, TestLabel::Utilities)
