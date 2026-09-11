#include "NTRIPHttpTransportTest.h"

#include <QtCore/QRegularExpression>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslServer>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QTcpServer>
#include <QtTest/QSignalSpy>

#include "GpsTestHelpers.h"
#include "ManualScheduler.h"
#include "NMEAUtils.h"
#include "NTRIPError.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPRequest.h"
#include "NTRIPTransportConfig.h"

namespace {
const QByteArray kTestServerCertPem =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDJTCCAg2gAwIBAgIUb8FRX3IsccsbhaXjRNj9FdOorocwDQYJKoZIhvcNAQEL\n"
    "BQAwFDESMBAGA1UEAwwJbG9jYWxob3N0MB4XDTI2MDYyNTA5NTAyNFoXDTM2MDYy\n"
    "MjA5NTAyNFowFDESMBAGA1UEAwwJbG9jYWxob3N0MIIBIjANBgkqhkiG9w0BAQEF\n"
    "AAOCAQ8AMIIBCgKCAQEAv+2XaNn8MaC6KztEfvozWDq+lfrF66xr0MdPCZEGXffK\n"
    "JFJSzI+bsT2foa/dDvCaYb8FkOGfWGQnVZ2FhAJl9fMN9KAwuKQV0TGShJxcSxms\n"
    "h3S6N2TfuYIe3YDHExej2vIZ3yCpLfjPb4VkAxYV+6Tsndppq7JbCXd0aGJELh3U\n"
    "hqnNWJyOcz8Sc/460Tn3Q007Hj0bHGVErIYtcBjQwrU0xwZGuWH2tE8+viSb5StD\n"
    "P+n5mQDpR/DGmjTInPGwHUBhIT4FvnuM5IlQo2+yDpUSFg0A1H2AwEBlthA6gO0K\n"
    "rSu9wkVU3ace3VCrb9jlb2j9iH8IdhwLNH6GKK4RxQIDAQABo28wbTAdBgNVHQ4E\n"
    "FgQURMj0V2oCrsYYry3/sA3I7/xXPAowHwYDVR0jBBgwFoAURMj0V2oCrsYYry3/\n"
    "sA3I7/xXPAowDwYDVR0TAQH/BAUwAwEB/zAaBgNVHREEEzARhwR/AAABgglsb2Nh\n"
    "bGhvc3QwDQYJKoZIhvcNAQELBQADggEBAKoRhhZtgWhxI+sY1FnEeUtDt6zmu18R\n"
    "u0KAIALMJJZGTREpgk9Qu/f9nDYdE1fDuiBYFiGpbHreLanHmtXVjK6XX6cJUnVU\n"
    "BpUPL67XT5se2Frtyg25ptEBdH/bZtNnuJ/FKpOARdIMDsHkfHat3ZRKyx6AbcKU\n"
    "A+gyAbs6VEhVC7NonPqborskxGUXri1K7dzCBUw04efLP3eV6821U5g0rRXRz2rF\n"
    "tHRwOww1g8e2LkZF6+tY0qVIZV6WX1BP/owA1+PpgCDGiyohkza8ptrehQedPU3M\n"
    "j3fxDlJPYavVFCmPlcYND+wlTghr6oFIGZIsEYKmAQywrV2Lr+VX4d4=\n"
    "-----END CERTIFICATE-----\n";

const QByteArray kTestServerKeyPem =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQC/7Zdo2fwxoLor\n"
    "O0R++jNYOr6V+sXrrGvQx08JkQZd98okUlLMj5uxPZ+hr90O8JphvwWQ4Z9YZCdV\n"
    "nYWEAmX18w30oDC4pBXRMZKEnFxLGayHdLo3ZN+5gh7dgMcTF6Pa8hnfIKkt+M9v\n"
    "hWQDFhX7pOyd2mmrslsJd3RoYkQuHdSGqc1YnI5zPxJz/jrROfdDTTsePRscZUSs\n"
    "hi1wGNDCtTTHBka5Yfa0Tz6+JJvlK0M/6fmZAOlH8MaaNMic8bAdQGEhPgW+e4zk\n"
    "iVCjb7IOlRIWDQDUfYDAQGW2EDqA7QqtK73CRVTdpx7dUKtv2OVvaP2Ifwh2HAs0\n"
    "foYorhHFAgMBAAECggEAO6YLuHqG9qWNNoJk91GrQ3B+av5VJLmhiHFpDwATioDI\n"
    "QiGTuh+ns54DTqzpdwsv79D+WdjFPSNjVihupmhYZ+fyHmTqv3e/kBRoBO7TgEOq\n"
    "ay7L8QtYvL7D+PNc64IdWp6Di+UKr070qSQ7wPnMOzk2kJig3su/n2GQvCBOMEZp\n"
    "7p345MSfcQ49jcgloPIkj2NMCiB2eD3DGrn1Zxz2lyvpzjtW+inoTasMw56Eg5R7\n"
    "IsZLpv8k9/uz2nTTaTO+w1xdCUEoxXFe9uhgTfUXNI3VfSPFCf5TqpWivpzsCmBw\n"
    "qSpQi15UMbPbKwAx6PBLcWaTrorzgy3Qvg3FKAsWwQKBgQD/R8sqX5Uh45bEwGxR\n"
    "f4OMVFCt1MqSWiZ0eCpzGWqg74BN3zcwhT+xZZcDrJMiSsPRWRL3ZNgDmcnr9T3j\n"
    "rDFhai6lTAy7u/46nyA0f9ska2p+LZDwtBPev1vt494uin8mLBTC1TxfrTfaTPBq\n"
    "IX6Fy+58c4qFLrPRO+0c28kVzwKBgQDAeBVxREWCz9Zfcr6DP1CXjvaTjaoHvpKH\n"
    "gDW36XQZDH0Vol1mX49v2G5LxtGGC4KcCp4d/VMtyRc4ewbO2CCIul6//DqDpHLP\n"
    "DM8tjhho6n5PZXPKX4QBnW6oO5NyOwxr7L3FXKjWa6hrY/3BSrScwGlYzCyYBAWj\n"
    "Zi+i8WQYKwKBgQCmWVBI0nRJ2xaaK5HqIZ/FSAQy4mEGsXwxlUSEMGHNcYQ4Omaq\n"
    "VYpFvR+FI2XViMbFmrfDQpGI9yQfgHXN8J1VD25KBJ6fj0eBR8QisdZJiz2f721t\n"
    "jMsN8cCj6kMULOfiJgN5Wp628hddR5m6bw0VfuhvbJMtalt+0wAWOBp2/QKBgQCM\n"
    "cNNMpAGIszl8ylCDmpanEJWSE4PnRMLNBturyDiD8p3vRFuc0MvsU+QffQL0Kb/z\n"
    "Nrgrr+aa+Sntd6//DKuouT1cH6Ne3Yc8197xIcdj/v+N1byJBetf9k2Bin9LkhS9\n"
    "R7EtqzAzzbjGK99ExMtHugrk1Y8QmZa3pV/LKRLdowKBgAZ0r6/+gHUfpE+38lGc\n"
    "CCwTWRGnM/ReiLq/GpdhQye3kDDGf68+xDVYCLO7gjFdUv8mYDaue5o3Ru5Y13/b\n"
    "9a0JB8roMy3yUa98FwKG5Cn8tTDMIunK7AHlT8AD3xvodw8+wxbQjV6Sm+LpS0gh\n"
    "UKCIFGe/9AGBDYeWyV1Mlf7H\n"
    "-----END PRIVATE KEY-----\n";
}  // namespace

