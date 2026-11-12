#include "NTRIPHttpSessionTest.h"

#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "MultiSignalSpy.h"
#include "NTRIP/Support/NTRIPTestHelpers.h"
#include "NTRIP/Support/ScriptedNTRIPCaster.h"
#include "NTRIPHttpSession.h"
#include "QGCNetworkClient.h"
#include "Support/GPSTestHelpers.h"

using namespace GPSTest;

namespace {
QByteArray requestFor(const NTRIPConnectionConfig& config)
{
    return NTRIPHttpRequest::build(config).bytes;
}

QString disconnectedBeforeResponse()
{
    return QCoreApplication::translate("NTRIPHttpTransport", "Caster disconnected before completing the HTTP response");
}
}  // namespace

void NTRIPHttpSessionTest::_responseEndsWithFinished()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    NTRIPHttpSession session;
    QByteArray received;
    QStringList events;
    connect(&session, &NTRIPHttpSession::responseStarted, this, [&]() {
        events << QStringLiteral("started");
        QVERIFY(session.isConnected());
    });
    connect(&session, &NTRIPHttpSession::bodyReceived, this, [&](const QByteArray& bytes, qint64 receivedAtMs) {
        QVERIFY(receivedAtMs > 0);
        received += bytes;
    });
    connect(&session, &NTRIPHttpSession::finished, this, [&]() {
        events << QStringLiteral("finished:%1").arg(received.size());
        QVERIFY(!session.isConnected());
    });
    QSignalSpy failed(&session, &NTRIPHttpSession::failed);
    QVERIFY(session.open(caster.connectionConfig()).isEmpty());
    QVERIFY(session.open(connectionConfig(1)).isEmpty());

    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    QCOMPARE(connection->waitForRequest(), requestFor(caster.connectionConfig()));
    const QByteArray body(3 * NTRIPHttpSession::READ_CHUNK_BYTES + 7, 'x');
    const QByteArray response = "HTTP/1.1 200 OK\r\n\r\n" + body;
    QCOMPARE(connection->write(response), response.size());
    connection->disconnectFromHost();

    QTRY_COMPARE_WITH_TIMEOUT(events.size(), 2, TestTimeout::mediumMs());
    QCOMPARE(events, (QStringList{QStringLiteral("started"), QStringLiteral("finished:%1").arg(body.size())}));
    QCOMPARE(received, body);
    QVERIFY(failed.isEmpty());
    QVERIFY(!session.write(QByteArrayLiteral("late")));
}

void NTRIPHttpSessionTest::_invalidConfigDoesNotConnect()
{
    NTRIPHttpSession session;
    MultiSignalSpy spy;
    QVERIFY(spy.init(&session));
    const NTRIPConnectionConfig withoutMountpoint = connectionConfig(2101, QString());
    const QString error = session.open(withoutMountpoint);
    QVERIFY(!error.isEmpty());
    QCOMPARE(error, withoutMountpoint.streamValidationError());
    QVERIFY(!session.isConnected());
    deliverQueuedCalls();
    QVERIFY_NO_SIGNALS(spy);
}

void NTRIPHttpSessionTest::_refusedConnectionFailsOnce()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    const quint16 port = caster.port();
    caster.close();
    ignoreLogMessage("GPS.NTRIP.NTRIPHttpSession", QtWarningMsg, QRegularExpression(QStringLiteral("^Socket error")));
    NTRIPHttpSession session;
    MultiSignalSpy spy;
    QVERIFY(spy.init(&session));
    QVERIFY(session.open(connectionConfig(port)).isEmpty());
    QVERIFY_WAIT_SIGNAL(spy, "failed", TestTimeout::mediumMs());
    const auto failure = spy.argument<NTRIPFailure>("failed");
    QCOMPARE(failure.code, NTRIPError::SocketError);
    QVERIFY(!failure.detail.isEmpty());
    deliverQueuedCalls();
    QVERIFY_ONLY_SIGNAL(spy, "failed");
}

