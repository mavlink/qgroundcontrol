#include "GPSReceiverIntegrationTest.h"

#include <QtCore/QIODevice>
#include <QtNetwork/QUdpSocket>

#include "GPSReceiverProfile.h"
#include "GPSTransport.h"
#include "NMEAConnectionAttempt.h"

void GPSReceiverIntegrationTest::_passiveAttemptNeverConfigures()
{
    GPSReceiverProfile profile;
    profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::UdpListener;
    NMEAConnectionAttempt passive(profile);
    QSignalSpy ready(&passive, &NMEAConnectionAttempt::deviceReady);
    passive.start();
    QCOMPARE(ready.size(), 1);
    QVERIFY(passive.device());
    QVERIFY(passive.device()->open(QIODevice::ReadOnly));
    QVERIFY(passive.device()->isReadable());
    QUdpSocket sender;
    const QByteArray bytes("passive-input\n");
    QCOMPARE(sender.writeDatagram(bytes, QHostAddress::LocalHost, passive.localPort()), bytes.size());
    QTRY_COMPARE_WITH_TIMEOUT(passive.device()->bytesAvailable(), bytes.size(), TestTimeout::mediumMs());
    QCOMPARE(passive.device()->readAll(), bytes);
    passive.shutdown();

    NMEAConnectionAttempt rejected(profile);
    QSignalSpy failed(&rejected, &NMEAConnectionAttempt::failed);
    bool factoryCalled = false;
    rejected.start([&](const std::atomic_bool&) {
        factoryCalled = true;
        return std::unique_ptr<GPSTransport>();
    });
    QCOMPARE(failed.size(), 1);
    QVERIFY(!factoryCalled);
    QVERIFY(!rejected.device());
    rejected.shutdown();
}

void GPSReceiverIntegrationTest::_passiveTerminalTransitionIsIdempotent()
{
    GPSReceiverProfile profile;
    profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::Tcp;
    profile.endpoint.port = -1;
    NMEAConnectionAttempt attempt(profile, nullptr, 42);
    int terminalTransitions = 0;
    connect(&attempt, &NMEAConnectionAttempt::attemptChanged, &attempt, [&](const GPSReceiverAttempt& snapshot) {
        QCOMPARE(snapshot.generation, quint64(42));
        terminalTransitions += snapshot.terminal() ? 1 : 0;
    });
    QSignalSpy errors(&attempt, &NMEAConnectionAttempt::failed);
    QSignalSpy stopped(&attempt, &NMEAConnectionAttempt::stopped);
    attempt.start();
    const auto failure = attempt.attempt();
    QCOMPARE(failure.phase, GPSReceiverAttempt::Phase::Failed);
    QVERIFY(!failure.errorDetail.isEmpty());
    attempt.stop();
    attempt.stop();
    attempt.shutdown();
    QCOMPARE(errors.size(), 1);
    QCOMPARE(stopped.size(), 1);
    QCOMPARE(terminalTransitions, 1);
    QCOMPARE(attempt.attempt().errorDetail, failure.errorDetail);
}

UT_REGISTER_TEST(GPSReceiverIntegrationTest, TestLabel::Unit)