// ---------------------------------------------------------------------------
// Transport Config Validation
// ---------------------------------------------------------------------------

void NTRIPHttpTransportTest::testConfigValidEmpty()
{
    NTRIPTransportConfig cfg;
    QVERIFY(!cfg.isValid());  // empty host
}

void NTRIPHttpTransportTest::testConfigValidGood()
{
    NTRIPTransportConfig cfg;
    cfg.host = QStringLiteral("caster.example.com");
    cfg.port = 2101;
    QVERIFY(cfg.isValid());

    cfg.port = 1;
    QVERIFY(cfg.isValid());

    cfg.port = 65535;
    QVERIFY(cfg.isValid());
}

void NTRIPHttpTransportTest::testConfigValidBadPort()
{
    NTRIPTransportConfig cfg;
    cfg.host = QStringLiteral("caster.example.com");

    cfg.port = 0;
    QVERIFY(!cfg.isValid());

    cfg.port = -1;
    QVERIFY(!cfg.isValid());

    cfg.port = 65536;
    QVERIFY(!cfg.isValid());
}

void NTRIPHttpTransportTest::testConfigRejectsColonUsername()
{
    NTRIPTransportConfig cfg;
    cfg.host = QStringLiteral("caster.example.com");
    cfg.port = 2101;
    cfg.username = QStringLiteral("user:name");
    QVERIFY(!cfg.isValid());

    cfg.username = QStringLiteral("username");
    QVERIFY(cfg.isValid());
}

void NTRIPHttpTransportTest::testConfigRejectsControlChars()
{
    NTRIPTransportConfig cfg;
    cfg.port = 2101;

    cfg.host = QStringLiteral("caster.example.com\r\nEvil: header");
    QVERIFY(!cfg.isValid());

    cfg.host = QStringLiteral("caster.example.com");
    cfg.mountpoint = QStringLiteral("MP\r\nInjected");
    QVERIFY(!cfg.isValid());

    cfg.mountpoint = QStringLiteral("MP1");
    QVERIFY(cfg.isValid());
}

void NTRIPHttpTransportTest::testConfigDiffClassifiersCoverIndependentFields()
{
    NTRIPTransportConfig baseline;
    baseline.host = QStringLiteral("caster.example.com");
    baseline.port = 2101;
    baseline.username = QStringLiteral("user");
    baseline.password = QStringLiteral("pass");
    baseline.mountpoint = QStringLiteral("MOUNT1");
    baseline.whitelist = QStringLiteral("1005,1077");
    baseline.useTls = true;
    baseline.allowSelfSignedCerts = false;

    auto changed = baseline;
    changed.host = QStringLiteral("other.example.com");
    QVERIFY(changed.transportDiffers(baseline));
    QVERIFY(!changed.whitelistDiffers(baseline));

    changed = baseline;
    changed.whitelist = QStringLiteral("1005");
    QVERIFY(!changed.transportDiffers(baseline));
    QVERIFY(changed.whitelistDiffers(baseline));
}

void NTRIPHttpTransportTest::testConfigCasterIdentityExcludesMountpointAndFilter()
{
    NTRIPTransportConfig baseline;
    baseline.host = QStringLiteral("caster.example.com");
    baseline.port = 2101;
    baseline.username = QStringLiteral("user");
    baseline.password = QStringLiteral("pass");
    baseline.mountpoint = QStringLiteral("MOUNT1");
    baseline.whitelist = QStringLiteral("1005");
    baseline.useTls = true;

    auto sameCaster = baseline;
    sameCaster.mountpoint = QStringLiteral("MOUNT2");
    sameCaster.whitelist = QStringLiteral("1005,1077");
    QCOMPARE(sameCaster.casterIdentity(), baseline.casterIdentity());

    auto differentCaster = baseline;
    differentCaster.useTls = false;
    QVERIFY(differentCaster.casterIdentity() != baseline.casterIdentity());

    differentCaster = baseline;
    differentCaster.password = QStringLiteral("other-pass");
    QVERIFY(differentCaster.casterIdentity() != baseline.casterIdentity());
}

