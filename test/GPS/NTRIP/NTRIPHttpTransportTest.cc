#include "NTRIPHttpTransportTest.h"

#include <QtCore/QRegularExpression>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslServer>
#include <QtNetwork/QSslSocket>
#include <QtTest/QSignalSpy>

#include "GpsTestHelpers.h"
#include "NMEAUtils.h"
#include "NTRIPError.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPTransportConfig.h"
#include "RTCMParser.h"

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
    baseline.udpForwardEnabled = true;
    baseline.udpTargetAddress = QStringLiteral("127.0.0.1");
    baseline.udpTargetPort = 3000;

    auto changed = baseline;
    changed.host = QStringLiteral("other.example.com");
    QVERIFY(changed.transportDiffers(baseline));
    QVERIFY(!changed.udpForwardDiffers(baseline));
    QVERIFY(!changed.whitelistDiffers(baseline));

    changed = baseline;
    changed.udpTargetPort = 3001;
    QVERIFY(!changed.transportDiffers(baseline));
    QVERIFY(changed.udpForwardDiffers(baseline));
    QVERIFY(!changed.whitelistDiffers(baseline));

    changed = baseline;
    changed.whitelist = QStringLiteral("1005");
    QVERIFY(!changed.transportDiffers(baseline));
    QVERIFY(!changed.udpForwardDiffers(baseline));
    QVERIFY(changed.whitelistDiffers(baseline));
}

void NTRIPHttpTransportTest::testConfigCasterIdentityExcludesMountpointAndSinks()
{
    NTRIPTransportConfig baseline;
    baseline.host = QStringLiteral("caster.example.com");
    baseline.port = 2101;
    baseline.username = QStringLiteral("user");
    baseline.password = QStringLiteral("pass");
    baseline.mountpoint = QStringLiteral("MOUNT1");
    baseline.whitelist = QStringLiteral("1005");
    baseline.useTls = true;
    baseline.udpForwardEnabled = true;
    baseline.udpTargetAddress = QStringLiteral("127.0.0.1");
    baseline.udpTargetPort = 3000;

    auto sameCaster = baseline;
    sameCaster.mountpoint = QStringLiteral("MOUNT2");
    sameCaster.whitelist = QStringLiteral("1005,1077");
    sameCaster.udpForwardEnabled = false;
    sameCaster.udpTargetAddress = QStringLiteral("192.0.2.1");
    sameCaster.udpTargetPort = 3001;
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

    ignoreLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg, QRegularExpression(QStringLiteral("TLS error:")));
    ignoreLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Rejecting self-signed certificate")));

    NTRIPHttpTransport transport(cfg);
    QSignalSpy errorSpy(&transport, &NTRIPHttpTransport::error);
    transport.start();

    QVERIFY_SIGNAL_WAIT(errorSpy, TestTimeout::mediumMs());
    QCOMPARE(errorSpy.count(), 1);

    // abort() inside _failFatal must not re-enter the error/disconnect handlers.
    QTest::qWait(200);
    QCOMPARE(errorSpy.count(), 1);
}

// ---------------------------------------------------------------------------
// Whitelist Parsing
// ---------------------------------------------------------------------------

void NTRIPHttpTransportTest::_testWhitelistEmpty()
{
    NTRIPHttpTransport t(NTRIPTransportConfig{});
    QVERIFY(t._rtcmParser.isWhitelisted(9999));  // empty whitelist = accept all
    QVERIFY(t._rtcmParser.isWhitelisted(1005));

    NTRIPTransportConfig cfg;
    cfg.whitelist = QStringLiteral("");
    NTRIPHttpTransport t2(cfg);
    QVERIFY(t2._rtcmParser.isWhitelisted(1005));
}

void NTRIPHttpTransportTest::_testWhitelistSingle()
{
    NTRIPTransportConfig cfg;
    cfg.whitelist = QStringLiteral("1005");
    NTRIPHttpTransport t(cfg);
    QVERIFY(t._rtcmParser.isWhitelisted(1005));
    QVERIFY(!t._rtcmParser.isWhitelisted(1077));
}

void NTRIPHttpTransportTest::_testWhitelistMultiple()
{
    NTRIPTransportConfig cfg;
    cfg.whitelist = QStringLiteral("1005,1077,1087");
    NTRIPHttpTransport t(cfg);
    QVERIFY(t._rtcmParser.isWhitelisted(1005));
    QVERIFY(t._rtcmParser.isWhitelisted(1077));
    QVERIFY(t._rtcmParser.isWhitelisted(1087));
    QVERIFY(!t._rtcmParser.isWhitelisted(1234));
}

void NTRIPHttpTransportTest::_testWhitelistInvalidEntries()
{
    NTRIPTransportConfig cfg;
    cfg.whitelist = QStringLiteral("1005,abc,,1077");
    NTRIPHttpTransport t(cfg);
    QVERIFY(t._rtcmParser.isWhitelisted(1005));
    QVERIFY(t._rtcmParser.isWhitelisted(1077));
    QVERIFY(!t._rtcmParser.isWhitelisted(9999));
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
    connect(&t, &NTRIPHttpTransport::RTCMDataUpdate, this, [&](const QByteArray& msg) { received.append(msg); });

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
    connect(&t, &NTRIPHttpTransport::RTCMDataUpdate, this, [&](const QByteArray& msg) {
        if (msg.size() >= 5) {
            uint16_t id = (static_cast<uint8_t>(msg[3]) << 4) | (static_cast<uint8_t>(msg[4]) >> 4);
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

    int count = 0;
    connect(&t, &NTRIPHttpTransport::RTCMDataUpdate, this, [&](const QByteArray&) { count++; });

    QByteArray bad = GpsTestHelpers::buildRtcmFrame(1005, 4);
    bad[bad.size() - 1] = static_cast<char>(bad[bad.size() - 1] ^ 0xFF);

    QByteArray good = GpsTestHelpers::buildRtcmFrame(1077, 2);

    expectLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg, QRegularExpression(QStringLiteral("RTCM CRC mismatch")));
    t._parseRtcm(bad + good);
    verifyExpectedLogMessage();

    QCOMPARE(count, 1);
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

    const auto request = NTRIPHttpTransport::buildHttpRequest(cfg);

    QVERIFY(request.credentialsInClear);
    QVERIFY(request.bytes.startsWith("GET /MOUNT01 HTTP/1.1\r\n"));
    QVERIFY(request.bytes.contains("Host: caster.example.com\r\n"));
    QVERIFY(request.bytes.contains("Authorization: Basic "));
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

    const auto request = NTRIPHttpTransport::buildHttpRequest(cfg);

    QVERIFY(!request.credentialsInClear);
    QVERIFY(request.bytes.contains("Authorization: Basic "));
}

void NTRIPHttpTransportTest::_testBuildRequestNoCredentialsNoWarn()
{
    NTRIPTransportConfig cfg;
    cfg.host = QStringLiteral("caster.example.com");
    cfg.mountpoint = QStringLiteral("MOUNT01");
    cfg.useTls = false;

    const auto request = NTRIPHttpTransport::buildHttpRequest(cfg);

    QVERIFY(!request.credentialsInClear);
    QVERIFY(!request.bytes.contains("Authorization"));
}

UT_REGISTER_TEST(NTRIPHttpTransportTest, TestLabel::Unit)
