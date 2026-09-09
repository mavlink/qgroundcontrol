#include "GPSReceiverSessionTest.h"

#include <QtCore/QEvent>
#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtPositioning/QGeoPositionInfoSource>

#include <memory>

#include "GPSConnectionConfig.h"
#include "GPSReceiverSession.h"
#include "GPSTransport.h"
#include "NMEADecoderSession.h"
#include "NMEAStreamSplitter.h"
#include "NMEAUtils.h"

namespace {
const QByteArray kRecordedFix =
    "$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A*43\r\n"
    "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76\r\n";
}

void GPSReceiverSessionTest::_cancelBeforeStart_data()
{
    QTest::addColumn<QString>("action");
    QTest::newRow("disconnect") << QStringLiteral("disconnect");
    QTest::newRow("shutdown") << QStringLiteral("shutdown");
    QTest::newRow("destroy") << QStringLiteral("destroy");
}

void GPSReceiverSessionTest::_cancelBeforeStart()
{
    QFETCH(QString, action);
    auto session = std::make_unique<GPSReceiverSession>();
    QPointer<GPSProvider> provider;
    auto lease = std::make_shared<int>(0);
    const std::weak_ptr<int> reservation = lease;
    bool opened = false;
    auto factory = [lease = std::move(lease), &opened](const std::atomic_bool&) {
        opened = true;
        return std::unique_ptr<GPSTransport>();
    };
    const auto cancellation = connect(session.get(), &GPSReceiverSession::receiverTypeChanged, this, [&]() {
        provider = session->_provider;
        if (action == QStringLiteral("shutdown")) {
            session->shutdown();
        } else if (action == QStringLiteral("destroy")) {
            session.reset();
        } else {
            session->stop();
        }
    });
    session->start(GPSConnectionConfig{.receiverType = GPSType::u_blox, .receiver = {}}.profile(), std::move(factory));
    QVERIFY(!opened);
    QVERIFY(provider.isNull());
    QVERIFY(reservation.expired());
    if (session) {
        QVERIFY(!session->hasReceiver());
        QVERIFY(!session->stopping());
        disconnect(cancellation);
        if (action == QStringLiteral("disconnect")) {
            session->start(GPSConnectionConfig{.receiverType = GPSType::u_blox, .receiver = {}}.profile(), {});
            QVERIFY(session->hasReceiver());
            QTRY_VERIFY_WITH_TIMEOUT(!session->hasReceiver(), TestTimeout::mediumMs());
        }
        session->shutdown();
    }
}

void GPSReceiverSessionTest::_nmeaStreamBoundsPendingBytes()
{
    GPSByteStream stream;
    const auto buffer = stream.buffer();
    int notifications = 0;
    for (int index = 0; index < 1024; ++index) {
        notifications += buffer->append(QByteArray(4096, 'x')) ? 1 : 0;
    }
    QCOMPARE(notifications, 1);
    QVERIFY(stream.bytesAvailable() <= 64 * 1024);
    const QByteArray nextSentence("$GPRMC,next*00\r\n");
    QVERIFY(!buffer->append(nextSentence));
    const auto pending = stream.readAll();
    QVERIFY(pending.endsWith(nextSentence));
    QCOMPARE(stream.bytesAvailable(), 0);
    QVERIFY(buffer->append(nextSentence));
    QCOMPARE(stream.readAll(), nextSentence);
}

void GPSReceiverSessionTest::_nmeaStreamPreservesReceiptAge_data()
{
    QTest::addColumn<int>("ageMs");
    QTest::newRow("queued-four-seconds") << 4000;
    QTest::newRow("expired-before-delivery") << 6000;
}

void GPSReceiverSessionTest::_nmeaStreamPreservesReceiptAge()
{
    QFETCH(int, ageMs);
    GPSByteStream stream;
    NMEADecoderSession decoder;
    QVERIFY(decoder.start(&stream));
    decoder.positionSource()->startUpdates();
    const quint64 queuedAt = GPSObservation::monotonicNowUs() - ageMs * 1000u;
    stream.buffer()->append(kRecordedFix, queuedAt);
    stream.buffer()->append(NMEAUtils::repairChecksum("$GPGSV,1,1,00"));
    stream.notifyReadyRead();
    if (ageMs < GPSSourceHealth::FRESHNESS_TIMEOUT_MS) {
        QTRY_VERIFY_WITH_TIMEOUT(decoder.health()->usable(), TestTimeout::shortMs());
        const auto observation = decoder.health()->observation();
        QVERIFY(GPSSourceHealth::ageMilliseconds(observation.monotonicTimestampUs) >= ageMs);
        QTRY_VERIFY_WITH_TIMEOUT(!decoder.health()->usable(), TestTimeout::mediumMs());
    } else {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QVERIFY(!decoder.health()->usable());
        QVERIFY(!decoder.positionSource()->lastKnownPosition().isValid());
    }
}