void NTRIPHttpSessionTest::_unansweredRequestIsRepeatedInVersion1_data()
{
    QTest::addColumn<bool>("credentials");
    QTest::newRow("anonymous") << false;
    QTest::newRow("with-credentials") << true;
}

void NTRIPHttpSessionTest::_unansweredRequestIsRepeatedInVersion1()
{
    QFETCH(bool, credentials);
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    auto config = caster.connectionConfig();
    if (credentials) {
        config.username = QStringLiteral("user");
        config.password = QStringLiteral("pass");
    }
    NTRIPHttpSession session;
    MultiSignalSpy spy;
    QVERIFY(spy.init(&session));
    QByteArray received;
    connect(&session, &NTRIPHttpSession::bodyReceived, this, [&](const QByteArray& bytes) { received += bytes; });
    expectLogMessage("GPS.NTRIP.NTRIPHttpSession", QtWarningMsg, repeatedInVersion1());
    QVERIFY(session.open(config).isEmpty());

    auto* connection = caster.waitForVersion1Request();
    QVERIFY(connection && connection->peer);
    verifyExpectedLogMessage();
    QCOMPARE(caster.requestLines(), (QList<QByteArray>{"GET /TEST HTTP/1.1", "GET /TEST HTTP/1.0"}));
    // The request of RTKLIB's client (reqntrip_c in src/stream.c), which sends neither Host nor Ntrip-Version.
    QCOMPARE(connection->request,
             "GET /TEST HTTP/1.0\r\nUser-Agent: NTRIP " + QGCNetworkHelper::defaultUserAgent().toLatin1() + "\r\n" +
                 (credentials ? QByteArray("Authorization: Basic dXNlcjpwYXNz\r\n") : QByteArray()) + "\r\n");

    // Binary, as a stream is: printable text after the status line could still be a header.
    const QByteArray body(64, '\xd3');
    const QByteArray response = "ICY 200 OK\r\n" + body;
    QCOMPARE(connection->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(received, body, TestTimeout::mediumMs());
    // The owner sees one exchange: the unanswered request reports nothing.
    QCOMPARE(spy.count("responseStarted"), 1);
    QVERIFY_NO_SIGNAL(spy, "failed");
    session.abort();
}

void NTRIPHttpSessionTest::_requestIsRepeatedOnlyOnce()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    NTRIPHttpSession session;
    MultiSignalSpy spy;
    QVERIFY(spy.init(&session));
    // A second repeat would log again, which fails the test.
    expectLogMessage("GPS.NTRIP.NTRIPHttpSession", QtWarningMsg, repeatedInVersion1());
    QVERIFY(session.open(caster.connectionConfig()).isEmpty());
    auto* connection = caster.waitForVersion1Request();
    QVERIFY(connection && connection->peer);
    verifyExpectedLogMessage();
    connection->disconnectFromHost();
    QVERIFY_WAIT_SIGNAL(spy, "failed", TestTimeout::mediumMs());
    const auto failure = spy.argument<NTRIPFailure>("failed");
    QCOMPARE(failure.code, NTRIPError::InvalidHttpResponse);
    QCOMPARE(failure.detail, disconnectedBeforeResponse());
    deliverQueuedCalls();
    QVERIFY_ONLY_SIGNAL(spy, "failed");
    QCOMPARE(caster.requestLines().size(), 2);
}

void NTRIPHttpSessionTest::_answeredRequestIsNotRepeated()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    NTRIPHttpSession session;
    MultiSignalSpy spy;
    QVERIFY(spy.init(&session));
    QVERIFY(session.open(caster.connectionConfig()).isEmpty());
    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    QCOMPARE(connection->waitForRequest(), requestFor(caster.connectionConfig()));
    // One byte of an answer is enough: the caster read the request, so another form of it is not tried. A repeat
    // would be refused, and would log.
    caster.close();
    QCOMPARE(connection->write("H"), 1);
    connection->disconnectFromHost();
    QVERIFY_WAIT_SIGNAL(spy, "failed", TestTimeout::mediumMs());
    const auto failure = spy.argument<NTRIPFailure>("failed");
    QCOMPARE(failure.code, NTRIPError::InvalidHttpResponse);
    QCOMPARE(failure.detail, disconnectedBeforeResponse());
    deliverQueuedCalls();
    // The byte is reported as having arrived, and nothing else is made of it.
    QCOMPARE(spy.count("failed"), 1);
    QCOMPARE(spy.count("dataReceived"), 1);
    QCOMPARE(spy.argument<qint64>("dataReceived"), qint64(1));
    QVERIFY(spy.notEmitted("certificatePinned", "responseStarted", "bodyReceived", "finished"));
}

