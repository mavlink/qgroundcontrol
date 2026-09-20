#include "NTRIPHttpTransportTest.h"

#include <chrono>
#include <memory>

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
#include "NTRIPConfiguration.h"
#include "NTRIPError.h"
#include "NTRIPHttpDecoder.h"
#include "NTRIPHttpRequest.h"
#include "NTRIPHttpTransport.h"
#include "RTCMDecodedFrame.h"

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
    NTRIPConnectionConfig cfg;
    QVERIFY(!cfg.isValid());  // empty host
}

void NTRIPHttpTransportTest::testConfigValidGood()
{
    NTRIPConnectionConfig cfg;
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
    NTRIPConnectionConfig cfg;
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
    NTRIPConnectionConfig cfg;
    cfg.host = QStringLiteral("caster.example.com");
    cfg.port = 2101;
    cfg.username = QStringLiteral("user:name");
    QVERIFY(!cfg.isValid());

    cfg.username = QStringLiteral("username");
    QVERIFY(cfg.isValid());
}

void NTRIPHttpTransportTest::testConfigRejectsControlChars()
{
    NTRIPConnectionConfig cfg;
    cfg.port = 2101;

    cfg.host = QStringLiteral("caster.example.com\r\nEvil: header");
    QVERIFY(!cfg.isValid());

    cfg.host = QStringLiteral("caster.example.com");
    cfg.mountpoint = QStringLiteral("MP\r\nInjected");
    QVERIFY(!cfg.isValid());

    cfg.mountpoint = QStringLiteral("MP1");
    QVERIFY(cfg.isValid());
}

void NTRIPHttpTransportTest::testConfigurationDomainsCompareIndependently()
{
    NTRIPConfiguration baseline;
    baseline.connection.host = QStringLiteral("caster.example.com");
    baseline.connection.username = QStringLiteral("user");
    baseline.connection.password = QStringLiteral("pass");
    baseline.connection.mountpoint = QStringLiteral("MOUNT1");
    baseline.connection.useTls = true;
    baseline.filter.whitelist = QStringLiteral("1005,1077");
    baseline.udpForward = {.enabled = true, .address = QStringLiteral("127.0.0.1"), .port = 3000};

    auto changed = baseline;
    QCOMPARE(changed, baseline);
    changed.connection.host = QStringLiteral("other.example.com");
    QVERIFY(changed != baseline);
    QVERIFY(changed.connection != baseline.connection);
    QCOMPARE(changed.udpForward, baseline.udpForward);
    QCOMPARE(changed.filter, baseline.filter);

    changed = baseline;
    changed.udpForward.port = 3001;
    QVERIFY(changed != baseline);
    QCOMPARE(changed.connection, baseline.connection);
    QVERIFY(changed.udpForward != baseline.udpForward);
    QCOMPARE(changed.filter, baseline.filter);

    changed = baseline;
    changed.filter.whitelist = QStringLiteral("1005");
    QVERIFY(changed != baseline);
    QCOMPARE(changed.connection, baseline.connection);
    QCOMPARE(changed.udpForward, baseline.udpForward);
    QVERIFY(changed.filter != baseline.filter);
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

    NTRIPConnectionConfig cfg;
    cfg.host = QStringLiteral("127.0.0.1");
    cfg.port = server.serverPort();
    cfg.useTls = true;
    cfg.allowSelfSignedCerts = false;
    cfg.mountpoint = QStringLiteral("TEST");

    ignoreLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg, QRegularExpression(QStringLiteral("TLS error:")));
    ignoreLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Rejecting self-signed certificate")));

    NTRIPHttpTransport transport(cfg, {});
    QSignalSpy errorSpy(&transport, &NTRIPHttpTransport::error);
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

void NTRIPHttpTransportTest::_testWhitelist_data()
{
    QTest::addColumn<QString>("whitelist");
    QTest::addColumn<QList<int>>("expectedIds");
    QTest::newRow("empty") << QString() << QList<int>{1005, 1077, 1087};
    QTest::newRow("single") << QStringLiteral("1005") << QList<int>{1005};
    QTest::newRow("multiple") << QStringLiteral("1005,1077,1087") << QList<int>{1005, 1077, 1087};
    QTest::newRow("invalid-entries") << QStringLiteral("1005,abc,,1077") << QList<int>{1005, 1077};
}

