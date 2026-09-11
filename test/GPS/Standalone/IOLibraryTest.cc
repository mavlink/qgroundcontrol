#include <QtTest/QTest>

#include "LittleEndian.h"
#include "ManualScheduler.h"
#include "ScheduledTask.h"
#include "TimestampedByteBuffer.h"

class IOLibraryTest : public QObject
{
    Q_OBJECT
private slots:

    void timestampAndGapEvidence()
    {
        TimestampedByteBuffer buffer;
        QVERIFY(buffer.append(QByteArray(50000, 'a'), 100));
        QByteArray bytes(10000, '\0');
        auto result = buffer.read(bytes.data(), bytes.size(), 200, 1000);
        QCOMPARE(result.bytes, 10000);
        QCOMPARE(result.receivedAtUs, 100);
        buffer.append(QByteArray(30000, 'b'), 200);
        result = buffer.read(bytes.data(), bytes.size(), 200, 1000);
        QVERIFY(result.gap);
        QCOMPARE(result.bytes, 0);
        result = buffer.read(bytes.data(), bytes.size(), 1200, 1000);
        QVERIFY(result.gap);
        QCOMPARE(buffer.size(), 0);
    }

    void unalignedWireAndBounds()
    {
        std::array<uint8_t, 10> bytes{};
        QVERIFY(LittleEndian::write<double>(bytes, 1, -12.25));
        QCOMPARE(LittleEndian::read<double>(bytes, 1).value(), -12.25);
        QVERIFY(!LittleEndian::read<uint64_t>(bytes, 3));
        QVERIFY(!LittleEndian::write<uint32_t>(bytes, SIZE_MAX, 1));
    }

    void contextDestructionAndReplacement()
    {
        ManualScheduler clock;
        auto context = std::make_unique<QObject>();
        ScheduledTask task(&clock, context.get());
        int calls = 0;
        QVERIFY(task.schedule(std::chrono::seconds(1), [&] { ++calls; }));
        QVERIFY(task.schedule(std::chrono::microseconds(0), [&] { calls += 2; }));
        QCOMPARE(calls, 0);
        QVERIFY(clock.advanceBy(std::chrono::microseconds(0)));
        QCOMPARE(calls, 2);
        QVERIFY(task.schedule(std::chrono::seconds(1), [&] { ++calls; }));
        context.reset();
        QVERIFY(clock.advanceBy(std::chrono::seconds(1)));
        QCOMPARE(calls, 2);
    }
};
QTEST_GUILESS_MAIN(IOLibraryTest)
#include "IOLibraryTest.moc"
