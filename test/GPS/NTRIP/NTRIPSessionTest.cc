#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "MockNTRIPStream.h"
#include "NTRIPHttpDecoder.h"
#include "NTRIPSession.h"

class NTRIPSessionTest : public QObject
{
    Q_OBJECT

    static NTRIPTransportConfig config()
    {
        NTRIPTransportConfig result;
        result.host = QStringLiteral("caster.example.com");
        result.mountpoint = QStringLiteral("BASE");
        return result;
    }

private slots:

    void decoderFragmentation_data()
    {
        QTest::addColumn<QByteArray>("header");
        QTest::addColumn<bool>("chunked");
        QTest::newRow("http") << QByteArray("HTTP/1.1 200 OK\r\n\r\n") << false;
        QTest::newRow("icy-bare") << QByteArray("ICY 200 OK\r\n") << false;
        QTest::newRow("icy-headers") << QByteArray("ICY 200 OK\r\nServer: caster\r\n\r\n") << false;
        QTest::newRow("chunked") << QByteArray("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n") << true;
        QTest::newRow("continue") << QByteArray("HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 200 OK\r\n\r\n") << false;
    }

    void decoderFragmentation()
    {
        QFETCH(QByteArray, header);
        QFETCH(bool, chunked);
        const QByteArray payload = QByteArray::fromHex("d3001234567890abcdef0123456789abcdef123456");
        QByteArray wire = header;
        if (chunked) {
            for (qsizetype offset = 0; offset < payload.size(); offset += 3) {
                const auto chunk = payload.mid(offset, 3);
                wire += QByteArray::number(chunk.size(), 16) + ";extension=value\r\n" + chunk + "\r\n";
            }
            wire += "0\r\nTrailer: value\r\n\r\n";
        } else {
            wire += payload;
        }
        for (qsizetype fragment = 1; fragment <= wire.size(); ++fragment) {
            NTRIPHttpDecoder decoder;
            QByteArray decoded;
            int connections = 0;
            bool complete = false;
            for (qsizetype offset = 0; offset < wire.size(); offset += fragment) {
                const auto result =
                    decoder.feed(QByteArrayView(wire).sliced(offset, qMin(fragment, wire.size() - offset)));
                QVERIFY2(!result.failure, result.failure ? qPrintable(result.failure->detail) : "");
                connections += result.connected;
                complete |= result.complete;
                decoded += result.body;
                QVERIFY(decoder.bufferedBytes() <= NTRIPHttpDecoder::MAX_LINE_BYTES);
            }
            QCOMPARE(decoded, payload);
            QCOMPARE(connections, 1);
            QCOMPARE(complete, chunked);
        }
    }

    void decoderRejectsInvalidBodies_data()
    {
        QTest::addColumn<QByteArray>("wire");
        QTest::newRow("oversized-header") << QByteArray("HTTP/1.1 200 OK\r\nX: ") + QByteArray(40000, 'a');
        QTest::newRow("encoding") << QByteArray("HTTP/1.1 200 OK\r\nContent-Encoding: gzip\r\n\r\n");
        QTest::newRow("chunk-size") << QByteArray(
            "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nffffffffffffff\r\n");
        QTest::newRow("chunk-terminator")
            << QByteArray("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n1\r\nx!!");
        QTest::newRow("false-status") << QByteArray("garbage\r\nHTTP/1.1 200 OK\r\n\r\n");
    }

    void decoderRejectsInvalidBodies()
    {
        QFETCH(QByteArray, wire);
        NTRIPHttpDecoder decoder;
        const auto result = decoder.feed(wire);
        QVERIFY(result.failure);
        QVERIFY(!result.failure->retryable);
        QVERIFY(decoder.bufferedBytes() <= NTRIPHttpDecoder::MAX_LINE_BYTES);
        QVERIFY(decoder.feed("ignored").body.isEmpty());
    }

    void httpRecoveryClassification_data()
    {
        QTest::addColumn<int>("status");
        QTest::addColumn<bool>("retryable");
        QTest::newRow("unauthorized") << 401 << false;
        QTest::newRow("forbidden") << 403 << false;
        QTest::newRow("missing-mount") << 404 << false;
        QTest::newRow("rate-limit") << 429 << true;
        QTest::newRow("server-error") << 503 << true;
    }

    void httpRecoveryClassification()
    {
        QFETCH(int, status);
        QFETCH(bool, retryable);
        NTRIPHttpDecoder decoder;
        const auto result =
            decoder.feed("HTTP/1.1 " + QByteArray::number(status) + " Error\r\nRetry-After: 17\r\n\r\n");
        QVERIFY(result.failure);
        QCOMPARE(result.failure->httpStatus, status);
        QCOMPARE(result.failure->retryable, retryable);
        QCOMPARE(result.failure->retryAfter, std::chrono::seconds{17});
    }