void NTRIPHttpTransportTest::_testWhitelist()
{
    QFETCH(QString, whitelist);
    QFETCH(QList<int>, expectedIds);
    NTRIPHttpTransport transport({}, {.whitelist = whitelist});
    QList<int> receivedIds;
    connect(&transport, &NTRIPTransport::correctionFrameReceived, this, [&](const RTCMDecodedFrame& frame) {
        if (frame.valid && !frame.filtered) {
            receivedIds.append(frame.messageId);
        }
    });
    transport._parseRtcm(GpsTestHelpers::buildRtcmFrame(1005) + GpsTestHelpers::buildRtcmFrame(1077) +
                         GpsTestHelpers::buildRtcmFrame(1087));
    QCOMPARE(receivedIds, expectedIds);
}

// ---------------------------------------------------------------------------
// HTTP Status Line Parsing
// ---------------------------------------------------------------------------

void NTRIPHttpTransportTest::_testParseHttpStatus_data()
{
    QTest::addColumn<QByteArray>("line");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<int>("code");
    QTest::addColumn<QString>("reason");
    QTest::newRow("http-200") << QByteArray("HTTP/1.1 200 OK") << true << 200 << QStringLiteral("OK");
    QTest::newRow("icy-200") << QByteArray("ICY 200 OK") << true << 200 << QStringLiteral("OK");
    QTest::newRow("source-table") << QByteArray("SOURCETABLE 200 OK") << true << 200 << QStringLiteral("OK");
    QTest::newRow("unauthorized") << QByteArray("HTTP/1.0 401 Unauthorized") << true << 401
                                  << QStringLiteral("Unauthorized");
    QTest::newRow("not-found") << QByteArray("HTTP/1.1 404 Not Found") << true << 404 << QStringLiteral("Not Found");
    QTest::newRow("created") << QByteArray("HTTP/1.1 201 Created") << true << 201 << QStringLiteral("Created");
    QTest::newRow("server-error") << QByteArray("HTTP/1.0 500 Internal Server Error") << true << 500
                                  << QStringLiteral("Internal Server Error");
    QTest::newRow("no-reason") << QByteArray("HTTP/1.1 400") << true << 400 << QString();
    QTest::newRow("empty") << QByteArray() << false << 0 << QString();
    QTest::newRow("garbage") << QByteArray("garbage data") << false << 0 << QString();
    QTest::newRow("missing-protocol") << QByteArray("200 OK") << false << 0 << QString();
}

void NTRIPHttpTransportTest::_testParseHttpStatus()
{
    QFETCH(QByteArray, line);
    QFETCH(bool, valid);
    QFETCH(int, code);
    QFETCH(QString, reason);
    const auto status = NTRIPHttpDecoder::parseStatusLine(line);
    QCOMPARE(status.valid, valid);
    QCOMPARE(status.code, code);
    QCOMPARE(status.reason, reason);
}

// ---------------------------------------------------------------------------
// RTCM Filtering
// ---------------------------------------------------------------------------

void NTRIPHttpTransportTest::_testFilterNoWhitelist()
{
    NTRIPConnectionConfig cfg;
    cfg.mountpoint = QStringLiteral("TEST");
    NTRIPHttpTransport t(cfg, {});

    QVector<QByteArray> received;
    connect(&t, &NTRIPTransport::correctionFrameReceived, this, [&](const RTCMDecodedFrame& frame) {
        QVERIFY(frame.valid);
        QVERIFY(!frame.filtered);
        received.append(frame.data);
    });

    QByteArray stream = GpsTestHelpers::buildRtcmFrame(1005, 4) + GpsTestHelpers::buildRtcmFrame(1077, 8) +
                        GpsTestHelpers::buildRtcmFrame(1087, 2);
    t._parseRtcm(stream);

    QCOMPARE(received.size(), 3);
}

