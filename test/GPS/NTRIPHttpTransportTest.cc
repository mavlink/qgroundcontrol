#include "NTRIPHttpTransportTest.h"
#include "NTRIPHttpTransport.h"
#include "GpsTestHelpers.h"

// ---------------------------------------------------------------------------
// Whitelist Parsing
// ---------------------------------------------------------------------------

void NTRIPHttpTransportTest::_testWhitelistEmpty()
{
    NTRIPHttpTransport t(NTRIPTransportConfig{});
    QVERIFY(t._whitelist.isEmpty());

    NTRIPTransportConfig cfg;
    cfg.whitelist = QStringLiteral("");
    NTRIPHttpTransport t2(cfg);
    QVERIFY(t2._whitelist.isEmpty());
}

void NTRIPHttpTransportTest::_testWhitelistSingle()
{
    NTRIPTransportConfig cfg;
    cfg.whitelist = QStringLiteral("1005");
    NTRIPHttpTransport t(cfg);
    QCOMPARE(t._whitelist.size(), 1);
    QCOMPARE(t._whitelist[0], 1005);
}

void NTRIPHttpTransportTest::_testWhitelistMultiple()
{
    NTRIPTransportConfig cfg;
    cfg.whitelist = QStringLiteral("1005,1077,1087");
    NTRIPHttpTransport t(cfg);
    QCOMPARE(t._whitelist.size(), 3);
    QVERIFY(t._whitelist.contains(1005));
    QVERIFY(t._whitelist.contains(1077));
    QVERIFY(t._whitelist.contains(1087));
}

void NTRIPHttpTransportTest::_testWhitelistInvalidEntries()
{
    // "abc" and "" should be skipped (toInt returns 0)
    NTRIPTransportConfig cfg;
    cfg.whitelist = QStringLiteral("1005,abc,,1077");
    NTRIPHttpTransport t(cfg);
    QCOMPARE(t._whitelist.size(), 2);
    QVERIFY(t._whitelist.contains(1005));
    QVERIFY(t._whitelist.contains(1077));
}

// ---------------------------------------------------------------------------
// HTTP Status Line Parsing
// ---------------------------------------------------------------------------

void NTRIPHttpTransportTest::_testParseHttpStatus200()
{
    const auto status = NTRIPHttpTransport::parseHttpStatusLine("HTTP/1.1 200 OK");
    QVERIFY(status.valid);
    QCOMPARE(status.code, 200);
    QCOMPARE(status.reason, QStringLiteral("OK"));
}

void NTRIPHttpTransportTest::_testParseHttpStatusICY()
{
    const auto status = NTRIPHttpTransport::parseHttpStatusLine("ICY 200 OK");
    QVERIFY(status.valid);
    QCOMPARE(status.code, 200);
    QCOMPARE(status.reason, QStringLiteral("OK"));
}

void NTRIPHttpTransportTest::_testParseHttpStatus401()
{
    const auto status = NTRIPHttpTransport::parseHttpStatusLine("HTTP/1.0 401 Unauthorized");
    QVERIFY(status.valid);
    QCOMPARE(status.code, 401);
    QCOMPARE(status.reason, QStringLiteral("Unauthorized"));
}

void NTRIPHttpTransportTest::_testParseHttpStatus404()
{
    const auto status = NTRIPHttpTransport::parseHttpStatusLine("HTTP/1.1 404 Not Found");
    QVERIFY(status.valid);
    QCOMPARE(status.code, 404);
    QCOMPARE(status.reason, QStringLiteral("Not Found"));
}

void NTRIPHttpTransportTest::_testParseHttpStatusInvalid()
{
    QVERIFY(!NTRIPHttpTransport::parseHttpStatusLine("").valid);
    QVERIFY(!NTRIPHttpTransport::parseHttpStatusLine("garbage data").valid);
    QVERIFY(!NTRIPHttpTransport::parseHttpStatusLine("200 OK").valid);

    const auto sourceTable = NTRIPHttpTransport::parseHttpStatusLine("SOURCETABLE 200 OK");
    QVERIFY(sourceTable.valid);
    QCOMPARE(sourceTable.code, 200);
    QCOMPARE(sourceTable.reason, QStringLiteral("OK"));
}