    void retriesRequireSustainedCorrections()
    {
        qint64 now = 1000;
        MockNTRIPStream* stream = nullptr;
        NTRIPSession session(
            [&](const NTRIPTransportConfig&, QObject* owner) {
                stream = new MockNTRIPStream(owner);
                return stream;
            },
            nullptr, [&]() { return now; });
        session.start(config());
        for (int failed = 1; failed <= 3; ++failed) {
            QCOMPARE(session.state(), NTRIPSession::State::Connected);
            stream->simulateError(NTRIPError::DataWatchdog, QStringLiteral("no corrections"));
            QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
            QCOMPARE(session.state(), NTRIPSession::State::Reconnecting);
            QCOMPARE(session.failedAttempts(), failed);
            QCOMPARE(session.nextRetryDelay(), std::chrono::milliseconds{1000 * (1 << (failed - 1))});
            session._cancelRetry();
            const auto retiredAttempt = session._nextAttemptId;
            session._beginAttempt(session._generation);
            QVERIFY(session.activeAttemptId() > retiredAttempt);
        }
        for (int i = 0; i < 4; ++i) {
            stream->simulateRtcmData(QByteArray("frame"), 1005, now);
            now += 4000;
        }
        QCOMPARE(session.failedAttempts(), 0);
        stream->simulateError(NTRIPError::SslError, QStringLiteral("certificate expired"));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCOMPARE(session.state(), NTRIPSession::State::Error);
        QVERIFY(!session.retryPending());
    }

    void attemptIdentitySurvivesReentrantRestart_data()
    {
        QTest::addColumn<bool>("rejected");
        QTest::newRow("successful-frame") << false;
        QTest::newRow("rejected-frame") << true;
    }

    void attemptIdentitySurvivesReentrantRestart()
    {
        QFETCH(bool, rejected);
        QList<MockNTRIPStream*> streams;
        NTRIPSession session([&](const NTRIPTransportConfig&, QObject* owner) {
            auto* stream = new MockNTRIPStream(owner);
            streams.append(stream);
            return stream;
        });
        session.start(config());
        const auto retiredAttempt = session.activeAttemptId();
        const auto restart = [&]() {
            session.stop();
            session.start(config());
        };
        // A preceding observer replaces the session before downstream observers see the old emission.
        if (rejected) {
            connect(&session, &NTRIPSession::correctionRejected, &session, restart);
        } else {
            connect(&session, &NTRIPSession::correctionReceived, &session, restart);
        }
        QSignalSpy received(&session, &NTRIPSession::correctionReceived);
        QSignalSpy invalid(&session, &NTRIPSession::correctionRejected);
        if (rejected) {
            emit streams.first()->correctionRejectedAt(QByteArrayLiteral("bad"), 1005, 1000);
            QCOMPARE(invalid.size(), 1);
            QCOMPARE(invalid.first().last().toULongLong(), retiredAttempt);
        } else {
            streams.first()->simulateRtcmData(QByteArrayLiteral("frame"), 1005);
            QCOMPARE(received.size(), 1);
            QCOMPARE(received.first().last().toULongLong(), retiredAttempt);
        }
        QVERIFY(session.activeAttemptId() > retiredAttempt);
        QCOMPARE(session.sourceId(), QStringLiteral("ntrip://caster.example.com:2101/BASE"));
    }

    void observerCanCancelOrReplace_data()
    {
        QTest::addColumn<bool>("replace");
        QTest::newRow("cancel") << false;
        QTest::newRow("replace") << true;
    }

    void observerCanCancelOrReplace()
    {
        QFETCH(bool, replace);
        QList<MockNTRIPStream*> streams;
        NTRIPSession session([&](const NTRIPTransportConfig&, QObject* owner) {
            auto* stream = new MockNTRIPStream(owner);
            streams.append(stream);
            return stream;
        });
        bool first = true;
        connect(&session, &NTRIPSession::streamStarted, &session, [&]() {
            if (first) {
                first = false;
                session.stop();
                if (replace) {
                    session.start(config());
                }
            }
        });
        session.start(config());
        QCOMPARE(streams.first()->startCount, 0);
        QCOMPARE(streams.first()->stopCount, 1);
        QCOMPARE(streams.size(), replace ? 2 : 1);
        QCOMPARE(session.state(), replace ? NTRIPSession::State::Connected : NTRIPSession::State::Disconnected);
    }

    void staleErrorsAndGgaCannotCrossAttempts()
    {
        QList<MockNTRIPStream*> streams;
        NTRIPSession session([&](const NTRIPTransportConfig&, QObject* owner) {
            auto* stream = new MockNTRIPStream(owner);
            streams.append(stream);
            return stream;
        });
        QSignalSpy rejected(&session, &NTRIPSession::correctionRejected);
        auto cfg = config();
        cfg.username = QStringLiteral("secret-user");
        cfg.password = QStringLiteral("secret-password");
        session.start(cfg);
        emit streams.first()->correctionRejectedAt(QByteArrayLiteral("bad-frame"), 1005, 1000);
        QCOMPARE(rejected.size(), 1);
        session.sendNMEA("gga");
        QCOMPARE(streams.first()->sentNmea.size(), 1);
        streams.first()->simulateError(NTRIPError::SocketError, QStringLiteral("old failure"));
        session.stop();
        session.sendNMEA("stale");
        QCOMPARE(streams.first()->sentNmea.size(), 1);
        session.start(cfg);
        emit streams.first()->correctionRejectedAt(QByteArrayLiteral("retired-frame"), 1005, 2000);
        QCOMPARE(rejected.size(), 1);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCOMPARE(session.state(), NTRIPSession::State::Connected);
        QCOMPARE(session.failedAttempts(), 0);
        QVERIFY(!session.sourceId().contains("secret"));
    }
};

QTEST_GUILESS_MAIN(NTRIPSessionTest)
#include "NTRIPSessionTest.moc"