void NTRIPHttpTransportTest::_testFilterWithWhitelist()
{
    NTRIPConnectionConfig cfg;
    cfg.mountpoint = QStringLiteral("TEST");
    NTRIPHttpTransport t(cfg, {.whitelist = QStringLiteral("1005,1087")});
    QSignalSpy detailed(&t, &NTRIPTransport::correctionFrameReceived);

    QByteArray stream = GpsTestHelpers::buildRtcmFrame(1005, 4) + GpsTestHelpers::buildRtcmFrame(1077, 8) +
                        GpsTestHelpers::buildRtcmFrame(1087, 2);
    t._parseRtcm(stream.first(1), 100);
    t._parseRtcm(stream.sliced(1), 200);

    QCOMPARE(detailed.size(), 3);
    const auto first = qvariant_cast<RTCMDecodedFrame>(detailed[0][0]);
    const auto filtered = qvariant_cast<RTCMDecodedFrame>(detailed[1][0]);
    QCOMPARE(first.receivedAtMs, 100);
    QVERIFY(first.valid);
    QVERIFY(!first.filtered);
    QCOMPARE(first.messageId, 1005);
    QCOMPARE(filtered.receivedAtMs, 200);
    QVERIFY(filtered.valid);
    QVERIFY(filtered.filtered);
    QCOMPARE(filtered.messageId, 1077);
    const auto last = qvariant_cast<RTCMDecodedFrame>(detailed[2][0]);
    QVERIFY(last.valid && !last.filtered);
    QCOMPARE(last.messageId, 1087);
}

void NTRIPHttpTransportTest::_testFilterRejectsInvalidFrame_data()
{
    QTest::addColumn<QByteArray>("bad");
    QTest::addColumn<bool>("embedded");
    auto badCrc = GpsTestHelpers::buildRtcmFrame(1005, 4);
    badCrc.back() ^= 0xff;
    QTest::newRow("bad-crc") << badCrc << false;
    const auto good = GpsTestHelpers::buildRtcmFrame(1077, 2);
    auto enclosing = GpsTestHelpers::buildRtcmFrame(1005, good.size());
    enclosing.replace(5, good.size(), good);
    enclosing.back() ^= 0xff;
    QTest::newRow("embedded-frame-at-end-of-stream") << enclosing << true;
    QTest::newRow("reserved-header-bits") << QByteArray::fromHex("d380") << false;
    QTest::newRow("impossible-payload-length") << QByteArray::fromHex("d30001") << false;
}

void NTRIPHttpTransportTest::_testFilterRejectsInvalidFrame()
{
    QFETCH(QByteArray, bad);
    QFETCH(bool, embedded);
    NTRIPConnectionConfig cfg;
    cfg.mountpoint = QStringLiteral("TEST");
    NTRIPHttpTransport t(cfg, {});

    QSignalSpy detailed(&t, &NTRIPTransport::correctionFrameReceived);

    const QByteArray good = GpsTestHelpers::buildRtcmFrame(1077, 2);
    expectLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg, QRegularExpression(QStringLiteral("Invalid RTCM frame")));
    t._parseRtcm(embedded ? bad : bad + good, 123);
    verifyExpectedLogMessage();

    QCOMPARE(detailed.size(), 2);
    const auto rejected = qvariant_cast<RTCMDecodedFrame>(detailed[0][0]);
    QCOMPARE(rejected.data, bad);
    QVERIFY(!rejected.valid);
    const auto recovered = qvariant_cast<RTCMDecodedFrame>(detailed[1][0]);
    QVERIFY(recovered.valid);
    QCOMPARE(recovered.data, good);
    QCOMPARE(recovered.receivedAtMs, 123);
}

void NTRIPHttpTransportTest::_testFrameCallbackRetiresTransport_data()
{
    QTest::addColumn<int>("action");
    QTest::newRow("stop") << 0;
    QTest::newRow("delete") << 1;
    QTest::newRow("restart") << 2;
}