void NTRIPHttpTransportTest::testTlsFatalErrorEmitsSingleError()
{
    if (!QSslSocket::supportsSsl()) {
        QSKIP("No TLS backend available");
    }

    const QSslCertificate cert(kTestServerCertPem, QSsl::Pem);
    QVERIFY(!cert.isNull());
    const QSslKey key(kTestServerKeyPem, QSsl::Rsa, QSsl::Pem);
    QVERIFY(!key.isNull());

    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setLocalCertificate(cert);
    sslConfig.setPrivateKey(key);

    QSslServer server;
    server.setSslConfiguration(sslConfig);
    QVERIFY(server.listen(QHostAddress::LocalHost));

    NTRIPTransportConfig cfg;
    cfg.host = QStringLiteral("127.0.0.1");
    cfg.port = server.serverPort();
    cfg.useTls = true;
    cfg.allowSelfSignedCerts = false;
    cfg.mountpoint = QStringLiteral("TEST");

    ignoreLogMessage("GPS.NTRIP.NTRIPHttpResponse", QtWarningMsg, QRegularExpression(QStringLiteral("TLS error:")));
    ignoreLogMessage("GPS.NTRIP.NTRIPHttpTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Rejecting self-signed certificate")));

    NTRIPHttpTransport transport(cfg);
    QSignalSpy errorSpy(&transport, &NTRIPHttpTransport::failed);
    transport.start();

    QVERIFY_SIGNAL_WAIT(errorSpy, TestTimeout::mediumMs());
    QCOMPARE(errorSpy.count(), 1);

    // Drain queued callbacks after abort; a second error must not escape teardown.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(errorSpy.count(), 1);
}

// ---------------------------------------------------------------------------
// Whitelist Parsing
// ---------------------------------------------------------------------------

void NTRIPHttpTransportTest::_testWhitelistParsing_data()
{
    QTest::addColumn<QString>("whitelist");
    QTest::addColumn<bool>("filtered");
    QTest::newRow("empty") << QString() << false;
    QTest::newRow("included") << QStringLiteral("1005") << false;
    QTest::newRow("excluded") << QStringLiteral("1077,1087") << true;
    QTest::newRow("invalid-tokens") << QStringLiteral("abc,,1005") << false;
}

void NTRIPHttpTransportTest::_testWhitelistParsing()
{
    QFETCH(QString, whitelist);
    QFETCH(bool, filtered);
    NTRIPTransportConfig config;
    config.whitelist = whitelist;
    NTRIPHttpTransport transport(config);
    QSignalSpy frames(&transport, &NTRIPStream::correctionReceivedAt);
    transport._receivedAtMs = 1000;
    transport._parseRtcm(GpsTestHelpers::buildRtcmFrame(1005, 4));
    QCOMPARE(frames.size(), 1);
    QCOMPARE(frames.first().at(1).toInt(), 1005);
    QCOMPARE(frames.first().at(2).toBool(), filtered);
    QCOMPARE(frames.first().at(3).toLongLong(), qint64(1000));
}

void NTRIPHttpTransportTest::_testFragmentedReceiptTime()
{
    NTRIPHttpTransport transport(NTRIPTransportConfig{});
    QSignalSpy frames(&transport, &NTRIPStream::correctionReceivedAt);
    const auto frame = GpsTestHelpers::buildRtcmFrame(1005, 20);
    transport._receivedAtMs = 1000;
    transport._parseRtcm(frame.first(5));
    QVERIFY(frames.isEmpty());
    transport._receivedAtMs = 8000;
    transport._parseRtcm(frame.sliced(5) + frame);
    QCOMPARE(frames.size(), 2);
    QCOMPARE(frames[0][3].toLongLong(), qint64(1000));
    QCOMPARE(frames[1][3].toLongLong(), qint64(8000));
}

// ---------------------------------------------------------------------------
// HTTP Status Line Parsing
// ---------------------------------------------------------------------------

void NTRIPHttpTransportTest::testResponseDecoder_data()
{
    QTest::addColumn<QByteArray>("response");
    QTest::addColumn<bool>("accepted");
    QTest::addColumn<int>("httpStatus");
    QTest::newRow("http200") << QByteArray("HTTP/1.1 200 OK\r\n\r\n") << true << 0;
    QTest::newRow("icy200") << QByteArray("ICY 200 OK\r\n\r\n") << true << 0;
    QTest::newRow("http201") << QByteArray("HTTP/1.1 201 Created\r\n\r\n") << true << 0;
    QTest::newRow("unauthorized") << QByteArray("HTTP/1.0 401 Unauthorized\r\n\r\n") << false << 401;
    QTest::newRow("missing") << QByteArray("HTTP/1.1 404 Not Found\r\n\r\n") << false << 404;
    QTest::newRow("server-error") << QByteArray("HTTP/1.0 500 Internal Server Error\r\n\r\n") << false << 500;
    QTest::newRow("no-reason") << QByteArray("HTTP/1.1 400\r\n\r\n") << false << 400;
    QTest::newRow("empty") << QByteArray("\r\n\r\n") << false << 0;
    QTest::newRow("garbage") << QByteArray("garbage data\r\n\r\n") << false << 0;
    QTest::newRow("no-protocol") << QByteArray("200 OK\r\n\r\n") << false << 0;
    QTest::newRow("source-table-on-stream") << QByteArray("SOURCETABLE 200 OK\r\n\r\n") << false << 0;
}

