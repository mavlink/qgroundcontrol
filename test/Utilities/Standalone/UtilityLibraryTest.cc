#include <QtTest/QTest>

#include <array>
#include <limits>
#include <memory>

#include "CRC32.h"
#include "DataRateTracker.h"
#include "JsonValidation.h"
#include "LittleEndian.h"
#include "ManualScheduler.h"
#include "QGCLoggingCategory.h"
#include "ScheduledTask.h"
#include "TimestampedByteBuffer.h"
#include "UdpForwarder.h"
#include "UdpIODevice.h"

class UtilityLibraryTest : public QObject
{
    Q_OBJECT

private slots:

    void unalignedWireAndBounds()
    {
        const std::array<uint8_t, 5> encoded{0xaa, 0x00, 0x00, 0x44, 0xc1};
        QCOMPARE(LittleEndian::read<float>(encoded, 1).value(), -12.25f);
        std::array<uint8_t, 5> output{0xaa};
        QVERIFY(LittleEndian::write<float>(output, 1, -12.25f));
        QVERIFY(output == encoded);
        QVERIFY(!LittleEndian::read<uint64_t>(encoded, 1));
        QVERIFY(!LittleEndian::write<uint32_t>(output, SIZE_MAX, 1));
        QVERIFY(output == encoded);
    }

    void incrementalCrc()
    {
        const std::array<uint8_t, 9> input{'1', '2', '3', '4', '5', '6', '7', '8', '9'};
        const auto initial = std::numeric_limits<uint32_t>::max();
        const auto bytes = std::span(input);
        const auto prefix = QGC::crc32Update(bytes.first(4), initial);
        QCOMPARE(QGC::crc32Update(bytes.subspan(4), prefix) ^ initial, uint32_t{0xcbf43926});
        QCOMPARE(QGC::crc32Update({}, prefix), prefix);
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

    void validationWithoutApplicationServices()
    {
        const QList<JsonParsing::KeyValidateInfo> keys{{"value", QJsonValue::Double, true}};
        QString error;
        QVERIFY(JsonParsing::validateKeysStrict({{"value", 1}}, keys, error));
        QVERIFY(!JsonParsing::validateKeysStrict({{"value", 1}, {"extra", true}}, keys, error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!JsonParsing::validateKeysStrict({{"value", "wrong type"}}, keys, error));
        QVERIFY(!JsonParsing::validateKeysStrict({}, keys, error));
    }

    void loggingWithoutManager()
    {
        const QString earlyName = QStringLiteral("Utilities.Standalone.Early");
        const QString lateName = QStringLiteral("Utilities.Standalone.Late");
        const QGCLoggingCategory early(earlyName);
        QStringList received;
        auto context = std::make_unique<QObject>();
        const auto snapshot =
            qgcObserveLoggingCategories(context.get(), [&](const QString& category) { received.append(category); });
        QVERIFY(snapshot.contains(earlyName));
        const QGCLoggingCategory late(lateName);
        QTRY_COMPARE_WITH_TIMEOUT(received, QStringList{lateName}, 1000);
        const QGCLoggingCategory queued(QStringLiteral("Utilities.Standalone.Cancelled"));
        context.reset();
        QCoreApplication::sendPostedEvents();
        QCOMPARE(received, QStringList{lateName});
    }

    void networkAndRateLibraryLinkage()
    {
        UdpIODevice input;
        QVERIFY(input.bind(QHostAddress::LocalHost, 0));
        UdpForwarder output;
        DataRateTracker rate;
        QCOMPARE(rate.totalBytes(), quint64{0});
        QCOMPARE(input.bytesAvailable(), qint64{0});
        input.close();
    }
};

QTEST_GUILESS_MAIN(UtilityLibraryTest)
#include "UtilityLibraryTest.moc"
