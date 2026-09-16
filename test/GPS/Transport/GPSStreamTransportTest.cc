#include <chrono>
#include <memory>
#include <thread>

#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QTest>

#include "TCPGPSTransport.h"

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(QGC_NO_SERIAL_LINK)
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

#include "SerialGPSTransport.h"
#endif

class GPSStreamTransportTest : public QObject
{
    Q_OBJECT

private slots:

    void writes_data()
    {
        QTest::addColumn<bool>("serial");
        QTest::addColumn<QByteArray>("outcome");
        for (bool serial : {false, true}) {
            for (const QByteArray outcome : {"complete", "expired", "cancelled", "cancelled-forever", "timeout"}) {
                QTest::newRow(((serial ? "serial-" : "tcp-") + outcome).constData()) << serial << outcome;
            }
        }
    }

    void writes()
    {
        QFETCH(bool, serial);
        QFETCH(QByteArray, outcome);
        std::atomic_bool stop = false;
        QFile master;
        QTcpServer server;
        QTcpSocket* peer = nullptr;
        std::unique_ptr<GPSTransport> transport;
        if (serial) {
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(QGC_NO_SERIAL_LINK)
            const int descriptor = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC | O_NONBLOCK);
            QVERIFY(descriptor >= 0);
            if (!master.open(descriptor, QIODevice::ReadWrite, QFileDevice::AutoCloseHandle)) {
                close(descriptor);
                QFAIL("Cannot open pseudo-terminal");
            }
            QVERIFY(grantpt(descriptor) == 0);
            QVERIFY(unlockpt(descriptor) == 0);
            const char* slave = ptsname(descriptor);
            QVERIFY(slave);
            transport = std::make_unique<SerialGPSTransport>(QString::fromLocal8Bit(slave), stop);
#else
            QSKIP("Desktop serial integration requires a Linux pseudo-terminal");
#endif
        } else {
            QVERIFY(server.listen(QHostAddress::LocalHost));
            transport = std::make_unique<TCPGPSTransport>(QStringLiteral("127.0.0.1"), server.serverPort(), stop);
        }
        QCOMPARE(transport->open().status, GPSOpenStatus::Opened);
        if (!serial) {
            QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 1000);
            peer = server.nextPendingConnection();
            QVERIFY(peer);
            if (outcome != "complete") {
                peer->setReadBufferSize(1);
            }
        }
        const uint8_t byte = 42;
        QCOMPARE(transport->writeBounded(nullptr, 1, QDeadlineTimer(100)).status, GPSWriteStatus::InvalidData);
        QCOMPARE(transport->writeBounded(&byte, -1, QDeadlineTimer(100)).status, GPSWriteStatus::InvalidData);
        QCOMPARE(transport->writeBounded(&byte, 0, QDeadlineTimer(0)).status, GPSWriteStatus::Completed);
        if (outcome == "expired") {
            const auto result = transport->writeBounded(&byte, 1, QDeadlineTimer(0));
            QCOMPARE(result.status, GPSWriteStatus::TimedOut);
            QCOMPARE(result.acceptedBytes, 0);
            QCOMPARE(result.writtenBytes, 0);
            QCOMPARE(result.uncertainBytes, 0);
            QVERIFY(!transport->fatalError());
            return;
        }
        if (outcome == "complete") {
            QByteArray expected;
            // Consecutive writes must not credit a previous operation's signals.
            for (int size : {1, 127, 3, 512, 17, 1029, 2}) {
                const QByteArray payload(size, static_cast<char>(size));
                const auto result = transport->writeBounded(reinterpret_cast<const uint8_t*>(payload.constData()), size,
                                                            QDeadlineTimer(1000));
                QCOMPARE(result.status, GPSWriteStatus::Completed);
                QCOMPARE(result.acceptedBytes, size);
                QCOMPARE(result.writtenBytes, size);
                QCOMPARE(result.uncertainBytes, 0);
                expected.append(payload);
            }
            QByteArray received;
            const auto readPeer = [&]() {
                if (peer) {
                    received.append(peer->readAll());
                } else {
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(QGC_NO_SERIAL_LINK)
                    char bytes[2048];
                    const auto count = ::read(master.handle(), bytes, sizeof(bytes));
                    if (count > 0) {
                        received.append(bytes, count);
                    }
#endif
                }
                return received == expected;
            };
            QTRY_VERIFY_WITH_TIMEOUT(readPeer(), 1000);
            QVERIFY(!transport->fatalError());
            return;
        }
        QCOMPARE(transport->write(&byte, 1).writtenBytes, 1);
        const QByteArray payload(16 * 1024 * 1024, 'x');
        std::jthread cancellation;
        const bool cancelled = outcome.startsWith("cancelled");
        if (cancelled) {
            cancellation = std::jthread([&]() {
                // Cancel after submission, before the bounded deadline expires.
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                stop = true;
            });
        }
        QElapsedTimer elapsed;
        elapsed.start();
        const auto result = transport->writeBounded(
            reinterpret_cast<const uint8_t*>(payload.constData()), payload.size(),
            outcome == "cancelled-forever" ? QDeadlineTimer(QDeadlineTimer::Forever) : QDeadlineTimer(100));
        QCOMPARE(result.status, cancelled ? GPSWriteStatus::Cancelled : GPSWriteStatus::TimedOut);
        QVERIFY(elapsed.elapsed() < 1000);
        QVERIFY(result.acceptedBytes > 0);
        QVERIFY(result.acceptedBytes < payload.size());
        QVERIFY(result.writtenBytes >= 0);
        QVERIFY(result.uncertainBytes >= 0);
        QVERIFY(result.uncertainBytes <= 4096);
        QCOMPARE(result.acceptedBytes, result.writtenBytes + result.uncertainBytes);
        QVERIFY(transport->fatalError());
        stop = false;
        QCOMPARE(transport->writeBounded(&byte, 1, QDeadlineTimer(100)).acceptedBytes, 0);
    }
};

QTEST_GUILESS_MAIN(GPSStreamTransportTest)
#include "GPSStreamTransportTest.moc"