void NTRIPHttpTransportTest::testResponseDecoder()
{
    QFETCH(QByteArray, response);
    QFETCH(bool, accepted);
    QFETCH(int, httpStatus);
    NTRIPHttpDecoder decoder;
    bool connected = false;
    std::optional<NTRIPFailure> failure;
    for (const char byte : response) {
        const auto result = decoder.feed(QByteArray(1, byte));
        connected |= result.connected;
        if (result.failure) {
            failure = result.failure;
            break;
        }
    }
    QCOMPARE(connected, accepted);
    QCOMPARE(failure.has_value(), !accepted);
    if (failure) {
        QCOMPARE(failure->httpStatus, httpStatus);
    }
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
    const QByteArray result = NMEAUtils::repairChecksum(input);

    QVERIFY(result.endsWith("\r\n"));
    QVERIFY(hasValidNmeaChecksum(result.trimmed()));
    QVERIFY(result.startsWith("$GPGGA,"));
}

void NTRIPHttpTransportTest::_testRepairNmeaChecksumWrong()
{
    const QByteArray input = "$GPGGA,120000,4723.8620,N,00832.7360,E,1,12,1.0,100.0,M,0.0,M,,*FF";
    const QByteArray result = NMEAUtils::repairChecksum(input);

    QVERIFY(result.endsWith("\r\n"));
    QVERIFY(hasValidNmeaChecksum(result.trimmed()));
    QVERIFY(!result.contains("*FF"));
}

void NTRIPHttpTransportTest::_testRepairNmeaChecksumMissing()
{
    const QByteArray input = "$GPGGA,120000,4723.8620,N,00832.7360,E,1,12,1.0,100.0,M,0.0,M,,";
    const QByteArray result = NMEAUtils::repairChecksum(input);

    QVERIFY(result.contains("*"));
    QVERIFY(result.endsWith("\r\n"));
    QVERIFY(hasValidNmeaChecksum(result.trimmed()));
}

void NTRIPHttpTransportTest::_testRepairNmeaChecksumTruncated()
{
    const QByteArray input = "$GPGGA,120000,0000.0000,N,00000.0000,E,1,12,1.0,0.0,M,0.0,M,,*";
    const QByteArray result = NMEAUtils::repairChecksum(input);

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

    const QByteArray result = NMEAUtils::repairChecksum(input);
    QVERIFY(result.endsWith("\r\n"));

    const QByteArray input2 = input + "\r\n";
    const QByteArray result2 = NMEAUtils::repairChecksum(input2);
    QVERIFY(result2.endsWith("\r\n"));
    QVERIFY(!result2.endsWith("\r\n\r\n"));
}

void NTRIPHttpTransportTest::_testRepairNmeaChecksumShortSentence()
{
    const QByteArray input = "$GP";
    const QByteArray result = NMEAUtils::repairChecksum(input);
    QVERIFY(result.endsWith("\r\n"));

    const QByteArray empty = "";
    const QByteArray resultEmpty = NMEAUtils::repairChecksum(empty);
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
    connect(&t, &NTRIPHttpTransport::correctionReceivedAt, this, [&](const QByteArray& msg) { received.append(msg); });

    QByteArray stream = GpsTestHelpers::buildRtcmFrame(1005, 4) + GpsTestHelpers::buildRtcmFrame(1077, 8) +
                        GpsTestHelpers::buildRtcmFrame(1087, 2);
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
    connect(&t, &NTRIPHttpTransport::correctionReceivedAt, this, [&](const QByteArray&, int id, bool filtered, qint64) {
        if (!filtered) {
            receivedIds.append(id);
        }
    });

    QByteArray stream = GpsTestHelpers::buildRtcmFrame(1005, 4) + GpsTestHelpers::buildRtcmFrame(1077, 8) +
                        GpsTestHelpers::buildRtcmFrame(1087, 2);
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
    QSignalSpy rejected(&t, &NTRIPStream::correctionRejectedAt);

    int count = 0;
    connect(&t, &NTRIPHttpTransport::correctionReceivedAt, this, [&](const QByteArray&) { count++; });

    QByteArray bad = GpsTestHelpers::buildRtcmFrame(1005, 4);
    bad[bad.size() - 1] = static_cast<char>(bad[bad.size() - 1] ^ 0xFF);

    QByteArray good = GpsTestHelpers::buildRtcmFrame(1077, 2);

    expectLogMessage("GPS.NTRIP.NTRIPHttpTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid RTCM framing or CRC")));
    t._parseRtcm(bad + good);
    verifyExpectedLogMessage();

    QCOMPARE(count, 1);
    QCOMPARE(rejected.size(), 1);
    QCOMPARE(rejected.first().at(0).toByteArray(), bad);
    QCOMPARE(rejected.first().at(1).toInt(), 1005);
}

// ---------------------------------------------------------------------------
// HTTP Request Building
// ---------------------------------------------------------------------------

void NTRIPHttpTransportTest::_testBuildRequestPlaintextCredentialsWarns()
{
    NTRIPTransportConfig cfg;
    cfg.host = QStringLiteral("caster.example.com");
    cfg.mountpoint = QStringLiteral("MOUNT01");
    cfg.username = QStringLiteral("user");
    cfg.password = QStringLiteral("pass");
    cfg.useTls = false;

    const auto request = NTRIPRequest::build(cfg);

    QVERIFY(request.credentialsInClear);
    QVERIFY(request.bytes.startsWith("GET /MOUNT01 HTTP/1.1\r\n"));
    QVERIFY(request.bytes.contains("host: caster.example.com:2101\r\n"));
    QVERIFY(request.bytes.contains("authorization: Basic "));
    QVERIFY(request.bytes.endsWith("\r\n\r\n"));
}

