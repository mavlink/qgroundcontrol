#include "LogManagerTest.h"
#include "LogManager.h"
#include "LogModel.h"
#include "LogEntry.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QThread>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

UT_REGISTER_TEST(LogManagerTest, TestLabel::Unit, TestLabel::Utilities)

namespace {

void addTestLogEntry(const QString& message)
{
    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.level = LogEntry::Debug;
    entry.category = "Test";
    entry.message = message;
    LogManager::instance()->handleLogEntry(entry);
}

void addTestLogEntryDirect(LogModel* model, const QString& message)
{
    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.level = LogEntry::Debug;
    entry.category = "Test";
    entry.message = message;
    model->append(entry);
}

} // namespace

void LogManagerTest::init()
{
    UnitTest::init();
    _tempDir = new QTemporaryDir();
    QVERIFY(_tempDir->isValid());
}

void LogManagerTest::cleanup()
{
    LogManager* manager = LogManager::instance();
    manager->setDiskLoggingEnabled(false);
    manager->setRemoteLoggingEnabled(false);
    manager->clear();

    delete _tempDir;
    _tempDir = nullptr;
    UnitTest::cleanup();
}

void LogManagerTest::_testSingleton()
{
    LogManager* instance1 = LogManager::instance();
    LogManager* instance2 = LogManager::instance();

    QVERIFY(instance1 != nullptr);
    QCOMPARE(instance1, instance2);
}

void LogManagerTest::_testModel()
{
    LogManager* manager = LogManager::instance();

    QVERIFY(manager->model() != nullptr);

    LogModel* model = manager->model();
    QVERIFY(model->rowCount() >= 0);
}

void LogManagerTest::_testDiskLogging()
{
    LogManager* manager = LogManager::instance();

    QSignalSpy enabledSpy(manager, &LogManager::diskLoggingEnabledChanged);

    QVERIFY(!manager->isDiskLoggingEnabled());

    manager->setLogDirectory(_tempDir->path());
    manager->setDiskLoggingEnabled(true);

    QVERIFY(manager->isDiskLoggingEnabled());
    QCOMPARE(enabledSpy.count(), 1);

    manager->setDiskLoggingEnabled(false);
    QVERIFY(!manager->isDiskLoggingEnabled());
    QCOMPARE(enabledSpy.count(), 2);
}

void LogManagerTest::_testRemoteLogging()
{
    LogManager* manager = LogManager::instance();

    QSignalSpy enabledSpy(manager, &LogManager::remoteLoggingEnabledChanged);

    QVERIFY(!manager->isRemoteLoggingEnabled());

    manager->setRemoteEndpoint("127.0.0.1:9999");
    QCOMPARE(manager->remoteEndpoint(), QString("127.0.0.1:9999"));

    manager->setRemoteLoggingEnabled(true);
    QVERIFY(manager->isRemoteLoggingEnabled());
    QCOMPARE(enabledSpy.count(), 1);

    manager->setRemoteLoggingEnabled(false);
    QVERIFY(!manager->isRemoteLoggingEnabled());
}

void LogManagerTest::_testSetLogDirectory()
{
    LogManager* manager = LogManager::instance();

    manager->setLogDirectory(_tempDir->path());
    manager->setDiskLoggingEnabled(true);

    addTestLogEntry("Test message for log directory");

    manager->flush();
    QThread::msleep(200);

    const QString logPath = QDir(_tempDir->path()).absoluteFilePath("QGCConsole.log");
    QVERIFY(QFile::exists(logPath));

    manager->setDiskLoggingEnabled(false);
}

void LogManagerTest::_testClear()
{
    LogManager* manager = LogManager::instance();
    LogModel* model = manager->model();

    manager->clear();
    QCOMPARE(model->rowCount(), 0);

    for (int i = 0; i < 5; ++i) {
        addTestLogEntryDirect(model, QString("Test entry %1").arg(i));
    }

    QTest::qWait(50);

    QVERIFY(model->rowCount() >= 5);

    manager->clear();
    QCOMPARE(model->rowCount(), 0);
}

void LogManagerTest::_testFlush()
{
    LogManager* manager = LogManager::instance();

    manager->setLogDirectory(_tempDir->path());
    manager->setDiskLoggingEnabled(true);

    addTestLogEntry("Flush test message");

    manager->flush();

    manager->setDiskLoggingEnabled(false);
}

void LogManagerTest::_testExportToFile()
{
    LogManager* manager = LogManager::instance();
    LogModel* model = manager->model();

    manager->clear();
    QCOMPARE(model->rowCount(), 0);

    for (int i = 0; i < 3; ++i) {
        addTestLogEntryDirect(model, QString("Export test message %1").arg(i));
    }

    QTest::qWait(50);

    QVERIFY(model->rowCount() >= 3);

    QSignalSpy startedSpy(manager, &LogManager::exportStarted);
    QSignalSpy finishedSpy(manager, &LogManager::exportFinished);

    const QString exportPath = _tempDir->filePath("export.log");

    manager->exportToFile(exportPath);

    QCOMPARE(startedSpy.count(), 1);

    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);

    const QList<QVariant> args = finishedSpy.takeFirst();
    QVERIFY(args.at(0).toBool());

    QVERIFY(QFile::exists(exportPath));

    QFile file(exportPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString content = QString::fromUtf8(file.readAll());

    QVERIFY(content.contains("Export test message"));
}

void LogManagerTest::_testErrorState()
{
    LogManager* manager = LogManager::instance();

    QVERIFY(!manager->hasError());

    manager->clearError();
    QVERIFY(!manager->hasError());
}
