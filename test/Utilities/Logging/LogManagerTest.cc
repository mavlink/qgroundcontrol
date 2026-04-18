#include "LogManagerTest.h"


#include "LogManager.h"
#include "UnitTestList.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(LogManagerTestLog, "Test.Logging.LogManagerTest")

void LogManagerTest::_buildEntry()
{
    LogManager::setCaptureEnabled(true);

    qInfo("test info message");

    const auto msgs = LogManager::capturedMessages();
    QVERIFY(!msgs.isEmpty());

    bool found = false;
    for (const auto& m : msgs) {
        if (m.message.contains(QStringLiteral("test info message"))) {
            found = true;
            QCOMPARE(m.level, LogEntry::Info);
            QVERIFY(!m.formatted.isEmpty());
            break;
        }
    }
    QVERIFY(found);

    LogManager::clearCapturedMessages();
    LogManager::setCaptureEnabled(false);
}

void LogManagerTest::_captureMessages()
{
    LogManager::setCaptureEnabled(true);
    LogManager::clearCapturedMessages();

    qWarning("capture test warning");
    qInfo("capture test info");

    const auto msgs = LogManager::capturedMessages();
    QVERIFY(msgs.size() >= 2);

    LogManager::clearCapturedMessages();
    QCOMPARE(LogManager::capturedMessages().size(), 0);

    LogManager::setCaptureEnabled(false);
}

void LogManagerTest::_captureByCategory()
{
    LogManager::setCaptureEnabled(true);
    LogManager::clearCapturedMessages();

    qCWarning(LogManagerTestLog) << "categorized message";
    qWarning("uncategorized message");

    const auto all = LogManager::capturedMessages();
    const auto filtered = LogManager::capturedMessages(QStringLiteral("Test.Logging.LogManagerTest"));
    QVERIFY(all.size() >= filtered.size());

    bool foundCategorized = false;
    for (const auto& m : filtered) {
        if (m.message.contains(QStringLiteral("categorized message"))) {
            foundCategorized = true;
            break;
        }
    }
    QVERIFY(foundCategorized);

    LogManager::clearCapturedMessages();
    LogManager::setCaptureEnabled(false);
}

void LogManagerTest::_hasCapturedWarning()
{
    LogManager::setCaptureEnabled(true);
    LogManager::clearCapturedMessages();

    qCWarning(LogManagerTestLog) << "a warning";

    QVERIFY(LogManager::hasCapturedWarning(QStringLiteral("Test.Logging.LogManagerTest")));
    QVERIFY(!LogManager::hasCapturedCritical(QStringLiteral("Test.Logging.LogManagerTest")));

    LogManager::clearCapturedMessages();
    LogManager::setCaptureEnabled(false);
}

void LogManagerTest::_hasCapturedCritical()
{
    LogManager::setCaptureEnabled(true);
    LogManager::clearCapturedMessages();

    qCCritical(LogManagerTestLog) << "a critical";

    QVERIFY(LogManager::hasCapturedCritical(QStringLiteral("Test.Logging.LogManagerTest")));

    LogManager::clearCapturedMessages();
    LogManager::setCaptureEnabled(false);
}

void LogManagerTest::_hasCapturedUncategorized()
{
    LogManager::setCaptureEnabled(true);
    LogManager::clearCapturedMessages();

    qInfo("uncategorized");

    QVERIFY(LogManager::hasCapturedUncategorizedMessage());

    LogManager::clearCapturedMessages();
    LogManager::setCaptureEnabled(false);
}

void LogManagerTest::_categoryLogLevelNames()
{
    const auto names = LogManager::categoryLogLevelNames();
    QCOMPARE(names.size(), 4);
    QVERIFY(names.contains(QStringLiteral("Debug")));
    QVERIFY(names.contains(QStringLiteral("Warning")));

    const auto values = LogManager::categoryLogLevelValues();
    QCOMPARE(values.size(), 4);
    QCOMPARE(values[0].toInt(), static_cast<int>(QtDebugMsg));
}

UT_REGISTER_TEST(LogManagerTest, TestLabel::Unit, TestLabel::Utilities)