void NTRIPHttpTransportTest::_testFrameCallbackRetiresTransport()
{
    QFETCH(int, action);
    NTRIPConnectionConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.mountpoint = QStringLiteral("TEST");
    auto transport = std::make_unique<NTRIPHttpTransport>(config, NTRIPRtcmFilterConfig{});
    int acceptedFrames = 0;
    connect(transport.get(), &NTRIPTransport::correctionFrameReceived, this, [&](const RTCMDecodedFrame& result) {
        QVERIFY(result.valid && !result.filtered);
        QCOMPARE(result.receivedAtMs, 123);
        ++acceptedFrames;
        if (action == 1) {
            transport.reset();
        } else if (action == 2) {
            transport->start();
        } else {
            transport->stop();
        }
    });
    const auto frame = GpsTestHelpers::buildRtcmFrame(1005);

    transport->_parseRtcm(frame + frame, 123);

    QCOMPARE(acceptedFrames, 1);
    QCOMPARE(!transport, action == 1);
    if (transport) {
        QCOMPARE(transport->_stopped, action == 0);
        QCOMPARE(transport->_connectTimeoutTimer.isActive(), action == 2);
    }
}

// ---------------------------------------------------------------------------
// HTTP Request Building
// ---------------------------------------------------------------------------

void NTRIPHttpTransportTest::_testBuildRequestPlaintextCredentialsWarns()
{
    NTRIPConnectionConfig cfg;
    cfg.host = QStringLiteral("caster.example.com");
    cfg.mountpoint = QStringLiteral("MOUNT01");
    cfg.username = QStringLiteral("user");
    cfg.password = QStringLiteral("pass");
    cfg.useTls = false;

    const auto request = NTRIPHttpRequest::build(cfg);

    QVERIFY(request.credentialsInClear);
    QVERIFY(request.bytes.startsWith("GET /MOUNT01 HTTP/1.1\r\n"));
    QVERIFY(request.error.isEmpty());
    QVERIFY(request.bytes.contains("Host: caster.example.com\r\n"));
    QVERIFY(request.bytes.contains("Authorization: Basic "));
    QVERIFY(request.bytes.endsWith("\r\n\r\n"));
}

void NTRIPHttpTransportTest::_testBuildRequestTlsCredentialsNoWarn()
{
    NTRIPConnectionConfig cfg;
    cfg.host = QStringLiteral("caster.example.com");
    cfg.mountpoint = QStringLiteral("MOUNT01");
    cfg.username = QStringLiteral("user");
    cfg.password = QStringLiteral("pass");
    cfg.useTls = true;

    const auto request = NTRIPHttpRequest::build(cfg);

    QVERIFY(!request.credentialsInClear);
    QVERIFY(request.error.isEmpty());
    QVERIFY(request.bytes.contains("Authorization: Basic "));
}

void NTRIPHttpTransportTest::_testBuildRequestNoCredentialsNoWarn()
{
    NTRIPConnectionConfig cfg;
    cfg.host = QStringLiteral("caster.example.com");
    cfg.mountpoint = QStringLiteral("MOUNT01");
    cfg.useTls = false;

    const auto request = NTRIPHttpRequest::build(cfg);

    QVERIFY(!request.credentialsInClear);
    QVERIFY(request.error.isEmpty());
    QVERIFY(!request.bytes.contains("Authorization:"));
}

void NTRIPHttpTransportTest::_testBuildRequestPreservesValues_data()
{
    QTest::addColumn<QString>("username");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<bool>("useTls");
    for (const bool useTls : {false, true}) {
        const QByteArray suffix = useTls ? "-tls" : "-plaintext";
        QTest::newRow(("anonymous" + suffix).constData()) << QString{} << QString{} << QByteArray{} << useTls;
        QTest::newRow(("username-and-password" + suffix).constData())
            << QStringLiteral("user") << QStringLiteral("pass") << QByteArray("dXNlcjpwYXNz") << useTls;
        QTest::newRow(("username-only" + suffix).constData())
            << QStringLiteral("user") << QString{} << QByteArray("dXNlcjo=") << useTls;
        QTest::newRow(("password-only" + suffix).constData())
            << QString{} << QStringLiteral("pass") << QByteArray("OnBhc3M=") << useTls;
        QTest::newRow(("utf8-credentials" + suffix).constData())
            << QString::fromUtf8("Us\xc3\xa9r") << QString::fromUtf8("Pa\xc3\x9fs") << QByteArray("VXPDqXI6UGHDn3M=")
            << useTls;
    }
}

