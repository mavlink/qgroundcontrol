#include "NTRIPSourceTableControllerTest.h"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QUrl>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslServer>
#include <QtNetwork/QSslSocket>
#include <QtHttpServer/QHttpServer>
#include <QtHttpServer/QHttpServerResponse>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "LocalHttpTestServer.h"
#include "NTRIPSettings.h"
#include "NTRIPSourceTable.h"
#include "NTRIPSourceTableController.h"
#include "NTRIPTransportConfig.h"
#include "SettingsManager.h"

static NTRIPTransportConfig casterConfig(const QString& host, int port = 2101)
{
    NTRIPTransportConfig config;
    config.host = host;
    config.port = port;
    return config;
}

static const QString kValidTable = QStringLiteral(
    "SOURCETABLE 200 OK\r\n"
    "STR;MP1;Id1;RTCM 3.2;details;2;GPS;NET;USA;40.0;-74.0;0;1;gen;none;B;N;4800;misc\r\n"
    "ENDSOURCETABLE\r\n");

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

void NTRIPSourceTableControllerTest::testInitialState()
{
    NTRIPSourceTableController ctrl;

    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Idle);
    QVERIFY(ctrl.fetchError().isEmpty());
    QVERIFY(ctrl.mountpointModel() != nullptr);
    QCOMPARE(ctrl.mountpointModel()->rowCount(), 0);
}

void NTRIPSourceTableControllerTest::testFetchEmptyHostTriggersError()
{
    NTRIPSourceTableController ctrl;
    QSignalSpy statusSpy(&ctrl, &NTRIPSourceTableController::fetchStatusChanged);
    QSignalSpy errorSpy(&ctrl, &NTRIPSourceTableController::fetchErrorChanged);

    ctrl.fetch(casterConfig(QString()));

    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QVERIFY(!ctrl.fetchError().isEmpty());
    QVERIFY(statusSpy.count() >= 1);
    QVERIFY(errorSpy.count() >= 1);
}

void NTRIPSourceTableControllerTest::testFetchInvalidConfigTriggersError()
{
    NTRIPSourceTableController ctrl;

    NTRIPTransportConfig config = casterConfig(QStringLiteral("caster.example.com"));
    config.username = QStringLiteral("bad:user");

    ctrl.fetch(config);

    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QVERIFY(!ctrl.fetchError().isEmpty());
}

void NTRIPSourceTableControllerTest::testFetchErrorInvalidatesCache()
{
    NTRIPSourceTableController ctrl;
    const NTRIPTransportConfig config = casterConfig(QStringLiteral("caster.example.com"));

    ctrl.fetch(config);
    ctrl.injectSourceTableForTest(kValidTable);
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);
    QCOMPARE(ctrl.mountpointModel()->rowCount(), 1);

    ctrl.injectFetchErrorForTest(QStringLiteral("network down"));
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QCOMPARE(ctrl.mountpointModel()->rowCount(), 0);

    ctrl.fetch(config);
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);
}

void NTRIPSourceTableControllerTest::testFetchValidHostGoesInProgress()
{
    NTRIPSourceTableController ctrl;
    QSignalSpy statusSpy(&ctrl, &NTRIPSourceTableController::fetchStatusChanged);

    ctrl.fetch(casterConfig(QStringLiteral("caster.example.com")));

    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);
    QVERIFY(statusSpy.count() >= 1);
}

void NTRIPSourceTableControllerTest::testFetchAbortsOversizedSourceTable()
{
    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    server.installHttpResponder(QByteArray(9 * 1024 * 1024, 'X'), 200, "text/plain");

    const QUrl base(server.url());
    NTRIPTransportConfig config;
    config.host = base.host();
    config.port = base.port();
    config.useTls = false;

    NTRIPSourceTableController ctrl;
    ctrl.fetch(config);

    QCOMPARE_TRUE_WAIT(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error, TestTimeout::mediumMs());
    QVERIFY(ctrl.fetchError().contains(QStringLiteral("too large")));
}

void NTRIPSourceTableControllerTest::testFetchAllowsSelfSignedSourceTableWhenConfigured()
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

    QHttpServer httpServer;
    QSslServer server;
    server.setSslConfiguration(sslConfig);
    QVERIFY(server.listen(QHostAddress::LocalHost));

    httpServer.route("/", []() {
        return QHttpServerResponse("text/plain", kValidTable.toUtf8());
    });
    QVERIFY(httpServer.bind(&server));

    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = server.serverPort();
    config.useTls = true;
    config.allowSelfSignedCerts = true;

    NTRIPSourceTableController ctrl;
    ctrl.fetch(config);

    QTRY_VERIFY_WITH_TIMEOUT(ctrl.fetchStatus() != NTRIPSourceTableController::FetchStatus::InProgress,
                             TestTimeout::mediumMs());
    QVERIFY2(ctrl.fetchStatus() == NTRIPSourceTableController::FetchStatus::Success,
             qPrintable(ctrl.fetchError()));
    QCOMPARE(ctrl.mountpointModel()->rowCount(), 1);
}

void NTRIPSourceTableControllerTest::testCacheTtlPreventsFetch()
{
    NTRIPSourceTableController ctrl;

    ctrl.fetch(casterConfig(QStringLiteral("caster.example.com")));
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);

    ctrl.injectSourceTableForTest(kValidTable);

    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);
    QVERIFY(ctrl.mountpointModel() != nullptr);
    QVERIFY(ctrl.mountpointModel()->rowCount() > 0);

    QSignalSpy statusSpy(&ctrl, &NTRIPSourceTableController::fetchStatusChanged);
    ctrl.fetch(casterConfig(QStringLiteral("caster.example.com")));

    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);
    QVERIFY(statusSpy.count() >= 1);
}

void NTRIPSourceTableControllerTest::testConfigChangeInvalidatesCache()
{
    NTRIPSourceTableController ctrl;

    ctrl.fetch(casterConfig(QStringLiteral("caster.example.com")));
    ctrl.injectSourceTableForTest(kValidTable);
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);

    ctrl.fetch(casterConfig(QStringLiteral("other.caster.com")));
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);

    ctrl.injectSourceTableForTest(kValidTable);
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);

    ctrl.fetch(casterConfig(QStringLiteral("other.caster.com"), 9999));
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);
}

void NTRIPSourceTableControllerTest::testSelectMountpointEmitsSignal()
{
    NTRIPSourceTableController ctrl;
    QSignalSpy spy(&ctrl, &NTRIPSourceTableController::mountpointSelected);

    ctrl.selectMountpoint(QStringLiteral("TEST_MP"));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("TEST_MP"));
}

UT_REGISTER_TEST(NTRIPSourceTableControllerTest, TestLabel::Unit)