void NTRIPHttpSessionTest::_casterVersionsKeepTheLatestCasters()
{
    const auto caster = [](qsizetype number) { return connectionConfig(2101 + static_cast<int>(number)); };
    const qsizetype most = NTRIPCasterVersions::MAX_CASTERS;
    NTRIPCasterVersions versions;
    QSignalSpy changes(&versions, &NTRIPCasterVersions::changed);
    for (qsizetype number = 0; number < most; ++number) {
        versions.setVersion(caster(number), NTRIPVersion::Version1);
    }
    QCOMPARE(changes.size(), most);
    QCOMPARE(versions.saved().split(QLatin1Char(' ')).size(), most);

    // A caster that accepts another version 1 request is the latest one again, which the latest one already is.
    versions.setVersion(caster(most - 1), NTRIPVersion::Version1);
    QCOMPARE(changes.size(), most);
    versions.setVersion(caster(0), NTRIPVersion::Version1);
    QCOMPARE(changes.size(), most + 1);

    // One more than are kept: the caster that accepted such a request longest ago makes room.
    versions.setVersion(caster(most), NTRIPVersion::Version1);
    QCOMPARE(changes.size(), most + 2);
    QCOMPARE(versions.versionFor(caster(1)), NTRIPVersion::Version2);
    for (const qsizetype kept : {qsizetype(0), qsizetype(2), most}) {
        QCOMPARE(versions.versionFor(caster(kept)), NTRIPVersion::Version1);
    }
    QCOMPARE(versions.saved().split(QLatin1Char(' ')).size(), most);

    // Forgetting a caster that is not remembered changes nothing.
    versions.setVersion(caster(1), NTRIPVersion::Version2);
    QCOMPARE(changes.size(), most + 2);

    // The caster is its host, port and TLS: neither the mountpoint nor the account.
    auto other = caster(0);
    other.mountpoint = QStringLiteral("OTHER");
    other.username = QStringLiteral("user");
    QCOMPARE(versions.versionFor(other), NTRIPVersion::Version1);
    other.useTls = true;
    QCOMPARE(versions.versionFor(other), NTRIPVersion::Version2);
}

void NTRIPHttpSessionTest::_casterVersionsAreSavedAndRestored_data()
{
    QTest::addColumn<QString>("saved");
    QTest::addColumn<QString>("restored");
    const QString plain = QStringLiteral("http://127.0.0.1:2101/");
    const QString tls = QStringLiteral("https://caster.example.com:443/");
    QTest::newRow("none") << QString() << QString();
    QTest::newRow("as-saved") << plain + QLatin1Char(' ') + tls << plain + QLatin1Char(' ') + tls;
    // What another hand wrote: each caster once, where it was named last.
    QTest::newRow("spaces-and-repeats") << QStringLiteral("  ") + plain + QStringLiteral("   ") + tls +
                                               QStringLiteral("\n") + plain + QLatin1Char(' ')
                                        << tls + QLatin1Char(' ') + plain;
    QStringList many;
    for (int port = 1; port <= NTRIPCasterVersions::MAX_CASTERS + 4; ++port) {
        many.append(QStringLiteral("http://caster.example.com:%1/").arg(port));
    }
    QTest::newRow("more-than-are-kept") << many.join(QLatin1Char(' '))
                                        << many.last(NTRIPCasterVersions::MAX_CASTERS).join(QLatin1Char(' '));
}