void NTRIPHttpTransportTest::_testBuildRequestPreservesValues()
{
    QFETCH(QString, username);
    QFETCH(QString, password);
    QFETCH(QByteArray, encoded);
    QFETCH(bool, useTls);
    NTRIPConnectionConfig config;
    config.host = QStringLiteral("Caster.Example.com");
    config.mountpoint = QStringLiteral("MixedCase_1");
    config.username = username;
    config.password = password;
    config.useTls = useTls;
    QVERIFY(config.streamValidationError().isEmpty());
    const auto request = NTRIPHttpRequest::build(config);
    const QByteArray authorization = encoded.isEmpty() ? QByteArray{} : "Authorization: Basic " + encoded + "\r\n";
    QVERIFY(request.error.isEmpty());
    QCOMPARE(request.credentialsInClear, !encoded.isEmpty() && !useTls);
    QCOMPARE(request.bytes,
             "GET /MixedCase_1 HTTP/1.1\r\n"
             "Host: Caster.Example.com\r\n"
             "Ntrip-Version: Ntrip/2.0\r\n"
             "User-Agent: NTRIP QGroundControl/1.0\r\n" +
                 authorization + "\r\n");
}

void NTRIPHttpTransportTest::_testHttpDecoderReset()
{
    NTRIPHttpDecoder decoder;
    const auto result = decoder.feed("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabc\r\n0\r\n\r\n", {});
    QVERIFY(result.connected);
    QVERIFY(result.complete);
    QVERIFY(!result.failure);
    QCOMPARE(result.body, QByteArray("abc"));
    decoder.reset();
    const auto failure = decoder.feed("HTTP/1.1 503 Unavailable\r\nRetry-After: 17\r\nContent-Length: 0\r\n\r\n", {});
    QVERIFY(failure.failure);
    QCOMPARE(failure.failure->code, NTRIPError::HttpError);
    QCOMPARE(failure.failure->retryAfter, std::chrono::seconds(17));
}

void NTRIPHttpTransportTest::_testBuildRequestRejectsInvalidConfig_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("mountpoint");
    QTest::newRow("host-crlf") << QStringLiteral("caster\r\nInjected: value") << QStringLiteral("TEST");
    QTest::newRow("host-space") << QStringLiteral("caster example") << QStringLiteral("TEST");
    QTest::newRow("host-del") << QStringLiteral("caster\x7f") << QStringLiteral("TEST");
    QTest::newRow("mountpoint-crlf") << QStringLiteral("localhost") << QStringLiteral("TEST\r\nInjected: value");
    QTest::newRow("mountpoint-space") << QStringLiteral("localhost") << QStringLiteral("TEST OTHER");
    QTest::newRow("mountpoint-del") << QStringLiteral("localhost") << QStringLiteral("TEST\x7f");
    QTest::newRow("mountpoint-empty") << QStringLiteral("localhost") << QString();
}

void NTRIPHttpTransportTest::_testBuildRequestRejectsInvalidConfig()
{
    QFETCH(QString, host);
    QFETCH(QString, mountpoint);
    NTRIPConnectionConfig config;
    config.host = host;
    config.mountpoint = mountpoint;
    config.username = QStringLiteral("user");
    config.password = QStringLiteral("pass");
    const auto request = NTRIPHttpRequest::build(config);
    QVERIFY(!request.error.isEmpty());
    QVERIFY(request.bytes.isEmpty());
    QVERIFY(!request.credentialsInClear);

    NTRIPHttpTransport transport(config, {});
    transport._socket = new QTcpSocket(&transport);
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    QSignalSpy credentialsWarning(&transport, &NTRIPTransport::plaintextCredentialsWarning);
    transport._sendHttpRequest();
    QCOMPARE(errors.size(), 1);
    QCOMPARE(qvariant_cast<NTRIPFailure>(errors.first().first()).code, NTRIPError::InvalidConfig);
    QCOMPARE(transport._socket->bytesToWrite(), 0);
    QVERIFY(credentialsWarning.isEmpty());
}

UT_REGISTER_TEST(NTRIPHttpTransportTest, TestLabel::Unit)