void NTRIPHttpTransportTest::_testBuildRequestTlsCredentialsNoWarn()
{
    NTRIPTransportConfig cfg;
    cfg.host = QStringLiteral("caster.example.com");
    cfg.mountpoint = QStringLiteral("MOUNT01");
    cfg.username = QStringLiteral("user");
    cfg.password = QStringLiteral("pass");
    cfg.useTls = true;

    const auto request = NTRIPRequest::build(cfg);

    QVERIFY(!request.credentialsInClear);
    QVERIFY(request.bytes.contains("authorization: Basic "));
}

void NTRIPHttpTransportTest::_testBuildRequestNoCredentialsNoWarn()
{
    NTRIPTransportConfig cfg;
    cfg.host = QStringLiteral("caster.example.com");
    cfg.mountpoint = QStringLiteral("MOUNT01");
    cfg.useTls = false;

    const auto request = NTRIPRequest::build(cfg);

    QVERIFY(!request.credentialsInClear);
    QVERIFY(!request.bytes.contains("Authorization"));
}

UT_REGISTER_TEST(NTRIPHttpTransportTest, TestLabel::Unit)

void NTRIPHttpTransportTest::testStreamingRequiresMountpoint()
{
    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    QVERIFY(config.validationError().isEmpty());
    for (const QString& mountpoint : {QString(), QStringLiteral("   ")}) {
        config.mountpoint = mountpoint;
        QVERIFY(!config.streamValidationError().isEmpty());
        NTRIPHttpTransport transport(config);
        QSignalSpy errors(&transport, &NTRIPStream::failed);
        QSignalSpy connected(&transport, &NTRIPStream::connected);
        transport.start();
        QCOMPARE(errors.size(), 1);
        QCOMPARE(qvariant_cast<NTRIPFailure>(errors.first().first()).code, NTRIPError::InvalidConfig);
        QVERIFY(connected.isEmpty());
        QVERIFY(!transport._response._socket);
    }
}

void NTRIPHttpTransportTest::testConnectionWaitsForHttpResponse()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = server.serverPort();
    config.mountpoint = QStringLiteral("TEST");
    NTRIPHttpTransport transport(config);
    QSignalSpy connected(&transport, &NTRIPStream::connected);
    QSignalSpy frames(&transport, &NTRIPStream::correctionReceivedAt);
    transport.start();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QVERIFY(peer);
    QTRY_VERIFY_WITH_TIMEOUT(peer->bytesAvailable() > 0, TestTimeout::mediumMs());
    QVERIFY(peer->readAll().startsWith("GET /TEST HTTP/1.1\r\n"));
    QVERIFY(connected.isEmpty());
    QVERIFY(transport._response._deadline.active());
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 4);
    const QByteArray response = "HTTP/1.1 200 OK\r\nContent-Type: gnss/data\r\n\r\n" + frame;
    QCOMPARE(peer->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(connected.size(), 1, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 1, TestTimeout::mediumMs());
    QVERIFY(!transport._response._deadline.active());
    QVERIFY(transport._dataWatchdog.active());
    transport.stop();
}

void NTRIPHttpTransportTest::testHandshakeTimeoutClosesSocket()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = server.serverPort();
    config.mountpoint = QStringLiteral("TEST");
    ManualScheduler scheduler;
    NTRIPHttpTransport transport(config, nullptr, &scheduler);
    QSignalSpy connected(&transport, &NTRIPStream::connected);
    QSignalSpy errors(&transport, &NTRIPStream::failed);
    transport.start();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QVERIFY(peer);
    QTRY_VERIFY_WITH_TIMEOUT(peer->bytesAvailable() > 0, TestTimeout::mediumMs());
    QVERIFY(connected.isEmpty());
    QVERIFY(scheduler.advanceBy(NTRIPHttpTransport::kConnectTimeout));
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(qvariant_cast<NTRIPFailure>(errors.first().first()).code, NTRIPError::ConnectionTimeout);
    QVERIFY(!transport._response.active());
    QVERIFY(!transport._dataWatchdog.active());
    QVERIFY(connected.isEmpty());
}

void NTRIPHttpTransportTest::testRemoteCloseEmitsSingleError()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = server.serverPort();
    config.mountpoint = QStringLiteral("TEST");
    NTRIPHttpTransport transport(config);
    QSignalSpy errors(&transport, &NTRIPStream::failed);
    transport.start();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QVERIFY(peer);
    peer->disconnectFromHost();
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::mediumMs());
    QVERIFY(!transport._response.active());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(errors.size(), 1);
    QVERIFY(!transport._response._deadline.active());
    QCOMPARE(qvariant_cast<NTRIPFailure>(errors.first().first()).code, NTRIPError::InterruptedResponse);
}

void NTRIPHttpTransportTest::testCorrectionWatchdog_data()
{
    QTest::addColumn<bool>("validFilteredFrames");
    QTest::newRow("garbage-does-not-renew-watchdog") << false;
    QTest::newRow("filtered-valid-renews-watchdog") << true;
}