void NTRIPHttpSessionTest::_casterVersionsAreSavedAndRestored()
{
    QFETCH(QString, saved);
    QFETCH(QString, restored);
    NTRIPCasterVersions versions;
    versions.setVersion(connectionConfig(4000), NTRIPVersion::Version1);
    QSignalSpy changes(&versions, &NTRIPCasterVersions::changed);
    versions.restore(saved);
    QCOMPARE(versions.saved(), restored);
    QCOMPARE(changes.size(), 1);
    // The casters restored take the place of the ones remembered.
    QCOMPARE(versions.versionFor(connectionConfig(4000)), NTRIPVersion::Version2);
    QCOMPARE(versions.versionFor(connectionConfig(2101)),
             restored.contains(QStringLiteral("127.0.0.1:2101")) ? NTRIPVersion::Version1 : NTRIPVersion::Version2);

    // What was saved is restored as it is.
    versions.restore(versions.saved());
    QCOMPARE(versions.saved(), restored);
    QCOMPARE(changes.size(), 1);
}

void NTRIPHttpSessionTest::_version1MemoryFollowsTheCaster_data()
{
    // What the caster answers a version 1 request with; without an answer it closes the connection.
    QTest::addColumn<QByteArray>("answer");
    QTest::addColumn<bool>("remembered");
    QTest::newRow("accepted") << QByteArray("ICY 200 OK\r\n") << true;
    QTest::newRow("unauthorized") << QByteArray("HTTP/1.0 401 Unauthorized\r\n\r\n") << false;
    // As a server that takes HTTP/1.1 requests only answers one without a Host header.
    QTest::newRow("bad-request") << QByteArray("HTTP/1.1 400 Bad Request\r\n\r\n") << false;
    QTest::newRow("no-answer") << QByteArray() << false;
}

void NTRIPHttpSessionTest::_version1MemoryFollowsTheCaster()
{
    QFETCH(QByteArray, answer);
    QFETCH(bool, remembered);
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    const auto config = caster.connectionConfig();
    const auto versions = std::make_shared<NTRIPCasterVersions>();
    versions->setVersion(config, NTRIPVersion::Version1);
    QSignalSpy changes(versions.get(), &NTRIPCasterVersions::changed);
    NTRIPHttpSession session;
    MultiSignalSpy spy;
    QVERIFY(spy.init(&session));
    QVERIFY(session.open(config, versions).isEmpty());

    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest().startsWith("GET /TEST HTTP/1.0\r\n"));
    QCOMPARE(connection->write(answer), answer.size());
    if (remembered) {
        QVERIFY_WAIT_SIGNAL(spy, "responseStarted", TestTimeout::mediumMs());
    } else {
        connection->disconnectFromHost();
        QVERIFY_WAIT_SIGNAL(spy, "failed", TestTimeout::mediumMs());
    }
    // A caster that does not accept the request is asked in version 2 form next time; this request is not repeated.
    QCOMPARE(versions->versionFor(config), remembered ? NTRIPVersion::Version1 : NTRIPVersion::Version2);
    QCOMPARE(changes.size(), remembered ? 0 : 1);
    QCOMPARE(caster.requestLines().size(), 1);
    session.abort();
}