void NTRIPHttpTransportTest::testStreamingRequiresMountpoint()
{
    NTRIPConnectionConfig config;
    config.host = QStringLiteral("127.0.0.1");
    QVERIFY(config.validationError().isEmpty());
    for (const QString& mountpoint : {QString(), QStringLiteral("   ")}) {
        config.mountpoint = mountpoint;
        QVERIFY(!config.streamValidationError().isEmpty());
        NTRIPHttpTransport transport(config, {});
        QSignalSpy errors(&transport, &NTRIPTransport::error);
        QSignalSpy connected(&transport, &NTRIPTransport::connected);
        transport.start();
        QCOMPARE(errors.size(), 1);
        QCOMPARE(qvariant_cast<NTRIPFailure>(errors.first().first()).code, NTRIPError::InvalidConfig);
        QVERIFY(connected.isEmpty());
        QVERIFY(!transport._socket);
    }
}

void NTRIPHttpTransportTest::testConnectionWaitsForHttpResponse()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    NTRIPConnectionConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = server.serverPort();
    config.mountpoint = QStringLiteral("TEST");
    NTRIPHttpTransport transport(config, {});
    QSignalSpy connected(&transport, &NTRIPTransport::connected);
    QSignalSpy frames(&transport, &NTRIPTransport::correctionFrameReceived);
    transport.start();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QVERIFY(peer);
    QTRY_VERIFY_WITH_TIMEOUT(peer->bytesAvailable() > 0, TestTimeout::mediumMs());
    QVERIFY(peer->readAll().startsWith("GET /TEST HTTP/1.1\r\n"));
    QVERIFY(connected.isEmpty());
    QVERIFY(transport._connectTimeoutTimer.isActive());
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 4);
    const QByteArray response = "HTTP/1.1 200 OK\r\nContent-Type: gnss/data\r\n\r\n" + frame;
    QCOMPARE(peer->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(connected.size(), 1, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 1, TestTimeout::mediumMs());
    const auto result = qvariant_cast<RTCMDecodedFrame>(frames.first().first());
    QVERIFY(result.valid && !result.filtered);
    QCOMPARE(result.data, frame);
    QVERIFY(!transport._connectTimeoutTimer.isActive());
    QVERIFY(transport._dataWatchdogTimer.isActive());
    const QByteArray gga = "$GPGGA,120000,4723.8620,N,00832.7360,E,1,12,1.0,100.0,M,0.0,M,,";
    const QByteArray expected = gga + "*77\r\n";
    transport.sendNMEA(gga + "\r\n");
    QTRY_COMPARE_WITH_TIMEOUT(peer->bytesAvailable(), expected.size(), TestTimeout::mediumMs());
    QCOMPARE(peer->readAll(), expected);
    transport.stop();
}

void NTRIPHttpTransportTest::testHandshakeTimeoutClosesSocket()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    NTRIPConnectionConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = server.serverPort();
    config.mountpoint = QStringLiteral("TEST");
    NTRIPHttpTransport transport(config, {});
    QSignalSpy connected(&transport, &NTRIPTransport::connected);
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    transport.start();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QVERIFY(peer);
    QTRY_VERIFY_WITH_TIMEOUT(peer->bytesAvailable() > 0, TestTimeout::mediumMs());
    QVERIFY(connected.isEmpty());
    expectLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg, QRegularExpression(QStringLiteral("Connection timeout")));
    transport._connectTimeoutTimer.setInterval(std::chrono::milliseconds(50));
    transport._connectTimeoutTimer.start();
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(qvariant_cast<NTRIPFailure>(errors.first().first()).code, NTRIPError::ConnectionTimeout);
    QCOMPARE(transport._socket->state(), QAbstractSocket::UnconnectedState);
    QVERIFY(!transport._dataWatchdogTimer.isActive());
    QVERIFY(connected.isEmpty());
    verifyExpectedLogMessage();
}

void NTRIPHttpTransportTest::testRemoteCloseEmitsSingleError()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    NTRIPConnectionConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = server.serverPort();
    config.mountpoint = QStringLiteral("TEST");
    NTRIPHttpTransport transport(config, {});
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    transport.start();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QVERIFY(peer);
    peer->disconnectFromHost();
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(transport._socket->state(), QAbstractSocket::UnconnectedState);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(errors.size(), 1);
    QCOMPARE(qvariant_cast<NTRIPFailure>(errors.first().first()).code, NTRIPError::InvalidHttpResponse);
    QVERIFY(!transport._connectTimeoutTimer.isActive());
}
