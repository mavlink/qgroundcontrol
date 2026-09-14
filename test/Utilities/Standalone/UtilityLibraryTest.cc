#include <QtTest/QTest>

#include <memory>

#include "DataRateTracker.h"
#include "JsonValidation.h"
#include "ManualScheduler.h"
#include "QGCLoggingCategory.h"
#include "ScheduledTask.h"
#include "UdpForwarder.h"
#include "UdpIODevice.h"

class UtilityLibraryTest : public QObject
{
    Q_OBJECT

private slots:

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

    void requiredEmptyJsonKey()
    {
        QString error;
        QVERIFY(!JsonParsing::validateRequiredKeys({}, {QString()}, error));
        QCOMPARE(error, QStringLiteral("The following required keys are missing: "));
        const QList<JsonParsing::KeyValidateInfo> keys{{"", QJsonValue::String, true}};
        QVERIFY(!JsonParsing::validateKeys({}, keys, error));
        QVERIFY(!JsonParsing::validateKeysStrict({}, keys, error));
        QVERIFY(JsonParsing::validateKeysStrict({{QString(), QStringLiteral("present")}}, keys, error));
    }

    void validationDiagnosticOrder()
    {
        const QList<JsonParsing::KeyValidateInfo> keys{{"value", QJsonValue::Double, true},
                                                       {"second", QJsonValue::Bool, true},
                                                       {"first", QJsonValue::String, true}};
        QJsonObject object{{"value", QStringLiteral("wrong")}, {"extra", true}};
        QString error;
        QVERIFY(!JsonParsing::validateKeysStrict(object, keys, error));
        QCOMPARE(error, QStringLiteral("The following required keys are missing: second, first"));
        object.insert(QStringLiteral("second"), true);
        object.insert(QStringLiteral("first"), QStringLiteral("present"));
        QVERIFY(!JsonParsing::validateKeysStrict(object, keys, error));
        QCOMPARE(error, QStringLiteral("Incorrect value type - key:type:expected value:String:Double"));
        object.insert(QStringLiteral("value"), 1);
        QVERIFY(JsonParsing::validateKeys(object, keys, error));
        QVERIFY(!JsonParsing::validateKeysStrict(object, keys, error));
        QCOMPARE(error, QStringLiteral("Unknown key: extra"));
    }

    void validationSpecialTypes_data()
    {
        QTest::addColumn<QJsonValue>("value");
        QTest::addColumn<bool>("valid");
        QTest::newRow("null") << QJsonValue(QJsonValue::Null) << true;
        QTest::newRow("double") << QJsonValue(1.5) << true;
        QTest::newRow("bool") << QJsonValue(true) << false;
        QTest::newRow("object") << QJsonValue(QJsonObject{{"key", 1}}) << false;
    }

    void validationSpecialTypes()
    {
        QFETCH(QJsonValue, value);
        QFETCH(bool, valid);
        QString error;
        const QJsonObject object{{"value", value}};
        QCOMPARE(JsonParsing::validateKeyTypes(object, {"value"}, {QJsonValue::Null}, error), valid);
        QCOMPARE(JsonParsing::validateKeysStrict(object, {{"value", QJsonValue::Null, true}}, error), valid);
        QVERIFY(JsonParsing::validateKeyTypes(object, {"value"}, {QJsonValue::Undefined}, error));
        QVERIFY(JsonParsing::validateKeysStrict(object, {{"value", QJsonValue::Undefined, true}}, error));
    }

    void validationUtf8Keys()
    {
        const char* key = "\xC3\xA9";
        const QList<JsonParsing::KeyValidateInfo> keys{{key, QJsonValue::Double, true}};
        const QJsonObject object{{QString::fromUtf8(key), 1}};
        QString error;
        QVERIFY(JsonParsing::validateKeys(object, keys, error));
        QVERIFY(JsonParsing::validateKeysStrict(object, keys, error));
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