void NTRIPHttpTransportTest::testCorrectionWatchdog()
{
    QFETCH(bool, validFilteredFrames);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = server.serverPort();
    config.mountpoint = QStringLiteral("TEST");
    config.whitelist = QStringLiteral("1077");
    ManualScheduler scheduler;
    NTRIPHttpTransport transport(config, nullptr, &scheduler);
    QSignalSpy connected(&transport, &NTRIPStream::connected);
    QSignalSpy errors(&transport, &NTRIPStream::failed);
    QSignalSpy validated(&transport, &NTRIPStream::correctionReceivedAt);
    transport.start();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QVERIFY(peer);
    QTRY_VERIFY_WITH_TIMEOUT(peer->bytesAvailable() > 0, TestTimeout::mediumMs());
    peer->readAll();
    peer->write("HTTP/1.1 200 OK\r\n\r\n");
    QTRY_COMPARE_WITH_TIMEOUT(connected.size(), 1, TestTimeout::mediumMs());
    QVERIFY(scheduler.advanceBy(NTRIPHttpTransport::kDataWatchdog - std::chrono::seconds{1}));
    const QByteArray data =
        validFilteredFrames ? GpsTestHelpers::buildRtcmFrame(1005, 20) : QByteArrayLiteral("garbage");
    QSignalSpy received(&transport, &NTRIPStream::bytesReceived);
    peer->write(data);
    QTRY_COMPARE_WITH_TIMEOUT(received.size(), 1, TestTimeout::mediumMs());
    if (validFilteredFrames) {
        QCOMPARE(validated.size(), 1);
        QVERIFY(validated.first().at(2).toBool());
        QCOMPARE(validated.first().at(3).toLongLong(), scheduler.nowMs());
        QVERIFY(scheduler.advanceBy(std::chrono::seconds{2}));
        QVERIFY(errors.isEmpty());
    } else {
        expectLogMessage("GPS.NTRIP.NTRIPHttpTransport", QtWarningMsg,
                         QRegularExpression(QStringLiteral("No valid corrections received")));
        QVERIFY(scheduler.advanceBy(std::chrono::seconds{2}));
        QCOMPARE(errors.size(), 1);
        QCOMPARE(qvariant_cast<NTRIPFailure>(errors.first().first()).code, NTRIPError::DataWatchdog);
        QVERIFY(validated.isEmpty());
        verifyExpectedLogMessage();
    }
    transport.stop();
    QCOMPARE(scheduler.pendingCount(), 0);
}

void NTRIPHttpTransportTest::testChunkedCorrectionsYieldBetweenReadBatches()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = server.serverPort();
    config.mountpoint = QStringLiteral("TEST");
    NTRIPHttpTransport transport(config);
    QSignalSpy frames(&transport, &NTRIPStream::correctionReceivedAt);
    transport.start();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QTRY_VERIFY_WITH_TIMEOUT(peer->bytesAvailable() > 0, TestTimeout::mediumMs());
    peer->readAll();
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 150);
    QByteArray body;
    constexpr int frameCount = 500;
    for (int i = 0; i < frameCount; ++i) {
        body += frame;
    }
    QByteArray wire("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
    for (qsizetype offset = 0; offset < body.size(); offset += 71) {
        const auto chunk = body.mid(offset, 71);
        wire += QByteArray::number(chunk.size(), 16) + "\r\n" + chunk + "\r\n";
    }
    bool heartbeatQueued = false;
    int framesAtHeartbeat = -1;
    connect(&transport, &NTRIPStream::bytesReceived, &transport, [&]() {
        if (!heartbeatQueued) {
            heartbeatQueued = true;
            QMetaObject::invokeMethod(&transport, [&]() { framesAtHeartbeat = frames.size(); }, Qt::QueuedConnection);
        }
    });
    QCOMPARE(peer->write(wire), wire.size());
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), frameCount, TestTimeout::mediumMs());
    QVERIFY(framesAtHeartbeat > 0);
    QVERIFY(framesAtHeartbeat < frameCount);
    for (const auto& received : frames) {
        QCOMPARE(received.first().toByteArray(), frame);
    }
    transport.stop();
}

void NTRIPHttpTransportTest::testPublicObserverCanStop_data()
{
    QTest::addColumn<bool>("warning");
    QTest::newRow("credentials-warning") << true;
    QTest::newRow("handshake-connected") << false;
}

void NTRIPHttpTransportTest::testPublicObserverCanStop()
{
    QFETCH(bool, warning);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = server.serverPort();
    config.mountpoint = QStringLiteral("TEST");
    if (warning) {
        config.username = QStringLiteral("user");
        expectLogMessage("GPS.NTRIP.NTRIPHttpResponse", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Sending credentials without TLS")));
    }
    NTRIPHttpTransport transport(config);
    bool stopped = false;
    const auto stop = [&]() {
        stopped = true;
        transport.stop();
    };
    if (warning) {
        connect(&transport, &NTRIPStream::plaintextCredentialsWarning, &transport, stop);
    } else {
        connect(&transport, &NTRIPStream::connected, &transport, stop);
    }
    transport.start();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    if (!warning) {
        QTRY_VERIFY_WITH_TIMEOUT(peer->bytesAvailable() > 0, TestTimeout::mediumMs());
        peer->write("HTTP/1.1 200 OK\r\n\r\n");
    }
    QTRY_VERIFY_WITH_TIMEOUT(stopped, TestTimeout::mediumMs());
    QVERIFY(!transport._response._socket);
    QVERIFY(!transport._response._deadline.active());
    QVERIFY(!transport._dataWatchdog.active());
    if (warning) {
        verifyExpectedLogMessage();
    }
}

void NTRIPHttpTransportTest::testNormalizedRequest_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<bool>("tls");
    QTest::addColumn<QByteArray>("authority");
    QTest::newRow("non-default") << QStringLiteral("Caster.Example.COM") << 2101 << false
                                 << QByteArray("caster.example.com:2101");
    QTest::newRow("http-default") << QStringLiteral("caster.example.com") << 80 << false
                                  << QByteArray("caster.example.com");
    QTest::newRow("https-default") << QStringLiteral("caster.example.com") << 443 << true
                                   << QByteArray("caster.example.com");
    QTest::newRow("ipv6") << QStringLiteral("2001:db8::1") << 2101 << false << QByteArray("[2001:db8::1]:2101");
    QTest::newRow("bracketed-ipv6") << QStringLiteral("[2001:db8::1]") << 8443 << true
                                    << QByteArray("[2001:db8::1]:8443");
}