void NTRIPHttpSessionTest::_version1MemoryOutlastsLocalFailures()
{
    const auto versions = std::make_shared<NTRIPCasterVersions>();
    QSignalSpy changes(versions.get(), &NTRIPCasterVersions::changed);

    // A connection that is not made says nothing of the caster.
    {
        ScriptedNTRIPCaster caster;
        QVERIFY(caster.isListening());
        const auto config = caster.connectionConfig();
        caster.close();
        versions->setVersion(config, NTRIPVersion::Version1);
        ignoreLogMessage("GPS.NTRIP.NTRIPHttpSession", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Socket error")));
        NTRIPHttpSession session;
        MultiSignalSpy spy;
        QVERIFY(spy.init(&session));
        QVERIFY(session.open(config, versions).isEmpty());
        QVERIFY_WAIT_SIGNAL(spy, "failed", TestTimeout::mediumMs());
        QCOMPARE(versions->versionFor(config), NTRIPVersion::Version1);
    }

    // Nor does a request its owner gives up before the caster has answered, as at a timeout.
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    const auto config = caster.connectionConfig();
    versions->setVersion(config, NTRIPVersion::Version1);
    NTRIPHttpSession session;
    QVERIFY(session.open(config, versions).isEmpty());
    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest().startsWith("GET /TEST HTTP/1.0\r\n"));
    session.abort();
    QCOMPARE(versions->versionFor(config), NTRIPVersion::Version1);
    QCOMPARE(changes.size(), 2);
}

void NTRIPHttpSessionTest::_acceptedVersion2RequestForgetsVersion1()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    const auto config = caster.connectionConfig();
    const auto versions = std::make_shared<NTRIPCasterVersions>();
    NTRIPHttpSession session;
    MultiSignalSpy spy;
    QVERIFY(spy.init(&session));
    QVERIFY(session.open(config, versions).isEmpty());
    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest().startsWith("GET /TEST HTTP/1.1\r\n"));

    // The caster is remembered while the version 2 request is under way, as restored settings do. Its answer to that
    // request is the later evidence.
    versions->setVersion(config, NTRIPVersion::Version1);
    const QByteArray response = "HTTP/1.1 200 OK\r\n\r\n" + QByteArray(16, '\xd3');
    QCOMPARE(connection->write(response), response.size());
    QVERIFY_WAIT_SIGNAL(spy, "responseStarted", TestTimeout::mediumMs());
    QCOMPARE(versions->versionFor(config), NTRIPVersion::Version2);
    session.abort();
}

void NTRIPHttpSessionTest::_abortIsSilent()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    NTRIPHttpSession session;
    MultiSignalSpy spy;
    QVERIFY(spy.init(&session));
    QVERIFY(session.open(caster.connectionConfig()).isEmpty());
    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    QCOMPARE(connection->waitForRequest(), requestFor(caster.connectionConfig()));
    session.abort();
    QVERIFY(!session.isConnected());
    QTRY_COMPARE_WITH_TIMEOUT(connection->peer->state(), QAbstractSocket::UnconnectedState, TestTimeout::mediumMs());
    deliverQueuedCalls();
    QVERIFY_NO_SIGNALS(spy);
}

void NTRIPHttpSessionTest::_retireFromDeliveryDetachesOwner()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    auto owner = std::make_unique<QObject>();
    QPointer<NTRIPHttpSession> session = new NTRIPHttpSession(owner.get());
    int deliveries = 0;
    int ownerCallbacks = 0;
    bool detached = false;
    connect(session, &NTRIPHttpSession::finished, owner.get(), [&]() { ++ownerCallbacks; });
    connect(session, &NTRIPHttpSession::bodyReceived, this, [&]() {
        ++deliveries;
        session->retire();
        detached = !session->parent();
        // Deleting the owner must not delete the retired session during its delivery.
        owner.reset();
    });
    QVERIFY(session->open(caster.connectionConfig()).isEmpty());
    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    const QByteArray response = "HTTP/1.1 200 OK\r\n\r\n" + QByteArray(2 * NTRIPHttpSession::READ_CHUNK_BYTES, 'x');
    QCOMPARE(connection->write(response), response.size());
    connection->disconnectFromHost();
    QTRY_COMPARE_WITH_TIMEOUT(deliveries, 1, TestTimeout::mediumMs());
    QVERIFY(detached);
    QVERIFY(!owner);
    QTRY_VERIFY_WITH_TIMEOUT(!session, TestTimeout::mediumMs());
    QCOMPARE(deliveries, 1);
    QCOMPARE(ownerCallbacks, 0);
}

UT_REGISTER_TEST_LIGHTWEIGHT(NTRIPHttpSessionTest, TestLabel::Unit)