void GPSReceiverSessionTest::_nmeaStreamDiscardsPartialFixAcrossGap()
{
    GPSByteStream stream;
    NMEAStreamSplitter splitter(&stream);
    const qsizetype split = kRecordedFix.indexOf('*');
    stream.buffer()->append(kRecordedFix.first(split));
    stream.notifyReadyRead();
    QVERIFY(splitter.positionDevice()->readAll().isEmpty());
    stream.buffer()->append(QByteArray("lost"), GPSObservation::monotonicNowUs() - 6000000u);
    const QByteArray fresh = NMEAUtils::repairChecksum("$GPGSV,1,1,00");
    stream.buffer()->append(kRecordedFix.mid(split, 5) + fresh);
    stream.notifyReadyRead();
    QCOMPARE(splitter.positionDevice()->readAll(), fresh);
}

void GPSReceiverSessionTest::_destroyDuringStreamClose()
{
    struct Gate
    {
        QSemaphore entered;
        QSemaphore release;
    };

    const auto gate = std::make_shared<Gate>();
    auto session = std::make_unique<GPSReceiverSession>();
    const auto cleanup = qScopeGuard([&]() { gate->release.release(); });
    GPSReceiverConfig config;
    config.role = GPSReceiverConfig::Role::Position;
    config.outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA;
    session->start(GPSConnectionConfig{.receiverType = GPSType::u_blox, .receiver = config}.profile(),
                   [gate](const std::atomic_bool&) {
                       gate->entered.release();
                       gate->release.acquire();
                       return std::unique_ptr<GPSTransport>();
                   });
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> worker = session->_provider;
    connect(session->nmeaDevice(), &QObject::destroyed, this, [&]() { session.reset(); });
    session->stop();
    QVERIFY(!session);
    QVERIFY(worker);
    QVERIFY(!worker->parent());
    gate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(worker.isNull(), TestTimeout::mediumMs());
}

UT_REGISTER_TEST(GPSReceiverSessionTest, TestLabel::Unit)