void NTRIPHttpTransportTest::testNormalizedRequest()
{
    QFETCH(QString, host);
    QFETCH(int, port);
    QFETCH(bool, tls);
    QFETCH(QByteArray, authority);
    NTRIPTransportConfig config;
    config.host = host;
    config.port = port;
    config.useTls = tls;
    config.mountpoint = QString::fromUtf8("/München base?x#y%z");
    config.username = QStringLiteral("user");
    config.password = QStringLiteral("password");
    QVERIFY2(config.streamValidationError().isEmpty(), qPrintable(config.streamValidationError()));
    const auto stream = NTRIPRequest::build(config);
    const auto discovery = NTRIPRequest::build(config, true);
    QVERIFY(stream.bytes.startsWith("GET /M%C3%BCnchen%20base%3Fx%23y%25z HTTP/1.1\r\n"));
    QVERIFY(discovery.bytes.startsWith("GET / HTTP/1.1\r\n"));
    QCOMPARE(stream.headers.toListOfPairs(), discovery.headers.toListOfPairs());
    QVERIFY(stream.bytes.contains("host: " + authority + "\r\n"));
    QVERIFY(stream.bytes.contains("authorization: Basic dXNlcjpwYXNzd29yZA==\r\n"));
    QCOMPARE(stream.credentialsInClear, !tls);
    QVERIFY(stream.url.userName().isEmpty());
    QVERIFY(stream.url.password().isEmpty());
    QVERIFY(stream.url.query().isEmpty());
    QVERIFY(stream.url.fragment().isEmpty());
}

void NTRIPHttpTransportTest::testEncodedRequestReachesCaster()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = server.serverPort();
    config.mountpoint = QStringLiteral("base name?variant#1");
    NTRIPHttpTransport transport(config);
    QSignalSpy connected(&transport, &NTRIPStream::connected);
    transport.start();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QByteArray request;
    QTRY_VERIFY_WITH_TIMEOUT((request += peer->readAll()).contains("\r\n\r\n"), TestTimeout::mediumMs());
    QVERIFY(request.startsWith("GET /base%20name%3Fvariant%231 HTTP/1.1\r\n"));
    QVERIFY(request.contains("host: 127.0.0.1:" + QByteArray::number(server.serverPort()) + "\r\n"));
    peer->write("HTTP/1.1 200 OK\r\n\r\n");
    QTRY_COMPARE_WITH_TIMEOUT(connected.size(), 1, TestTimeout::mediumMs());
    transport.stop();
}

void NTRIPHttpTransportTest::testAddressValidation_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<bool>("valid");
    QTest::newRow("host") << QStringLiteral("caster.example.com") << true;
    QTest::newRow("ipv6") << QStringLiteral("::1") << true;
    QTest::newRow("ipv6-brackets") << QStringLiteral("[::1]") << true;
    QTest::newRow("url") << QStringLiteral("https://caster.example.com") << false;
    QTest::newRow("path") << QStringLiteral("caster.example.com/path") << false;
    QTest::newRow("authority") << QStringLiteral("user@caster.example.com") << false;
    QTest::newRow("port-in-host") << QStringLiteral("caster.example.com:2101") << false;
    QTest::newRow("whitespace") << QStringLiteral("caster example.com") << false;
}

void NTRIPHttpTransportTest::testAddressValidation()
{
    QFETCH(QString, host);
    QFETCH(bool, valid);
    NTRIPTransportConfig config;
    config.host = host;
    QCOMPARE(config.validationError().isEmpty(), valid);
    if (valid) {
        config.username = QStringLiteral("user\nname");
        QVERIFY(!config.validationError().isEmpty());
        config.username.clear();
        config.password = QStringLiteral("password\r");
        QVERIFY(!config.validationError().isEmpty());
    }
}

void NTRIPHttpTransportTest::testBodyBeforeMalformedChunk_data()
{
    QTest::addColumn<int>("split");
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 4);
    const QByteArray wire = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" +
                            QByteArray::number(frame.size(), 16) + "\r\n" + frame + "X";
    for (int split = 0; split <= wire.size(); ++split) {
        QTest::newRow(qPrintable(QString::number(split))) << split;
    }
}

void NTRIPHttpTransportTest::testBodyBeforeMalformedChunk()
{
    QFETCH(int, split);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = server.serverPort();
    config.mountpoint = QStringLiteral("TEST");
    NTRIPHttpTransport transport(config);
    QSignalSpy frames(&transport, &NTRIPStream::correctionReceivedAt);
    QSignalSpy failures(&transport, &NTRIPStream::failed);
    int framesAtFailure = -1;
    connect(&transport, &NTRIPStream::failed, this, [&]() { framesAtFailure = frames.size(); });
    transport.start();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QSignalSpy reads(transport._response._socket, &QTcpSocket::readyRead);
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 4);
    const QByteArray wire = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" +
                            QByteArray::number(frame.size(), 16) + "\r\n" + frame + "X";
    if (split > 0) {
        peer->write(wire.first(split));
        QTRY_VERIFY_WITH_TIMEOUT(!reads.isEmpty(), TestTimeout::mediumMs());
    }
    if (split < wire.size()) {
        peer->write(wire.sliced(split));
    }
    QTRY_COMPARE_WITH_TIMEOUT(failures.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(frames.size(), 1);
    QCOMPARE(framesAtFailure, 1);
    QCOMPARE(frames.first().first().toByteArray(), frame);
    const auto failure = qvariant_cast<NTRIPFailure>(failures.first().first());
    QCOMPARE(failure.code, NTRIPError::InvalidHttpResponse);
    QVERIFY(!failure.retryable);
}

