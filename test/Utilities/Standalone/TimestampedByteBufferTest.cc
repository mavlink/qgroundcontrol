#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtTest/QTest>

#include <atomic>
#include <thread>

#include "TimestampedByteBuffer.h"

class TimestampedByteBufferTest : public QObject
{
    Q_OBJECT
private slots:

    void notificationAndPartialReads()
    {
        TimestampedByteBuffer buffer;
        QVERIFY(!buffer.append({}, 100));
        QVERIFY(buffer.append("first", 100));
        QVERIFY(!buffer.append("second", 200));
        char bytes[32]{};
        auto result = buffer.read(bytes, 3, 200, 1000);
        QCOMPARE(QByteArray(bytes, result.bytes), QByteArray("fir"));
        QCOMPARE(result.receivedAtUs, 100);
        result = buffer.read(bytes, sizeof(bytes), 200, 1000);
        QCOMPARE(QByteArray(bytes, result.bytes), QByteArray("st"));
        QCOMPARE(result.receivedAtUs, 100);
        result = buffer.read(bytes, sizeof(bytes), 200, 1000);
        QCOMPARE(QByteArray(bytes, result.bytes), QByteArray("second"));
        QCOMPARE(result.receivedAtUs, 200);
        QCOMPARE(buffer.size(), 0);
        QVERIFY(buffer.append("next", 300));
        QCOMPARE(buffer.read(bytes, sizeof(bytes), 399, 100).bytes, 4);
    }

    void discardThenRetainedData_data()
    {
        QTest::addColumn<int>("reason");
        QTest::newRow("byte-limit") << 0;
        QTest::newRow("oversized-append") << 1;
        QTest::newRow("chunk-limit") << 2;
        QTest::newRow("exact-expiry-boundary") << 3;
        QTest::newRow("future-timestamp") << 4;
    }

    void discardThenRetainedData()
    {
        QFETCH(int, reason);
        TimestampedByteBuffer buffer;
        QByteArray expected;
        if (reason == 0) {
            QVERIFY(buffer.append(QByteArray(50000, 'a'), 100));
            QVERIFY(!buffer.append(QByteArray(30000, 'b'), 200));
            expected = QByteArray(30000, 'b');
        } else if (reason == 1) {
            QVERIFY(buffer.append(QByteArray(70000, 'b'), 200));
            expected = QByteArray(65535, 'b');
        } else if (reason == 2) {
            for (int i = 0; i < 1025; ++i)
                buffer.append("b", 200);
            expected = QByteArray(1024, 'b');
        } else {
            QVERIFY(buffer.append("discard", reason == 3 ? 100 : 300));
            QVERIFY(!buffer.append("retained", 200));
            expected = "retained";
        }
        char bytes[4096];
        const auto gap = buffer.read(bytes, sizeof(bytes), 200, 100);
        QVERIFY(gap.gap);
        QCOMPARE(gap.bytes, 0);
        QVERIFY(!buffer.gapPending());
        QCOMPARE(buffer.size(), expected.size());
        QByteArray received;
        while (buffer.size()) {
            const auto result = buffer.read(bytes, sizeof(bytes), 200, 100);
            QVERIFY(!result.gap);
            QVERIFY(result.bytes > 0);
            QCOMPARE(result.receivedAtUs, 200);
            received.append(bytes, result.bytes);
        }
        QCOMPARE(received, expected);
        QCOMPARE(buffer.read(bytes, sizeof(bytes), 200, 100).bytes, 0);
        QVERIFY(buffer.append("notify-again", 200));
    }

    void producerConsumerNotifications()
    {
        TimestampedByteBuffer buffer;
        QSemaphore ready;
        QSemaphore consumed;
        std::atomic_bool stop = false;
        std::jthread producer([&] {
            for (quint64 i = 1; i <= 32 && !stop; ++i) {
                const bool notify = buffer.append(QByteArray(50000, 'a'), i);
                const bool redundant = buffer.append(QByteArray(30000, 'b'), i);
                if (notify)
                    ready.release();
                if (redundant)
                    ready.release();
                if (!consumed.tryAcquire(1, 3000))
                    break;
            }
        });
        // Ensure any failing assertion still unblocks and joins the producer.
        const auto cleanup = qScopeGuard([&] {
            stop = true;
            consumed.release();
        });
        QByteArray bytes(30000, '\0');
        for (quint64 i = 1; i <= 32; ++i) {
            QVERIFY(ready.tryAcquire(1, 3000));
            QCOMPARE(ready.available(), 0);
            auto result = buffer.read(bytes.data(), bytes.size(), i, 1000);
            QVERIFY(result.gap);
            result = buffer.read(bytes.data(), bytes.size(), i, 1000);
            QCOMPARE(result.bytes, 30000);
            QCOMPARE(result.receivedAtUs, i);
            QCOMPARE(bytes, QByteArray(30000, 'b'));
            QCOMPARE(buffer.size(), 0);
            consumed.release();
        }
    }
};
QTEST_GUILESS_MAIN(TimestampedByteBufferTest)
#include "TimestampedByteBufferTest.moc"