void GPSReceiverSessionTest::_terminalStateExactlyOnce()
{
    GPSReceiverSession session;
    auto release = std::make_shared<QSemaphore>();
    const auto cleanup = qScopeGuard([&]() {
        session.stop();
        release->release();
        session.shutdown();
    });
    const auto profile =
        GPSConnectionConfig{.transport = GPSConnectionConfig::Tcp, .host = QStringLiteral("localhost"), .port = 2101}
            .profile();
    QSignalSpy disconnected(&session, &GPSReceiverSession::disconnected);
    QSignalSpy failed(&session, &GPSReceiverSession::connectionError);
    int terminalTransitions = 0;
    connect(&session, &GPSReceiverSession::attemptChanged, &session,
            [&](const GPSReceiverAttempt& attempt) { terminalTransitions += attempt.terminal() ? 1 : 0; });
    session.start(profile, [release](const std::atomic_bool&) {
        release->acquire();
        return std::unique_ptr<GPSTransport>();
    });
    const auto snapshot = session.attempt();
    QCOMPARE(*snapshot.profile, profile);
    const auto worker = session._provider;
    emit worker->connectionErrorDetail(GPSConnectionError::ConfigFailed, QStringLiteral("Configuration rejected"));
    emit worker->connectionError(GPSConnectionError::ConfigFailed);
    QTRY_COMPARE_WITH_TIMEOUT(failed.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(disconnected.size(), 1);
    QCOMPARE(session.attempt().phase, GPSReceiverAttempt::Phase::Failed);
    release->release();
    QTRY_VERIFY_WITH_TIMEOUT(!session.hasReceiver(), TestTimeout::mediumMs());
    session.stop();
    session.stop();
    QCOMPARE(disconnected.size(), 1);
    QCOMPARE(failed.size(), 1);
    QCOMPARE(terminalTransitions, 1);
    QCOMPARE(session.attempt().generation, snapshot.generation);
    QCOMPARE(session.errorDetail(), QStringLiteral("Configuration rejected"));
}

void GPSReceiverSessionTest::_attemptSnapshotSurvivesRestart()
{
    GPSReceiverSession session;
    const auto cleanup = qScopeGuard([&]() { session.shutdown(); });
    GPSConnectionConfig config;
    config.receiver.role = GPSReceiverConfig::Role::Position;
    const auto first = config.profile();
    config.receiver.outputRateHz = 5;
    const auto second = config.profile();
    quint64 firstGeneration = 0;
    bool restarted = false;
    connect(&session, &GPSReceiverSession::attemptChanged, &session, [&](const GPSReceiverAttempt& attempt) {
        if (!restarted && attempt.phase == GPSReceiverAttempt::Phase::Connecting) {
            restarted = true;
            firstGeneration = attempt.generation;
            const auto original = attempt.profile;
            session.start(second, {});
            // Reentrant replacement must not mutate the snapshot already being delivered.
            QCOMPARE(attempt.generation, firstGeneration);
            QCOMPARE(*attempt.profile, first);
            QCOMPARE(original, attempt.profile);
        }
    });
    session.start(first, {});
    QVERIFY(restarted);
    QVERIFY(session.attempt().generation > firstGeneration);
    QCOMPARE(session.profile(), second);
    QTRY_VERIFY_WITH_TIMEOUT(!session.hasReceiver(), TestTimeout::mediumMs());
}

void GPSReceiverSessionTest::_typedOperationEvidenceSurvivesFailureAndRejectsRetiredWorker()
{
    GPSReceiverSession session;
    auto release = std::make_shared<QSemaphore>();
    const auto cleanup = qScopeGuard([&]() {
        session.stop();
        release->release(2);
        session.shutdown();
    });
    const auto profile = GPSConnectionConfig{}.profile();
    const auto factory = [release](const std::atomic_bool&) {
        release->acquire();
        return std::unique_ptr<GPSTransport>();
    };
    session.start(profile, factory);
    const auto firstWorker = session._provider;
    const quint64 firstGeneration = session.attempt().generation;
    const GPSConfigurationResult configuration{GPSConfigurationStatus::TransportError,
                                               QStringLiteral("Write deadline"),
                                               {},
                                               GPSWriteResult{GPSWriteStatus::TimedOut, 8, 3, 5}};
    emit firstWorker->transportOpenFinished({GPSOpenStatus::Opened});
    emit firstWorker->configurationFinished(configuration);
    emit firstWorker->connectionErrorDetail(GPSConnectionError::ConfigFailed, configuration.error);
    emit firstWorker->connectionError(GPSConnectionError::ConfigFailed);
    QTRY_VERIFY_WITH_TIMEOUT(session.attempt().terminal(), TestTimeout::shortMs());
    const auto failed = session.attempt();
    QVERIFY(failed.transportOpen.has_value());
    QVERIFY(failed.configurationResult.has_value());
    QVERIFY(failed.configurationResult->transportWrite.has_value());
    QCOMPARE(failed.configurationResult->transportWrite->writtenBytes, 3);
    QCOMPARE(failed.configurationResult->transportWrite->uncertainBytes, 5);

    session.start(profile, factory);
    QVERIFY(session.attempt().generation > firstGeneration);
    QVERIFY(!session.attempt().transportOpen);
    QVERIFY(!session.attempt().configurationResult);
    const auto secondWorker = session._provider;
    emit firstWorker->transportReadFailed({GPSReadStatus::Overflow, 0, QStringLiteral("retired")});
    emit firstWorker->configurationFinished(configuration);
    emit secondWorker->transportOpenFinished({GPSOpenStatus::Opened});
    emit secondWorker->transportReadFailed({GPSReadStatus::Overflow, 0, QStringLiteral("Serial input exhausted")});
    QTRY_VERIFY_WITH_TIMEOUT(session.attempt().transportRead.has_value(), TestTimeout::shortMs());
    QCOMPARE(session.attempt().transportRead->detail, QStringLiteral("Serial input exhausted"));
    QVERIFY(!session.attempt().configurationResult);
    QCOMPARE(failed.generation, firstGeneration);
    QCOMPARE(failed.configurationResult->transportWrite->uncertainBytes, 5);
}