void NTRIPHttpTransportTest::testEofFinalization_data()
{
    QTest::addColumn<QByteArray>("wire");
    QTest::addColumn<int>("frameCount");
    QTest::addColumn<NTRIPError>("expectedError");
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 150);
    const QByteArray identity = "HTTP/1.1 200 OK\r\n\r\n";
    const QByteArray chunked = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n";
    QTest::newRow("headers") << QByteArray("HTTP/1.1 200 OK\r\nContent-Type:") << 0 << NTRIPError::InterruptedResponse;
    QTest::newRow("chunk-payload") << (chunked + QByteArray::number(frame.size() + 1, 16) + "\r\n" + frame) << 1
                                   << NTRIPError::InterruptedResponse;
    QTest::newRow("chunk-terminator") << (chunked + QByteArray::number(frame.size(), 16) + "\r\n" + frame + "\r") << 1
                                      << NTRIPError::InterruptedResponse;
    QTest::newRow("trailers") << (chunked + "0\r\nTrailer:") << 0 << NTRIPError::InterruptedResponse;
    QTest::newRow("content-length") << (QByteArray("HTTP/1.1 200 OK\r\nContent-Length: ") +
                                        QByteArray::number(frame.size() + 1) + "\r\n\r\n" + frame)
                                    << 1 << NTRIPError::InterruptedResponse;
    QTest::newRow("multiple-read-budgets")
        << (identity + frame.repeated(1000)) << 1000 << NTRIPError::ServerDisconnected;
    QTest::newRow("complete-chunk") << (chunked + QByteArray::number(frame.size(), 16) + "\r\n" + frame +
                                        "\r\n0\r\n\r\n")
                                    << 1 << NTRIPError::ServerDisconnected;
}

void NTRIPHttpTransportTest::testEofFinalization()
{
    QFETCH(QByteArray, wire);
    QFETCH(int, frameCount);
    QFETCH(NTRIPError, expectedError);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = server.serverPort();
    config.mountpoint = QStringLiteral("TEST");
    NTRIPHttpTransport transport(config);
    QSignalSpy frames(&transport, &NTRIPStream::correctionReceivedAt);
    QSignalSpy errors(&transport, &NTRIPStream::failed);
    int framesAtError = -1;
    connect(&transport, &NTRIPStream::failed, this, [&]() { framesAtError = frames.size(); });
    transport.start();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QTRY_VERIFY_WITH_TIMEOUT(peer->bytesAvailable() > 0, TestTimeout::mediumMs());
    peer->readAll();
    QCOMPARE(peer->write(wire), wire.size());
    peer->disconnectFromHost();
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::mediumMs());
    QVERIFY(qvariant_cast<NTRIPFailure>(errors.first().first()).retryable);
    QCOMPARE(frames.size(), frameCount);
    QCOMPARE(framesAtError, frameCount);
    QCOMPARE(qvariant_cast<NTRIPFailure>(errors.first().first()).code, expectedError);
    QVERIFY(!transport._response._deadline.active());
    QVERIFY(!transport._dataWatchdog.active());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(errors.size(), 1);
}

void NTRIPHttpTransportTest::testBodyObserverRetiresAttempt_data()
{
    QTest::addColumn<bool>("restart");
    QTest::newRow("stop") << false;
    QTest::newRow("restart") << true;
}

void NTRIPHttpTransportTest::testBodyObserverRetiresAttempt()
{
    QFETCH(bool, restart);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = server.serverPort();
    config.mountpoint = QStringLiteral("TEST");
    NTRIPHttpTransport transport(config);
    QSignalSpy frames(&transport, &NTRIPStream::correctionReceivedAt);
    QSignalSpy failures(&transport, &NTRIPStream::failed);
    transport.start();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    connect(&transport, &NTRIPStream::correctionReceivedAt, this, [&]() {
        transport.stop();
        if (restart) {
            transport.start();
        }
    });
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 4);
    const QByteArray wire = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" +
                            QByteArray::number(frame.size(), 16) + "\r\n" + frame + "X";
    peer->write(wire);
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 1, TestTimeout::mediumMs());
    if (restart) {
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    }
    QVERIFY(failures.isEmpty());
    transport.stop();
}

void NTRIPHttpTransportTest::testSourceTableRejectsMountpoint_data()
{
    QTest::addColumn<QByteArray>("response");
    QTest::newRow("legacy") << QByteArray("SOURCETABLE 200 OK\r\n\r\nENDSOURCETABLE\r\n");
    QTest::newRow("http") << QByteArray(
        "HTTP/1.1 200 OK\r\nContent-Type: gnss/sourcetable; charset=utf-8\r\n"
        "Content-Length: 16\r\n\r\nENDSOURCETABLE\r\n");
}

void NTRIPHttpTransportTest::testSourceTableRejectsMountpoint()
{
    QFETCH(QByteArray, response);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = server.serverPort();
    config.mountpoint = QStringLiteral("missing");
    NTRIPHttpTransport transport(config);
    QSignalSpy connected(&transport, &NTRIPStream::connected);
    QSignalSpy failures(&transport, &NTRIPStream::failed);
    transport.start();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QTRY_VERIFY_WITH_TIMEOUT(peer->bytesAvailable() > 0, TestTimeout::mediumMs());
    peer->readAll();
    peer->write(response);
    QTRY_COMPARE_WITH_TIMEOUT(failures.size(), 1, TestTimeout::mediumMs());
    const auto failure = qvariant_cast<NTRIPFailure>(failures.first().first());
    QCOMPARE(failure.code, NTRIPError::InvalidMountpoint);
    QVERIFY(!failure.retryable);
    QVERIFY(connected.isEmpty());
}