void NTRIPHttpTransportTest::_testParseHttpStatus201()
{
    const auto status = NTRIPHttpTransport::parseHttpStatusLine("HTTP/1.1 201 Created");
    QVERIFY(status.valid);
    QCOMPARE(status.code, 201);
    QCOMPARE(status.reason, QStringLiteral("Created"));
}

void NTRIPHttpTransportTest::_testParseHttpStatus500()
{
    const auto status = NTRIPHttpTransport::parseHttpStatusLine("HTTP/1.0 500 Internal Server Error");
    QVERIFY(status.valid);
    QCOMPARE(status.code, 500);
    QCOMPARE(status.reason, QStringLiteral("Internal Server Error"));
}

void NTRIPHttpTransportTest::_testParseHttpStatusNoReason()
{
    const auto status = NTRIPHttpTransport::parseHttpStatusLine("HTTP/1.1 400");
    QVERIFY(status.valid);
    QCOMPARE(status.code, 400);
    QVERIFY(status.reason.isEmpty());
}

// ---------------------------------------------------------------------------
// NMEA Checksum Repair
// ---------------------------------------------------------------------------

static bool hasValidNmeaChecksum(const QByteArray& sentence)
{
    int star = sentence.lastIndexOf('*');
    if (star < 2 || star + 3 > sentence.size()) {
        return false;
    }

    quint8 calc = 0;
    for (int i = 1; i < star; ++i) {
        calc ^= static_cast<quint8>(sentence.at(i));
    }

    QByteArray expected = QByteArray::number(calc, 16).rightJustified(2, '0').toUpper();
    QByteArray actual = sentence.mid(star + 1, 2).toUpper();
    return actual == expected;
}

void NTRIPHttpTransportTest::_testRepairNmeaChecksumCorrect()
{
    const QByteArray input = "$GPGGA,120000,4723.8620,N,00832.7360,E,1,12,1.0,100.0,M,0.0,M,,*64";
    const QByteArray result = NTRIPHttpTransport::repairNmeaChecksum(input);

    QVERIFY(result.endsWith("\r\n"));
    QVERIFY(hasValidNmeaChecksum(result.trimmed()));
    QVERIFY(result.startsWith("$GPGGA,"));
}

void NTRIPHttpTransportTest::_testRepairNmeaChecksumWrong()
{
    const QByteArray input = "$GPGGA,120000,4723.8620,N,00832.7360,E,1,12,1.0,100.0,M,0.0,M,,*FF";
    const QByteArray result = NTRIPHttpTransport::repairNmeaChecksum(input);

    QVERIFY(result.endsWith("\r\n"));
    QVERIFY(hasValidNmeaChecksum(result.trimmed()));
    QVERIFY(!result.contains("*FF"));
}

void NTRIPHttpTransportTest::_testRepairNmeaChecksumMissing()
{
    const QByteArray input = "$GPGGA,120000,4723.8620,N,00832.7360,E,1,12,1.0,100.0,M,0.0,M,,";
    const QByteArray result = NTRIPHttpTransport::repairNmeaChecksum(input);

    QVERIFY(result.contains("*"));
    QVERIFY(result.endsWith("\r\n"));
    QVERIFY(hasValidNmeaChecksum(result.trimmed()));
}

void NTRIPHttpTransportTest::_testRepairNmeaChecksumTruncated()
{
    const QByteArray input = "$GPGGA,120000,0000.0000,N,00000.0000,E,1,12,1.0,0.0,M,0.0,M,,*";
    const QByteArray result = NTRIPHttpTransport::repairNmeaChecksum(input);

    QVERIFY(result.endsWith("\r\n"));
    const QByteArray trimmed = result.trimmed();
    int star = trimmed.lastIndexOf('*');
    QVERIFY(star > 0);
    QVERIFY(star + 3 <= trimmed.size());
    QVERIFY(hasValidNmeaChecksum(trimmed));
}

