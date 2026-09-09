#include <QtCore/QElapsedTimer>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtTest/QTest>

#include <atomic>
#include <thread>

#include "AndroidSerialWrite.h"

#ifdef Q_OS_UNIX
#include <fcntl.h>
#include <unistd.h>
#endif

using AndroidSerialWrite::Result;
using AndroidSerialWrite::Status;

class AndroidSerialWriteTest : public QObject
{
    Q_OBJECT

private slots:

    void initialWriteUsesDeadlineAndWireBudget()
    {
        QByteArray payload(1029, 'x');
        int calls = 0;
        const auto result = AndroidSerialWrite::run(
            payload.data(), payload.size(), 9600, QDeadlineTimer(2000), []() { return false; },
            [&](const char*, int count, int timeout) {
                ++calls;
                if (timeout <= 0 || timeout > 50 || count > 48) {
                    return Result{Status::Error};
                }
                return Result{Status::Completed, count};
            });
        QCOMPARE(result.status, Status::Completed);
        QCOMPARE(result.writtenBytes, payload.size());
        QCOMPARE(result.uncertainBytes, 0);
        QVERIFY(calls > 1);
    }

    void uncertainPrefixIsNeverRetried()
    {
        QByteArray payload(64, 'x');
        int calls = 0;
        const auto result = AndroidSerialWrite::run(
            payload.data(), payload.size(), 115200, QDeadlineTimer(500), []() { return false; },
            [&](const char*, int count, int) {
                ++calls;
                return Result{Status::TimedOut, 2, count - 2};
            });
        QCOMPARE(result.status, Status::TimedOut);
        QCOMPARE(result.writtenBytes, 2);
        QCOMPARE(result.uncertainBytes, 62);
        QCOMPARE(calls, 1);
    }

    void cancellationPreservesCompletedPrefix()
    {
        QByteArray payload(1029, 'x');
        bool cancelled = false;
        int transferred = 0;
        const auto result = AndroidSerialWrite::run(
            payload.data(), payload.size(), 9600, QDeadlineTimer(2000), [&]() { return cancelled; },
            [&](const char*, int count, int) {
                transferred += count;
                cancelled = true;
                return Result{Status::Completed, count};
            });
        QCOMPARE(result.status, Status::Cancelled);
        QCOMPARE(result.writtenBytes, transferred);
        QVERIFY(transferred > 0 && transferred < payload.size());
        QCOMPARE(result.uncertainBytes, 0);
    }

    void lateInitialCompletionRetainsTimeout()
    {
        const auto result = AndroidSerialWrite::run(
            "data", 4, 115200, QDeadlineTimer(10), []() { return false; },
            [](const char*, int count, int) {
                QSemaphore delay;
                delay.tryAcquire(1, 30);
                return Result{Status::Completed, count};
            });
        QCOMPARE(result.status, Status::TimedOut);
        QCOMPARE(result.writtenBytes, 4);
    }

    void expiredOperationDoesNotWrite()
    {
        int calls = 0;
        const auto result = AndroidSerialWrite::run(
            "data", 4, 115200, QDeadlineTimer(0), []() { return false; },
            [&](const char*, int, int) {
                ++calls;
                return Result{Status::Error};
            });
        QCOMPARE(result.status, Status::TimedOut);
        QCOMPARE(result.writtenBytes, 0);
        QCOMPARE(calls, 0);
    }

    void posixStalledWrite_data()
    {
        QTest::addColumn<bool>("cancel");
        QTest::newRow("deadline") << false;
        QTest::newRow("cancel") << true;
    }

    void posixStalledWrite()
    {
#ifdef Q_OS_UNIX
        QFETCH(bool, cancel);
        int descriptors[2]{};
        QVERIFY(::pipe(descriptors) == 0);
        const auto closePipe = qScopeGuard([&]() {
            ::close(descriptors[0]);
            ::close(descriptors[1]);
        });
        QVERIFY(::fcntl(descriptors[0], F_SETFL, O_NONBLOCK) == 0);
        QVERIFY(::fcntl(descriptors[1], F_SETFL, O_NONBLOCK) == 0);
        std::atomic_bool stopped = false;
        std::jthread cancellation([&]() {
            if (cancel) {
                QSemaphore delay;
                delay.tryAcquire(1, 50);
                stopped = true;
            }
        });
        const QByteArray payload(1024 * 1024, 'x');
        QElapsedTimer elapsed;
        elapsed.start();
        const auto result =
            AndroidSerialWrite::writePosix(descriptors[1], payload.constData(), payload.size(), 115200,
                                           QDeadlineTimer(cancel ? 2000 : 80), [&]() { return stopped.load(); });
        QCOMPARE(result.status, cancel ? Status::Cancelled : Status::TimedOut);
        QVERIFY(result.writtenBytes > 0 && result.writtenBytes < payload.size());
        QCOMPARE(result.uncertainBytes, 0);
        QVERIFY(elapsed.elapsed() < 500);
        QByteArray actual(result.writtenBytes, Qt::Uninitialized);
        QCOMPARE(::read(descriptors[0], actual.data(), actual.size()), result.writtenBytes);
        QCOMPARE(actual, payload.first(actual.size()));
#else
        QSKIP("POSIX serial write requires a Unix descriptor");
#endif
    }
};

QTEST_GUILESS_MAIN(AndroidSerialWriteTest)
#include "AndroidSerialWriteTest.moc"
