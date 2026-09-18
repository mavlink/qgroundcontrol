#include "MAVLinkBandwidthProtocolTest.h"

#include <QtCore/QByteArray>
#include <QtCore/QtEndian>

#include "MAVLinkBandwidthProtocol.h"

void MAVLinkBandwidthProtocolTest::_roundTripTest_data()
{
    QTest::addColumn<int>("opCode");
    QTest::addColumn<int>("direction");

    QTest::newRow("hello-upload") << static_cast<int>(MAVLinkBandwidth::OpCode::Hello)
                                  << static_cast<int>(MAVLinkBandwidth::Direction::QgcToVehicle);
    QTest::newRow("data-upload") << static_cast<int>(MAVLinkBandwidth::OpCode::Data)
                                 << static_cast<int>(MAVLinkBandwidth::Direction::QgcToVehicle);
    QTest::newRow("report-download") << static_cast<int>(MAVLinkBandwidth::OpCode::Report)
                                     << static_cast<int>(MAVLinkBandwidth::Direction::VehicleToQgc);
    QTest::newRow("abort-download") << static_cast<int>(MAVLinkBandwidth::OpCode::Abort)
                                    << static_cast<int>(MAVLinkBandwidth::Direction::VehicleToQgc);
}

void MAVLinkBandwidthProtocolTest::_roundTripTest()
{
    QFETCH(int, opCode);
    QFETCH(int, direction);

    MAVLinkBandwidth::Packet source;
    source.opCode = static_cast<MAVLinkBandwidth::OpCode>(opCode);
    source.direction = static_cast<MAVLinkBandwidth::Direction>(direction);
    source.flags = 0x3C;
    source.sessionId = 0x12345678;
    source.sequence = 0xFEDCBA98;
    source.values = {1, 2, 3, 4, 5};
    source.data = QByteArray::fromHex("00112233445566778899AABBCCDDEEFF");

    const MAVLinkBandwidth::Payload payload = MAVLinkBandwidth::encode(source);
    MAVLinkBandwidth::Packet decoded;
    QVERIFY(MAVLinkBandwidth::decode(payload.data(), payload.size(), decoded));
    QCOMPARE(decoded.opCode, source.opCode);
    QCOMPARE(decoded.direction, source.direction);
    QCOMPARE(decoded.flags, source.flags);
    QCOMPARE(decoded.sessionId, source.sessionId);
    QCOMPARE(decoded.sequence, source.sequence);
    QCOMPARE(decoded.values, source.values);
    QCOMPARE(decoded.data, source.data);
}

void MAVLinkBandwidthProtocolTest::_maximumDataTest()
{
    MAVLinkBandwidth::Packet source;
    source.opCode = MAVLinkBandwidth::OpCode::Data;
    source.data = QByteArray(static_cast<qsizetype>(MAVLinkBandwidth::kDataCapacity + 20), '\x5A');

    const MAVLinkBandwidth::Payload payload = MAVLinkBandwidth::encode(source);
    QCOMPARE(payload.back(), static_cast<uint8_t>(0x5A));

    MAVLinkBandwidth::Packet decoded;
    QVERIFY(MAVLinkBandwidth::decode(payload.data(), payload.size(), decoded));
    QCOMPARE(decoded.data.size(), static_cast<qsizetype>(MAVLinkBandwidth::kDataCapacity));
}

void MAVLinkBandwidthProtocolTest::_invalidHeaderTest_data()
{
    QTest::addColumn<int>("offset");
    QTest::addColumn<int>("value");

    QTest::newRow("magic") << 0 << 0;
    QTest::newRow("version") << 4 << 2;
    QTest::newRow("opcode") << 5 << 0;
    QTest::newRow("direction") << 6 << 0;
}

void MAVLinkBandwidthProtocolTest::_invalidHeaderTest()
{
    QFETCH(int, offset);
    QFETCH(int, value);

    MAVLinkBandwidth::Packet source;
    const MAVLinkBandwidth::Payload validPayload = MAVLinkBandwidth::encode(source);
    MAVLinkBandwidth::Payload invalidPayload = validPayload;
    invalidPayload[static_cast<std::size_t>(offset)] = static_cast<uint8_t>(value);

    MAVLinkBandwidth::Packet decoded;
    QVERIFY(!MAVLinkBandwidth::decode(invalidPayload.data(), invalidPayload.size(), decoded));
    QVERIFY(!MAVLinkBandwidth::decode(validPayload.data(), validPayload.size() - 1, decoded));
}

void MAVLinkBandwidthProtocolTest::_invalidDataLengthTest()
{
    MAVLinkBandwidth::Packet source;
    MAVLinkBandwidth::Payload payload = MAVLinkBandwidth::encode(source);
    qToLittleEndian(static_cast<uint16_t>(MAVLinkBandwidth::kDataCapacity + 1), payload.data() + 36);

    MAVLinkBandwidth::Packet decoded;
    QVERIFY(!MAVLinkBandwidth::decode(payload.data(), payload.size(), decoded));
}

UT_REGISTER_TEST(MAVLinkBandwidthProtocolTest, TestLabel::Unit, TestLabel::AnalyzeView)