void NTRIPHttpTransportTest::_testRepairNmeaChecksumAppendsCrLf()
{
    const QByteArray input = "$GPGGA,000000,0000.0000,N,00000.0000,E,1,12,1.0,0.0,M,0.0,M,,*00";
    QVERIFY(!input.endsWith("\r\n"));

    const QByteArray result = NTRIPHttpTransport::repairNmeaChecksum(input);
    QVERIFY(result.endsWith("\r\n"));

    const QByteArray input2 = input + "\r\n";
    const QByteArray result2 = NTRIPHttpTransport::repairNmeaChecksum(input2);
    QVERIFY(result2.endsWith("\r\n"));
    QVERIFY(!result2.endsWith("\r\n\r\n"));
}

void NTRIPHttpTransportTest::_testRepairNmeaChecksumShortSentence()
{
    const QByteArray input = "$GP";
    const QByteArray result = NTRIPHttpTransport::repairNmeaChecksum(input);
    QVERIFY(result.endsWith("\r\n"));

    const QByteArray empty = "";
    const QByteArray resultEmpty = NTRIPHttpTransport::repairNmeaChecksum(empty);
    QVERIFY(resultEmpty.endsWith("\r\n"));
}

// ---------------------------------------------------------------------------
// RTCM Filtering
// ---------------------------------------------------------------------------

void NTRIPHttpTransportTest::_testFilterNoWhitelist()
{
    NTRIPTransportConfig cfg;
    cfg.mountpoint = QStringLiteral("TEST");
    NTRIPHttpTransport t(cfg);

    QVector<QByteArray> received;
    connect(&t, &NTRIPHttpTransport::RTCMDataUpdate, this, [&](const QByteArray& msg) {
        received.append(msg);
    });

    QByteArray stream = GpsTestHelpers::buildRtcmFrame(1005, 4) + GpsTestHelpers::buildRtcmFrame(1077, 8) + GpsTestHelpers::buildRtcmFrame(1087, 2);
    t._parseRtcm(stream);

    QCOMPARE(received.size(), 3);
}

void NTRIPHttpTransportTest::_testFilterWithWhitelist()
{
    NTRIPTransportConfig cfg;
    cfg.mountpoint = QStringLiteral("TEST");
    cfg.whitelist = QStringLiteral("1005,1087");
    NTRIPHttpTransport t(cfg);

    QVector<uint16_t> receivedIds;
    connect(&t, &NTRIPHttpTransport::RTCMDataUpdate, this, [&](const QByteArray& msg) {
        if (msg.size() >= 5) {
            uint16_t id = (static_cast<uint8_t>(msg[3]) << 4) | (static_cast<uint8_t>(msg[4]) >> 4);
            receivedIds.append(id);
        }
    });

    QByteArray stream = GpsTestHelpers::buildRtcmFrame(1005, 4) + GpsTestHelpers::buildRtcmFrame(1077, 8) + GpsTestHelpers::buildRtcmFrame(1087, 2);
    t._parseRtcm(stream);

    QCOMPARE(receivedIds.size(), 2);
    QVERIFY(receivedIds.contains(1005));
    QVERIFY(receivedIds.contains(1087));
    QVERIFY(!receivedIds.contains(1077));
}

void NTRIPHttpTransportTest::_testFilterRejectsBadCrc()
{
    NTRIPTransportConfig cfg;
    cfg.mountpoint = QStringLiteral("TEST");
    NTRIPHttpTransport t(cfg);

    int count = 0;
    connect(&t, &NTRIPHttpTransport::RTCMDataUpdate, this, [&](const QByteArray&) {
        count++;
    });

    QByteArray bad = GpsTestHelpers::buildRtcmFrame(1005, 4);
    bad[bad.size() - 1] = static_cast<char>(bad[bad.size() - 1] ^ 0xFF);

    QByteArray good = GpsTestHelpers::buildRtcmFrame(1077, 2);

    t._parseRtcm(bad + good);

    QCOMPARE(count, 1);
}

UT_REGISTER_TEST(NTRIPHttpTransportTest, TestLabel::Unit)
